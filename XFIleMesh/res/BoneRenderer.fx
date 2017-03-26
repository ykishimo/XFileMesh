/////////////////////////////////////////////////////////////////////////////////////
// CBPerFrame constant buffer
/////////////////////////////////////////////////////////////////////////////////////
cbuffer CBPerFrame : register( b0 )
{
    matrix  World;      //  World Matrix
    matrix  View;       //  View Matrix
    matrix  Proj;       //  Projection Matrix
    float4  diffuse;    //  line color
};

/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;     //  position
};

/////////////////////////////////////////////////////////////////////////////////////
// GSPSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct GSPSInput
{
    float4 Position : SV_POSITION;  //  position
    float4 Color    : COLOR0;       //  
};

//-----------------------------------------------------------------------------------
//  Entry point of the vertex shader
//-----------------------------------------------------------------------------------

GSPSInput BRVSFunc( VSInput input )
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

    return output;
}

//------------------------------------------------------------------------------------
//  Entry point of the pixel shader
//------------------------------------------------------------------------------------

float4 BRPSFunc( GSPSInput psin ) : SV_TARGET0
{
	float4 pixel  = psin.Color;
	return pixel;
}
