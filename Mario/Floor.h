#pragma once
#include "GameObject.h"

#define FLOOR_BBOX_WIDTH  16
#define FLOOR_BBOX_HEIGHT 16

class CFloor : public CGameObject
{
public:
	virtual void Render();
	virtual void GetBoundingBox(float& l, float& t, float& r, float& b);
};