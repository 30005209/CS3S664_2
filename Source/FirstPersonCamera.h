#pragma once

#include <DirectXMath.h>
#include "Camera.h"

class FirstPersonCamera : public Camera
{

private:
	DirectX::XMVECTOR dir;
	bool flying = true;

public:
	FirstPersonCamera(ID3D11Device *device) : Camera(device){ dir = XMVectorSet(-1, 0, 1, 1); };
	FirstPersonCamera(ID3D11Device *device, DirectX::XMVECTOR init_pos, DirectX::XMVECTOR init_up, DirectX::XMVECTOR init_dir) :Camera(device, init_pos, init_up, init_pos + init_dir){ dir = init_dir; };// -XMVector4Normalize(init_lookAt - init_pos)
	~FirstPersonCamera() {};
	void setHeight(float y);
	void move(float d);
	void turn(float d);
	void elevate(float d);
	bool getFlying(){ return flying; };
	void setFlying(bool _flying){ flying = _flying; };
	bool toggleFlying(){ flying = !flying; return flying; };
};
