#include <Windows.h>
#include "MyGame.h"

// COM例外ハンドラ
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
    return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // COMシステムの初期化
    CoInitializeEx(0, COINIT_MULTITHREADED);
    SetUnhandledExceptionFilter(ExportDump);

    // --- ゲームエンジン（MyGame）の実行 ---
    MyGame* game = MyGame::GetInstance();

    game->Initialize(); // 全ての初期化
    game->Run();        // メインループ（更新と描画）
    game->Finalize();   // 全ての解放処理

    // -------------------

    CoUninitialize();
    return 0;
}