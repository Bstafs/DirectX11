//--------------------------------------------------------------------------------------
// File: DX11 Framework.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);
    
//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
    float4 DiffuseMtrl;
    
    float4 DiffuseLight;
    float3 LightVecW;
    float gTime;
    float4 AmbientMtrl;
    float4 AmbientLight;
    
    float4 SpecularMtrl;
    float4 SpecularLight;
    float3 EyePosW;
    
    float  SpecularPower;
    
 
}
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};


struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
    
   
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 Pos : POSITION, float3 NormalL : NORMAL, float2 Tex : TEXCOORD0)
{

	VS_OUTPUT output = (VS_OUTPUT) 0;

	output.Pos = mul(Pos, World);
    output.PosW = normalize(EyePosW - output.Pos.xyz);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);

    // Convert from local space to world space
    // W component of vector is 0 as vectors cannot be translated

	float3 normalW = mul(float4(NormalL, 0.0f), World).xyz;
	normalW = normalize(normalW);
    output.Norm = normalW;

    // Compute Colour using Diffuse lighting only
	
	
    // Commpute Specular
  
    
    // Texture Shader
    
    output.Tex = Tex;
    
	return output;
}
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
    float4 f = (0.0f, 0.0f, 0.0f, 1.0f);
    
    float3 r = reflect(LightVecW, input.Norm);
    float3 specularAmount = pow(max(dot(r, input.PosW), 0.0f), SpecularPower);
    float diffuseAmount = max(dot(LightVecW, input.Norm), 0.0f);
    
    float3 specular = specularAmount * (SpecularMtrl * SpecularLight).rgb;
    float3 ambient = AmbientMtrl * AmbientLight;
    float3 diffuse = DiffuseMtrl * AmbientLight;
    
    float4 textureColour = { 1, 1, 1, 1 };
    textureColour = txDiffuse.Sample(samLinear, input.Tex);
    
    f.rbg = textureColour.rbg * (diffuse + ambient + specular);
    f.a = textureColour.a;
    
    return f;
    
}



