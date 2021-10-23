#include "stdafx.h"
#include "CSommetTerrain.h"

CSommetTerrain::CSommetTerrain(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}



float CSommetTerrain::getX()
{
	return x;
}
float CSommetTerrain::getY()
{
	return y;
}
float CSommetTerrain::getZ()
{
	return z;
}
;
