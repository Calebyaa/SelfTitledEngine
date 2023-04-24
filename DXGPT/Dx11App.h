#pragma once

#include <d3d11.h>

#include <string>

#include "types.h"

class Dx11App {
public:
    Dx11App() :
        _device(nullptr),
        _context(nullptr),
        _swapChain(nullptr),
        _renderTarget(nullptr),
        _vertexShader(nullptr),
        _pixelShader(nullptr),
        _vertexBuffer(nullptr),
        _indexBuffer(nullptr),
        _vertexLayout(nullptr) {}

    ~Dx11App();
    HRESULT Init(HWND hWnd);
    void Render();
    void Cleanup();

private:
    std::wstring getErrorMessageFromHRESULT(HRESULT hr);


private:
    ID3D11Device* _device;
    ID3D11DeviceContext* _context;
    IDXGISwapChain* _swapChain;
    ID3D11RenderTargetView* _renderTarget;

    ID3D11VertexShader* _vertexShader;
    ID3D11PixelShader* _pixelShader;
    ID3D11Buffer* _vertexBuffer;
    ID3D11Buffer* _indexBuffer;
    ID3D11InputLayout* _vertexLayout;
};