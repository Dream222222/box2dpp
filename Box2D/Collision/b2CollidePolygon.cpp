/*
* Copyright (c) 2006-2009 Erin Catto http://www.box2d.org
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

#include <Box2D/Collision/b2Collision.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>

#include <array>
#include <iostream>

using namespace box2d;

// Find the max separation between poly1 and poly2 using edge normals from
// poly1.
float box2d::b2FindMaxSeparation(int32_t* edgeIndex, const b2PolygonShape* poly1,
                                   const b2Transform& xf1, const b2PolygonShape* poly2,
                                   const b2Transform& xf2)
{
    auto n1s = poly1->GetNormals();
    auto v1s = poly1->GetVertices();
    auto v2s = poly2->GetVertices();
    auto xf = b2MulT(xf2, xf1);

    int32_t bestIndex = 0;
    float maxSeparation = -MAX_FLOAT;
    for (std::size_t i = 0; i < v1s.size(); ++i)
    {
        // Get poly1 normal in frame2.
        b2Vec<float, 2> n = b2Mul(xf.q, n1s[i]);
        b2Vec<float, 2> v1 = b2Mul(xf, v1s[i]);

        // Find deepest point for normal i.
        float si = MAX_FLOAT;
        for (std::size_t j = 0; j < v2s.size(); ++j)
        {
            float sij = b2Dot(n, v2s[j] - v1);
            if (sij < si)
            {
                si = sij;
            }
        }

        if (si > maxSeparation)
        {
            maxSeparation = si;
            bestIndex = i;
        }
    }

    *edgeIndex = bestIndex;
    return maxSeparation;
}

void box2d::b2FindIncidentEdge(std::array<b2ClipVertex, 2>& c, const b2PolygonShape* poly1,
                               const b2Transform& xf1, int32_t edge1, const b2PolygonShape* poly2,
                               const b2Transform& xf2)
{
    auto normals1 = poly1->GetNormals();

    auto vertices2 = poly2->GetVertices();
    auto normals2 = poly2->GetNormals();

    b2Assert(0 <= edge1 && edge1 < poly1->GetVertexCount());

    // Get the normal of the reference edge in poly2's frame.
    b2Vec<float, 2> normal1 = b2MulT(xf2.q, b2Mul(xf1.q, normals1[edge1]));

    // Find the incident edge on poly2.
    std::size_t index = 0;
    float minDot = MAX_FLOAT;
    for (std::size_t i = 0; i < normals2.size(); ++i)
    {
        float dot = b2Dot(normal1, normals2[i]);
        if (dot < minDot)
        {
            minDot = dot;
            index = i;
        }
    }

    // Build the clip vertices for the incident edge.
    auto i1 = index;
    auto i2 = i1 + 1 < normals2.size() ? i1 + 1 : 0;

    c[0].v = b2Mul(xf2, vertices2[i1]);
    c[0].id.cf.indexA = (uint8_t)edge1;
    c[0].id.cf.indexB = (uint8_t)i1;
    c[0].id.cf.typeA = b2ContactFeature::e_face;
    c[0].id.cf.typeB = b2ContactFeature::e_vertex;

    c[1].v = b2Mul(xf2, vertices2[i2]);
    c[1].id.cf.indexA = (uint8_t)edge1;
    c[1].id.cf.indexB = (uint8_t)i2;
    c[1].id.cf.typeA = b2ContactFeature::e_face;
    c[1].id.cf.typeB = b2ContactFeature::e_vertex;
}

// Find edge normal of max separation on A - return if separating axis is found
// Find edge normal of max separation on B - return if separation axis is found
// Choose reference edge as min(minA, minB)
// Find incident edge
// Clip

// The normal points from 1 to 2
void box2d::b2CollidePolygons(b2Manifold* manifold, const b2PolygonShape* polyA,
                              const b2Transform& xfA, const b2PolygonShape* polyB,
                              const b2Transform& xfB)
{
    manifold->pointCount = 0;
    float totalRadius = polyA->GetRadius() + polyB->GetRadius();

    int32_t edgeA = 0;
    float separationA = b2FindMaxSeparation(&edgeA, polyA, xfA, polyB, xfB);
    if (separationA > totalRadius)
        return;

    int32_t edgeB = 0;
    float separationB = b2FindMaxSeparation(&edgeB, polyB, xfB, polyA, xfA);
    if (separationB > totalRadius)
        return;

    const b2PolygonShape* poly1;  // reference polygon
    const b2PolygonShape* poly2;  // incident polygon
    b2Transform xf1, xf2;
    std::size_t edge1;  // reference edge
    uint8_t flip;
    const float k_tol = 0.1f * LINEAR_SLOP;
    
    if (separationB > separationA + k_tol)
    {
        poly1 = polyB;
        poly2 = polyA;
        xf1 = xfB;
        xf2 = xfA;
        edge1 = edgeB;
        manifold->type = b2Manifold::e_faceB;
        flip = 1;
    }
    else
    {
        poly1 = polyA;
        poly2 = polyB;
        xf1 = xfA;
        xf2 = xfB;
        edge1 = edgeA;
        manifold->type = b2Manifold::e_faceA;
        flip = 0;
    }

    std::array<b2ClipVertex, 2> incidentEdge;
    b2FindIncidentEdge(incidentEdge, poly1, xf1, edge1, poly2, xf2);

    auto vertices1 = poly1->GetVertices();

    auto iv1 = edge1;
    auto iv2 = edge1 + 1 < vertices1.size() ? edge1 + 1 : 0;

    b2Vec<float, 2> v11 = vertices1[iv1];
    b2Vec<float, 2> v12 = vertices1[iv2];

    b2Vec<float, 2> localTangent = v12 - v11;
    localTangent.Normalize();
    
    b2Vec<float, 2> localNormal = b2Cross(localTangent, 1.0f);
    b2Vec<float, 2> planePoint = 0.5f * (v11 + v12);

    b2Vec<float, 2> tangent = b2Mul(xf1.q, localTangent);
    b2Vec<float, 2> normal = b2Cross(tangent, 1.0f);

    v11 = b2Mul(xf1, v11);
    v12 = b2Mul(xf1, v12);

    // Face offset.
    float frontOffset = b2Dot(normal, v11);

    // Side offsets, extended by polytope skin thickness.
    float sideOffset1 = -b2Dot(tangent, v11) + totalRadius;
    float sideOffset2 = b2Dot(tangent, v12) + totalRadius;

    // Clip incident edge against extruded edge1 side edges.
    std::array<b2ClipVertex, 2> clipPoints1;
    std::array<b2ClipVertex, 2> clipPoints2;
    
    // Clip to box side 1
    auto np = b2ClipSegmentToLine(clipPoints1, incidentEdge, -tangent, sideOffset1, iv1);

    if (np < 2)
    {
        return;
    }

    // Clip to negative box side 1
    np = b2ClipSegmentToLine(clipPoints2, clipPoints1, tangent, sideOffset2, iv2);

    if (np < 2)
    {
        return;
    }

    // Now clipPoints2 contains the clipped points.
    manifold->localNormal = localNormal;
    manifold->localPoint = planePoint;
    
    int32_t pointCount = 0;
    for (auto& elem : clipPoints2)
    {
        float separation = b2Dot(normal, elem.v) - frontOffset;

        if (separation <= totalRadius)
        {
            b2ManifoldPoint* cp = manifold->points + pointCount;
            cp->localPoint = b2MulT(xf2, elem.v);
            cp->id = elem.id;
            if (flip)
            {
                // Swap features
                b2ContactFeature cf = cp->id.cf;
                cp->id.cf.indexA = cf.indexB;
                cp->id.cf.indexB = cf.indexA;
                cp->id.cf.typeA = cf.typeB;
                cp->id.cf.typeB = cf.typeA;
            }
            ++pointCount;
        }
    }

    manifold->pointCount = pointCount;
}
