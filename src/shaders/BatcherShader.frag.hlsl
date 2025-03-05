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

float4 ps_main(vs_out input) : SV_TARGET
{
    /*
	float4 color = u_texture.Sample(u_texture_sampler, input.texcoord);
	return
		input.mask.x * color * input.color + 
		input.mask.y * color.a * input.color + 
		input.mask.z * input.color;
        */

    return input.color; // Output the vertex color
}