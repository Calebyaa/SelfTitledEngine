#include "Dx11App.h"

#include <fstream>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <d3dcompiler.h>
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

    hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        &FeatureLevels,
        1,
        D3D11_SDK_VERSION,
        &sd,
        &_swapChain,
        &_device,
        &FeatureLevel,
        &_context
    );

    if (FAILED(hr)) {
        MessageBox(nullptr, helpers::GetErrorMessageFromHRESULT(hr).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return hr;
    }

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) {
        MessageBox(nullptr, helpers::GetErrorMessageFromHRESULT(hr).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return hr;
    }

    hr = _device->CreateRenderTargetView(pBackBuffer, nullptr, &_renderTarget);
    pBackBuffer->Release();
    if (FAILED(hr)) {
        MessageBox(nullptr, helpers::GetErrorMessageFromHRESULT(hr).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return hr;
    }

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
    std::vector<Mesh> meshes;
    loadModel("Assets/teapot.obj", meshes);
    //Vertex vertices[] =
    //{
    //    { DirectX::XMFLOAT3(0.0f, 0.5f, 0.5f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
    //    { DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
    //    { DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }
    //};

    auto mesh = meshes[0];

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = mesh.vertices.data();
    hr = _device->CreateBuffer(&bd, &InitData, &_vertexBuffer);
    if (FAILED(hr)) {
        MessageBox(nullptr, helpers::GetErrorMessageFromHRESULT(hr).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return hr;
    }

    // Set the vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);

    // Set the primitive topology
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Load and create the vertex shader
    std::vector<char> vs = loadCompiledShader(L"VertexShader.hlsl.cso");
    hr = _device->CreateVertexShader(vs.data(), vs.size(), nullptr, &_vertexShader);
    if (FAILED(hr)) {
        MessageBox(nullptr, helpers::GetErrorMessageFromHRESULT(hr).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return hr;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
    hr = _device->CreateInputLayout(layout, numElements, vs.data(), vs.size(), &_vertexLayout);
    if (FAILED(hr)) {
        MessageBox(nullptr, helpers::GetErrorMessageFromHRESULT(hr).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return hr;
    }

    // Set the input layout
    _context->IASetInputLayout(_vertexLayout);

    // Load and create the pixel shader
    std::vector<char> ps = loadCompiledShader(L"PixelShader.hlsl.cso");
    hr = _device->CreatePixelShader(ps.data(), ps.size(), nullptr, &_pixelShader);
    if (FAILED(hr)) {
        MessageBox(nullptr, helpers::GetErrorMessageFromHRESULT(hr).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return hr;
    }

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
    _context->Draw(3, 0);

    // Present the back buffer to the screen
    _swapChain->Present(1, 0);
}

void Dx11App::Cleanup() {

    if (_context)
        _context->ClearState();

    if (_vertexBuffer)
        _vertexBuffer->Release();

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

bool Dx11App::loadModel(const std::string& filePath, std::vector<Mesh>& meshes) {
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

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* ai_mesh = scene->mMeshes[i];
        Mesh mesh;

        for (unsigned int j = 0; j < ai_mesh->mNumVertices; ++j) {
            Vertex vertex;
            vertex.Pos.x = ai_mesh->mVertices[j].x;
            vertex.Pos.y = ai_mesh->mVertices[j].y;
            vertex.Pos.z = ai_mesh->mVertices[j].z - 1.0f;
            vertex.Color.x = 0.949f;
            vertex.Color.y = 0.353f;
            vertex.Color.z = 0.114f;
            vertex.Color.w = 1.0f;
            mesh.vertices.push_back(vertex);
        }

        meshes.push_back(mesh);
    }

    return true;
}