/*
* Copyright (c) 2006-2010 Erin Catto http://www.box2d.org
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

#ifndef CHARACTER_COLLISION_H
#define CHARACTER_COLLISION_H

/// This is a test of typical character collision scenarios. This does not
/// show how you should implement a character in your application.
/// Instead this is used to test smooth collision on edge chains.
class CharacterCollision : public Test
{
public:
    CharacterCollision()
    {
        // Ground body
        {
            b2BodyDef bd;
            b2Body* ground = m_world->CreateBody(&bd);

            b2EdgeShape shape;
            shape.Set({{-20.0f, 0.0f}}, {{20.0f, 0.0f}});
            ground->CreateFixture(&shape, 0.0f);
        }

        // Collinear edges with no adjacency information.
        // This shows the problematic case where a box shape can hit
        // an internal vertex.
        {
            b2BodyDef bd;
            b2Body* ground = m_world->CreateBody(&bd);

            b2EdgeShape shape;
            shape.Set({{-8.0f, 1.0f}}, {{-6.0f, 1.0f}});
            ground->CreateFixture(&shape, 0.0f);
            shape.Set({{-6.0f, 1.0f}}, {{-4.0f, 1.0f}});
            ground->CreateFixture(&shape, 0.0f);
            shape.Set({{-4.0f, 1.0f}}, {{-2.0f, 1.0f}});
            ground->CreateFixture(&shape, 0.0f);
        }

        // Chain shape
        {
            b2BodyDef bd;
            bd.angle = 0.25f * B2_PI;
            b2Body* ground = m_world->CreateBody(&bd);

            b2Vec2 vs[4];
            vs[0] = {{5.0f, 7.0f}};
            vs[1] = {{6.0f, 8.0f}};
            vs[2] = {{7.0f, 8.0f}};
            vs[3] = {{8.0f, 7.0f}};
            b2ChainShape shape;
            shape.CreateChain(vs, 4);
            ground->CreateFixture(&shape, 0.0f);
        }

        // Square tiles. This shows that adjacency shapes may
        // have non-smooth collision. There is no solution
        // to this problem.
        {
            b2BodyDef bd;
            b2Body* ground = m_world->CreateBody(&bd);

            b2PolygonShape shape;
            shape.SetAsBox(1.0f, 1.0f, {{4.0f, 3.0f}}, 0.0f);
            ground->CreateFixture(&shape, 0.0f);
            shape.SetAsBox(1.0f, 1.0f, {{6.0f, 3.0f}}, 0.0f);
            ground->CreateFixture(&shape, 0.0f);
            shape.SetAsBox(1.0f, 1.0f, {{8.0f, 3.0f}}, 0.0f);
            ground->CreateFixture(&shape, 0.0f);
        }

        // Square made from an edge loop. Collision should be smooth.
        {
            b2BodyDef bd;
            b2Body* ground = m_world->CreateBody(&bd);

            b2Vec2 vs[4];
            vs[0] = {{-1.0f, 3.0f}};
            vs[1] = {{1.0f, 3.0f}};
            vs[2] = {{1.0f, 5.0f}};
            vs[3] = {{-1.0f, 5.0f}};
            b2ChainShape shape;
            shape.CreateLoop(vs, 4);
            ground->CreateFixture(&shape, 0.0f);
        }

        // Edge loop. Collision should be smooth.
        {
            b2BodyDef bd;
            bd.position = {{10.0f, 4.0f}};
            b2Body* ground = m_world->CreateBody(&bd);

            b2Vec2 vs[10];
            vs[0] = {{0.0f, 0.0f}};
            vs[1] = {{6.0f, 0.0f}};
            vs[2] = {{6.0f, 2.0f}};
            vs[3] = {{4.0f, 1.0f}};
            vs[4] = {{2.0f, 2.0f}};
            vs[5] = {{0.0f, 2.0f}};
            vs[6] = {{-2.0f, 2.0f}};
            vs[7] = {{-4.0f, 3.0f}};
            vs[8] = {{-6.0f, 2.0f}};
            vs[9] = {{-6.0f, 0.0f}};
            b2ChainShape shape;
            shape.CreateLoop(vs, 10);
            ground->CreateFixture(&shape, 0.0f);
        }

        // Square character 1
        {
            b2BodyDef bd;
            bd.position = {{3.0f, 8.0f}};
            bd.type = b2BodyType::DYNAMIC_BODY;
            bd.fixedRotation = true;
            bd.allowSleep = false;

            b2Body* body = m_world->CreateBody(&bd);

            b2PolygonShape shape;
            shape.SetAsBox(0.5f, 0.5f);

            b2FixtureDef fd;
            fd.shape = &shape;
            fd.density = 20.0f;
            body->CreateFixture(&fd);
        }

        // Square character 2
        {
            b2BodyDef bd;
            bd.position = {{5.0f, 5.0f}};
            bd.type = b2BodyType::DYNAMIC_BODY;
            bd.fixedRotation = true;
            bd.allowSleep = false;

            b2Body* body = m_world->CreateBody(&bd);

            b2PolygonShape shape;
            shape.SetAsBox(0.25f, 0.25f);

            b2FixtureDef fd;
            fd.shape = &shape;
            fd.density = 20.0f;
            body->CreateFixture(&fd);
        }

        // Hexagon character
        {
            b2BodyDef bd;
            bd.position = {{5.0f, 8.0f}};
            bd.type = b2BodyType::DYNAMIC_BODY;
            bd.fixedRotation = true;
            bd.allowSleep = false;

            b2Body* body = m_world->CreateBody(&bd);

            float32 angle = 0.0f;
            float32 delta = B2_PI / 3.0f;
            b2Vec2 vertices[6];
            for (auto& vertice : vertices)
            {
                vertice = {{0.5f * cosf(angle), 0.5f * sinf(angle)}};
                angle += delta;
            }

            b2PolygonShape shape;
            shape.Set(vertices, 6);

            b2FixtureDef fd;
            fd.shape = &shape;
            fd.density = 20.0f;
            body->CreateFixture(&fd);
        }

        // Circle character
        {
            b2BodyDef bd;
            bd.position = {{3.0f, 5.0f}};
            bd.type = b2BodyType::DYNAMIC_BODY;
            bd.fixedRotation = true;
            bd.allowSleep = false;

            b2Body* body = m_world->CreateBody(&bd);

            b2CircleShape shape;
            shape.SetRadius( 0.5f);

            b2FixtureDef fd;
            fd.shape = &shape;
            fd.density = 20.0f;
            body->CreateFixture(&fd);
        }

        // Circle character
        {
            b2BodyDef bd;
            bd.position = {{7.0f, 6.0f}};
            bd.type = b2BodyType::DYNAMIC_BODY;
            bd.allowSleep = false;

            m_character = m_world->CreateBody(&bd);

            b2CircleShape shape;
            shape.SetRadius( 0.25f);

            b2FixtureDef fd;
            fd.shape = &shape;
            fd.density = 20.0f;
            fd.friction = 1.0f;
            m_character->CreateFixture(&fd);
        }
    }

    void Step(Settings* settings) override
    {
        b2Vec2 v = m_character->GetLinearVelocity();
        v[b2VecX] = -5.0f;
        m_character->SetLinearVelocity(v);

        Test::Step(settings);
        g_debugDraw.DrawString({{5.0f, static_cast<float>(m_textLine)}}, "This tests various character collision shapes.");
        m_textLine += DRAW_STRING_NEW_LINE;
        g_debugDraw.DrawString({{5.0f, static_cast<float>(m_textLine)}},
                               "Limitation: square and hexagon can snag on aligned boxes.");
        m_textLine += DRAW_STRING_NEW_LINE;
        g_debugDraw.DrawString({{5.0f, static_cast<float>(m_textLine)}},
                               "Feature: edge chains have smooth collision inside and out.");
        m_textLine += DRAW_STRING_NEW_LINE;
    }

    static Test* Create()
    {
        return new CharacterCollision;
    }

    b2Body* m_character;
};

#endif
