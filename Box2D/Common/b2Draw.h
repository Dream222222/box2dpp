/*
* Copyright (c) 2011 Erin Catto http://box2d.org
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

#ifndef B2_DRAW_H
#define B2_DRAW_H

#include <Box2D/Common/b2Math.h>

#include <vector>

namespace box2d
{
/// Color for debug drawing. Each value has the range [0,1].
struct b2Color
{
    b2Color()
    {
    }
    b2Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a)
    {
    }
    void Set(float ri, float gi, float bi, float ai = 1.0f)
    {
        r = ri;
        g = gi;
        b = bi;
        a = ai;
    }
    float r, g, b, a;
};

/// Implement and register this class with a b2World to provide debug drawing of physics
/// entities in your game.
class b2Draw
{
public:
    b2Draw();

    virtual ~b2Draw()
    {
    }

    enum
    {
        e_shapeBit = 0x0001,        ///< draw shapes
        e_jointBit = 0x0002,        ///< draw joint connections
        e_aabbBit = 0x0004,         ///< draw axis aligned bounding boxes
        e_pairBit = 0x0008,         ///< draw broad-phase pairs
        e_centerOfMassBit = 0x0010  ///< draw center of mass frame
    };

    /// Set the drawing flags.
    void SetFlags(uint32_t flags);

    /// Get the drawing flags.
    uint32_t GetFlags() const;

    /// Append flags to the current flags.
    void AppendFlags(uint32_t flags);

    /// Clear flags from the current flags.
    void ClearFlags(uint32_t flags);

    /// Draw a closed polygon provided in CCW order.
    virtual void DrawPolygon(const std::vector<b2Vec<float, 2>>& vertices, const b2Color& color) = 0;

    /// Draw a solid closed polygon provided in CCW order.
    virtual void DrawSolidPolygon(const std::vector<b2Vec<float, 2>>& vertices,
                                  const b2Color& color) = 0;

    /// Draw a circle.
    virtual void DrawCircle(const b2Vec<float, 2>& center, float radius, const b2Color& color) = 0;

    /// Draw a solid circle.
    virtual void DrawSolidCircle(const b2Vec<float, 2>& center, float radius, const b2Vec<float, 2>& axis,
                                 const b2Color& color) = 0;

    /// Draw a line segment.
    virtual void DrawSegment(const b2Vec<float, 2>& p1, const b2Vec<float, 2>& p2, const b2Color& color) = 0;

    /// Draw a transform. Choose your own length scale.
    /// @param xf a transform.
    virtual void DrawTransform(const b2Transform& xf) = 0;

protected:
    uint32_t m_drawFlags;
};
}

#endif
