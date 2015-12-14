/*
* Copyright (c) 2006-2011 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include <Box2D/Dynamics/Joints/b2WeldJoint.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2TimeStep.h>

using namespace box2d;

// Point-to-point constraint
// C = p2 - p1
// Cdot = v2 - v1
//      = v2 + cross(w2, r2) - v1 - cross(w1, r1)
// J = [-I -r1_skew I r2_skew ]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)

// Angle constraint
// C = angle2 - angle1 - referenceAngle
// Cdot = w2 - w1
// J = [0 0 -1 0 0 1]
// K = invI1 + invI2

void b2WeldJointDef::Initialize(b2Body* bA, b2Body* bB, const b2Vec<float, 2>& anchor)
{
    bodyA = bA;
    bodyB = bB;
    localAnchorA = bodyA->GetLocalPoint(anchor);
    localAnchorB = bodyB->GetLocalPoint(anchor);
    referenceAngle = bodyB->GetAngle() - bodyA->GetAngle();
}

b2WeldJoint::b2WeldJoint(const b2WeldJointDef* def) : b2Joint(def)
{
    m_localAnchorA = def->localAnchorA;
    m_localAnchorB = def->localAnchorB;
    m_referenceAngle = def->referenceAngle;
    m_frequencyHz = def->frequencyHz;
    m_dampingRatio = def->dampingRatio;

    m_impulse = {0.0f, 0.0f, 0.0f};
}

void b2WeldJoint::InitVelocityConstraints(const b2SolverData& data)
{
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterA = m_bodyA->m_sweep.localCenter;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassA = m_bodyA->m_invMass;
    m_invMassB = m_bodyB->m_invMass;
    m_invIA = m_bodyA->m_invI;
    m_invIB = m_bodyB->m_invI;

    float aA = data.positions[m_indexA].a;
    b2Vec<float, 2> vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    float aB = data.positions[m_indexB].a;
    b2Vec<float, 2> vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qA(aA), qB(aB);

    m_rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    m_rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

    // J = [-I -r1_skew I r2_skew]
    //     [ 0       -1 0       1]
    // r_skew = [-ry; rx]

    // Matlab
    // K = [ mA+r1y^2*iA+mB+r2y^2*iB,  -r1y*iA*r1x-r2y*iB*r2x,          -r1y*iA-r2y*iB]
    //     [  -r1y*iA*r1x-r2y*iB*r2x, mA+r1x^2*iA+mB+r2x^2*iB,           r1x*iA+r2x*iB]
    //     [          -r1y*iA-r2y*iB,           r1x*iA+r2x*iB,                   iA+iB]

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    b2Mat33 K;
    K.ex[b2VecX] = mA + mB + m_rA[b2VecY] * m_rA[b2VecY] * iA + m_rB[b2VecY] * m_rB[b2VecY] * iB;
    K.ey[b2VecX] = -m_rA[b2VecY] * m_rA[b2VecX] * iA - m_rB[b2VecY] * m_rB[b2VecX] * iB;
    K.ez[b2VecX] = -m_rA[b2VecY] * iA - m_rB[b2VecY] * iB;
    K.ex[b2VecY] = K.ey[b2VecX];
    K.ey[b2VecY] = mA + mB + m_rA[b2VecX] * m_rA[b2VecX] * iA + m_rB[b2VecX] * m_rB[b2VecX] * iB;
    K.ez[b2VecY] = m_rA[b2VecX] * iA + m_rB[b2VecX] * iB;
    K.ex[b2VecZ] = K.ez[b2VecX];
    K.ey[b2VecZ] = K.ez[b2VecY];
    K.ez[b2VecZ] = iA + iB;

    if (m_frequencyHz > 0.0f)
    {
        K.GetInverse22(&m_mass);

        float invM = iA + iB;
        float m = invM > 0.0f ? 1.0f / invM : 0.0f;

        float C = aB - aA - m_referenceAngle;

        // Frequency
        float omega = 2.0f * B2_PI * m_frequencyHz;

        // Damping coefficient
        float d = 2.0f * m * m_dampingRatio * omega;

        // Spring stiffness
        float k = m * omega * omega;

        // magic formulas
        float h = data.step.dt;
        m_gamma = h * (d + h * k);
        m_gamma = m_gamma != 0.0f ? 1.0f / m_gamma : 0.0f;
        m_bias = C * h * k * m_gamma;

        invM += m_gamma;
        m_mass.ez[b2VecZ] = invM != 0.0f ? 1.0f / invM : 0.0f;
    }
    else if (K.ez[b2VecZ] == 0.0f)
    {
        K.GetInverse22(&m_mass);
        m_gamma = 0.0f;
        m_bias = 0.0f;
    }
    else
    {
        K.GetSymInverse33(&m_mass);
        m_gamma = 0.0f;
        m_bias = 0.0f;
    }

    if (data.step.warmStarting)
    {
        // Scale impulses to support a variable time step.
        m_impulse *= data.step.dtRatio;

        b2Vec<float, 2> P{{m_impulse[b2VecX], m_impulse[b2VecY]}};

        vA -= mA * P;
        wA -= iA * (b2Cross(m_rA, P) + m_impulse[b2VecZ]);

        vB += mB * P;
        wB += iB * (b2Cross(m_rB, P) + m_impulse[b2VecZ]);
    }
    else
    {
        m_impulse = {0.0f, 0.0f, 0.0f};
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2WeldJoint::SolveVelocityConstraints(const b2SolverData& data)
{
    b2Vec<float, 2> vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec<float, 2> vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    if (m_frequencyHz > 0.0f)
    {
        float Cdot2 = wB - wA;

        float impulse2 = -m_mass.ez[b2VecZ] * (Cdot2 + m_bias + m_gamma * m_impulse[b2VecZ]);
        m_impulse[b2VecZ] += impulse2;

        wA -= iA * impulse2;
        wB += iB * impulse2;

        b2Vec<float, 2> Cdot1 = vB + b2Cross(wB, m_rB) - vA - b2Cross(wA, m_rA);

        b2Vec<float, 2> impulse1 = -b2Mul22(m_mass, Cdot1);
        m_impulse[b2VecX] += impulse1[b2VecX];
        m_impulse[b2VecY] += impulse1[b2VecY];

        b2Vec<float, 2> P = impulse1;

        vA -= mA * P;
        wA -= iA * b2Cross(m_rA, P);

        vB += mB * P;
        wB += iB * b2Cross(m_rB, P);
    }
    else
    {
        b2Vec<float, 2> Cdot1 = vB + b2Cross(wB, m_rB) - vA - b2Cross(wA, m_rA);
        float Cdot2 = wB - wA;
        b2Vec<float, 3> Cdot(Cdot1[b2VecX], Cdot1[b2VecY], Cdot2);

        b2Vec<float, 3> impulse = -b2Mul(m_mass, Cdot);
        m_impulse += impulse;

        b2Vec<float, 2> P{{impulse[b2VecX], impulse[b2VecY]}};

        vA -= mA * P;
        wA -= iA * (b2Cross(m_rA, P) + impulse[b2VecZ]);

        vB += mB * P;
        wB += iB * (b2Cross(m_rB, P) + impulse[b2VecZ]);
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

bool b2WeldJoint::SolvePositionConstraints(const b2SolverData& data)
{
    b2Vec<float, 2> cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec<float, 2> cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;

    b2Rot qA(aA), qB(aB);

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    b2Vec<float, 2> rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    b2Vec<float, 2> rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

    float positionError, angularError;

    b2Mat33 K;
    K.ex[b2VecX] = mA + mB + rA[b2VecY] * rA[b2VecY] * iA + rB[b2VecY] * rB[b2VecY] * iB;
    K.ey[b2VecX] = -rA[b2VecY] * rA[b2VecX] * iA - rB[b2VecY] * rB[b2VecX] * iB;
    K.ez[b2VecX] = -rA[b2VecY] * iA - rB[b2VecY] * iB;
    K.ex[b2VecY] = K.ey[b2VecX];
    K.ey[b2VecY] = mA + mB + rA[b2VecX] * rA[b2VecX] * iA + rB[b2VecX] * rB[b2VecX] * iB;
    K.ez[b2VecY] = rA[b2VecX] * iA + rB[b2VecX] * iB;
    K.ex[b2VecZ] = K.ez[b2VecX];
    K.ey[b2VecZ] = K.ez[b2VecY];
    K.ez[b2VecZ] = iA + iB;

    if (m_frequencyHz > 0.0f)
    {
        b2Vec<float, 2> C1 = cB + rB - cA - rA;

        positionError = C1.Length();
        angularError = 0.0f;

        b2Vec<float, 2> P = -K.Solve22(C1);

        cA -= mA * P;
        aA -= iA * b2Cross(rA, P);

        cB += mB * P;
        aB += iB * b2Cross(rB, P);
    }
    else
    {
        b2Vec<float, 2> C1 = cB + rB - cA - rA;
        float C2 = aB - aA - m_referenceAngle;

        positionError = C1.Length();
        angularError = std::abs(C2);

        b2Vec<float, 3> C(C1[b2VecX], C1[b2VecY], C2);

        b2Vec<float, 3> impulse;
        if (K.ez[b2VecZ] > 0.0f)
        {
            impulse = -K.Solve33(C);
        }
        else
        {
            b2Vec<float, 2> impulse2 = -K.Solve22(C1);
            impulse = {impulse2[b2VecX], impulse2[b2VecY], 0.0f};
        }

        b2Vec<float, 2> P{{impulse[b2VecX], impulse[b2VecY]}};

        cA -= mA * P;
        aA -= iA * (b2Cross(rA, P) + impulse[b2VecZ]);

        cB += mB * P;
        aB += iB * (b2Cross(rB, P) + impulse[b2VecZ]);
    }

    data.positions[m_indexA].c = cA;
    data.positions[m_indexA].a = aA;
    data.positions[m_indexB].c = cB;
    data.positions[m_indexB].a = aB;

    return positionError <= LINEAR_SLOP && angularError <= ANGULAR_SLOP;
}

b2Vec<float, 2> b2WeldJoint::GetAnchorA() const
{
    return m_bodyA->GetWorldPoint(m_localAnchorA);
}

b2Vec<float, 2> b2WeldJoint::GetAnchorB() const
{
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

b2Vec<float, 2> b2WeldJoint::GetReactionForce(float inv_dt) const
{
    b2Vec<float, 2> P{{m_impulse[b2VecX], m_impulse[b2VecY]}};
    return inv_dt * P;
}

float b2WeldJoint::GetReactionTorque(float inv_dt) const
{
    return inv_dt * m_impulse[b2VecZ];
}

void b2WeldJoint::Dump()
{
    int32_t indexA = m_bodyA->m_islandIndex;
    int32_t indexB = m_bodyB->m_islandIndex;

    b2Log("  b2WeldJointDef jd;\n");
    b2Log("  jd.bodyA = bodies[%d];\n", indexA);
    b2Log("  jd.bodyB = bodies[%d];\n", indexB);
    b2Log("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Log("  jd.localAnchorA.Set(%.15lef, %.15lef);\n", m_localAnchorA[b2VecX], m_localAnchorA[b2VecY]);
    b2Log("  jd.localAnchorB.Set(%.15lef, %.15lef);\n", m_localAnchorB[b2VecX], m_localAnchorB[b2VecY]);
    b2Log("  jd.referenceAngle = %.15lef;\n", m_referenceAngle);
    b2Log("  jd.frequencyHz = %.15lef;\n", m_frequencyHz);
    b2Log("  jd.dampingRatio = %.15lef;\n", m_dampingRatio);
    b2Log("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}
