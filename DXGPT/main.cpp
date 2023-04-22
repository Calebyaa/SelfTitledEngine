#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

// disable SAL anotation warning
#pragma warning(disable: 28251)

// Global variables
HWND g_hWnd = nullptr;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

// Vertex structure
struct SimpleVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

// Shaders and buffers
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11Buffer* g_pVertexBuffer = nullptr;
ID3D11Buffer* g_pIndexBuffer = nullptr;
ID3D11InputLayout* g_pVertexLayout = nullptr;

// Function prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT InitWindow(HINSTANCE, int);
HRESULT InitDevice();
void Render();
void CleanupDevice();

// Entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    if (FAILED(InitDevice()))
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return (int)msg.wParam;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

// Initialize window
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register window class
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"TriangleApp";
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    g_hWnd = CreateWindow(
        L"TriangleApp",
        L"DirectX Example - Triangle",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_hWnd)
        return E_FAIL;

    // Show window
    ShowWindow(g_hWnd, nCmdShow);

    return S_OK;
}

// Initialize device
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hWnd, &rc);
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
    sd.OutputWindow = g_hWnd;
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
        &g_pSwapChain,
        &g_pd3dDevice,
        &FeatureLevel,
        &g_pImmediateContext
    );

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

    // Set the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);

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
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
    if (FAILED(hr))
        return hr;

    // Set the vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // Set the primitive topology
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_4_0", 0, 0, &pVSBlob, nullptr);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Error compiling vertex shader", L"Error", MB_OK);
        return hr;
    }

    // Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
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
    hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

    // Compile the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    hr = D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_4_0", 0, 0, &pPSBlob, nullptr);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Error compiling pixel shader", L"Error", MB_OK);
        return hr;
    }

    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    pPSBlob->Release();
    if (FAILED(hr))
        return hr;

    return S_OK;
}

// Render function
void Render()
{
    // Clear the back buffer
    float clearColor[4] = { 0.392f, 0.584f, 0.929f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

    // Set the vertex and pixel shaders
    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // Draw the triangle
    g_pImmediateContext->Draw(3, 0);

    // Present the back buffer to the screen
    g_pSwapChain->Present(1, 0);
}

// Cleanup function
void CleanupDevice()
{
    if (g_pImmediateContext)
        g_pImmediateContext->ClearState();

    if (g_pVertexBuffer)
        g_pVertexBuffer->Release();

    if (g_pVertexLayout)
        g_pVertexLayout->Release();

    if (g_pVertexShader)
        g_pVertexShader->Release();

    if (g_pPixelShader)
        g_pPixelShader->Release();

    if (g_pRenderTargetView)
        g_pRenderTargetView->Release();

    if (g_pSwapChain)
        g_pSwapChain->Release();

    if (g_pImmediateContext)
        g_pImmediateContext->Release();

    if (g_pd3dDevice)
        g_pd3dDevice->Release();
}