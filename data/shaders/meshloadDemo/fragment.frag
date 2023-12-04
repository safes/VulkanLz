
Texture2D texturecolor : register(t1);
SamplerState samplerColorMap : register(s1);

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	[[vk::location(0)]] float3 color : COLOR0;
	[[vk::location(1)]] float2 texcoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 color;
	color = texturecolor.Sample(samplerColorMap, input.texcoord);
	//return color;
	return float4(color.rgb * input.color ,1.f);
}