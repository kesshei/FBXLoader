
#include <windows.h>
#include <DirectXMath.h>
#include <string>
#include "FBXModel.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR cmdLine, int nCmdShow)
{
	FBXModel model = FBXModel();
	model.Load("pet.fbx");
}