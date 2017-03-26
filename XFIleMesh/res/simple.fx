/////////////////////////////////////////////////////////////////////////////////////
// CBPerFrame constant buffer
/////////////////////////////////////////////////////////////////////////////////////
cbuffer CBPerFrame : register( b0 )
{
    matrix  World;  //!< ���[���h�s��.
    matrix  View;   //!< �r���[�s��.
    matrix  Proj;   //!< �ˉe�s��.
};

/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;     //!< �ʒu���W�ł�.
    float3 Normal : NORMAL;     //!< �ʒu���W�ł�.

};

/////////////////////////////////////////////////////////////////////////////////////
// GSPSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct GSPSInput
{
    float4 Position : SV_POSITION;  //!< �ʒu���W�ł�.
};

//-----------------------------------------------------------------------------------
//  Entry point of the vertex shader
//-----------------------------------------------------------------------------------
GSPSInput VSFunc( VSInput input )
{
    GSPSInput output = (GSPSInput)0;

    // ���̓f�[�^�����̂܂ܗ���.
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

        // ���[���h��Ԃɕϊ�.
        float4 worldPos = mul( World, input[ i ].Position );

        // �r���[��Ԃɕϊ�.
        float4 viewPos  = mul( View,  worldPos );

        // �ˉe��Ԃɕϊ�.
        float4 projPos  = mul( Proj,  viewPos );

        // �o�͒l�ݒ�.
        output.Position = projPos;

        // �X�g���[���ɒǉ�.
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

