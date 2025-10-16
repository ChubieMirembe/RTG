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

inline constexpr uint32_t RESTART_INDEX = 0xFFFFFFFFu;

inline MeshData createGridStrip(float width, float depth, uint32_t m, uint32_t n, glm::vec3 color) {
    MeshData md;
    md.vertices.reserve(m * n);
    md.indices.reserve((n - 1) * (2 * m + 1));

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
        for (uint32_t i = 0; i < m; ++i) {
            uint32_t top = j * m + i;
            uint32_t bot = (j + 1) * m + i;
            md.indices.push_back(top);
            md.indices.push_back(bot);
        }
        md.indices.push_back(RESTART_INDEX);
    }
    return md;
}

inline MeshData createCylinderStrip(float bottomR, float topR, float height,
                                    uint32_t sliceCount, uint32_t stackCount, glm::vec3 color) {
    MeshData md;

    float stackHeight = height / stackCount;
    float radiusStep = (topR - bottomR) / stackCount;

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

    uint32_t ring = sliceCount + 1;
    for (uint32_t i = 0; i < stackCount; ++i) {
        for (uint32_t j = 0; j <= sliceCount; ++j) {
            uint32_t i0 = i * ring + j;
            uint32_t i1 = (i + 1) * ring + j;
            md.indices.push_back(i0);
            md.indices.push_back(i1);
        }
        md.indices.push_back(RESTART_INDEX);
    }

    // Top cap as small strips (center, vj, vj+1)
    uint32_t topCenter = (uint32_t)md.vertices.size();
    md.vertices.push_back({ {0.0f, +0.5f * height, 0.0f}, color });
    uint32_t topRingStart = stackCount * ring;
    for (uint32_t j = 0; j < sliceCount; ++j) {
        md.indices.push_back(topCenter);
        md.indices.push_back(topRingStart + j);
        md.indices.push_back(topRingStart + j + 1);
        md.indices.push_back(RESTART_INDEX);
    }

    // Bottom cap
    uint32_t bottomCenter = (uint32_t)md.vertices.size();
    md.vertices.push_back({ {0.0f, -0.5f * height, 0.0f}, color });
    for (uint32_t j = 0; j < sliceCount; ++j) {
        md.indices.push_back(bottomCenter);
        md.indices.push_back(j + 1);
        md.indices.push_back(j);
        md.indices.push_back(RESTART_INDEX);
    }

    return md;
}

inline MeshData createSphereStrip(float r, uint32_t sliceCount, uint32_t stackCount, glm::vec3 color) {
    MeshData md;
    md.vertices.reserve((sliceCount + 1) * (stackCount - 1) + 2);

    md.vertices.push_back({ {0, +r, 0}, color }); // top

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

    md.vertices.push_back({ {0, -r, 0}, color }); // bottom

    uint32_t top = 0;
    uint32_t south = (uint32_t)md.vertices.size() - 1;
    uint32_t base = 1;
    uint32_t ring = sliceCount + 1;
    uint32_t interior = (stackCount >= 2 ? stackCount - 2 : 0);

    // top cap
    for (uint32_t j = 0; j < sliceCount; ++j) {
        md.indices.push_back(top);
        md.indices.push_back(base + j);
        md.indices.push_back(base + j + 1);
        md.indices.push_back(RESTART_INDEX);
    }

    // middle bands
    for (uint32_t i = 0; i + 1 < interior; ++i) {
        for (uint32_t j = 0; j <= sliceCount; ++j) {
            uint32_t i0 = base + i * ring + j;
            uint32_t i1 = i0 + ring;
            md.indices.push_back(i0);
            md.indices.push_back(i1);
        }
        md.indices.push_back(RESTART_INDEX);
    }

    // bottom cap
    if (interior > 0) {
        uint32_t lastRingStart = south - ring;
        for (uint32_t j = 0; j < sliceCount; ++j) {
            md.indices.push_back(south);
            md.indices.push_back(lastRingStart + j + 1);
            md.indices.push_back(lastRingStart + j);
            md.indices.push_back(RESTART_INDEX);
        }
    }

    return md;
}
