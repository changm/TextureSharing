// This is all of our constant buffer data
cbuffer WorldMatrix : register(b0)
{
	matrix worldMatrix;
}

cbuffer ProjectionMatrix : register(b1)
{
	matrix projectionMatrix;
}

cbuffer ViewMatrix : register(b2)
{
	matrix viewMatrix;
}

struct VertexShaderInput
{
	float3 Position: POSITION;
	float3 Color: COLOR;
};

struct VertexShaderOutput
{
	float4 Color: COLOR;
	float4 Position: SV_POSITION;
};

VertexShaderOutput Vertex_main(VertexShaderInput input)
{
	VertexShaderOutput output;

	matrix mvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
	output.Position = mul(mvp, float4(input.Position, 1.0f));
	output.Color = float4(input.Color, 1.0f);
	return output;
}