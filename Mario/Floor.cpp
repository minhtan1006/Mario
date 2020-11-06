#include "Floor.h"

void CFloor::Render()
{
	animations[0]->Render(x, y);
	//RenderBoundingBox();
}

void CFloor::GetBoundingBox(float& l, float& t, float& r, float& b)
{
	l = x;
	t = y;
	r = x + FLOOR_BBOX_WIDTH;
	b = y + FLOOR_BBOX_HEIGHT;
}