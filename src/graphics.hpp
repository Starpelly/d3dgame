#pragma once

enum class RendererType
{
	None = -1,
	OpenGL,
	D3D11,
};

class Shader
{
protected:
	Shader() = default;

public:
	// Copy / Moves not allowed
	Shader(const Shader&) = delete;
	Shader(Shader&&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader& operator=(Shader&&) = delete;

	virtual ~Shader() = default;
};

class Texture
{
protected:
	Texture() = default;

public:
	// Copy / Moves not allowed
	Texture(const Texture&) = delete;
	Texture(Texture&&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture& operator=(Texture&&) = delete;

	virtual ~Texture() = default;
};

class RenderTarget
{
protected:
	RenderTarget() = default;

public:
	// Copy / Moves not allowed
	RenderTarget(const RenderTarget&) = delete;
	RenderTarget(RenderTarget&&) = delete;
	RenderTarget& operator=(const RenderTarget&) = delete;
	RenderTarget& operator=(RenderTarget&&) = delete;

	virtual ~RenderTarget() = default;
};

class Mesh
{
protected:
	Mesh() = default;

public:
	// Copy / Moves not allowed
	Mesh(const Mesh&) = delete;
	Mesh(Mesh&&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh& operator=(Mesh&&) = delete;
};

struct DrawCall
{

};