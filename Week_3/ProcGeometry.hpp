#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cmath>
#include <glm/glm.hpp>

// ============= Vertex used across the lab =============
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

namespace ProcGeometry {

    // A simple container for generated mesh data
    struct Mesh {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices; // VK_INDEX_TYPE_UINT32
    };

    // -------------------------------------------
    // EX1: Flat Grid (triangle-list, 2 tris/quad)
    // -------------------------------------------
    inline Mesh CreateGrid(int width, int depth, float cellSize, const glm::vec3& color = glm::vec3(1))
    {
        if (width < 1 || depth < 1) throw std::invalid_argument("CreateGrid: width/depth >= 1");

        Mesh m;
        const int vx = width + 1;
        const int vz = depth + 1;
        m.vertices.reserve(vx * vz);

        const float halfW = 0.5f * width * cellSize;
        const float halfD = 0.5f * depth * cellSize;

        // vertices (y=0 plane)
        for (int j = 0; j < vz; ++j) {
            for (int i = 0; i < vx; ++i) {
                float x = -halfW + i * cellSize;
                float z = -halfD + j * cellSize;
                m.vertices.push_back({ glm::vec3(x, 0.0f, z), color });
            }
        }

        // indices: two triangles per cell
        auto idx = [&](int ii, int jj) { return (uint32_t)(jj * vx + ii); };
        m.indices.reserve(width * depth * 6);
        for (int j = 0; j < depth; ++j) {
            for (int i = 0; i < width; ++i) {
                uint32_t i0 = idx(i, j);
                uint32_t i1 = idx(i + 1, j);
                uint32_t i2 = idx(i, j + 1);
                uint32_t i3 = idx(i + 1, j + 1);
                // (i0,i1,i2), (i1,i3,i2)
                m.indices.insert(m.indices.end(), { i0, i1, i2,  i1, i3, i2 });
            }
        }
        return m;
    }

    // -------------------------------------------
    // EX2: Wavy Terrain (same topology as grid)
    //    y = noise(x,z)  (plug any func you have)
    // -------------------------------------------
    template <typename FNoise>
    inline Mesh CreateTerrain(int width, int depth, float cellSize, FNoise noise, const glm::vec3& color = glm::vec3(1))
    {
        if (width < 1 || depth < 1) throw std::invalid_argument("CreateTerrain: width/depth >= 1");

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
                m.vertices.push_back({ glm::vec3(x, y, z), color });
            }
        }

        // same index pattern as grid (triangle list)
        auto idx = [&](int ii, int jj) { return (uint32_t)(jj * vx + ii); };
        m.indices.reserve(width * depth * 6);
        for (int j = 0; j < depth; ++j) {
            for (int i = 0; i < width; ++i) {
                uint32_t i0 = idx(i, j);
                uint32_t i1 = idx(i + 1, j);
                uint32_t i2 = idx(i, j + 1);
                uint32_t i3 = idx(i + 1, j + 1);
                m.indices.insert(m.indices.end(), { i0, i1, i2,  i1, i3, i2 });
            }
        }
        return m;
    }

    // ----------------------------------------------------------
    // EX3: Cylinder (top cap + wall + bottom cap) — triangle list
    // (You can switch to triangle strips + primitive restart later;
    //  triangle list keeps your Lab_Tutorial buffer code unchanged.)
    // ----------------------------------------------------------
    inline Mesh CreateCylinder(float radius, float height, uint32_t segments,
        const glm::vec3& colorBottom = glm::vec3(1, 0.4f, 0.4f),
        const glm::vec3& colorTop = glm::vec3(0.4f, 0.4f, 1))
    {
        if (segments < 3) segments = 3;

        Mesh m;
        const float y0 = -0.5f * height;
        const float y1 = 0.5f * height;
        const float TWO_PI = 6.28318530718f;

        // ring vertices
        std::vector<uint32_t> ringB, ringT;
        ringB.reserve(segments);
        ringT.reserve(segments);
        for (uint32_t i = 0; i < segments; ++i) {
            float a = TWO_PI * float(i) / float(segments);
            float c = std::cos(a), s = std::sin(a);

            ringB.push_back((uint32_t)m.vertices.size());
            m.vertices.push_back({ glm::vec3(radius * c, y0, radius * s), colorBottom });

            ringT.push_back((uint32_t)m.vertices.size());
            m.vertices.push_back({ glm::vec3(radius * c, y1, radius * s), colorTop });
        }

        // caps centers
        uint32_t bottomCenter = (uint32_t)m.vertices.size();
        m.vertices.push_back({ glm::vec3(0, y0, 0), colorBottom });

        uint32_t topCenter = (uint32_t)m.vertices.size();
        m.vertices.push_back({ glm::vec3(0, y1, 0), colorTop });

        auto wrap = [&](uint32_t i) { return i % segments; };

        // bottom cap: triangles (center, next, curr) for CCW seen from -Y
        for (uint32_t i = 0; i < segments; ++i) {
            uint32_t i0 = bottomCenter;
            uint32_t i1 = ringB[wrap(i + 1)];
            uint32_t i2 = ringB[wrap(i)];
            m.indices.insert(m.indices.end(), { i0, i1, i2 });
        }

        // wall: two tris per slice
        for (uint32_t i = 0; i < segments; ++i) {
            uint32_t b0 = ringB[wrap(i)];
            uint32_t b1 = ringB[wrap(i + 1)];
            uint32_t t0 = ringT[wrap(i)];
            uint32_t t1 = ringT[wrap(i + 1)];
            m.indices.insert(m.indices.end(), { b0, b1, t0,  b1, t1, t0 });
        }

        // top cap: triangles (center, curr, next) CCW seen from +Y
        for (uint32_t i = 0; i < segments; ++i) {
            uint32_t i0 = topCenter;
            uint32_t i1 = ringT[wrap(i)];
            uint32_t i2 = ringT[wrap(i + 1)];
            m.indices.insert(m.indices.end(), { i0, i1, i2 });
        }

        return m;
    }

} // namespace ProcGeometry
