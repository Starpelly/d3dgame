#include <SDL3/SDL.h>

#include <d3d11.h>
#include <d3dcompiler.h> // shader compiler
#include <assert.h>

#include <vector>

#include <DirectXMath.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace DirectX;

struct Matrix4x4
{
    float m11;
    float m12;
    float m13;
    float m14;

    float m21;
    float m22;
    float m23;
    float m24;

    float m31;
    float m32;
    float m33;
    float m34;

    float m41;
    float m42;
    float m43;
    float m44;

    static const Matrix4x4 identity;
};

const Matrix4x4 Matrix4x4::identity = Matrix4x4{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

static Matrix4x4 CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Matrix4x4 result{};

    result.m11 = 2 / (right - left);
    result.m12 = result.m13 = result.m14 = 0;
    result.m22 = 2 / (top - bottom);
    result.m21 = result.m23 = result.m24 = 0;
    result.m33 = 1 / (zNear - zFar);
    result.m31 = result.m32 = result.m34 = 0;
    result.m41 = (left + right) / (left - right);
    result.m42 = (top + bottom) / (bottom - top);
    result.m43 = zNear / (zNear - zFar);
    result.m44 = 1;

    return result;
}

class Camera2D
{
public:
    glm::vec2 position = { 0.0f, 0.0f };
    float zoom = 1.0f;

    glm::mat4 GetViewMatrix() const
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(-position, 0.0f));
        transform = glm::scale(transform, glm::vec3(zoom, zoom, 1.0f));
        return transform;
    }
};

ID3DBlob* CompileShader(const wchar_t* file, const char* entry, const char* profile) {
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    flags |= D3DCOMPILE_DEBUG; // add more debug output
#endif

    HRESULT hr = D3DCompileFromFile(file, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, profile, flags, 0, &shaderBlob, &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            std::cerr << "Shader Compilation Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
        return nullptr;
    }

    return shaderBlob;
}

struct ConstantBuffer {
    Matrix4x4 Matrix;
};

struct Vertex {
    glm::vec2 position;
    glm::vec2 texCoord;
    glm::vec4 color;
    glm::vec4 mask;
};

static Vertex MakeVertex(glm::vec2 position, glm::vec4 color)
{
    return Vertex
    {
        position,
        { 0, 0 },
        color,
        { 0, 0, 255, 0 }
    };
}

class DrawingSystem
{
public:
    DrawingSystem(ID3D11Device* device, ID3D11DeviceContext* context)
        : device(device), context(context), vertexBuffer(nullptr) {
        InitBuffer();
    }

    ~DrawingSystem() {
        if (vertexBuffer) vertexBuffer->Release();
    }

    void DrawingSystem::UpdateConstantBuffer(ID3D11DeviceContext* context, const Matrix4x4& matrix) {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        *reinterpret_cast<Matrix4x4*>(mappedResource.pData) = matrix;
        context->Unmap(constantBuffer, 0);

        context->VSSetConstantBuffers(0, 1, &constantBuffer);
    }

    void DrawRectangle(float x, float y, float width, float height, glm::vec4 color) {
        glm::vec2 p1 = { x, y };
        glm::vec2 p2 = { x + width, y };
        glm::vec2 p3 = { x, y + height };
        glm::vec2 p4 = { x + width, y + height };

        Vertex quad[6] = {
            MakeVertex(p1, color),
            MakeVertex(p2, color),
            MakeVertex(p3, color),
            MakeVertex(p2, color),
            MakeVertex(p4, color),
            MakeVertex(p3, color)
        };
        vertices.insert(vertices.end(), std::begin(quad), std::end(quad));
    }

    void Flush() {
        if (vertices.empty()) return;

        // context->PSSetShaderResources(0, 1, &texture);

        // Update dynamic vertex buffer
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, vertices.data(), sizeof(Vertex) * vertices.size());
        context->Unmap(vertexBuffer, 0);

        // Set buffer and draw
        UINT stride = sizeof(Vertex), offset = 0;
        context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context->Draw(static_cast<UINT>(vertices.size()), 0);

        vertices.clear();
    }

private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11Buffer* vertexBuffer;
    ID3D11Buffer* constantBuffer;
    ID3D11ShaderResourceView* texture;

    std::vector<Vertex> vertices;
    const float screenWidth = 1280;
    const float screenHeight = 720;

    void InitBuffer() {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.ByteWidth = sizeof(Vertex) * 1024;  // Initial size, can be adjusted
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        device->CreateBuffer(&bufferDesc, nullptr, &vertexBuffer);

        D3D11_BUFFER_DESC cbd = {};
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        device->CreateBuffer(&cbd, nullptr, &constantBuffer);
    }
};

int main(int argc, char* argv[])
{
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }
    
    // Create SDL window
    SDL_Window* window = SDL_CreateWindow("game", 1280, 720, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Get native window handle
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    // Create D3D11 device and swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = 1280;
    swapChainDesc.BufferDesc.Height = 720;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGISwapChain* swapChain;
    D3D_FEATURE_LEVEL featureLevel;

    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined(DEBUG) || defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION,
                                             &swapChainDesc, &swapChain, &device, &featureLevel, &context)))
    {
        SDL_Log("Failed to create D3D11 device and swap chain.");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Get back buffer and create render target view
    ID3D11RenderTargetView* renderTargetView = nullptr;
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    backBuffer->Release();

    context->OMSetRenderTargets(1, &renderTargetView, NULL);

    // Create drawing system
    DrawingSystem drawer(device, context);

    // Load shaders
    ID3D11VertexShader* vertexShader;
    ID3D11PixelShader* pixelShader;
    ID3D11InputLayout* inputLayout;
    {
        // Compile shaders
        ID3DBlob* vsBlob = CompileShader(L"D:/d3dgame/src/shaders/BatcherShader.vert.hlsl", "vs_main", "vs_5_0");
        ID3DBlob* psBlob = CompileShader(L"D:/d3dgame/src/shaders/BatcherShader.frag.hlsl", "ps_main", "ps_5_0");

        // Create shaders
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

        // Define input layout
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"MASK", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        // Create input layout
        auto hr = device->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
        assert(SUCCEEDED(hr));

        // Set shaders
        context->IASetInputLayout(inputLayout);
        context->VSSetShader(vertexShader, nullptr, 0);
        context->PSSetShader(pixelShader, nullptr, 0);

        // Release shader blobs
        vsBlob->Release();
        psBlob->Release();
    }

    // Setup viewport
    RECT winRect;
    GetClientRect(hwnd, &winRect);
    D3D11_VIEWPORT viewport = {
      0.0f,
      0.0f,
      (FLOAT)(winRect.right - winRect.left),
      (FLOAT)(winRect.bottom - winRect.top),
      0.0f,
      1.0f };
    context->RSSetViewports(1, &viewport);

#if false
    // Compile shaders
        UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
        shaderFlags |= D3DCOMPILE_DEBUG; // add more debug output
#endif
        ID3DBlob* vs_blob_ptr = NULL, * ps_blob_ptr = NULL, * error_blob = NULL;

        // COMPILE VERTEX SHADER
        HRESULT hr = D3DCompileFromFile(
            L"D:/d3dgame/src/shaders.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "vs_main",
            "vs_5_0",
            shaderFlags,
            0,
            &vs_blob_ptr,
            &error_blob);
        if (FAILED(hr)) {
            if (error_blob) {
                OutputDebugStringA((char*)error_blob->GetBufferPointer());
                error_blob->Release();
            }
            if (vs_blob_ptr) { vs_blob_ptr->Release(); }
            assert(false);
        }

        // COMPILE PIXEL SHADER
        hr = D3DCompileFromFile(
            L"D:/d3dgame/src/shaders.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "ps_main",
            "ps_5_0",
            shaderFlags,
            0,
            &ps_blob_ptr,
            &error_blob);
        if (FAILED(hr)) {
            if (error_blob) {
                OutputDebugStringA((char*)error_blob->GetBufferPointer());
                error_blob->Release();
            }
            if (ps_blob_ptr) { ps_blob_ptr->Release(); }
            assert(false);
        }

        ID3D11VertexShader* vertex_shader_ptr = NULL;
        ID3D11PixelShader* pixel_shader_ptr = NULL;

        hr = device->CreateVertexShader(
            vs_blob_ptr->GetBufferPointer(),
            vs_blob_ptr->GetBufferSize(),
            NULL,
            &vertex_shader_ptr);
        assert(SUCCEEDED(hr));

        hr = device->CreatePixelShader(
            ps_blob_ptr->GetBufferPointer(),
            ps_blob_ptr->GetBufferSize(),
            NULL,
            &pixel_shader_ptr);
        assert(SUCCEEDED(hr));

        ID3D11InputLayout* input_layout_ptr = NULL;
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
          { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
          /*
          { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
          { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
          { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
          */
        };
        hr = device->CreateInputLayout(
            inputElementDesc,
            ARRAYSIZE(inputElementDesc),
            vs_blob_ptr->GetBufferPointer(),
            vs_blob_ptr->GetBufferSize(),
            &input_layout_ptr);
        assert(SUCCEEDED(hr));

    // Create vertex buffer
        float vertex_data_array[] = {
           0.0f,  0.5f,  0.0f, // point at top
           0.5f, -0.5f,  0.0f, // point at bottom-right
          -0.5f, -0.5f,  0.0f, // point at bottom-left
        };
        UINT vertex_stride = 3 * sizeof(float);
        UINT vertex_offset = 0;
        UINT vertex_count = 3;

        ID3D11Buffer* vertex_buffer_ptr = NULL;
        { /*** load mesh data into vertex buffer **/
            D3D11_BUFFER_DESC vertex_buff_descr = {};
            vertex_buff_descr.ByteWidth = sizeof(vertex_data_array);
            vertex_buff_descr.Usage = D3D11_USAGE_DEFAULT;
            vertex_buff_descr.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA sr_data = { 0 };
            sr_data.pSysMem = vertex_data_array;
            HRESULT hr = device->CreateBuffer(
                &vertex_buff_descr,
                &sr_data,
                &vertex_buffer_ptr);
            assert(SUCCEEDED(hr));
        }
#endif

    // Main loop
    bool running = true;
    bool mouse1 = false;
    SDL_Event event;

    const float speed = 5.0f;
    const float zoomSpeed = 0.05f;

    Camera2D camera;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            {
                running = false;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                auto button = event.button.button - 1;
                if (button == 2) button = 1; 		// Right mouse button = 2
                else if (button == 1) button = 2; 	// Middle mouse button = 2

                if (button == 0)
                {
                    mouse1 = true;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                auto button = event.button.button - 1;
                if (button == 2) button = 1;		// Right mouse button = 2
                else if (button == 1) button = 2;	// Middle mouse button = 2

                if (button == 0)
                {
                    mouse1 = false;
                }
            } break;
            case SDL_EVENT_MOUSE_WHEEL:
            {
                const auto xOffset = static_cast<float>(event.wheel.x);
                const auto yOffset = static_cast<float>(event.wheel.y);

                camera.zoom += yOffset;
                camera.zoom = glm::max(0.1f, camera.zoom - zoomSpeed);
            } break;
            }

        }

        // Clear screen to Cornflower Blue
        const float clearColor[4] = { 0.392f, 0.584f, 0.929f, 1.0f }; // RGB(100, 149, 237)
        context->ClearRenderTargetView(renderTargetView, clearColor);

        // Input assembler
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->IASetInputLayout(inputLayout);

        drawer.UpdateConstantBuffer(context, CreateOrthographicOffCenter(0, 1280, 720, 0, 0, std::numeric_limits<float>::max()));

        // Draw rectangles
        drawer.DrawRectangle(100, 100, 50, 50, { 1, 0, 0, 1 }); // Red rectangle
        drawer.DrawRectangle(200, 100, 50, 50, { 1, 1, 0, 1 }); // Red rectangle

        drawer.Flush();

        // Present frame
        swapChain->Present(1, 0);
    }

    // Cleanup
    SDL_DestroyWindow(window);
    SDL_Quit();
	return 0;
}