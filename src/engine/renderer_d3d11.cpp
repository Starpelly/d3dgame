#include "renderer.hpp"
#include "app.hpp"
#include "platform.hpp"

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <iostream>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/gtc/matrix_transform.hpp>

#define RENDERER ((Renderer_D3D11*)Internal::app_renderer())

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

namespace Engine
{
	class Renderer_D3D11 : public Renderer
	{
	public:
		bool init() override;
		void shutdown() override;
		void update() override;
		void before_render() override;
		void after_render() override;
		void render(const DrawCall& drawCall) override;
		void clear_backbuffer(const glm::vec4& color, float depth, uint8_t stencil, ClearMask mask) override;

	private:
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		IDXGISwapChain* swapChain = nullptr;
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
		ID3D11RenderTargetView* backBufferView = nullptr;

		ID3D11VertexShader* vertexShader = nullptr;
		ID3D11PixelShader* pixelShader = nullptr;
		ID3D11InputLayout* inputLayout = nullptr;

	private:
		glm::ivec2 lastWindowSize;
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
}

DrawingSystem* test_drawer = nullptr;

using namespace Engine;

bool Renderer_D3D11::init()
{
	// Define Swap Chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = (HWND)Platform::d3d11_get_hwnd();
	swapChainDesc.Windowed = true;

	UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined(DEBUG) || defined(_DEBUG)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create D3D device and context and swap chain
	HRESULT hr = (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION,
		&swapChainDesc,
		&swapChain,
		&device,
		&featureLevel,
		&context));

	// Exit out if it's not okay
	if (!SUCCEEDED(hr) || !swapChain || !device || !context)
		return false;

	// Get back buffer and create render target view
	ID3D11Texture2D* backBuffer = nullptr;
	swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

	if (backBuffer)
	{
		device->CreateRenderTargetView(backBuffer, nullptr, &backBufferView);
		backBuffer->Release();
	}

	context->OMSetRenderTargets(1, &backBufferView, NULL);

	// Load shaders
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
	GetClientRect((HWND)Platform::d3d11_get_hwnd(), &winRect);
	D3D11_VIEWPORT viewport = {
	  0.0f,
	  0.0f,
	  (FLOAT)(winRect.right - winRect.left),
	  (FLOAT)(winRect.bottom - winRect.top),
	  0.0f,
	  1.0f };
	context->RSSetViewports(1, &viewport);

	// Create drawing system
	test_drawer = new DrawingSystem(device, context);

	lastWindowSize = App::get_size();

	return true;
}

void Renderer_D3D11::shutdown()
{
	// Release shaders
	inputLayout->Release();
	vertexShader->Release();
	pixelShader->Release();

	// Release main devices
	if (backBufferView)
		backBufferView->Release();
	swapChain->Release();
	context->ClearState();
	context->Flush();
	context->Release();
	device->Release();
}

void Renderer_D3D11::update()
{
	// empty
}

void Renderer_D3D11::before_render()
{
	HRESULT hr;

	auto nextWindowSize = App::get_size();
	if (lastWindowSize != nextWindowSize)
	{
		lastWindowSize = nextWindowSize;

		// Release old buffer
		if (backBufferView)
			backBufferView->Release();

		// Perform resize
		hr = swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
		assert(SUCCEEDED(hr), "Failed to update backbuffer on resize!");

		// Get the new buffer
		ID3D11Texture2D* backBuffer = nullptr;
		hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		if (SUCCEEDED(hr) && backBuffer)
		{
			// Get backbuffer drawable size
			// D3D11_TEXTURE2D_DESC desc;
			// backBuffer->GetDesc(&desc);

			// Create view
			hr = device->CreateRenderTargetView(backBuffer, nullptr, &backBufferView);
			assert(SUCCEEDED(hr), "Failed to update backbuffer on resize");
			backBuffer->Release();
		}
	}
}

void Renderer_D3D11::after_render()
{
	auto vsync = false;
	auto hr = swapChain->Present(vsync ? 1 : 0, 0);
	assert(SUCCEEDED(hr), "Failed to present swap chain");
}

void Renderer_D3D11::render(const DrawCall& pass)
{
	// RS
	{
		// Set the viewport
		{

		}

		// Scissor rect
		{

		}

		// Rasterizer
		{

		}
	}

	// Input assembler
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(inputLayout);

	auto stdmax = 24043493898349;
	test_drawer->UpdateConstantBuffer(context, CreateOrthographicOffCenter(0, 1280, 720, 0, 0, stdmax));

	// Draw rectangles
	test_drawer->DrawRectangle(100, 100, 50, 50, { 1, 0, 0, 1 }); // Red rectangle
	test_drawer->DrawRectangle(200, 100, 50, 50, { 1, 1, 0, 1 }); // Yellow rectangle
	test_drawer->DrawRectangle(300, 100, 50, 50, { 1, 1, 1, 1 }); // White rectangle

	test_drawer->Flush();
}

void Renderer_D3D11::clear_backbuffer(const glm::vec4& color, float depth, uint8_t stencil, ClearMask mask)
{
	if (((int)mask & (int)ClearMask::Color) == (int)ClearMask::Color)
	{
		float clearColor[4] = { color.r, color.g, color.b, color.a };
		context->ClearRenderTargetView(backBufferView, clearColor);
	}
}

Renderer* Renderer::try_make_d3d11()
{
	return new Renderer_D3D11();
}