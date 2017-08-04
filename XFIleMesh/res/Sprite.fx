//-----------------------------------------------------------------------------------
//  texture sampler
//-----------------------------------------------------------------------------------
SamplerState textureSampler : register(s0);
Texture2D	diffuseTexture : register(t0);


//-----------------------------------------------------------------------------------
// Constant buffer
//-----------------------------------------------------------------------------------
cbuffer VSConstantBuffer : register( b0 )
{
    matrix  Proj;   //  pojection matrix.
	float4  Color;  //  a color to modulate.
};


//-----------------------------------------------------------------------------------
// VSInput structure
//-----------------------------------------------------------------------------------
struct VSInput
{
    float3 Position : POSITION;     //	position
	float2 texCoord : TEXCOORD0;	//  texture coordinates
};

//-----------------------------------------------------------------------------------
// GSPSInput structure
//-----------------------------------------------------------------------------------
struct GSPSInput
{
    float4 Position : SV_POSITION;  //  position
	float2 texCoord : TEXCOORD0;	//  texture coordinates
	float4 diffuse  : COLOR;        //  color of fonts.
};

//-----------------------------------------------------------------------------------
//	entry point of the vertex shader.
//-----------------------------------------------------------------------------------
GSPSInput VSFunc( VSInput input )
{
    GSPSInput output = (GSPSInput)0;

    // conver the input data to float4.
	float4 pos = float4( input.Position, 1.0f );

	// transform to projection space.
    float4 projPos  = mul( Proj,  pos );

	output.Position = projPos;
	output.texCoord = input.texCoord;
	output.diffuse  = Color;
    return output;
}

//------------------------------------------------------------------------------------
//	Entry point of the pixel shader
//------------------------------------------------------------------------------------
float4 PSFunc( GSPSInput psin ) : SV_TARGET0
{
	float4 pixel = diffuseTexture.Sample(textureSampler, psin.texCoord);
    pixel = psin.diffuse * pixel;
	return pixel;
}
float4 PSFuncNoTex( GSPSInput psin ) : SV_TARGET0
{
	return psin.diffuse;
}

