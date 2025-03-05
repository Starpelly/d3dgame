struct VS_INPUT
{
    float2 position : POSITION;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.position = float4(input.position, 0.0f, 1.0f); // Convert to clip space
    output.color = input.color;
    return output;
}