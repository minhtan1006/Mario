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
#include "Floor.h"
#include "Goomba.h"

#define WINDOW_CLASS_NAME L"SampleWindow"
#define MAIN_WINDOW_TITLE L"04 - Collision"

#define BACKGROUND_COLOR D3DCOLOR_XRGB(156, 252, 240)
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
	case DIK_A: // reset
		mario->SetState(MARIO_STATE_IDLE);
		mario->SetLevel(MARIO_LEVEL_BIG);
		mario->SetPosition(2300.0f,100.0f);
		mario->SetSpeed(0, 0);
		break;
	case DIK_L:		// đổi dạng
		int Level = mario->GetLevel();
		int State = mario->GetState();
		if (Level == MARIO_LEVEL_FIRE)
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
		else if (Level == MARIO_LEVEL_BIG)
		{
			float px, py;
			mario->GetPosition(px, py);
			mario->SetLevel(MARIO_LEVEL_TAIL);
			mario->SetPosition(px, py - 1.0f);
		}
		else if (Level == MARIO_LEVEL_TAIL)
		{
			float px, py;
			mario->GetPosition(px, py);
			mario->SetLevel(MARIO_LEVEL_FIRE);
			mario->SetPosition(px, py);
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
	
	if (game->IsKeyDown(NULL))
		mario->SetState(MARIO_STATE_IDLE);

	else if (game->IsKeyDown(DIK_RIGHT))
		mario->SetState(MARIO_STATE_WALKING_RIGHT);
	
	else if (game->IsKeyDown(DIK_LEFT))
		mario->SetState(MARIO_STATE_WALKING_LEFT);
	
	else if (game->IsKeyDown(DIK_DOWN))
	{
		if (mario->GetLevel() != MARIO_LEVEL_SMALL)
		mario->SetState(MARIO_STATE_SIT);
	}
	
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
	sprites->Add(10000, 215, 243, 230, 270, texMario);			// idle right

	sprites->Add(10010, 176, 243, 191, 270, texMario);			// idle left

	sprites->Add(10001, 255, 243, 271, 270, texMario);			// walk right
	sprites->Add(10002, 295, 243, 311, 270, texMario);

	sprites->Add(10011, 135, 243, 151, 270, texMario);			// walk left
	sprites->Add(10012, 95, 243, 111, 270, texMario);

	sprites->Add(10020, 335, 243, 351, 270, texMario);			// jump right

	sprites->Add(10021, 55, 243, 71, 270, texMario);			// jump left

	sprites->Add(10090, 375, 247, 390, 266, texMario);			// sit right

	sprites->Add(10091, 16, 247, 31, 266, texMario);			// sit left


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

	sprites->Add(10290, 371, 447, 394, 466, texMario);			// sit right

	sprites->Add(10291, 12, 447, 35, 466, texMario);			// sit left



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

	sprites->Add(10300, 216, 683, 230, 710, texMario);			// idle right

	sprites->Add(10310, 176, 683, 191, 710, texMario);			// idle left

	sprites->Add(10301, 255, 683, 271, 710, texMario);			// walk right
	sprites->Add(10302, 295, 683, 311, 710, texMario);

	sprites->Add(10311, 135, 683, 151, 710, texMario);			// walk left
	sprites->Add(10312, 95, 683, 111, 710, texMario);

	sprites->Add(10320, 335, 683, 351, 710, texMario);			// jump right

	sprites->Add(10321, 55, 683, 71, 710, texMario);			// jump left

	sprites->Add(10330, 365, 802, 380, 830, texMario);			// fire right
	sprites->Add(10331, 385, 802, 400, 830, texMario);

	sprites->Add(10330, 26, 802, 41, 830, texMario);			// fire left
	sprites->Add(10331, 6, 802, 21, 830, texMario);

	sprites->Add(10390, 375, 687, 390, 706, texMario);			// sit right

	sprites->Add(10391, 16, 687, 31, 706, texMario);			// sit left



	LPDIRECT3DTEXTURE9 texMisc = textures->Get(ID_TEX_MISC);
	sprites->Add(20001, 443, 154, 459, 170, texMisc);		//Floor 1
	sprites->Add(20002, 460, 154, 476, 170, texMisc);		//Floor 2
	sprites->Add(20003, 477, 154, 493, 170, texMisc);		//Floor 3
	sprites->Add(20004, 443, 171, 459, 187, texMisc);		//Floor 4
	sprites->Add(20005, 460, 171, 476, 187, texMisc);		//Floor 5
	sprites->Add(20006, 477, 171, 493, 187, texMisc);		//Floor 6
	sprites->Add(20010, 1004, 18, 1020, 34, texMisc);		//Block đá

	sprites->Add(20101, 69, 35, 85, 51, texMisc);			//Nắp cống trái
	sprites->Add(20102, 86, 35, 102, 51, texMisc);			//Nắp cống phải
	sprites->Add(20103, 69, 52, 85, 68, texMisc);			//Thân cống trái
	sprites->Add(20104, 86, 52, 102, 68, texMisc);			//Thân cống phải

	sprites->Add(21000, 1038, 1, 1054, 17, texMisc);		//Gạch 
	sprites->Add(21001, 1055, 1, 1071, 17, texMisc);
	sprites->Add(21002, 1072, 1, 1088, 17, texMisc);
	sprites->Add(21003, 1089, 1, 1105, 17, texMisc);

	sprites->Add(21010, 1140, 1, 1156, 17, texMisc);		//Khối ?
	sprites->Add(21011, 1157, 1, 1173, 17, texMisc);
	sprites->Add(21012, 1174, 1, 1190, 17, texMisc);
	sprites->Add(21013, 1191, 1, 1207, 17, texMisc);



	LPDIRECT3DTEXTURE9 texEnemy = textures->Get(ID_TEX_ENEMY);
	sprites->Add(30001, 5, 14, 21, 30, texEnemy);
	sprites->Add(30002, 25, 14, 41, 30, texEnemy);

	sprites->Add(30003, 45, 21, 61, 30, texEnemy); // die sprite

	LPANIMATION ani;
	
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

	ani = new CAnimation(100);	// big sit right
	ani->Add(10090);
	animations->Add(1090, ani);

	ani = new CAnimation(100);	// big sit left
	ani->Add(10091);
	animations->Add(1091, ani);


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
	ani = new CAnimation(100);	// tail sit right
	ani->Add(10290);
	animations->Add(1290, ani);

	ani = new CAnimation(100);	// tail sit left
	ani->Add(10291);
	animations->Add(1291, ani);

	// Mario fire

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

	/*
	ani = new CAnimation(70);	// fire shoot right
	ani->Add(10330);
	ani->Add(10331)
	animations->Add(1330, ani);

	ani = new CAnimation(70);	// fire shoot left
	ani->Add(10332);
	ani->Add(10333)
	animations->Add(1331, ani);
	*/
	ani = new CAnimation(100);	// fire sit right
	ani->Add(10390);
	animations->Add(1390, ani);

	ani = new CAnimation(100);	// fire sit left
	ani->Add(10391);
	animations->Add(1391, ani);




	ani = new CAnimation(100);		// Floor 1
	ani->Add(20001);
	animations->Add(20001, ani);

	ani = new CAnimation(100);		// Floor 2
	ani->Add(20002);
	animations->Add(20002, ani);

	ani = new CAnimation(100);		// Floor 3
	ani->Add(20003);
	animations->Add(20003, ani);

	ani = new CAnimation(100);		// Floor 4
	ani->Add(20004);
	animations->Add(20004, ani);

	ani = new CAnimation(100);		// Floor 5
	ani->Add(20005);
	animations->Add(20005, ani);

	ani = new CAnimation(100);		// Floor 6
	ani->Add(20006);
	animations->Add(20006, ani);

	ani = new CAnimation(100);		// Block đá
	ani->Add(20010);
	animations->Add(20010, ani);

	ani = new CAnimation(100);		// Nắp cống trái
	ani->Add(20101);
	animations->Add(20101, ani);

	ani = new CAnimation(100);		// Nắp cống phải
	ani->Add(20102);
	animations->Add(20102, ani);

	ani = new CAnimation(100);		// Thân cống trái
	ani->Add(20103);
	animations->Add(20103, ani);

	ani = new CAnimation(100);		// Thân cống phải
	ani->Add(20104);
	animations->Add(20104, ani);

	ani = new CAnimation(100);		// Khối gạch
	ani->Add(21000);
	ani->Add(21001);
	ani->Add(21002);
	ani->Add(21003);
	animations->Add(21000, ani);

	ani = new CAnimation(100);		// Khối ?
	ani->Add(21010);
	ani->Add(21011);
	ani->Add(21012);
	ani->Add(21013);
	animations->Add(21010, ani);



	ani = new CAnimation(300);		// Goomba walk
	ani->Add(30001);
	ani->Add(30002);
	animations->Add(701, ani);

	ani = new CAnimation(1000);		// Goomba dead
	ani->Add(30003);
	animations->Add(702, ani);


	mario = new CMario();
	// Mario big
	mario->AddAnimation(1000);		// idle right
	mario->AddAnimation(1010);		// idle left
	mario->AddAnimation(1001);		// walk right
	mario->AddAnimation(1011);		// walk left
	mario->AddAnimation(1020);		// jump right
	mario->AddAnimation(1021);		// jump left
	mario->AddAnimation(1090);		// sit right
	mario->AddAnimation(1091);		// sit left

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
	mario->AddAnimation(1290);		// sit right
	mario->AddAnimation(1291);		// sit left

	// Mario fire
	mario->AddAnimation(1300);		// idle right
	mario->AddAnimation(1310);		// idle left
	mario->AddAnimation(1301);		// walk right
	mario->AddAnimation(1311);		// walk left
	mario->AddAnimation(1320);		// jump right
	mario->AddAnimation(1321);		// jump left
	/*
	mario->AddAnimation(1330);		// shoot right
	mario->AddAnimation(1331);		// shoot left
	*/
	mario->AddAnimation(1390);		// sit right
	mario->AddAnimation(1391);		// sit left

	mario->SetPosition(50.0f, 0);
	objects.push_back(mario);

	//vẽ nền
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20001);
		floor->SetPosition(0, 186.0f);
		objects.push_back(floor); 
	}
	for (int fl = 0; fl < 39; fl++)
	{
		CFloor *floor = new CFloor();
		floor->AddAnimation(20002);
		floor->SetPosition((fl + 1) * 16.0f, 186.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20003);
		floor->SetPosition(640.0f, 186.0f);
		objects.push_back(floor);
	}

	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20001);
		floor->SetPosition(656.0f, 170.0f);
		objects.push_back(floor);

		CFloor* floor2 = new CFloor();
		floor2->AddAnimation(20004);
		floor2->SetPosition(656.0f, 186.0f);
		objects.push_back(floor2);
	}
	for (int fl = 0; fl < 27; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20002);
		floor->SetPosition(fl * 16.0f + 672, 170.0f);
		objects.push_back(floor);

		CFloor* floor2 = new CFloor();
		floor2->AddAnimation(20005);
		floor2->SetPosition(fl * 16.0f + 672, 186.0f);
		objects.push_back(floor2);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20003);
		floor->SetPosition(1104.0f, 170.0f);
		objects.push_back(floor);

		CFloor* floor2 = new CFloor();
		floor2->AddAnimation(20006);
		floor2->SetPosition(1104.0f, 186.0f);
		objects.push_back(floor2);
	}

	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20001);
		floor->SetPosition(1184, 186.0f);
		objects.push_back(floor);
	}
	for (int fl = 0; fl < 20; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20002);
		floor->SetPosition(fl * 16.0f + 1200, 186.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20003);
		floor->SetPosition(1520, 186.0f);
		objects.push_back(floor);
	}

	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20001);
		floor->SetPosition(1568, 186.0f);
		objects.push_back(floor);
	}
	for (int fl = 0; fl < 3; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20002);
		floor->SetPosition(fl * 16.0f + 1584, 186.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20003);
		floor->SetPosition(1632, 186.0f);
		objects.push_back(floor);
	}

	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20001);
		floor->SetPosition(1696, 186.0f);
		objects.push_back(floor);
	}
	for (int fl = 0; fl < 34; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20002);
		floor->SetPosition(fl * 16.0f + 1712, 186.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20003);
		floor->SetPosition(2256, 186.0f);
		objects.push_back(floor);
	}

	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20001);
		floor->SetPosition(2288, 186.0f);
		objects.push_back(floor);
	}
	for (int fl = 0; fl < 33; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20002);
		floor->SetPosition(fl * 16.0f + 2304, 186.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20003);
		floor->SetPosition(2832, 186.0f);
		objects.push_back(floor);
	}


	//vẽ ống cống
	for (int pi = 0; pi < 2; pi++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20103);
		floor->SetPosition(352, 170.0f - pi * 16);
		objects.push_back(floor);
	}
	for (int pi = 0; pi < 2; pi++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20104);
		floor->SetPosition(368, 170.0f - pi * 16);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20101);
		floor->SetPosition(352, 138.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20102);
		floor->SetPosition(368, 138.0f);
		objects.push_back(floor);
	}

	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20103);
		floor->SetPosition(1824, 170.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20104);
		floor->SetPosition(1840, 170.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20101);
		floor->SetPosition(1824, 154.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20102);
		floor->SetPosition(1840, 154.0f);
		objects.push_back(floor);
	}

	for (int pi = 0; pi < 2; pi++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20103);
		floor->SetPosition(1888, 170.0f - pi * 16);
		objects.push_back(floor);
	}
	for (int pi = 0; pi < 2; pi++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20104);
		floor->SetPosition(1904, 170.0f - pi * 16);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20101);
		floor->SetPosition(1888, 138.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20102);
		floor->SetPosition(1904, 138.0f);
		objects.push_back(floor);
	}

	//cống dưới
	for (int pi = 0; pi < 2; pi++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20103);
		floor->SetPosition(2288, 170.0f - pi * 16);
		objects.push_back(floor);
	}
	for (int pi = 0; pi < 2; pi++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20104);
		floor->SetPosition(2304, 170.0f - pi * 16);
		objects.push_back(floor);
	}
	//cống trên
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20101);
		floor->SetPosition(2288, -118.0f);
		objects.push_back(floor); 
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20102);
		floor->SetPosition(2304, -118.0f);
		objects.push_back(floor);
	}
	for (int pi = 0; pi < 11; pi++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20103);
		floor->SetPosition(2288, 58.0f - pi * 16);
		objects.push_back(floor);
	}
	for (int pi = 0; pi < 11; pi++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20104);
		floor->SetPosition(2304, 58.0f - pi * 16);
		objects.push_back(floor);
	}



	//vẽ block đá
	for (int fl = 0; fl < 2; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20010);
		floor->SetPosition(fl * 16.0f + 1536, 106.0f);
		objects.push_back(floor);
	}

	for (int fl = 0; fl < 3; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20010);
		floor->SetPosition(fl * 16.0f + 1600, 170.0f);
		objects.push_back(floor);
	}
	for (int fl = 0; fl < 2; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20010);
		floor->SetPosition(fl * 16.0f + 1616, 154.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20010);
		floor->SetPosition(1632, 138.0f);
		objects.push_back(floor);
	}

	for (int fl = 0; fl < 3; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20010);
		floor->SetPosition(fl * 16.0f + 1696, 170.0f);
		objects.push_back(floor);
	}
	for (int fl = 0; fl < 2; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20010);
		floor->SetPosition(fl * 16.0f + 1696, 154.0f);
		objects.push_back(floor);
	}
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20010);
		floor->SetPosition(1696, 138.0f);
		objects.push_back(floor);
	}

	for (int fl = 0; fl < 2; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20010);
		floor->SetPosition(fl * 16.0f + 2288, 138.0f);
		objects.push_back(floor);
	}
	for (int fl = 0; fl < 2; fl++)
	{
		CFloor* floor = new CFloor();
		floor->AddAnimation(20010);
		floor->SetPosition(fl * 16.0f + 2288, 74.0f);
		objects.push_back(floor);
	}


	//vẽ block gạch
	for (int br = 0; br < 7; br++)
	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21000);
		brick->SetPosition(br * 16.0f + 1984, 170.0f);
		objects.push_back(brick);
	}
	for (int br = 0; br < 5; br++)
	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21000);
		brick->SetPosition(br * 16.0f + 2000, 154.0f);
		objects.push_back(brick);
	}
	for (int br = 0; br < 4; br++)
	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21000);
		brick->SetPosition(br * 16.0f + 2016, 138.0f);
		objects.push_back(brick);
	}

	for (int br = 0; br < 2; br++)
	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21000);
		brick->SetPosition(br * 16.0f + 2128, 170.0f);
		objects.push_back(brick);
	}
	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21000);
		brick->SetPosition(2128.0f, 154.0f);
		objects.push_back(brick);
	}


	//vẽ block ?
	for (int se = 0; se < 2; se++)
	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21010);
		brick->SetPosition(se * 16.0f + 192, 122.0f);
		objects.push_back(brick);
	}
	
	for (int se = 0; se < 2; se++)
	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21010);
		brick->SetPosition(se * 16.0f + 240, 90.0f);
		objects.push_back(brick);
	}

	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21010);
		brick->SetPosition(432, 74.0f);
		objects.push_back(brick);
	}

	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21010);
		brick->SetPosition(688.0f, 154.0f);
		objects.push_back(brick);
	}

	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21010);
		brick->SetPosition(736.0f, 122.0f);
		objects.push_back(brick);
	}

	{
		CBrick* brick = new CBrick();
		brick->AddAnimation(21010);
		brick->SetPosition(1504.0f, 138.0f);
		objects.push_back(brick);
	}

	// Goombas 
	for (int Goombas = 0; Goombas < 4; Goombas++)
	{
		goomba = new CGoomba();
		goomba->AddAnimation(701);
		goomba->AddAnimation(702);
		goomba->SetPosition(200 + Goombas *60, 170);
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

	cx -= SCREEN_WIDTH / 2 - 15.0f;
	cy -= SCREEN_HEIGHT / 2 - 100.0f;

	if (cx < 0)
		CGame::GetInstance()->SetCamPos(0, 0);
	else if (cx > 0 && cx < 2543.0f)
	{
		if (cy >= 0)
			CGame::GetInstance()->SetCamPos(cx, 0);
		else 
			CGame::GetInstance()->SetCamPos(cx, cy);
	}
	else if (cx > 2543.0f)
		CGame::GetInstance()->SetCamPos(2543, 0);


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