//
// stdafx.h
//

// Pre-compiled header

#pragma once

#include <targetver.h>


// Windows Header Files

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

#include <windows.h>


// Memory handling headers
#include <GUMemory.h>


// C RunTime and STL Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <cstdint>
#include <conio.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <vector>
#include <exception>


// Core types
#include <CGDClock.h>
#include <CGDConsole.h>

// DirectX headers
#include <d3d11_2.h>
#include <dxgi1_3.h>
#include <d2d1_1.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXTK\DDSTextureLoader.h>
#include <DirectXTK\WICTextureLoader.h>

#include <System.h>

using namespace std;
using namespace DirectX;
using DirectX::XMQuaternionRotationRollPitchYaw;
using DirectX::XMQuaternionSlerp;
using DirectX::XMMatrixRotationQuaternion;
using DirectX::XMMatrixRotationZ;
using DirectX::XMMatrixRotationY;
using DirectX::XMMatrixRotationX;
using DirectX::XMMatrixScaling;
using DirectX::XMMatrixTranslation;
using DirectX::XMConvertToRadians;
using DirectX::XMMatrixIdentity;
using DirectX::XMVectorZero;
using DirectX::XMVectorSet;
using DirectX::XMVECTOR;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT2;
using DirectX::XMMATRIX;