#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// =======================
// Single source of truth:
// =======================
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

namespace Geometry {

    struct Mesh {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices; // Assumes VK_INDEX_TYPE_UINT32
    };

    inline void tint(Mesh& m, const glm::vec3& color) {
        for (auto& v : m.vertices) v.color = color;
    }

    // Append 'src' into destination vectors with an optional transform.
    // Inserts a primitive-restart cut first (if dstIndices already has content),
    // so all parts can be drawn together with TRIANGLE_STRIP + primitiveRestart.
    inline void appendMeshWithTransform(
        const Mesh& src,
        const glm::mat4& transform,
        std::vector<Vertex>& dstVertices,
        std::vector<uint32_t>& dstIndices,
        bool insertRestartBetweenMeshes = true)
    {
        const uint32_t RESTART = 0xFFFFFFFFu;
        const uint32_t base = static_cast<uint32_t>(dstVertices.size());

        if (insertRestartBetweenMeshes && !dstIndices.empty())
            dstIndices.push_back(RESTART);

        dstVertices.reserve(dstVertices.size() + src.vertices.size());
        for (const auto& v : src.vertices) {
            glm::vec4 p = transform * glm::vec4(v.pos, 1.0f);
            dstVertices.push_back(Vertex{ glm::vec3(p), v.color });
        }

        dstIndices.reserve(dstIndices.size() + src.indices.size());
        for (auto idx : src.indices) {
            if (idx == 0xFFFFFFFFu) dstIndices.push_back(idx); // preserve restart
            else dstIndices.push_back(base + idx);
        }
    }

    // ---------------------
    // Grid (triangle strips)
    // ---------------------
    inline Mesh createGridStrip(int width, int depth, float cellSize)
    {
        if (width < 1 || depth < 1) throw std::invalid_argument("createGridStrip: width,depth >= 1");

        Mesh m;
        const int vx = width + 1;
        const int vz = depth + 1;
        m.vertices.reserve(vx * vz);

        const float halfW = 0.5f * width * cellSize;
        const float halfD = 0.5f * depth * cellSize;

        for (int j = 0; j < vz; ++j) {
            for (int i = 0; i < vx; ++i) {
                float x = -halfW + i * cellSize;
                float z = -halfD + j * cellSize;
                m.vertices.push_back({ glm::vec3(x, 0.f, z), glm::vec3(1.f) });
            }
        }

        const uint32_t RESTART = 0xFFFFFFFFu;
        auto idx = [&](int ii, int jj) { return static_cast<uint32_t>(jj * vx + ii); };

        for (int j = 0; j < depth; ++j) {
            for (int i = 0; i <= width; ++i) {
                m.indices.push_back(idx(i, j));
                m.indices.push_back(idx(i, j + 1));
            }
            m.indices.push_back(RESTART);
        }
        return m;
    }

    // -----------------------
    // Terrain (triangle strips)
    // Pass a noise func: y = f(x,z)
    // -----------------------
    template <typename FNoise>
    inline Mesh createTerrainStrip(int width, int depth, float cellSize, FNoise noise)
    {
        if (width < 1 || depth < 1) throw std::invalid_argument("createTerrainStrip: width,depth >= 1");

        Mesh m;
        const int vx = width + 1;
        const int vz = depth + 1;
        m.vertices.reserve(vx * vz);

        const float halfW = 0.5f * width * cellSize;
        const float halfD = 0.5f * depth * cellSize;

        for (int j = 0; j < vz; ++j) {
            for (int i = 0; i < vx; ++i) {
                float x = -halfW + i * cellSize;
                float z = -halfD + j * cellSize;
                float y = noise(x, z);
                m.vertices.push_back({ glm::vec3(x, y, z), glm::vec3(1.f) });
            }
        }

        const uint32_t RESTART = 0xFFFFFFFFu;
        auto idx = [&](int ii, int jj) { return static_cast<uint32_t>(jj * vx + ii); };

        for (int j = 0; j < depth; ++j) {
            for (int i = 0; i <= width; ++i) {
                m.indices.push_back(idx(i, j));
                m.indices.push_back(idx(i, j + 1));
            }
            m.indices.push_back(RESTART);
        }
        return m;
    }

    // -------------------------
    // Cylinder (caps + wall) —
    // all triangle strips with restart
    // -------------------------
    inline Mesh createCylinderStrip(float radius, float height, uint32_t segments,
        const glm::vec3& colorBottom, const glm::vec3& colorTop)
    {
        if (segments < 3) segments = 3;

        Mesh m;
        const float y0 = -0.5f * height;
        const float y1 = 0.5f * height;
        const float TWO_PI = 6.28318530718f;
        const uint32_t RESTART = 0xFFFFFFFFu;

        // Interleaved ring: (B0,T0, B1,T1, ..., B{n-1},T{n-1})
        for (uint32_t i = 0; i < segments; ++i) {
            float a = TWO_PI * float(i) / float(segments);
            float c = std::cos(a), s = std::sin(a);
            m.vertices.push_back({ glm::vec3(radius * c, y0, radius * s), colorBottom }); // Bi
            m.vertices.push_back({ glm::vec3(radius * c, y1, radius * s), colorTop }); // Ti
        }

        const uint32_t bottomCenter = static_cast<uint32_t>(m.vertices.size());
        m.vertices.push_back({ glm::vec3(0.f, y0, 0.f), colorBottom });

        const uint32_t topCenter = static_cast<uint32_t>(m.vertices.size());
        m.vertices.push_back({ glm::vec3(0.f, y1, 0.f), colorTop });

        auto idxB = [&](uint32_t i) { return 2u * (i % segments) + 0u; };
        auto idxT = [&](uint32_t i) { return 2u * (i % segments) + 1u; };

        // Bottom cap: (center, next, current)
        for (uint32_t i = 0; i < segments; ++i) {
            m.indices.push_back(bottomCenter);
            m.indices.push_back(idxB(i + 1));
            m.indices.push_back(idxB(i));
            m.indices.push_back(RESTART);
        }

        // Wall: B0,T0, B1,T1, ..., Bseg,Tseg
        for (uint32_t i = 0; i <= segments; ++i) {
            m.indices.push_back(idxB(i));
            m.indices.push_back(idxT(i));
        }
        m.indices.push_back(RESTART);

        // Top cap: (center, current, next)
        for (uint32_t i = 0; i < segments; ++i) {
            m.indices.push_back(topCenter);
            m.indices.push_back(idxT(i));
            m.indices.push_back(idxT(i + 1));
            m.indices.push_back(RESTART);
        }

        return m;
    }

} // namespace Geometry
