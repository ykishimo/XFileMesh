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
    matrix  World;  //!< ワールド行列.
    matrix  View;   //!< ビュー行列.
    matrix  Proj;   //!< 射影行列.
	float   AlphaThreshold;  //!< アルファ閾値
};


/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;     //!< 位置座標です.
    float  PSize    : PSIZE;        //!< ポイントサイズです
    float4 color    : DIFFUSE;      //!< 頂点カラーです
};

/////////////////////////////////////////////////////////////////////////////////////
// GSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct GSInput
{
    float4 Position : SV_POSITION;  //!< 位置座標です.
    float  PSize    : PSIZE;        //!< ポイントサイズです
    float4 color    : DIFFUSE;      //!< 頂点カラーです
};
struct PSInput
{
    float4 Position : SV_POSITION;  //!< 位置座標です.
    float4 color    : DIFFUSE;      //!< 頂点カラーです
    float2 texCoord : TEXCOORD0;    //!< uv
};

//-----------------------------------------------------------------------------------
//! @brief      頂点シェーダエントリーポイントです.
//-----------------------------------------------------------------------------------
GSInput VSFunc( VSInput input )
{
    GSInput output = (GSInput)0;

    // 入力データをそのまま流す.

    output.Position = float4( input.Position, 1.0f );
    output.PSize    = input.PSize;
    output.color    = input.color;
    return output;
}

//-----------------------------------------------------------------------------------
//! @brief      ジオメトリシェーダエントリーポイントです.
//-----------------------------------------------------------------------------------
[maxvertexcount(4)]
void GSFunc( point GSInput input[1], inout TriangleStream<PSInput> stream )
{
    PSInput output = (PSInput)0;
	float psize = input[0].PSize;

	output.color = input[0].color;

     // ワールド空間に変換.
    float4 worldPos = mul( World, input[0].Position );

    // ビュー空間に変換.
    float4 ordPos  = mul( View,  worldPos );

	//ordPos = float4(0.0f,0.0f, 4.0f,1.0f);

	output.Position = mul(Proj, ordPos + float4(-psize, psize, 0.0f, 0.0f));
	//output.Position = float4(-0.5f,0.5f,0.5f,1.0f);
	output.color = input[0].color;
	output.texCoord = float2(0.0f,0.0f);
    stream.Append( output );
	
	output.Position = mul(Proj, ordPos + float4( psize, psize, 0.0f, 0.0f));
	//output.Position = float4( 0.5f,0.5f,0.5f,1.0f);
	output.color = input[0].color;
	output.texCoord = float2(1.0f,0.0f);
    stream.Append( output );

	output.Position = mul(Proj, ordPos + float4(-psize,-psize, 0.0f, 0.0f));
	//output.Position = float4(-0.5f,-0.5f,0.5f,1.0f);
	output.color = input[0].color;
	output.texCoord = float2(0.0f,1.0f);
    stream.Append( output );

	output.Position = mul(Proj, ordPos + float4( psize,-psize, 0.0f, 0.0f));
	//output.Position = float4( 0.5f,-0.5f,0.5f,1.0f);
	output.color = input[0].color;
	output.texCoord = float2(1.0f,1.0f);
    stream.Append( output );

    stream.RestartStrip();
}
                            
//------------------------------------------------------------------------------------
//! @brief      ピクセルシェーダエントリーポイントです.
//------------------------------------------------------------------------------------
float4 PSFunc( PSInput output ) : SV_TARGET0
{
    //return float4( 1.0f, 1.0f, 1.0f, 1.0f );

	float4 pixel = diffuseTexture.Sample(textureSampler, output.texCoord);
	pixel *= output.color;
	return pixel;
}

//------------------------------------------------------------------------------------
//! @brief      ピクセルシェーダエントリーポイントです.
//------------------------------------------------------------------------------------
float4 PSFuncNoTex( PSInput output ) : SV_TARGET0
{
//    return float4( 1.0f, 1.0f, 0.0f, 1.0f );
	float4 pixel = output.color;
	if (pixel.w <= AlphaThreshold)
		discard;
	return pixel;
}

