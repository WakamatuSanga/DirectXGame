#include<Windows.h>
#include<cstdint>
#include<string>
#include<format>
#include<filesystem>// ファイルやディレクトリに関する操作を行うライブラリ
#include<fstream>	// ファイルに描いたり読んだりするライブラリ
#include<chrono>	//時間を扱うライブラリ
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

void Log(/*std::ofstream& os,*/ const std::string& message) {
	//os << message << std::endl;
	OutputDebugStringA(message.c_str());
}



//std::wstring ConvertString(const std::string& str) {
//	if (str.empty())
//	{
//		return std::wstring();
//	}
//}


std:: string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0,
	str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
if (sizeNeeded == 0) {
	return std::string();
}
std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>
		(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
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

	// ログのディレクトリを用意
	std::filesystem::create_directory("logs");
	//現在時刻を取得(UTC時刻)
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	//ログファイルの名前にコンマ何秒はいらないので、削って秒にする
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
	nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	//日本時間(PCの設定時間)に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };
	//formatを使って年月日_時分秒の文字列に変換
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	//時間を使ってファイル名を決定
	std::string logFilePath = std::string("logs/") + dateString + ".log";
	//ファイルを使って書き込み準備
	std::ofstream logStream(logFilePath);

	//文字列を格納する
	std::string str0{ "STRING!!!" };

	//整数を文字列にする
	std::string str1{ std::to_string(10) };
	Log(ConvertString(std::format(L"WSTRING{}\n", L"abc")));
	
	
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