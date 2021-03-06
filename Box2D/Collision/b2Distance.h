
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

#ifndef B2_DISTANCE_H
#define B2_DISTANCE_H

#include <Box2D/Common/b2Math.h>
#include <vector>

namespace box2d
{
class b2Shape;

struct b2GJKState
{
    static int32_t b2_gjkCalls;
    static int32_t b2_gjkIters;
    static int32_t b2_gjkMaxIters;
};
/// A distance proxy is used by the GJK algorithm.
/// It encapsulates any shape.
struct b2DistanceProxy
{
    b2DistanceProxy() : m_radius{}
    {
    }

    /// Initialize the proxy using the given shape. The shape
    /// must remain in scope while the proxy is in use.
    void Set(const b2Shape* shape, int32_t index);

    /// Get the supporting vertex index in the given direction.
    std::size_t GetSupport(const b2Vec<float, 2>& d) const;

    /// Get the supporting vertex in the given direction.
    const b2Vec<float, 2>& GetSupportVertex(const b2Vec<float, 2>& d) const;

    /// Get the vertex count.
    std::size_t GetVertexCount() const;

    /// Get a vertex by index. Used by b2Distance.
    const b2Vec<float, 2>& GetVertex(std::size_t index) const;

    std::vector<b2Vec<float, 2>> m_vertices;
    float m_radius;
};

/// Used to warm start b2Distance.
/// Set count to zero on first call.
struct b2SimplexCache
{
    float metric;  ///< length or area
    uint16_t count;
    uint8_t indexA[3];  ///< vertices on shape A
    uint8_t indexB[3];  ///< vertices on shape B
};

/// Input for b2Distance.
/// You have to option to use the shape radii
/// in the computation. Even
struct b2DistanceInput
{
    b2DistanceProxy proxyA;
    b2DistanceProxy proxyB;
    b2Transform transformA;
    b2Transform transformB;
    bool useRadii;
};

/// Output for b2Distance.
struct b2DistanceOutput
{
    b2Vec<float, 2> pointA;  ///< closest point on shapeA
    b2Vec<float, 2> pointB;  ///< closest point on shapeB
    float distance;
    int32_t iterations;  ///< number of GJK iterations used
};

/// Compute the closest points between two shapes. Supports any combination of:
/// b2CircleShape, b2PolygonShape, b2EdgeShape. The simplex cache is
/// input/output.
/// On the first call set b2SimplexCache.count to zero.
void b2Distance(b2DistanceOutput* output, b2SimplexCache* cache, const b2DistanceInput* input);

//////////////////////////////////////////////////////////////////////////

inline std::size_t b2DistanceProxy::GetVertexCount() const
{
    return m_vertices.size();
}

inline const b2Vec<float, 2>& b2DistanceProxy::GetVertex(std::size_t index) const
{
    return m_vertices[index];
}

inline std::size_t b2DistanceProxy::GetSupport(const b2Vec<float, 2>& d) const
{
    std::size_t bestIndex = 0;
    auto bestValue = b2Dot(m_vertices[0], d);
    for (std::size_t i = 1; i < m_vertices.size(); ++i)
    {
        float value = b2Dot(m_vertices[i], d);
        if (value > bestValue)
        {
            bestIndex = i;
            bestValue = value;
        }
    }

    return bestIndex;
}

inline const b2Vec<float, 2>& b2DistanceProxy::GetSupportVertex(const b2Vec<float, 2>& d) const
{
    const b2Vec<float, 2>* bestVec = &m_vertices.front();
    auto bestValue = b2Dot(m_vertices[0], d);
    for (const auto& vertex : m_vertices)
    {
        auto value = b2Dot(vertex, d);
        if (value > bestValue)
        {
            bestVec = &vertex;
            bestValue = value;
        }
    }

    return *bestVec;
}
}

#endif
