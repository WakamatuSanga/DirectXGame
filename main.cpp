#include<Windows.h>

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	//出力ウインドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	return 0;
}