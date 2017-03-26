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
    matrix  World;      //  World Matrix
    matrix  View;       //  View Matrix
    matrix  Proj;       //  Projection Matrix
    float4  lightDir;   //  Directional Light Dir
    float4  lightColor; //  Directional Light color
    float4  ambient;    //  Ambient light
    float4  matDiffuse;
    float4  matSpecular;
    float4  matEmissive;
    float4  matPower;
};

/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;     //  position
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
//  Entry point of the vertex shader
//-----------------------------------------------------------------------------------

GSPSInput SMVSFunc( VSInput input )
{

    GSPSInput output = (GSPSInput)0;

    float4 pos = float4( input.Position, 1.0f );

    // ワールド空間に変換.
    float4 worldPos = mul( World, pos );

    // ビュー空間に変換.
    float4 viewPos  = mul( View,  worldPos );

    // 射影空間に変換.
    float4 projPos  = mul( Proj,  viewPos );

    output.Position = projPos;

	//  color
	
	float4 worldNormal = mul ( World, float4( input.Normal, 0.0f ) );
	float  LdotN = clamp(dot( lightDir, worldNormal ),0.0f,1.0f);
	float4 diffuse = LdotN * lightColor;
	diffuse += ambient;
	diffuse *= matDiffuse;
	diffuse.w = matDiffuse.w;
	
    output.Color = diffuse;
    output.texCoord = input.texCoord;

    return output;
}

//------------------------------------------------------------------------------------
//	Entry point of the pixel shader
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
