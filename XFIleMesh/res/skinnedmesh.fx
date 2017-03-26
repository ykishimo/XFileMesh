/////////////////////////////////////////////
//  texture sampler
/////////////////////////////////////////////
SamplerState textureSampler : register(s0);
Texture2D	diffuseTexture : register(t0);

/////////////////////////////////////////////////////////////////////////////////////
// CBPerFrame constant buffer
/////////////////////////////////////////////////////////////////////////////////////
cbuffer CBPerFrame : register( b0 )
{
    matrix  View;       //  View Matrix
    matrix  Proj;       //  Projection Matrix
    float4  lightDir;   //  Directional Light Dir
    float4  lightColor; //  Directional Light color
    float4  ambient;    //  Ambient light
    float4  matDiffuse;
    float4  matSpecular;
    float4  matEmissive;
    float4  matPower;
    matrix  World[96];      //  World Matrix pallette
};

/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;     //  position
 	float4 weights  : BLENDWEIGHT;  //  blend weights
    int4   indices  : BLENDINDICES; //  blend indices
    float3 Normal   : NORMAL;       //  normal
    float2 texCoord : TEXCOORD0;    //  uv
};

/////////////////////////////////////////////////////////////////////////////////////
// GSPSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct GSPSInput
{
    float4 Position : SV_POSITION;  //  position
    float4 Color    : COLOR0;       //  
    float2 texCoord : TEXCOORD0;    //  uv
};

//-----------------------------------------------------------------------------------
//  Entry point of vertex shader
//-----------------------------------------------------------------------------------


GSPSInput SMVSFunc( VSInput input )
{

    GSPSInput output = (GSPSInput)0;

    float4 pos = float4( input.Position, 1.0f );

    // ���[���h��Ԃɕϊ�.
    
	float4 worldPos = mul(mul( World[input.indices.x], pos ),input.weights.x);
	worldPos += mul(mul( World[input.indices.y], pos ) ,input.weights.y);
	worldPos += mul(mul( World[input.indices.z], pos ) ,input.weights.z);
	worldPos += mul(mul( World[input.indices.w], pos ) ,input.weights.w);

	// �r���[��Ԃɕϊ�.
    float4 viewPos  = mul( View,  worldPos );

    // �ˉe��Ԃɕϊ�.
    float4 projPos  = mul( Proj,  viewPos );

    output.Position = projPos;

	//  color
	
	float4 worldNormal = mul( mul ( World[input.indices.x], float4( input.Normal, 0.0f ) ),input.weights.x);
	worldNormal += mul( mul ( World[input.indices.y], float4( input.Normal, 0.0f ) ),input.weights.y);
	worldNormal += mul( mul ( World[input.indices.z], float4( input.Normal, 0.0f ) ),input.weights.z);
	worldNormal += mul( mul ( World[input.indices.w], float4( input.Normal, 0.0f ) ),input.weights.w);

	float  LdotN = clamp(dot( lightDir, normalize(worldNormal) ),0.0f,1.0f);
	float4 diffuse = LdotN * lightColor;
	diffuse += ambient;
	diffuse *= matDiffuse;
	diffuse.w = matDiffuse.w;
	
    output.Color = diffuse;
    output.texCoord = input.texCoord;

    return output;
}

//------------------------------------------------------------------------------------
//	Entry point of pixel shader
//------------------------------------------------------------------------------------
float4 SMPSFunc( GSPSInput psin ) : SV_TARGET0
{
	float4 pixel = diffuseTexture.Sample(textureSampler, psin.texCoord);
	pixel  *= psin.Color;
	return pixel;
}
float4 SMPSFuncNoTex( GSPSInput psin ) : SV_TARGET0
{
	float4 pixel = psin.Color;
	return pixel;
}
