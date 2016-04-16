struct Input
{
	float4 color : COLOR;
	float4 position: SV_POSITION;
};

float4 PixelShader_main(Input input) : SV_TARGET0
{
	return input.color;
}