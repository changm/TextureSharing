struct VertexShaderInput
{
	float3 Position: POSITION;
	float2 Tex: TEXCOORD;
};

struct VertexShaderOutput
{
	float2 Tex: TEXCOORD;
	float4 Position: SV_POSITION;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;

	// Since we're just outputting the surface, don't have to do anything
	output.Position = float4(input.Position, 1.0);

	// And pass along the texture coordinates
	output.Tex = input.Tex;

	return output;
}