#pragma once

#include "renderer.hpp"
#include "platform.hpp"

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#define RENDERER ((Renderer_D3D11*)Internal::app_renderer())

class Renderer_D3D11 : public Renderer
{
public:
	bool init() override;
	void shutdown() override;
	void update() override;
	void before_render() override;
	void after_render() override;
	void render(const DrawCall& drawCall) override;

private:
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	D3D_FEATURE_LEVEL featureLevel;
};

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

	HRESULT hr = (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION,
		&swapChainDesc,
		&swapChain,
		&device,
		&featureLevel,
		&context));

	if (!SUCCEEDED(hr) || !swapChain || !device || !context)
		return false;



	return true;
}

void Renderer_D3D11::shutdown()
{
	swapChain->Release();
	context->ClearState();
	context->Flush();
	context->Release();
	device->Release();
}

void Renderer_D3D11::update()
{

}

void Renderer_D3D11::before_render()
{

}

void Renderer_D3D11::after_render()
{

}

void Renderer_D3D11::render(const DrawCall& pass)
{

}

Renderer* Renderer::try_make_d3d11()
{
	return new Renderer_D3D11();
}