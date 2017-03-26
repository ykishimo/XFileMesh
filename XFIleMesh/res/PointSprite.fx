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
    matrix  World;  //!< ���[���h�s��.
    matrix  View;   //!< �r���[�s��.
    matrix  Proj;   //!< �ˉe�s��.
	float   AlphaThreshold;  //!< �A���t�@臒l
};


/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;     //!< �ʒu���W�ł�.
    float  PSize    : PSIZE;        //!< �|�C���g�T�C�Y�ł�
    float4 color    : DIFFUSE;      //!< ���_�J���[�ł�
};

/////////////////////////////////////////////////////////////////////////////////////
// GSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct GSInput
{
    float4 Position : SV_POSITION;  //!< �ʒu���W�ł�.
    float  PSize    : PSIZE;        //!< �|�C���g�T�C�Y�ł�
    float4 color    : DIFFUSE;      //!< ���_�J���[�ł�
};
struct PSInput
{
    float4 Position : SV_POSITION;  //!< �ʒu���W�ł�.
    float4 color    : DIFFUSE;      //!< ���_�J���[�ł�
    float2 texCoord : TEXCOORD0;    //!< uv
};

//-----------------------------------------------------------------------------------
//! @brief      ���_�V�F�[�_�G���g���[�|�C���g�ł�.
//-----------------------------------------------------------------------------------
GSInput VSFunc( VSInput input )
{
    GSInput output = (GSInput)0;

    // ���̓f�[�^�����̂܂ܗ���.

    output.Position = float4( input.Position, 1.0f );
    output.PSize    = input.PSize;
    output.color    = input.color;
    return output;
}

//-----------------------------------------------------------------------------------
//! @brief      �W�I���g���V�F�[�_�G���g���[�|�C���g�ł�.
//-----------------------------------------------------------------------------------
[maxvertexcount(4)]
void GSFunc( point GSInput input[1], inout TriangleStream<PSInput> stream )
{
    PSInput output = (PSInput)0;
	float psize = input[0].PSize;

	output.color = input[0].color;

     // ���[���h��Ԃɕϊ�.
    float4 worldPos = mul( World, input[0].Position );

    // �r���[��Ԃɕϊ�.
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
//! @brief      �s�N�Z���V�F�[�_�G���g���[�|�C���g�ł�.
//------------------------------------------------------------------------------------
float4 PSFunc( PSInput output ) : SV_TARGET0
{
    //return float4( 1.0f, 1.0f, 1.0f, 1.0f );

	float4 pixel = diffuseTexture.Sample(textureSampler, output.texCoord);
	pixel *= output.color;
	return pixel;
}

//------------------------------------------------------------------------------------
//! @brief      �s�N�Z���V�F�[�_�G���g���[�|�C���g�ł�.
//------------------------------------------------------------------------------------
float4 PSFuncNoTex( PSInput output ) : SV_TARGET0
{
//    return float4( 1.0f, 1.0f, 0.0f, 1.0f );
	float4 pixel = output.color;
	if (pixel.w <= AlphaThreshold)
		discard;
	return pixel;
}

