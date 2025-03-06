#pragma once

#include <glm/glm.hpp>
#include "graphics.hpp"

class Renderer
{
public:
	virtual ~Renderer() = default;

	virtual bool init() = 0;

	virtual void shutdown() = 0;

	virtual void update() = 0;

	virtual void before_render() = 0;

	virtual void after_render() = 0;

	virtual void render(const DrawCall& drawCall) = 0;

	virtual void clear_backbuffer(const glm::vec4& color, float depth, uint8_t stencil, ClearMask mask) = 0;

private:
	static Renderer* try_make_opengl();
	static Renderer* try_make_d3d11();

public:
	static Renderer* try_make_renderer(RendererType type)
	{
		switch (type)
		{
		case RendererType::None: return nullptr;
		case RendererType::OpenGL: return try_make_opengl();
		case RendererType::D3D11: return try_make_d3d11();
		}

		return nullptr;
	}

	static RendererType default_type()
	{
		return RendererType::D3D11;
	}
};