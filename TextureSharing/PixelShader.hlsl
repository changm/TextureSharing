struct Input
{
	float4 color : COLOR;
	float4 position: SV_POSITION;
};

float4 main(Input input) : SV_TARGET
{
	//return input.color;
	return (1.0f, 1.0f, 1.0f, 1.0f);
}