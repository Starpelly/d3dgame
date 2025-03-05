cbuffer constants : register(b0)
{
	float4x4 u_matrix;
};

struct vs_in
{
	float2 position : POS;
	float2 texcoord : TEX;
	float4 color : COL;
	float4 mask : MASK;
};

struct vs_out
{
	float4 position : SV_POSITION;
	float2 texcoord : TEX;
	float4 color : COL;
	float4 mask : MASK;
};

vs_out vs_main(vs_in input)
{
	vs_out output;

	// output.position = mul(float4(input.position, 0.0f, 1.0f), u_matrix);
    output.position = mul(u_matrix, float4(input.position, 0.0, 1.0));
	output.texcoord = input.texcoord;
	output.color = input.color;
	output.mask = input.mask;

	return output;
}