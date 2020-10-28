/* =============================================================
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
	case DIK_X:
		mario->SetState(MARIO_STATE_JUMP);
		break;
	case DIK_A:		// reset
		mario->SetState(MARIO_STATE_IDLE_RIGHT);
		mario->SetLevel(MARIO_LEVEL_BIG);
		mario->SetPosition(50.0f,0.0f);
		mario->SetSpeed(0, 0);
		break;
	case DIK_L:		// đổi dạng
		int Level = mario->GetLevel();
		int State = mario->GetState();
		if (Level == MARIO_LEVEL_TAIL) 
		{
			float px, py;
			mario->GetPosition(px, py);
			mario->SetLevel(MARIO_LEVEL_SMALL);
			mario->SetPosition(px + 1.0f, py + 12.00f);
		}
		else if (Level == MARIO_LEVEL_SMALL)
		{
			float px, py;
			mario->GetPosition(px, py);
			mario->SetLevel(MARIO_LEVEL_BIG);
			mario->SetPosition(px - 1.0f, py - 12.00f);
		}
		else
		{
			float px, py;
			mario->GetPosition(px, py);
			mario->SetLevel(MARIO_LEVEL_TAIL);
			mario->SetPosition(px, py - 1.0f);
			break;
		}
	}
	
}

void CSampleKeyHander::OnKeyUp(int KeyCode)
{
	DebugOut(L"[INFO] KeyUp: %d\n", KeyCode);
	switch (KeyCode)
	{
	case DIK_RIGHT:
		mario->SetState(MARIO_STATE_IDLE_RIGHT);
		break;
	
	case DIK_LEFT:
		mario->SetState(MARIO_STATE_IDLE_LEFT);
		break;
	}
}

void CSampleKeyHander::KeyState(BYTE *states)
{
	// disable control key when Mario die 
	if (mario->GetState() == MARIO_STATE_DIE) return;

	if (game->IsKeyDown(DIK_RIGHT))
		mario->SetState(MARIO_STATE_WALKING_RIGHT);

	else if (game->IsKeyDown(DIK_LEFT))
		mario->SetState(MARIO_STATE_WALKING_LEFT);

	//else if (game->IsKeyDown(DIK_DOWN))
	//	mario->SetState(MARIO_STATE_SIT);
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
	sprites->Add(10000, 215, 243, 230, 270, texMario);			// idle right
	
	sprites->Add(10010, 176, 243, 191, 270, texMario);			// idle left

	sprites->Add(10001, 255, 243, 271, 270, texMario);			// walk right
	sprites->Add(10002, 295, 243, 311, 270, texMario);
	
	sprites->Add(10011, 135, 243, 151, 270, texMario);			// walk left
	sprites->Add(10012, 95, 243, 111, 270, texMario);

	sprites->Add(10020, 335, 243, 351, 270, texMario);			// jump right

	sprites->Add(10021, 55, 243, 71, 270, texMario);			// jump left

	sprites->Add(10090, 376, 247, 390, 266, texMario);			// sit right

	sprites->Add(10091, 16, 247, 30, 266, texMario);			// sit left


	// Mario small
	sprites->Add(10100, 216, 89, 229, 104, texMario);			// idle right

	sprites->Add(10110, 177, 89, 189, 104, texMario);			// idle left

	sprites->Add(10101, 256, 89, 271, 104, texMario);			// walk right		

	sprites->Add(10111, 135, 89, 150, 104, texMario);			// walk left

	sprites->Add(10120, 334, 89, 351, 104, texMario);			// jump right

	sprites->Add(10121, 55, 89, 72, 104, texMario);				// jump left

	sprites->Add(10199, 375, 208, 391, 225, texMario);			// die 



	// Mario tail
	
	sprites->Add(10200, 211, 443, 233, 472, texMario);			// idle right

	sprites->Add(10210, 173, 443, 195, 472, texMario);			// idle left

	sprites->Add(10201, 261, 443, 284, 472, texMario);			// walk right
	sprites->Add(10202, 291, 443, 314, 472, texMario);

	sprites->Add(10211, 122, 443, 145, 472, texMario);			// walk left
	sprites->Add(10212, 92, 443, 116, 472, texMario);

	sprites->Add(10220, 330, 443, 354, 472, texMario);			// jump right

	sprites->Add(10221, 52, 443, 76, 472, texMario);			// jump left

	/*
	// sprite đòn quay đuôi
	sprites->Add(10230, 0, 482, 16, 511, texMario);				// quay mặt sau
	sprites->Add(10231, 147, 444, 171, 472, texMario);			// quay mặt bên trái
	sprites->Add(10232, 235, 444, 259, 472, texMario);			// quay mặt bên phải
	sprites->Add(10233, 34, 482, 51, 511, texMario);			// quay hướng chính diện


	// sprite đập đuôi khi rơi xuống nhẹ
	sprites->Add(10222, 210, 602, 234, 631, texMario);			// right
	sprites->Add(10223, 251, 602, 274, 631, texMario);

	sprites->Add(10224, 172, 602, 196, 632, texMario);			// left
	sprites->Add(10225, 132, 602, 155, 632, texMario);
	*/


	// Mario fire
	/*
	sprites->Add(10300, 216, 682, 230, 710, texMario);			// idle right

	sprites->Add(10310, 176, 682, 191, 710, texMario);			// idle left

	sprites->Add(10301, 255, 682, 271, 710, texMario);			// walk right
	sprites->Add(10302, 295, 682, 311, 710, texMario);

	sprites->Add(10311, 135, 682, 151, 710, texMario);			// walk left
	sprites->Add(10312, 95, 682, 111, 710, texMario);

	sprites->Add(10320, 335, 682, 351, 710, texMario);			// jump right

	sprites->Add(10321, 55, 682, 71, 710, texMario);			// jump left

	sprites->Add(10330, 365, 802, 380, 830, texMario);			// fire right
	sprites->Add(10331, 385, 802, 400, 830, texMario);

	sprites->Add(10330, 26, 802, 41, 830, texMario);			// fire left
	sprites->Add(10331, 6, 802, 21, 830, texMario);
	*/

	LPDIRECT3DTEXTURE9 texMisc = textures->Get(ID_TEX_MISC);
	sprites->Add(20001, 408, 225, 424, 241, texMisc);

	LPDIRECT3DTEXTURE9 texEnemy = textures->Get(ID_TEX_ENEMY);
	sprites->Add(30001, 5, 14, 21, 29, texEnemy);
	sprites->Add(30002, 25, 14, 41, 29, texEnemy);

	sprites->Add(30003, 45, 21, 61, 29, texEnemy);	// die sprite


	LPANIMATION ani;
	
	// Tạo animation
	// Mario big
	ani = new CAnimation(70);	// big idle right
	ani->Add(10000);
	animations->Add(1000, ani); 

	ani = new CAnimation(70);	// big idle left
	ani->Add(10010);
	animations->Add(1010, ani);

	ani = new CAnimation(70);	// big walk right
	ani->Add(10001);
	ani->Add(10002);
	ani->Add(10000);
	animations->Add(1001, ani);

	ani = new CAnimation(70);	// big walk left
	ani->Add(10011);
	ani->Add(10012);
	ani->Add(10010);
	animations->Add(1011, ani);

	ani = new CAnimation(100);	// big jump right
	ani->Add(10020);
	animations->Add(1020, ani);

	ani = new CAnimation(100);	// big jump left
	ani->Add(10021);
	animations->Add(1021, ani);



	// Mario small
	ani = new CAnimation(70);	// small idle right
	ani->Add(10100);
	animations->Add(1100, ani);

	ani = new CAnimation(70);	// small idle left
	ani->Add(10110);
	animations->Add(1110, ani);

	ani = new CAnimation(50);	// small walk right
	ani->Add(10101);
	ani->Add(10100);
	animations->Add(1101, ani);

	ani = new CAnimation(50);	// small walk left
	ani->Add(10111);
	ani->Add(10110);
	animations->Add(1111, ani);

	ani = new CAnimation(100);	// small jump right
	ani->Add(10120);
	animations->Add(1120, ani);

	ani = new CAnimation(100);	// small jump left
	ani->Add(10121);
	animations->Add(1121, ani);

	ani = new CAnimation(100);		// Mario die
	ani->Add(10199);
	animations->Add(1199, ani);


	// Mario tail
	
	ani = new CAnimation(70);	// tail idle right
	ani->Add(10200);
	animations->Add(1200, ani);

	ani = new CAnimation(70);	// tail idle left
	ani->Add(10210);
	animations->Add(1210, ani);

	ani = new CAnimation(70);	// tail walk right
	ani->Add(10201);
	ani->Add(10202);
	ani->Add(10200);
	animations->Add(1201, ani);

	ani = new CAnimation(70);	// tail walk left
	ani->Add(10211);
	ani->Add(10212);
	ani->Add(10210);
	animations->Add(1211, ani);

	ani = new CAnimation(100);	// tail jump right
	ani->Add(10220);
	animations->Add(1220, ani);

	ani = new CAnimation(100);	// tail jump left
	ani->Add(10221);
	animations->Add(1221, ani);
	/*
	ani = new CAnimation(70);	// tail hit right
	ani->Add(10230);
	ani->Add(10231);
	ani->Add(10233);
	ani->Add(10232);
	animations->Add(1230, ani);

	ani = new CAnimation(70);	// tail hit left
	ani->Add(10230);
	ani->Add(10232);
	ani->Add(10233);
	ani->Add(10231);
	animations->Add(1231, ani);

	ani = new CAnimation(70);	// tail fly right
	ani->Add(10222);
	ani->Add(10220);
	ani->Add(10223);
	animations->Add(1222, ani);
	
	ani = new CAnimation(70);	// tail fly left
	ani->Add(10224);
	ani->Add(10221);
	ani->Add(10225);
	animations->Add(1223, ani);
	*/


	// Mario fire
	/*
	ani = new CAnimation(70);	// fire idle right
	ani->Add(10300);
	animations->Add(1300, ani); 

	ani = new CAnimation(70);	// fire idle left
	ani->Add(10310);
	animations->Add(1310, ani);

	ani = new CAnimation(70);	// fire walk right
	ani->Add(10301);
	ani->Add(10302);
	ani->Add(10300);
	animations->Add(1301, ani);

	ani = new CAnimation(70);	// fire walk left
	ani->Add(10311);
	ani->Add(10312);
	ani->Add(10310);
	animations->Add(1311, ani);

	ani = new CAnimation(100);	// fire jump right
	ani->Add(10320);
	animations->Add(1320, ani);

	ani = new CAnimation(100);	// fire jump left
	ani->Add(10321);
	animations->Add(1321, ani);

	ani = new CAnimation(70);	// fire shoot right
	ani->Add(10330);
	ani->Add(10331)
	animations->Add(1330, ani);

	ani = new CAnimation(70);	// fire shoot left
	ani->Add(10332);
	ani->Add(10333)
	animations->Add(1331, ani);
	*/

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



	// Add animation vào Mario
	mario = new CMario();
	// Mario big
	mario->AddAnimation(1000);		// idle right
	mario->AddAnimation(1010);		// idle left
	mario->AddAnimation(1001);		// walk right
	mario->AddAnimation(1011);		// walk left
	mario->AddAnimation(1020);		// jump right
	mario->AddAnimation(1021);		// jump left


	// Mario small
	mario->AddAnimation(1100);		// idle right
	mario->AddAnimation(1110);		// idle left
	mario->AddAnimation(1101);		// walk right
	mario->AddAnimation(1111);		// walk left
	mario->AddAnimation(1120);		// jump right
	mario->AddAnimation(1121);		// jump left

	mario->AddAnimation(1199);		// die


	// Mario tail
	
	mario->AddAnimation(1200);		// idle right
	mario->AddAnimation(1210);		// idle left
	mario->AddAnimation(1201);		// walk right
	mario->AddAnimation(1211);		// walk left
	mario->AddAnimation(1220);		// jump right
	mario->AddAnimation(1221);		// jump left
	/*
	mario->AddAnimation(1230);		// hit right
	mario->AddAnimation(1231);		// hit left
	mario->AddAnimation(1222);		// fly right
	mario->AddAnimation(1223);		// fly left
	*/


	// Mario fire
	/*
	mario->AddAnimation(1300);		// idle right
	mario->AddAnimation(1310);		// idle left
	mario->AddAnimation(1301);		// walk right
	mario->AddAnimation(1311);		// walk left
	mario->AddAnimation(1320);		// jump right
	mario->AddAnimation(1321);		// jump left
	mario->AddAnimation(1230);		// shoot right
	mario->AddAnimation(1231);		// shoot left
	*/
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