// Win32Project1.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <crtdbg.h>
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include <D2D1.h>
#include <wincodec.h>

#include "D3DContext.h"

#include "IMesh.h"
#include "MTAnimator.h"
#include "D3D11Font.h"
#include "D3D11Sprite.h"
#include "D3D11PointSprite.h"
#include "HighResTimer.h"

#include "BurnUp.h"

using namespace DirectX;


// グローバル変数:
UINT	g_uiCount=0;

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
ID3DContext			*g_pD3DContext = NULL;
	
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: ここにコードを挿入してください。
	MSG msg;

	//  メモリチェック
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    //  変数宣言
    WNDCLASSEX wcex;  //  ウィンドウクラス構造体
    HWND hWnd;        //  ウィンドウハンドル
    RECT    bounds,client;  //  RECT 構造体

	//  (1)-a ウィンドウクラスの登録
    wcex.cbSize = sizeof(WNDCLASSEX); 
    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;  //  ウィンドウプロシージャの登録
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;  //  アプリケーションインスタンス
    wcex.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = NULL;
    wcex.lpszClassName    = _T("Load Texture");  //  ウィンドウクラス名
    wcex.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wcex);
 

    //  (1)-b ウィンドウの生成
    hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,wcex.lpszClassName,_T("Load Texture"),
                WS_VISIBLE|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,
                CW_USEDEFAULT,0,852,480,NULL,NULL,hInstance,NULL);
    if( !hWnd )
        return FALSE;

    //  ウィンドウサイズの調整
    //  ここは無くても動く
    GetWindowRect(hWnd,&bounds);
    GetClientRect(hWnd,&client);
    MoveWindow(hWnd,bounds.left,bounds.top,
        852 * 2 - client.right,
        480 * 2 - client.bottom,
        false );

    //  ウィンドウの再表示
    ShowWindow( hWnd, nCmdShow );
    UpdateWindow( hWnd );
    //  再表示の際、WndProc が呼び出される事にも注意！

    CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);        //  COM を初期化

	ID3D11Font	*pFont = NULL;
	ID3D11Sprite *pSprite = NULL;
	ID3D11PointSprite *pPointSprite = NULL;

	//MeshFrame *pFrame = NULL;
	//AnimationSet    **ppAnimationSets = NULL;
	//INT				numAnimations = 0;
	ISkinnedMesh *pSkinnedMesh = NULL;
	IBoneRenderer *pBoneRenderer = NULL;
	ISimpleMesh *pSimpleMesh = NULL;
	CMTAnimator *ppMTAnimator[2] = {NULL,NULL};
	IMeshCollider *pFloor = NULL;

	//  背景関連
	ISimpleMesh *pBGMesh = NULL;
	IAnimatedMesh *pAnimatedMesh = NULL;
	IMeshFrameTracker *pTracker = NULL;

	IBurnUp *pBurnUp = NULL;

	XMMATRIX matView;
	XMMATRIX matProj;


	HRESULT hr;
	CHighResTimer *pTimer = new CHighResTimer();

	hr = CreateD3DContext(&g_pD3DContext,hWnd,true);

	if (FAILED(hr)){
		goto ERROR_EXIT;
	}

	//===  各種オブジェクトの生成 ===

	{
		ID3D11DeviceContext *pContext=NULL;
		pContext = g_pD3DContext->GetDeviceContext();
		ID3D11Device *pDevice = NULL;

		pContext->GetDevice(&pDevice);
		if (pDevice){
			ULONG ref = pDevice->Release();
			pDevice = NULL;
		}
		if (pContext != NULL){
			pFont = ID3D11Font::CreateAnkFont(pContext);	//  フォント
			pContext->GetDevice(&pDevice);
			if (pDevice){
				ULONG ref = pDevice->Release();
				pDevice = NULL;
			}
			pSimpleMesh = ISimpleMesh::CreateInstance(_T("res\\smile.x"));
			pSkinnedMesh = ISkinnedMesh::CreateInstance(_T("res\\kunekune.bin.cmp.x"));
			pAnimatedMesh = IAnimatedMesh::CreateInstance(_T("res\\motion.x"));
			pBGMesh = ISimpleMesh::CreateInstance(_T("res\\circuit.x"));

			if (pAnimatedMesh != NULL){
				if (FAILED(pAnimatedMesh->RestoreDeviceObjects(pContext))){
					SAFE_DELETE(pAnimatedMesh);
				}else{
					pTracker = IMeshFrameTracker::CreateInstance(pAnimatedMesh->GetRootFrame(),"MotionHolder");
				}
			}

			if (pSkinnedMesh != NULL)
				for (int i = 0; i < _countof(ppMTAnimator) ; ++i){
					ppMTAnimator[i] = new CMTAnimator(pSkinnedMesh);
					ppMTAnimator[i]->SetActiveAnimationSet(0,1.0f,true);
				}
			if (pSkinnedMesh != NULL){
				if (FAILED(pSkinnedMesh->RestoreDeviceObjects(pContext))){
					SAFE_DELETE(pSkinnedMesh);
				}else{

			//pSkinnedMesh->GetAnimator()->SetTrackAnimation(0,0,1.0f,true);
					pBoneRenderer = IBoneRenderer::CreateInstance(pSkinnedMesh->GetRootFrame());
					pBoneRenderer->RestoreDeviceObjects(pContext);
				}
			}

			if (pBGMesh != NULL){
				if (FAILED(pBGMesh->RestoreDeviceObjects(pContext))){
					SAFE_DELETE(pBGMesh);
				}
			}
			if (pSimpleMesh != NULL){
				if (FAILED(pSimpleMesh->RestoreDeviceObjects(pContext))){
					SAFE_DELETE(pSimpleMesh);
				}
			}
			pFloor = IMeshCollider::CreateInstance(_T("res\\land.x"));
			if (pFloor != NULL){
				pFloor->RestoreDeviceObjects(pContext);
			}
			{
				DirectX::XMVECTOR camPos    = DirectX::XMVectorSet( 0.0f, 2.0f, -9.00f, 0.0f );
				DirectX::XMVECTOR camTarget = DirectX::XMVectorSet( 0.0f, 0.0f,  0.0f, 0.0f );
				DirectX::XMVECTOR camUpward = DirectX::XMVectorSet( 0.0f, 1.0f,  0.0f, 0.0f );
				matView  = DirectX::XMMatrixLookAtLH(camPos,camTarget,camUpward);
			}
			matProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2*0.5f,16.0f/9.0f,0.1f,1000.0f);

			pSprite = ID3D11Sprite::CreateInstance();
			if (pSprite != NULL){
				pSprite->SetDiffuse(1.0f,1.0f,1.0f,1.0f);
				pSprite->SetTexture(0,_T("res\\smile2.rle.tga"));
				pSprite->RestoreDeviceObjects(pContext);
			}

			pPointSprite = ID3D11PointSprite::CreateInstance();
			if (pPointSprite != NULL){
				pPointSprite->SetTexture(0,_T("res\\FireBall.dds"));
				pPointSprite->RestoreDeviceObjects(pContext);
			}
			pContext->GetDevice(&pDevice);
			if (pDevice){
				ULONG ref = pDevice->Release();
				pDevice = NULL;
			}
			pContext->Release();

			pBurnUp = IBurnUp::Create(&XMFLOAT3(-3.5f,-1.0f,0));
		}

	}
	//  デバッグ表示の用意
	g_uiCount=0;

	pTimer->Reset();	//  タイマーをリセット

	//==========================================
	// メイン メッセージ ループ:
    //  (2)メッセージループ
    FLOAT	fAngle = 0;
	FLOAT	fRadius = 0.0f;
	FLOAT	fX, fY, fZ;
	FLOAT	fSign = 1.0f;
	INT		iPhase = 0;
	fY = 0.f;
	while(true){
        if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)){
            if(msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }else{
			double	elapsed = pTimer->GetElapsedTimeAndReset();
			if (elapsed > 2.0)
				elapsed = 2.0;

			//  アニメーション関連
			if (ppMTAnimator[0]!= NULL){
				static BOOL bKeyFlag[] = {false,false,false,false,false,false,false,false,false,false};
				static const char keyChar[] = {'1','2','3','4','5','6','7','8','9'};
				for (int i = 0; i < _countof(keyChar); ++i){
					if (GetAsyncKeyState(keyChar[i])){
						if (!bKeyFlag[i]){
							ppMTAnimator[0]->SetActiveAnimationSet(i,4.0f,true);
							bKeyFlag[i] = true;
						}
					}else{
						bKeyFlag[i] = false;
					}
				}
			}
			
			if (pBurnUp){
				pBurnUp->Update((FLOAT)elapsed);
			}

			if (pAnimatedMesh){
				pAnimatedMesh->Update((FLOAT)elapsed*0.1f);
			}

			fAngle += (FLOAT)(0.0675f * elapsed);
			if (fAngle > XM_PI*2.0f){
				fAngle -= XM_PI*2.0f;
			}

			fRadius += fSign * (FLOAT)(0.0675f/16.f)*elapsed;
			if (fRadius > 1.0f && fSign > 0){
				fSign = -1.0f;
			}else if (fRadius < 0.0f && fSign < 0){
				fSign = 1.0f;
				iPhase = (iPhase + 1) % 8;
			}
			switch(iPhase){
			case	0:
				fX = 0.0f;
				fZ = -fRadius*4.f;
				break;
			case	1:
				fX = -fRadius*4.f;
				fZ = -fRadius*4.f;
				break;
			case	2:
				fX = -fRadius*4.f;
				fZ = 0.0;
				break;
			case	3:
				fX = -fRadius*4.f;
				fZ =  fRadius*4.f;
				break;
			case	4:
				fX = 0.0f;
				fZ = fRadius*4.f;
				break;
			case	5:
				fX =  fRadius*4.f;
				fZ =  fRadius*4.f;
				break;
			case	6:
				fX = fRadius*4.f;
				fZ = 0.0f;
				break;
			case	7:
				fX =  fRadius*4.f;
				fZ = -fRadius*4.f;
				break;
			}
			fX += 0.25f * cosf(fAngle);
			fZ += 0.25f * sinf(fAngle);
			if (pFloor){
				FLOAT	fAlt,fDist;
				XMFLOAT3 vecMin, vecMax;
				XMFLOAT3 vecNormal;
				vecMin = XMFLOAT3(fX-2.f,-5.f,fZ-2.f);
				vecMax = XMFLOAT3(fX+2.f,+5.f,fZ+2.f);
				
				if (pFloor->ProbeTheGroundAltitude(&XMFLOAT3(fX,0,fZ),&vecMin,&vecMax,&vecNormal,&fAlt,&fDist)){
					fY = fAlt;
				}
			}

			ID3D11DeviceContext *pContext = g_pD3DContext->GetDeviceContext();

			//  指定色で画面クリア
			float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; //red,green,blue,alpha
			g_pD3DContext->SetDepthEnable(true,true);
			g_pD3DContext->ClearRenderTargetView( ClearColor );
			g_pD3DContext->ClearDepthStencilView(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
			{
				if (pTracker && pBGMesh){
					XMMATRIX matWorld = XMMatrixIdentity();
					XMMATRIX matCamera;
					XMMATRIX matCamView;
					XMVECTOR eye,at,up;
					eye = XMLoadFloat3(&XMFLOAT3(0,0,0));
					at  = XMLoadFloat3(&XMFLOAT3(0,0,1.0f));
					up  = XMLoadFloat3(&XMFLOAT3(0,1.0f,0));
					pTracker->GetTransform(&matCamera);
					eye = XMVector3TransformCoord(eye,matCamera);
					at  = XMVector3TransformCoord(at, matCamera);
					up  = XMVector3TransformNormal(up,matCamera);
					matCamView = XMMatrixLookAtLH(eye,at,up);
					pBGMesh->SetViewMatrix(&matCamView);
					pBGMesh->SetWorldMatrix(&matWorld);
					pBGMesh->SetProjectionMatrix(&matProj);
					pBGMesh->Render(pContext);
				}
			}

			g_pD3DContext->ClearDepthStencilView(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
			if (pFloor){
				XMMATRIX matWorld = XMMatrixIdentity();
				pFloor->SetProjectionMatrix(&matProj);
				pFloor->SetViewMatrix(&matView);
				pFloor->SetWorldMatrix(&matWorld);
				pFloor->Render(pContext);
			}
			//g_pD3DContext->ClearDepthStencilView(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );

			if (pContext){
				if (ppMTAnimator[0] != NULL){
					//  CMTAnimator を使って描画
					XMMATRIX matWorld;
					XMMATRIX matRotation = DirectX::XMMatrixRotationY(fAngle);
					XMMATRIX matScaling = XMMatrixScaling(3.0f,3.0f,3.0f);
					XMMATRIX matTranslation = XMMatrixTranslation(3.0f,-2.0f,0.0f);
					matWorld = matScaling * matRotation * matTranslation;
					pSkinnedMesh->SetProjectionMatrix(&matProj);
					pSkinnedMesh->SetViewMatrix(&matView);
					ppMTAnimator[0]->Update((FLOAT)elapsed,&matWorld);
				
					if (!GetAsyncKeyState(VK_SPACE))
						ppMTAnimator[0]->Render(pContext);
					else if (pBoneRenderer != NULL){
						ppMTAnimator[0]->UpdateBones();
						pBoneRenderer->SetProjectionMatrix(&matProj);
						pBoneRenderer->SetViewMatrix(&matView);
						pBoneRenderer->Render(pContext);
					}
					matTranslation = XMMatrixTranslation(-3.0f,-2.0f,0.0f);
					matWorld = matScaling * matTranslation;
					ppMTAnimator[1]->Update((FLOAT)elapsed,&matWorld);
					ppMTAnimator[1]->Render(pContext);

				}else if (pSkinnedMesh != NULL){
					DirectX::XMMATRIX matWorld = DirectX::XMMatrixRotationY(fAngle);
					XMMATRIX matScaling = XMMatrixScaling(3.0f,3.0f,3.0f);
					XMMATRIX matTranslation = XMMatrixTranslation(1.5f,-1.5f,0.0f);
					matWorld = matScaling * matWorld * matTranslation;
					pSkinnedMesh->SetProjectionMatrix(&matProj);
					pSkinnedMesh->SetViewMatrix(&matView);
					pSkinnedMesh->Update(elapsed,&matWorld);
					IAnimator *pAnimator = pSkinnedMesh->GetAnimator();
					if (pAnimator != NULL)
						pAnimator->AdjustAnimation();
					if (!GetAsyncKeyState(VK_SPACE))
						pSkinnedMesh->Render(pContext);
					else if (pBoneRenderer != NULL){
						pBoneRenderer->SetProjectionMatrix(&matProj);
						pBoneRenderer->SetViewMatrix(&matView);
						pBoneRenderer->Render(pContext);
					}
				}
			
				if (pSimpleMesh != NULL){
					//static XMMATRIX matWorld = XMMatrixRotationY(XM_PIDIV2);
					DirectX::XMMATRIX matWorld = DirectX::XMMatrixRotationY(fAngle);
					XMMATRIX matTranslation = XMMatrixTranslation(fX,fY+1.0f,fZ);
					XMMATRIX matScaling = XMMatrixScaling(1.0f,1.0f,1.0f);
					matWorld = matScaling * matWorld * matTranslation;
					pSimpleMesh->SetProjectionMatrix(&matProj);
					pSimpleMesh->SetViewMatrix(&matView);
					pSimpleMesh->SetWorldMatrix(&matWorld);
					pSimpleMesh->Render(pContext);
				}

				g_pD3DContext->SetDepthEnable(TRUE,FALSE);	//  Disable Z-Buffer

				if (pPointSprite){
					XMMATRIX matWorld = XMMatrixIdentity();
					pPointSprite->SetProjectionMatrix(&matProj);
					pPointSprite->SetViewMatrix(&matView);
					pPointSprite->SetWorldMatrix(&matWorld);

					if (pBurnUp){
						pBurnUp->Render(pContext,pPointSprite);
					}
				}

				g_pD3DContext->SetDepthEnable(FALSE,FALSE);	//  Disable Z-Buffer
				if (pSprite)
					pSprite->Render(pContext,852.f-256.f,10.f,256.f,256.f,0.f,0.f,256.f,256.f,0);


				//  フォントを描画
				//pFont->Render(pContext);
				FLOAT x = 0.0f, y = 0.0f;
				TCHAR tmp[32];
				int frame, sec, min, hour, day;
				frame = g_uiCount % 60;
				sec = (g_uiCount / 60) % 60;
				min = (g_uiCount / 3600) % 60;
				hour = (g_uiCount / (3600*60)) % 24;
				day =  (g_uiCount / (3600*60*24)) % 99;
				_stprintf_s(tmp,_T("%02d:%02d:%02d:%02d:%02d"),day,hour,min,sec,frame);
				pFont->DrawAnkText(pContext,tmp,XMFLOAT4(1.0f,1.0f,1.0f,0.75f),x,y);
				pFont->DrawAnkText(pContext,g_pD3DContext->GetDriverTypeText(),XMFLOAT4(1.0f,0.25f,0.25f,1.0f),x,y+=24.f);
				pFont->DrawAnkText(pContext,g_pD3DContext->GetFeatureLevelText(),XMFLOAT4(0.25f,1.0f,0.25f,1.0f),x,y+=24.f);
				g_uiCount++;
			}
			//結果をウインドウに反映
			g_pD3DContext->Present( 1, 0 );
			pContext->Release();
			pContext = NULL;
		}
    }

ERROR_EXIT:
	if (pTimer != NULL){
		delete pTimer;
		pTimer = NULL;
	}
	for (int i = 0; i < _countof(ppMTAnimator) ; ++i){
		SAFE_DELETE(ppMTAnimator[i]);   //  Destroy MTAnimator
	}
	SAFE_DELETE(pBurnUp);
	SAFE_DELETE(pPointSprite);
	SAFE_DELETE(pSprite);
	SAFE_DELETE(pFont);
	{
		ID3D11DeviceContext *pContext = g_pD3DContext->GetDeviceContext();
		ID3D11Device *pDevice = NULL;
		pContext->GetDevice(&pDevice);
		if (pDevice){
			ULONG ref = pDevice->Release();
			pDevice = NULL;
		}
		pContext->Release();
	}
	SAFE_DELETE(pFloor);		//  Destroy MeshCollider
	SAFE_DELETE(pSimpleMesh);   //  Destroy SimpleMesh
	SAFE_DELETE(pBoneRenderer); //  Destroy BoneRenderer
	SAFE_DELETE(pSkinnedMesh);  //  Destroy Skin Mesh
	SAFE_DELETE(pTracker);		//  Motion Tracker
	SAFE_DELETE(pAnimatedMesh); //  Destroy Animated Mesh
	SAFE_DELETE(pBGMesh);
	{
		ID3D11DeviceContext *pContext = g_pD3DContext->GetDeviceContext();
		ID3D11Device *pDevice = NULL;
		pContext->GetDevice(&pDevice);
		if (pDevice){
			ULONG ref = pDevice->Release();
			pDevice = NULL;
		}
		pContext->Release();
	}
	SAFE_DELETE(g_pD3DContext);

	CoUninitialize();
	return (int) msg.wParam;
}




//
//	関数：WndProc
//	説明：ウインドウに渡されたイベントのハンドラ
//
LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{

    switch (message){
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return    0;
}



