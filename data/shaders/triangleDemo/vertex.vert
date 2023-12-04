// The entry point and target profile are needed to compile this example:
// -T ps_6_6 -E VSMain

//cbuffer cbMeshtrans : register(b0)
//{
//	 float4x4 ModelViewProj : packoffset(c0);
//	 float4x4 World : packoffset(c4);
//
//};

struct VS_INPUT {
	[[vk::location(0)]] float2 pos : POSITION0;
	[[vk::location(1)]] float3 color : COLOR0;
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	[[vk::location(0)]] float3 color : COLOR0;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	//output.pos = mul(ModelViewProj , float4(input.pos.xy, 0.f, 1.f));
	output.pos =  float4(input.pos.xy, 0.f, 1.f);
	output.color = input.color;
	return output;
}