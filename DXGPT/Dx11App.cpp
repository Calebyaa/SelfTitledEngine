#include "Dx11App.h"


#include <d3dcompiler.h>
//#include <DirectXMath.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

// in the future make a log file.
#include <iostream>

Dx11App::~Dx11App() {
    Cleanup();
}

HRESULT Dx11App::Init(HWND hWnd) {
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Create a device, device context, and swap chain using DXGI factory
    // ChatGPT lied. We are not using DXGI factory.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //sd.BufferDesc.RefreshRate.Numerator = 60;
    //sd.BufferDesc.RefreshRate.Denominator = 1;
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
        std::wcout << getErrorMessageFromHRESULT(hr) << std::endl;
        return hr;
    }

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
    SimpleVertex vertices[] =
    {
        { DirectX::XMFLOAT3(0.0f, 0.5f, 0.5f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }
    };

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    hr = _device->CreateBuffer(&bd, &InitData, &_vertexBuffer);
    if (FAILED(hr))
        return hr;

    // Set the vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);

    // Set the primitive topology
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_4_0", 0, 0, &pVSBlob, nullptr);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Error compiling vertex shader", L"Error", MB_OK);
        return hr;
    }

    // Create the vertex shader
    hr = _device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_vertexShader);
    if (FAILED(hr)) {
        pVSBlob->Release();
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
    hr = _device->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &_vertexLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Set the input layout
    _context->IASetInputLayout(_vertexLayout);

    // Compile the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    hr = D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_4_0", 0, 0, &pPSBlob, nullptr);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Error compiling pixel shader", L"Error", MB_OK);
        return hr;
    }

    // Create the pixel shader
    hr = _device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pixelShader);
    pPSBlob->Release();
    if (FAILED(hr))
        return hr;

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

std::wstring Dx11App::getErrorMessageFromHRESULT(HRESULT hr) {
    LPWSTR messageBuffer = nullptr;
    DWORD bufferLength = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer,
        0,
        nullptr
    );

    std::wstring errorMessage;

    if (bufferLength) {
        errorMessage.assign(messageBuffer, bufferLength);
        LocalFree(messageBuffer);
    } else {
        errorMessage = L"Unknown error";
    }

    return errorMessage;
}