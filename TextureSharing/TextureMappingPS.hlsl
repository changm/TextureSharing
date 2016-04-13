Texture2D shaderTexture;
SamplerState SampleType;

struct PixelShaderInput 
{
	float2 Tex: TEXCOORD;
	float4 Position: SV_POSITION;
};

float4 main(PixelShaderInput input) : SV_TARGET0
{
	return shaderTexture.Sample(SampleType, input.Tex);
}