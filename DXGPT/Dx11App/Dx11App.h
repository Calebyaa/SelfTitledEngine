#pragma once

#define NOMINMAX

#include <d3d11.h>

#include <string>
#include <vector>

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
    // update this to return bool
    std::vector<char> loadCompiledShader(const std::wstring& filePath);
    bool loadModel(const std::string& filePath);


private:
    std::vector<Mesh> _meshes;

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