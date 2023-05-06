#include "Dx11App.h"

#include <fstream>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <d3dcompiler.h>
#include <dxgidebug.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include "../helpers/helpers.h"


Dx11App::~Dx11App() {
    Cleanup();
}

HRESULT Dx11App::Init(HWND hWnd) {
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Create a device, device context, and swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    // TODO: Maybe use 11_1 and include fallback versions? 
    D3D_FEATURE_LEVEL FeatureLevels = D3D_FEATURE_LEVEL_11_0;
    // TODO: Maybe actually store the returned feature level, if you ever use an array of feature levels
    D3D_FEATURE_LEVEL FeatureLevel;
    UINT createDeviceFlags = 0;

#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        &FeatureLevels,
        1,
        D3D11_SDK_VERSION,
        &sd,
        &_swapChain,
        &_device,
        &FeatureLevel,
        &_context
    );

    if (FAILED(hr)) 
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (FAILED(hr)) 
        return hr;

    hr = _device->CreateRenderTargetView(pBackBuffer, nullptr, &_renderTarget);
    pBackBuffer->Release();

    if (FAILED(hr))
        return hr;

    _context->OMSetRenderTargets(1, &_renderTarget, nullptr);

    // Set the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _context->RSSetViewports(1, &vp);

    // Create the vertex buffer
    loadModel("Assets/teapot.obj");
    auto mesh = _meshes[0];

    // vertex buffer
    D3D11_BUFFER_DESC vertBufferDesc;
    ZeroMemory(&vertBufferDesc, sizeof(vertBufferDesc));
    vertBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertBufferDesc.ByteWidth = sizeof(Vertex) * mesh.numberOfVertices;
    vertBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertBufferDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA vertData;
    ZeroMemory(&vertData, sizeof(vertData));
    vertData.pSysMem = mesh.vertices.data();
    hr = _device->CreateBuffer(&vertBufferDesc, &vertData, &_vertexBuffer);

    if (FAILED(hr))
        return hr;

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);

    //index buffer
    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(uint32_t) * mesh.numberOfIndices; // numIndices should be the total number of indices in your .obj file.
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA indexData;
    indexData.pSysMem = mesh.indices.data();; // indices should be an array containing the indices from your .obj file.
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    hr = _device->CreateBuffer(&indexBufferDesc, &indexData, &_indexBuffer); // 'device' should be your ID3D11Device pointer, and 'indexBuffer' should be your ID3D11Buffer pointer.

    if (FAILED(hr)) 
        return hr;

    _context->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R32_UINT, 0); // 'deviceContext' should be your ID3D11DeviceContext pointer.

    // Set the primitive topology
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Load and create the vertex shader
    std::vector<char> vs = loadCompiledShader(L"VertexShader.hlsl.cso");
    hr = _device->CreateVertexShader(vs.data(), vs.size(), nullptr, &_vertexShader);

    if (FAILED(hr)) 
        return hr;

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
    hr = _device->CreateInputLayout(layout, numElements, vs.data(), vs.size(), &_vertexLayout);

    if (FAILED(hr))
        return hr;

    // Set the input layout
    _context->IASetInputLayout(_vertexLayout);

    // Load and create the pixel shader
    std::vector<char> ps = loadCompiledShader(L"PixelShader.hlsl.cso");
    hr = _device->CreatePixelShader(ps.data(), ps.size(), nullptr, &_pixelShader);

    if (FAILED(hr))
        return hr;

    // Set up the camera
    D3D11_BUFFER_DESC cameraBufferDesc;
    cameraBufferDesc.ByteWidth = sizeof(Camera);
    cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cameraBufferDesc.MiscFlags = 0;

    hr = _device->CreateBuffer(&cameraBufferDesc, nullptr, &_cameraBuffer);

    if (FAILED(hr))
        return hr;

    // Camera position
    DirectX::XMFLOAT3 cameraPosition(0.0f, 0.0f, 3.0f);
    DirectX::XMFLOAT3 cameraTarget(0.0f, 0.0f, 0.0f);
    DirectX::XMFLOAT3 cameraUp(0.0f, 1.0f, 0.0f);

    DirectX::XMVECTOR position = XMLoadFloat3(&cameraPosition);
    DirectX::XMVECTOR target = XMLoadFloat3(&cameraTarget);
    DirectX::XMVECTOR up = XMLoadFloat3(&cameraUp);

    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    // Field of view angle (in radians)
    float fovAngleY = 3.0f; // 120 degrees

    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(position, target, up);
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);

    Camera camera{ DirectX::XMMatrixTranspose(viewMatrix), DirectX::XMMatrixTranspose(projectionMatrix) };

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = _context->Map(_cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    if (FAILED(hr))
        return hr;

    Camera* pBuffer = reinterpret_cast<Camera*>(mappedResource.pData);
    *pBuffer = camera;
    _context->Unmap(_cameraBuffer, 0);

    _context->VSSetConstantBuffers(0, 1, &_cameraBuffer);

    // Create a rasterizer state description that culls front-facing triangles
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_FRONT;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.ScissorEnable = false;
    rasterDesc.MultisampleEnable = false;
    rasterDesc.AntialiasedLineEnable = false;

    // Create a rasterizer state object from the description
    ID3D11RasterizerState* rasterState = nullptr;
    hr = _device->CreateRasterizerState(&rasterDesc, &rasterState);

    if (FAILED(hr))
        return hr;

    // Set the rasterizer state object for the graphics pipeline
    _context->RSSetState(rasterState);

    return S_OK;
}


void Dx11App::Render() {
    // Clear the back buffer
    float clearColor[4] = { 0.392f, 0.584f, 0.929f, 1.0f };
    _context->ClearRenderTargetView(_renderTarget, clearColor);

    // Set the vertex and pixel shaders
    _context->VSSetShader(_vertexShader, nullptr, 0);
    _context->PSSetShader(_pixelShader, nullptr, 0);

    // Draw the triangle
    _context->DrawIndexed(_meshes[0].numberOfIndices, 0, 0);

    // Present the back buffer to the screen
    _swapChain->Present(0, 0);
}

void Dx11App::Cleanup() {

    if (_context)
        _context->ClearState();

    if (_vertexBuffer)
        _vertexBuffer->Release();

    if (_indexBuffer)
        _indexBuffer->Release();

    if (_cameraBuffer)
        _cameraBuffer->Release();

    if (_vertexLayout)
        _vertexLayout->Release();

    if (_vertexShader)
        _vertexShader->Release();

    if (_pixelShader)
        _pixelShader->Release();

    if (_renderTarget)
        _renderTarget->Release();

    if (_swapChain)
        _swapChain->Release();

    if (_context)
        _context->Release();

    if (_device)
        _device->Release();
}

std::vector<char> Dx11App::loadCompiledShader(const std::wstring& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open compiled shader file");
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

bool Dx11App::loadModel(const std::string& filePath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        
        // maybe write a convert method for this, if it comes up a lot
        std::string msg(importer.GetErrorString());
        std::vector<wchar_t> wideMessage(msg.begin(), msg.end());
        wideMessage.push_back(L'\0');

        MessageBox(nullptr, wideMessage.data(), L"Assimp Error", MB_OK | MB_ICONERROR);
        return false;
    }

    for (size_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
        aiMesh* aiMesh = scene->mMeshes[meshIndex];
        Mesh mesh;

        // get the vertices
        for (size_t vertexIndex = 0; vertexIndex < aiMesh->mNumVertices; ++vertexIndex) {
            Vertex vertex;
            vertex.Pos.x = aiMesh->mVertices[vertexIndex].x;
            vertex.Pos.y = aiMesh->mVertices[vertexIndex].y;
            vertex.Pos.z = aiMesh->mVertices[vertexIndex].z;
            vertex.Color.x = 0.949f;
            vertex.Color.y = 0.353f;
            vertex.Color.z = 0.114f;
            vertex.Color.w = 1.0f;
            mesh.vertices.push_back(vertex);
        }

        // get the indices
        for (size_t triangleIndex = 0; triangleIndex < aiMesh->mNumFaces; triangleIndex++) {
            // TODO: error check this, in case there aren't three indices
            mesh.indices.push_back(DirectX::XMUINT3{
                aiMesh->mFaces[triangleIndex].mIndices[0],
                aiMesh->mFaces[triangleIndex].mIndices[1],
                aiMesh->mFaces[triangleIndex].mIndices[2]
            });
        }

        // normalize the verts
        //DirectX::XMFLOAT3 minCoord(mesh.vertices[0].Pos.x, mesh.vertices[0].Pos.y, mesh.vertices[0].Pos.z);
        //DirectX::XMFLOAT3 maxCoord(mesh.vertices[0].Pos.x, mesh.vertices[0].Pos.y, mesh.vertices[0].Pos.z);

        //for (const auto& vertex : mesh.vertices) {
        //    minCoord.x = std::min(minCoord.x, vertex.Pos.x);
        //    minCoord.y = std::min(minCoord.y, vertex.Pos.y);
        //    minCoord.z = std::min(minCoord.z, vertex.Pos.z);

        //    maxCoord.x = std::max(maxCoord.x, vertex.Pos.x);
        //    maxCoord.y = std::max(maxCoord.y, vertex.Pos.y);
        //    maxCoord.z = std::max(maxCoord.z, vertex.Pos.z);
        //}

        //std::cout << "max: { " << maxCoord.x << ", " << maxCoord.y << ", " << maxCoord.z << "}" << std::endl;
        //std::cout << "min: { " << minCoord.x << ", " << minCoord.y << ", " << minCoord.z << "}" << std::endl;

        //DirectX::XMFLOAT3 center(
        //    (minCoord.x + maxCoord.x) / 2.0f,
        //    (minCoord.y + maxCoord.y) / 2.0f,
        //    (minCoord.z + maxCoord.z) / 2.0f
        //);

        //float maxRange = std::max({ maxCoord.x - minCoord.x, maxCoord.y - minCoord.y, maxCoord.z - minCoord.z });
        //float scaleFactor = 2.0f / maxRange;

        //std::cout << "max range: " << maxRange << std::endl;
        //std::cout << "scale factor: " << scaleFactor << std::endl;


        //for (auto& vertex : mesh.vertices) {
        //    vertex.Pos.x = (vertex.Pos.x - center.x) * scaleFactor;
        //    vertex.Pos.y = (vertex.Pos.y - center.y) * scaleFactor;
        //    vertex.Pos.z = ((vertex.Pos.z - center.z) * scaleFactor);
        //}

        mesh.numberOfVertices = mesh.vertices.size();
        mesh.numberOfIndices = mesh.indices.size() * 3;

        _meshes.push_back(mesh);
    }

    return true;
}