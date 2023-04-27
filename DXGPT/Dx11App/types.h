#pragma once

#include <vector>

#include <DirectXMath.h>

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};



struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<DirectX::XMUINT3> indices;

    unsigned int numberOfVertices;
    unsigned int numberOfIndices;
};