﻿/* =============================================================
	INTRODUCTION TO GAME PROGRAMMING SE102
	
	SAMPLE 04 - COLLISION

	This sample illustrates how to:

		1/ Implement SweptAABB algorithm between moving objects
		2/ Implement a simple (yet effective) collision frame work

	Key functions: 
		CGame::SweptAABB
		CGameObject::SweptAABBEx
		CGameObject::CalcPotentialCollisions
		CGameObject::FilterCollision

		CGameObject::GetBoundingBox
		
================================================================ */

#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

#include "debug.h"
#include "Game.h"
#include "GameObject.h"
#include "Textures.h"

#include "Mario.h"
#include "Brick.h"
#include "Goomba.h"

#define WINDOW_CLASS_NAME L"SampleWindow"
#define MAIN_WINDOW_TITLE L"04 - Collision"

#define BACKGROUND_COLOR D3DCOLOR_XRGB(255, 255, 200)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define MAX_FRAME_RATE 120

#define ID_TEX_MARIO 0
#define ID_TEX_ENEMY 10
#define ID_TEX_MISC 20

CGame *game;

CMario *mario;
CGoomba *goomba;

vector<LPGAMEOBJECT> objects;

class CSampleKeyHander: public CKeyEventHandler
{
	virtual void KeyState(BYTE *states);
	virtual void OnKeyDown(int KeyCode);
	virtual void OnKeyUp(int KeyCode);
};

CSampleKeyHander * keyHandler; 

void CSampleKeyHander::OnKeyDown(int KeyCode)
{
	DebugOut(L"[INFO] KeyDown: %d\n", KeyCode);
	switch (KeyCode)
	{
	case DIK_SPACE:
		mario->SetState(MARIO_STATE_JUMP);
		break;
	case DIK_A: // reset
		mario->SetState(MARIO_STATE_IDLE);
		mario->SetLevel(MARIO_LEVEL_BIG);
		mario->SetPosition(50.0f,0.0f);
		mario->SetSpeed(0, 0);
		break;
	case DIK_L:		// đổi dạng
		int Level; 
		Level = mario->GetLevel();
		if (Level != MARIO_LEVEL_SMALL) 
		{
			float px, py;
			mario->GetPosition(px, py);
			mario->SetLevel(MARIO_LEVEL_SMALL);
			mario->SetPosition(px, py + 12.00f);
		}
		else 
		{
			float px, py;
			mario->GetPosition(px, py);
			mario->SetLevel(MARIO_LEVEL_BIG);
			mario->SetPosition(px, py - 12.00f);
		}
		break;
	}
	
}

void CSampleKeyHander::OnKeyUp(int KeyCode)
{
	DebugOut(L"[INFO] KeyUp: %d\n", KeyCode);
}

void CSampleKeyHander::KeyState(BYTE *states)
{
	// disable control key when Mario die 
	if (mario->GetState() == MARIO_STATE_DIE) return;
	if (game->IsKeyDown(DIK_RIGHT))
		mario->SetState(MARIO_STATE_WALKING_RIGHT);
	else if (game->IsKeyDown(DIK_LEFT))
		mario->SetState(MARIO_STATE_WALKING_LEFT);
	else
		mario->SetState(MARIO_STATE_IDLE);
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

/*
	Load all game resources 
	In this example: load textures, sprites, animations and mario object

	TO-DO: Improve this function by loading texture,sprite,animation,object from file
*/
void LoadResources()
{
	CTextures * textures = CTextures::GetInstance();

	textures->Add(ID_TEX_MARIO, L"textures\\mario.png",D3DCOLOR_XRGB(255, 255, 255));
	textures->Add(ID_TEX_MISC, L"textures\\misc.png", D3DCOLOR_XRGB(176, 224, 248));
	textures->Add(ID_TEX_ENEMY, L"textures\\enemies.png", D3DCOLOR_XRGB(3, 26, 110));


	textures->Add(ID_TEX_BBOX, L"textures\\bbox.png", D3DCOLOR_XRGB(255, 255, 255));


	CSprites * sprites = CSprites::GetInstance();
	CAnimations * animations = CAnimations::GetInstance();
	
	LPDIRECT3DTEXTURE9 texMario = textures->Get(ID_TEX_MARIO);

	// Mario big
	sprites->Add(10001, 246, 154, 260, 181, texMario);		// idle right

	sprites->Add(10002, 275, 154, 290, 181, texMario);		// walk right
	sprites->Add(10003, 304, 154, 321, 181, texMario);

	sprites->Add(10011, 186, 154, 200, 181, texMario);		// idle left

	sprites->Add(10012, 155, 154, 170, 181, texMario);		// walk left
	sprites->Add(10013, 125, 154, 141, 181, texMario);

	sprites->Add(10021, 395, 275, 411, 302, texMario);		// jump right

	sprites->Add(10022, 35, 275, 51, 302, texMario);		// jump left


	// Mario small
	sprites->Add(10101, 247, 0, 259, 15, texMario);			// idle right

	sprites->Add(10102, 275, 0, 291, 15, texMario);			// walk right
	sprites->Add(10103, 306, 0, 320, 15, texMario);			

	sprites->Add(10111, 187, 0, 198, 15, texMario);			// idle left

	sprites->Add(10112, 155, 0, 171, 15, texMario);			// walk left
	sprites->Add(10113, 125, 0, 140, 15, texMario);

	sprites->Add(10121, 395, 80, 411, 96, texMario);		// jump right

	sprites->Add(10122, 35, 80, 51, 96, texMario);			// jump left

	sprites->Add(10199, 215, 120, 231, 135, texMario);		// die 

	LPDIRECT3DTEXTURE9 texMisc = textures->Get(ID_TEX_MISC);
	sprites->Add(20001, 408, 225, 424, 241, texMisc);

	LPDIRECT3DTEXTURE9 texEnemy = textures->Get(ID_TEX_ENEMY);
	sprites->Add(30001, 5, 14, 21, 29, texEnemy);
	sprites->Add(30002, 25, 14, 41, 29, texEnemy);

	sprites->Add(30003, 45, 21, 61, 29, texEnemy); // die sprite

	LPANIMATION ani;
	
	// Mario big
	ani = new CAnimation(70);	// big idle right
	ani->Add(10001);
	animations->Add(1001, ani);

	ani = new CAnimation(70);	// big idle left
	ani->Add(10011);
	animations->Add(1011, ani);

	ani = new CAnimation(70);	// big walk right
	ani->Add(10001);
	ani->Add(10002);
	ani->Add(10003);
	animations->Add(1002, ani);

	ani = new CAnimation(70);	// big walk left
	ani->Add(10011);
	ani->Add(10012);
	ani->Add(10013);
	animations->Add(1012, ani);

	ani = new CAnimation(100);	// big jump right
	ani->Add(10021);
	animations->Add(1021, ani);

	ani = new CAnimation(100);	// big jump left
	ani->Add(10022);
	animations->Add(1022, ani);


	// Mario small
	ani = new CAnimation(70);	// small idle right
	ani->Add(10101);
	animations->Add(1101, ani);

	ani = new CAnimation(70);	// small idle left
	ani->Add(10111);
	animations->Add(1111, ani);

	ani = new CAnimation(70);	// small walk right
	ani->Add(10101);
	ani->Add(10102);
	ani->Add(10103);
	animations->Add(1102, ani);

	ani = new CAnimation(70);	// small walk left
	ani->Add(10111);
	ani->Add(10112);
	ani->Add(10113);
	animations->Add(1112, ani);

	ani = new CAnimation(100);	// small jump right
	ani->Add(10121);
	animations->Add(1121, ani);

	ani = new CAnimation(100);	// small jump left
	ani->Add(10122);
	animations->Add(1122, ani);

	ani = new CAnimation(100);		// Mario die
	ani->Add(10199);
	animations->Add(1199, ani);



	ani = new CAnimation(100);		// brick
	ani->Add(20001);
	animations->Add(601, ani);

	ani = new CAnimation(300);		// Goomba walk
	ani->Add(30001);
	ani->Add(30002);
	animations->Add(701, ani);

	ani = new CAnimation(1000);		// Goomba dead
	ani->Add(30003);
	animations->Add(702, ani);

	mario = new CMario();
	// big
	mario->AddAnimation(1001);		// idle right
	mario->AddAnimation(1011);		// idle left
	mario->AddAnimation(1002);		// walk right
	mario->AddAnimation(1012);		// walk left
	mario->AddAnimation(1021);		// jump right
	mario->AddAnimation(1022);		// jump left

	// small
	mario->AddAnimation(1101);		// idle right
	mario->AddAnimation(1111);		// idle left
	mario->AddAnimation(1102);		// walk right
	mario->AddAnimation(1112);		// walk left
	mario->AddAnimation(1121);		// jump right
	mario->AddAnimation(1122);		// jump left

	mario->AddAnimation(1199);		// die

	mario->SetPosition(50.0f, 0);
	objects.push_back(mario);

	for (int i = 0; i < 5; i++)
	{
		CBrick *brick = new CBrick();
		brick->AddAnimation(601);
		brick->SetPosition(100.0f + i*60.0f, 74.0f);
		objects.push_back(brick);

		brick = new CBrick();
		brick->AddAnimation(601);
		brick->SetPosition(100.0f + i*60.0f, 90.0f);
		objects.push_back(brick);

		brick = new CBrick();
		brick->AddAnimation(601);
		brick->SetPosition(84.0f + i*60.0f, 90.0f);
		objects.push_back(brick);
	}


	for (int i = 0; i < 30; i++)
	{
		CBrick *brick = new CBrick();
		brick->AddAnimation(601);
		brick->SetPosition(0 + i*16.0f, 150);
		objects.push_back(brick);
	}

	// and Goombas 
	for (int i = 0; i < 4; i++)
	{
		goomba = new CGoomba();
		goomba->AddAnimation(701);
		goomba->AddAnimation(702);
		goomba->SetPosition(200 + i*60, 135);
		goomba->SetState(GOOMBA_STATE_WALKING);
		objects.push_back(goomba);
	}

}

/*
	Update world status for this frame
	dt: time period between beginning of last frame and beginning of this frame
*/
void Update(DWORD dt)
{
	// We know that Mario is the first object in the list hence we won't add him into the colliable object list
	// TO-DO: This is a "dirty" way, need a more organized way 

	vector<LPGAMEOBJECT> coObjects;
	for (int i = 1; i < objects.size(); i++)
	{
		coObjects.push_back(objects[i]);
	}

	for (int i = 0; i < objects.size(); i++)
	{
		objects[i]->Update(dt,&coObjects);
	}


	// Update camera to follow mario
	float cx, cy;
	mario->GetPosition(cx, cy);

	cx -= SCREEN_WIDTH / 2;
	cy -= SCREEN_HEIGHT / 2;

	CGame::GetInstance()->SetCamPos(cx, 0.0f /*cy*/);
}

/*
	Render a frame 
*/
void Render()
{
	LPDIRECT3DDEVICE9 d3ddv = game->GetDirect3DDevice();
	LPDIRECT3DSURFACE9 bb = game->GetBackBuffer();
	LPD3DXSPRITE spriteHandler = game->GetSpriteHandler();

	if (d3ddv->BeginScene())
	{
		// Clear back buffer with a color
		d3ddv->ColorFill(bb, NULL, BACKGROUND_COLOR);

		spriteHandler->Begin(D3DXSPRITE_ALPHABLEND);

		for (int i = 0; i < objects.size(); i++)
			objects[i]->Render();

		spriteHandler->End();
		d3ddv->EndScene();
	}

	// Display back buffer content to the screen
	d3ddv->Present(NULL, NULL, NULL, NULL);
}

HWND CreateGameWindow(HINSTANCE hInstance, int nCmdShow, int ScreenWidth, int ScreenHeight)
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = hInstance;

	wc.lpfnWndProc = (WNDPROC)WinProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASS_NAME;
	wc.hIconSm = NULL;

	RegisterClassEx(&wc);

	HWND hWnd =
		CreateWindow(
			WINDOW_CLASS_NAME,
			MAIN_WINDOW_TITLE,
			WS_OVERLAPPEDWINDOW, // WS_EX_TOPMOST | WS_VISIBLE | WS_POPUP,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			ScreenWidth,
			ScreenHeight,
			NULL,
			NULL,
			hInstance,
			NULL);

	if (!hWnd) 
	{
		OutputDebugString(L"[ERROR] CreateWindow failed");
		DWORD ErrCode = GetLastError();
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

int Run()
{
	MSG msg;
	int done = 0;
	DWORD frameStart = GetTickCount();
	DWORD tickPerFrame = 1000 / MAX_FRAME_RATE;

	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) done = 1;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		DWORD now = GetTickCount();

		// dt: the time between (beginning of last frame) and now
		// this frame: the frame we are about to render
		DWORD dt = now - frameStart;

		if (dt >= tickPerFrame)
		{
			frameStart = now;

			game->ProcessKeyboard();
			
			Update(dt);
			Render();
		}
		else
			Sleep(tickPerFrame - dt);	
	}

	return 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd = CreateGameWindow(hInstance, nCmdShow, SCREEN_WIDTH, SCREEN_HEIGHT);

	game = CGame::GetInstance();
	game->Init(hWnd);

	keyHandler = new CSampleKeyHander();
	game->InitKeyboard(keyHandler);


	LoadResources();

	SetWindowPos(hWnd, 0, 0, 0, SCREEN_WIDTH*2, SCREEN_HEIGHT*2, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

	Run();

	return 0;
}