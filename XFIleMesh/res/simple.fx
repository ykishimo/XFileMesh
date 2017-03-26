/////////////////////////////////////////////////////////////////////////////////////
// CBPerFrame constant buffer
/////////////////////////////////////////////////////////////////////////////////////
cbuffer CBPerFrame : register( b0 )
{
    matrix  World;  //!< ワールド行列.
    matrix  View;   //!< ビュー行列.
    matrix  Proj;   //!< 射影行列.
};

/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;     //!< 位置座標です.
    float3 Normal : NORMAL;     //!< 位置座標です.

};

/////////////////////////////////////////////////////////////////////////////////////
// GSPSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct GSPSInput
{
    float4 Position : SV_POSITION;  //!< 位置座標です.
};

//-----------------------------------------------------------------------------------
//  Entry point of the vertex shader
//-----------------------------------------------------------------------------------
GSPSInput VSFunc( VSInput input )
{
    GSPSInput output = (GSPSInput)0;

    // 入力データをそのまま流す.
    output.Position = float4( input.Position, 1.0f );

    return output;
}

//-----------------------------------------------------------------------------------
//  Entry point of the geometry shader
//-----------------------------------------------------------------------------------
[maxvertexcount(3)]
void GSFunc( triangle GSPSInput input[3], inout TriangleStream<GSPSInput> stream )
{
    for( int i=0; i<3; ++i )
    {
        GSPSInput output = (GSPSInput)0;

        // ワールド空間に変換.
        float4 worldPos = mul( World, input[ i ].Position );

        // ビュー空間に変換.
        float4 viewPos  = mul( View,  worldPos );

        // 射影空間に変換.
        float4 projPos  = mul( Proj,  viewPos );

        // 出力値設定.
        output.Position = projPos;

        // ストリームに追加.
        stream.Append( output );
    }
    stream.RestartStrip();
}

//------------------------------------------------------------------------------------
//  Entry point of the pixel shader
//------------------------------------------------------------------------------------
float4 PSFunc( GSPSInput output ) : SV_TARGET0
{
//    return float4( 0.25f, 1.0f, 0.25f, 1.0f );
	float4 tmp = float4( output.Position.x, output.Position.x, output.Position.x, 1.0f );
	float4 c = float4( 1.0/300.0, 1.0/300.0, 1.0/300.0, 1.0);
	return tmp * c;
}

