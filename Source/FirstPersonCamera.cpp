#include "stdafx.h"
#include "FirstPersonCamera.h"
using namespace DirectX;

void FirstPersonCamera::move(float d)
{
	pos += (dir*d);
	lookAt = pos + dir;
}

void FirstPersonCamera::turn(float theta)
{
	//| x'| | x cos ? + z sin ? |
	//| y'|=|		  y			| 
	//| z'| |?x sin ? + z cos ?	|
	XMFLOAT3 dirF3;
	XMStoreFloat3(&dirF3, dir);
	dirF3.x = dirF3.x * cos(theta) + dirF3.z*sin(theta);
	dirF3.z = -dirF3.x * sin(theta) + dirF3.z*cos(theta);
	dir = XMLoadFloat3(&dirF3);
	dir=XMVector3Normalize(dir);
	lookAt = pos + dir;
}

void FirstPersonCamera::setHeight(float y)
{
	pos.vector4_f32[1]=y;
	//pos.m128_f32[1] = y;
	lookAt = pos + dir;
}

void FirstPersonCamera::elevate(float theta) 
{
	//| x'| |		  x			|
	//| y'|=| y cos ? ? z sin ?	| 
	//| z'| | y sin ? + z cos ?	|
	XMFLOAT3 dirF3;
	XMStoreFloat3(&dirF3, dir);
	dirF3.y = dirF3.y * cos(theta) - dirF3.z*sin(theta);
	dirF3.z = dirF3.y * sin(theta) + dirF3.z*cos(theta);
	dir = XMLoadFloat3(&dirF3);
	dir=XMVector3Normalize(dir);
	lookAt = pos + dir;
}
