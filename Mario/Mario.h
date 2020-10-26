#pragma once
#include "GameObject.h"

#define MARIO_WALKING_SPEED		0.1f 
//0.1f
#define MARIO_JUMP_SPEED_Y		0.5f
#define MARIO_JUMP_DEFLECT_SPEED 0.2f
#define MARIO_GRAVITY			0.002f
#define MARIO_DIE_DEFLECT_SPEED	 0.5f

#define MARIO_STATE_IDLE			0
#define MARIO_STATE_WALKING_RIGHT	100
#define MARIO_STATE_WALKING_LEFT	200
#define MARIO_STATE_JUMP			300
#define MARIO_STATE_DIE				400

#define MARIO_ANI_BIG_IDLE_RIGHT			0
#define MARIO_ANI_BIG_IDLE_LEFT				1
#define MARIO_ANI_BIG_WALKING_RIGHT			2
#define MARIO_ANI_BIG_WALKING_LEFT			3
#define MARIO_ANI_BIG_JUMP_RIGHT			4
#define MARIO_ANI_BIG_JUMP_LEFT				5

#define MARIO_ANI_SMALL_IDLE_RIGHT			6
#define MARIO_ANI_SMALL_IDLE_LEFT			7
#define MARIO_ANI_SMALL_WALKING_RIGHT		8
#define MARIO_ANI_SMALL_WALKING_LEFT		9
#define MARIO_ANI_SMALL_JUMP_RIGHT			10
#define MARIO_ANI_SMALL_JUMP_LEFT			11

#define MARIO_ANI_DIE				12

#define	MARIO_LEVEL_SMALL	1
#define	MARIO_LEVEL_BIG		2
#define	MARIO_LEVEL_TAIL	3
#define	MARIO_LEVEL_FIRE	4


#define MARIO_BIG_BBOX_WIDTH  14
#define MARIO_BIG_BBOX_HEIGHT 26

#define MARIO_SMALL_BBOX_WIDTH  13
#define MARIO_SMALL_BBOX_HEIGHT 14

#define MARIO_UNTOUCHABLE_TIME 5000


class CMario : public CGameObject
{

	int untouchable;
	DWORD untouchable_start;
public: 
	CMario() : CGameObject()
	{
		level = MARIO_LEVEL_BIG;
		untouchable = 0;
	}
	virtual void Update(DWORD dt, vector<LPGAMEOBJECT> *colliable_objects = NULL);
	virtual void Render();
	void SetState(int state);
	void SetLevel(int l) { level = l; }
	void StartUntouchable() { untouchable = 1; untouchable_start = GetTickCount(); }

	virtual void GetBoundingBox(float &left, float &top, float &right, float &bottom);
};