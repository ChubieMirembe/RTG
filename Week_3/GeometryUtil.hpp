#pragma once
#include <vector>
#include <cstdint>
#include <cmath>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

// -------- Grid (XZ plane, Y=0) --------
inline MeshData createGrid(float width, float depth, uint32_t m, uint32_t n, glm::vec3 color) {
    MeshData md;
    md.vertices.reserve(m * n);
    md.indices.reserve((m - 1) * (n - 1) * 6);

    const float dx = width / (m - 1);
    const float dz = depth / (n - 1);
    const float x0 = -width * 0.5f;
    const float z0 = -depth * 0.5f;

    for (uint32_t j = 0; j < n; ++j) {
        for (uint32_t i = 0; i < m; ++i) {
            float x = x0 + i * dx;
            float z = z0 + j * dz;
            md.vertices.push_back({ {x, 0.0f, z}, color });
        }
    }

    for (uint32_t j = 0; j < n - 1; ++j) {
        for (uint32_t i = 0; i < m - 1; ++i) {
            uint32_t i0 = j * m + i;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + m;
            uint32_t i3 = i2 + 1;
            md.indices.insert(md.indices.end(), { i0, i1, i3,  i3, i2, i0 });
        }
    }
    return md;
}

// -------- Terrain (same topology as grid, elevated Y) --------
inline MeshData createTerrain(float width, float depth, uint32_t m, uint32_t n, glm::vec3 color,
    float freqX = 1.0f, float freqZ = 1.0f, float amp = 0.5f) {
    MeshData md;
    md.vertices.reserve(m * n);
    md.indices.reserve((m - 1) * (n - 1) * 6);

    const float dx = width / (m - 1);
    const float dz = depth / (n - 1);
    const float x0 = -width * 0.5f;
    const float z0 = -depth * 0.5f;

    for (uint32_t j = 0; j < n; ++j) {
        for (uint32_t i = 0; i < m; ++i) {
            float x = x0 + i * dx;
            float z = z0 + j * dz;
            float y = std::sinf(freqX * x) * std::cosf(freqZ * z) * amp;  // wavy surface
            md.vertices.push_back({ {x, y, z}, color });
        }
    }

    for (uint32_t j = 0; j < n - 1; ++j) {
        for (uint32_t i = 0; i < m - 1; ++i) {
            uint32_t i0 = j * m + i;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + m;
            uint32_t i3 = i2 + 1;
            md.indices.insert(md.indices.end(), { i0, i1, i3,  i3, i2, i0 });
        }
    }
    return md;
}

// -------- Cylinder (top/bottom caps + side walls) --------
inline MeshData createCylinder(float bottomR, float topR, float height,
    uint32_t sliceCount, uint32_t stackCount,
    glm::vec3 color) {
    MeshData md;
    md.vertices.clear(); md.indices.clear();

    float stackHeight = height / stackCount;
    float radiusStep = (topR - bottomR) / stackCount;

    // side stacks
    for (uint32_t i = 0; i <= stackCount; ++i) {
        float y = -0.5f * height + i * stackHeight;
        float r = bottomR + i * radiusStep;
        for (uint32_t j = 0; j <= sliceCount; ++j) {
            float theta = j * 2.0f * 3.1415926535f / sliceCount;
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);
            md.vertices.push_back({ {x, y, z}, color });
        }
    }
    // indices for sides
    uint32_t ringCount = sliceCount + 1;
    for (uint32_t i = 0; i < stackCount; ++i) {
        for (uint32_t j = 0; j < sliceCount; ++j) {
            uint32_t i0 = i * ringCount + j;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = (i + 1) * ringCount + j;
            uint32_t i3 = i2 + 1;
            md.indices.insert(md.indices.end(), { i0, i1, i3,  i3, i2, i0 });
        }
    }

    // top cap
    {
        uint32_t baseIndex = (uint32_t)md.vertices.size();
        float y = +0.5f * height;
        for (uint32_t j = 0; j <= sliceCount; ++j) {
            float theta = j * 2.0f * 3.1415926535f / sliceCount;
            float x = topR * std::cos(theta);
            float z = topR * std::sin(theta);
            md.vertices.push_back({ {x, y, z}, color });
        }
        md.vertices.push_back({ {0.0f, y, 0.0f}, color }); // center
        uint32_t center = (uint32_t)md.vertices.size() - 1;
        for (uint32_t j = 0; j < sliceCount; ++j) {
            md.indices.insert(md.indices.end(), { center, baseIndex + j, baseIndex + j + 1 });
        }
    }

    // bottom cap
    {
        uint32_t baseIndex = (uint32_t)md.vertices.size();
        float y = -0.5f * height;
        for (uint32_t j = 0; j <= sliceCount; ++j) {
            float theta = j * 2.0f * 3.1415926535f / sliceCount;
            float x = bottomR * std::cos(theta);
            float z = bottomR * std::sin(theta);
            md.vertices.push_back({ {x, y, z}, color });
        }
        md.vertices.push_back({ {0.0f, y, 0.0f}, color }); // center
        uint32_t center = (uint32_t)md.vertices.size() - 1;
        for (uint32_t j = 0; j < sliceCount; ++j) {
            md.indices.insert(md.indices.end(), { center, baseIndex + j + 1, baseIndex + j });
        }
    }
    return md;
}

// -------- Sphere (latitude/longitude) --------
inline MeshData createSphere(float r, uint32_t sliceCount, uint32_t stackCount, glm::vec3 color) {
    MeshData md;
    md.vertices.reserve((sliceCount + 1) * (stackCount - 1) + 2);
    md.indices.reserve(sliceCount * 6 * (stackCount - 2) + sliceCount * 6);

    // top
    md.vertices.push_back({ {0, +r, 0}, color });

    for (uint32_t i = 1; i <= stackCount - 1; ++i) {
        float phi = 3.1415926535f * i / stackCount;
        float y = r * std::cos(phi);
        float s = r * std::sin(phi);
        for (uint32_t j = 0; j <= sliceCount; ++j) {
            float theta = 2.0f * 3.1415926535f * j / sliceCount;
            float x = s * std::cos(theta);
            float z = s * std::sin(theta);
            md.vertices.push_back({ {x, y, z}, color });
        }
    }

    // bottom
    md.vertices.push_back({ {0, -r, 0}, color });

    // top cap
    for (uint32_t j = 0; j < sliceCount; ++j) {
        md.indices.insert(md.indices.end(), { 0, 1 + j + 1, 1 + j });
    }

    // stacks
    uint32_t base = 1;
    uint32_t ring = sliceCount + 1;
    for (uint32_t i = 0; i < stackCount - 2; ++i) {
        for (uint32_t j = 0; j < sliceCount; ++j) {
            uint32_t i0 = base + i * ring + j;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + ring;
            uint32_t i3 = i2 + 1;
            md.indices.insert(md.indices.end(), { i0, i1, i3,  i3, i2, i0 });
        }
    }

    // bottom cap
    uint32_t south = (uint32_t)md.vertices.size() - 1;
    base = south - ring;
    for (uint32_t j = 0; j < sliceCount; ++j) {
        md.indices.insert(md.indices.end(), { south, base + j, base + j + 1 });
    }

    return md;
}
