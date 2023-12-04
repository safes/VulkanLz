
struct VSOutput {
	float4 pos : SV_POSITION;
	[[vk::location(0)]] float3 color : COLOR0;
};

float4 main(VSOutput input) : SV_TARGET
{
	return float4(input.color,1.f);
}