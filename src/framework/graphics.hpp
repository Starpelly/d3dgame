#pragma once

enum class RendererType
{
	None = -1,
	OpenGL,
	D3D11,
};

enum class DepthCompare
{
	None,
	Always,
	Never,
	Less,
	Equal,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual
};

enum class Cull
{
	None = 0,
	Front = 1,
	Back = 2,
};

enum class BlendOp
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max
};

enum class BlendFactor
{
	Zero,
	One,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinusDstAlpha,
	ConstantColor,
	OneMinusConstantColor,
	ConstantAlpha,
	OneMinusConstantAlpha,
	SrcAlphaSaturate,
	Src1Color,
	OneMinusSrc1Color,
	Src1Alpha,
	OneMinusSrc1Alpha
};

enum class BlendMask
{
	None = 0,
	Red = 1,
	Green = 2,
	Blue = 4,
	Alpha = 8,
	RGB = Red | Green | Blue,
	RGBA = Red | Green | Blue | Alpha,
};

enum class ClearMask
{
	None = 0,
	Color = 1,
	Depth = 2,
	Stencil = 4,
	All = (int)Color | (int)Depth | (int)Stencil
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