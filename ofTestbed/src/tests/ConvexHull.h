/*
* Copyright (c) 2011 Erin Catto http://www.box2d.org
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

#ifndef CONVEX_HULL_H
#define CONVEX_HULL_H

constexpr int CONVEX_HULL_COUNT = MAX_POLYGON_VERTICES;

class ConvexHull : public Test
{
public:
    ConvexHull()
    {
        Generate();
        m_auto = false;
    }

    void Generate()
    {
        b2Vec2 lowerBound{{-8.0f, -8.0f}};
        b2Vec2 upperBound{{8.0f, 8.0f}};

        for (auto& elem : m_points)
        {
            float32 x = 10.0f * RandomFloat();
            float32 y = 10.0f * RandomFloat();

            // Clamp onto a square to help create collinearities.
            // This will stress the convex hull algorithm.
            b2Vec2 v{{x, y}};
            v = b2Clamp(v, lowerBound, upperBound);
            elem = v;
        }

        m_count = CONVEX_HULL_COUNT;
    }

    static Test* Create()
    {
        return new ConvexHull;
    }

    void Keyboard(int key) override
    {
        switch (key)
        {
            case 'a':
                m_auto = !m_auto;
                break;

            case 'g':
                Generate();
                break;
        }
    }

    void Step(Settings* settings) override
    {
        Test::Step(settings);

        b2PolygonShape shape;
        shape.Set(m_points, m_count);

        g_debugDraw.DrawString({{5.0f, static_cast<float>(m_textLine)}}, "Press g to generate a new random convex hull");
        m_textLine += DRAW_STRING_NEW_LINE;

        g_debugDraw.DrawPolygon(shape.GetVertices(), b2Color(0.9f, 0.9f, 0.9f));

        for (int32_t i = 0; i < m_count; ++i)
        {
            g_debugDraw.DrawPoint(m_points[i], 3.0f, b2Color(0.3f, 0.9f, 0.3f));
            g_debugDraw.DrawString(m_points[i] + b2Vec2{{0.05f, 0.05f}}, "%d", i);
        }

        if (shape.Validate() == false)
        {
            m_textLine += 0;
        }

        if (m_auto)
        {
            Generate();
        }
    }

    b2Vec2 m_points[MAX_POLYGON_VERTICES];
    int32_t m_count;
    bool m_auto;
};

#endif