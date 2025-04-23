#include<Windows.h>
#include<cstdint>
#include<string>
#include<format>
#include<filesystem>// ファイルやディレクトリに関する操作を行うライブラリ
#include<fstream>	// ファイルに描いたり読んだりするライブラリ
#include<chrono>	//時間を扱うライブラリ
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
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
	//DXGIファクトリーに生成
	IDXGIFactory7* dxgiFactori = nullptr;
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactori));
	assert(SUCCEEDED(hr));

	//使用するアダプタ用の変数。最初にnullptrを入れておく
	IDXGIAdapter4* useAdapter = nullptr;
	//良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactori->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		//アダプターの情報を取得
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		//ソフトウェアアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			//採用したアダプタの情報をログに出力。wstringの方なので注意
			Log(ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	ID3D12Device* device = nullptr;
	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成出来るか試して行く
	for (size_t i = 0; i < _countof(featureLevels); i++)
	{
		//採用したアダプタでデバイスを生成
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		//指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr))
		{
			//生成出来たのでログ出力を行ってループを抜ける
			Log((std::format("FeatureLevel : {}\n", featureLevelStrings[i])));
			break;
		}
	}
	//デバイスの生成がうまくいかなかったので起動出来ない
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");//初期化完了のログを出す
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