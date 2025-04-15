#include<Windows.h>
#include<cstdint>
//ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam) {
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対して、アプリも終了を伝える
		PostQuitMessage(0);
		return 0;
	}
	//標準もメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	WNDCLASS wc{};
	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	//ウィンドウクラス名(何でも良い)
	wc.lpszClassName = L"CG2WindowClass";
	//インスタンスのハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass(&wc);

	//クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	//ウインドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	//クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウインドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,   //利用するクラス名
		L"CG2",             //タイトルバーに文字(何でも良い)
		WS_OVERLAPPEDWINDOW,//よく見るウインドウスタイル
		CW_USEDEFAULT,		//表示X座標(Windowsに任せる)
		CW_USEDEFAULT,		//表示Y座標(WindowsOSに任せる)
		wrc.right- wrc.left,//ウインドウ横幅
		wrc.bottom- wrc.top,//ウインドウ縦幅
		nullptr,			//親ウインドウハンドル
		nullptr,			//メニューハンドル
		wc.hInstance,		//インスタンスハンドル
		nullptr);		//オプション

	//ウインドウを表示する
	ShowWindow(hwnd, SW_SHOW);

	//出力ウインドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	MSG msg{};
	//ウインドウの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT){
		//Windowにメッセージが来てたら最優先で処理させる
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else{
			//ゲームの処理



		}
	}


	return 0;
}