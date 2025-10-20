#include <windows.h>
#include <cstdint>

class Input {
public:
	void Initialize(HINSTANCE hInstance, HWND hwnd);
	void Update();
	void Finalize();           // 任意：明示的に後片付けしたい場合
	~Input();                  // Unacquire だけ明示

	// 便利用途
	bool IsDown(int dik) const;      // 押されている
	bool IsPressed(int dik) const;   // 今フレーム押した
	bool IsReleased(int dik) const;  // 今フレーム離した

private:
	// DirectInput
	struct Impl;
	Impl* p_ = nullptr; // PIMPLでWindowsヘッダの汚染を減らすならこう。嫌なら直接メンバでもOK。
};
