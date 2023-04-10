// header.h: включаемый файл для стандартных системных включаемых файлов
// или включаемые файлы для конкретного проекта
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
#include <windows.h>
// Файлы заголовков среды выполнения C
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define DIRECTINPUT_VERSION 0x0800

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <dinput.h>
#include <directxmath.h>

#include "DDSTextureLoader11.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "D3DInclude.h"

using namespace DirectX;

#define SAFE_RELEASE(A) if ((A) != NULL) { (A)->Release(); (A) = NULL; }
