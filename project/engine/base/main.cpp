// --- 定数定義 ---
#define _USE_MATH_DEFINES

// --- Windows / 標準ライブラリ ---
#include <Windows.h>
// --- DirectInput（スライド：準備）---
#define DIRECTINPUT_VERSION 0x0800   // ← dinput.h より前に必ず
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")  // dxguid.lib は既に入っているのでOK
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <strsafe.h>
#include <vector>
#include <random> 
#include "Input.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include <cmath>
#include <algorithm>
#include "SpriteCommon.h"
#include "Sprite.h"
#include "Matrix4x4.h"

// --- Direct3D 12 / DXGI 関連 ---
#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// --- DirectX デバッグ支援 ---
#include <dbghelp.h>
#include <dxgidebug.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "dxguid.lib")

// --- DXC (Shader Compiler) ---
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")

// --- DirectXTex ---
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h" // d3dx12.h はヘッダのみ

// --- ImGui ---
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

// --- blender ---
#include <sstream>
#include <iostream>


// --- その他（必要ならアンコメント） ---
 #include <format>  // C++20 の format 機能


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);
struct Matrix3x3 {
	float m[3][3];
};
struct Particle {
	Transform transform;
	Vector3   velocity;
};
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};
struct Fragment {
	Vector3 position;
	Vector3 velocity;
	Vector3 rotation;
	Vector3 rotationSpeed;
	float alpha;
	bool active;
};
struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};
struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

// CG2_06_02
struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

// 変数//--------------------
// Lightingを有効にする
//

// 16分割
const int kSubdivision = 16;
// 頂点数
int kNumVertices = kSubdivision * kSubdivision * 6;
// --- 列挙体 ---
enum WaveType {
	WAVE_SINE,
	WAVE_CHAINSAW,
	WAVE_SQUARE,
};

enum AnimationType {
	ANIM_RESET,
	ANIM_NONE,
	ANIM_COLOR,
	ANIM_SCALE,
	ANIM_ROTATE,
	ANIM_TRANSLATE,
	ANIM_TORNADO,
	ANIM_PULSE,
	ANIM_AURORA,
	ANIM_BOUNCE,
	ANIM_TWIST,
	ANIM_ALL

};

WaveType waveType = WAVE_SINE;
AnimationType animationType = ANIM_NONE;
float waveTime = 0.0f;
//////////////---------------------------------------
// 関数の作成///
//////////////
#pragma region 行列関数
// 単位行列の作成
Matrix4x4 MakeIdentity4x4() {
	Matrix4x4 result{};
	for (int i = 0; i < 4; ++i)
		result.m[i][i] = 1.0f;
	return result;
}
// 拡大縮小行列S
Matrix4x4 Matrix4x4MakeScaleMatrix(const Vector3& s) {
	Matrix4x4 result = {};
	result.m[0][0] = s.x;
	result.m[1][1] = s.y;
	result.m[2][2] = s.z;
	result.m[3][3] = 1.0f;
	return result;
}

// X軸回転行列R
Matrix4x4 MakeRotateXMatrix(float radian) {
	Matrix4x4 result = {};

	result.m[0][0] = 1.0f;
	result.m[1][1] = std::cos(radian);
	result.m[1][2] = std::sin(radian);
	result.m[2][1] = -std::sin(radian);
	result.m[2][2] = std::cos(radian);
	result.m[3][3] = 1.0f;

	return result;
}
// Y軸回転行列R
Matrix4x4 MakeRotateYMatrix(float radian) {
	Matrix4x4 result = {};

	result.m[0][0] = std::cos(radian);
	result.m[0][2] = std::sin(radian);
	result.m[1][1] = 1.0f;
	result.m[2][0] = -std::sin(radian);
	result.m[2][2] = std::cos(radian);
	result.m[3][3] = 1.0f;

	return result;
}
// Z軸回転行列R
Matrix4x4 MakeRotateZMatrix(float radian) {
	Matrix4x4 result = {};

	result.m[0][0] = std::cos(radian);
	result.m[0][1] = -std::sin(radian);
	result.m[1][0] = std::sin(radian);
	result.m[1][1] = std::cos(radian);
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	return result;
}

// 平行移動行列T
Matrix4x4 MakeTranslateMatrix(const Vector3& tlanslate) {
	Matrix4x4 result = {};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	result.m[3][0] = tlanslate.x;
	result.m[3][1] = tlanslate.y;
	result.m[3][2] = tlanslate.z;

	return result;
}
// 行列の積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result{};
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			for (int k = 0; k < 4; ++k)
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
	return result;
}
// ワールドマトリックス、メイクアフィン
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate,
	const Vector3& translate) {
	Matrix4x4 scaleMatrix = Matrix4x4MakeScaleMatrix(scale);
	Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);
	Matrix4x4 rotateMatrix = Multiply(Multiply(rotateX, rotateY), rotateZ);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	Matrix4x4 worldMatrix =
		Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);
	return worldMatrix;
}
// 4x4 行列の逆行列を計算する関数
Matrix4x4 Inverse(Matrix4x4 m) {
	Matrix4x4 result;
	float det;
	int i;

	result.m[0][0] =
		m.m[1][1] * m.m[2][2] * m.m[3][3] - m.m[1][1] * m.m[2][3] * m.m[3][2] -
		m.m[2][1] * m.m[1][2] * m.m[3][3] + m.m[2][1] * m.m[1][3] * m.m[3][2] +
		m.m[3][1] * m.m[1][2] * m.m[2][3] - m.m[3][1] * m.m[1][3] * m.m[2][2];

	result.m[0][1] =
		-m.m[0][1] * m.m[2][2] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2] +
		m.m[2][1] * m.m[0][2] * m.m[3][3] - m.m[2][1] * m.m[0][3] * m.m[3][2] -
		m.m[3][1] * m.m[0][2] * m.m[2][3] + m.m[3][1] * m.m[0][3] * m.m[2][2];

	result.m[0][2] =
		m.m[0][1] * m.m[1][2] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[3][2] -
		m.m[1][1] * m.m[0][2] * m.m[3][3] + m.m[1][1] * m.m[0][3] * m.m[3][2] +
		m.m[3][1] * m.m[0][2] * m.m[1][3] - m.m[3][1] * m.m[0][3] * m.m[1][2];

	result.m[0][3] =
		-m.m[0][1] * m.m[1][2] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2] +
		m.m[1][1] * m.m[0][2] * m.m[2][3] - m.m[1][1] * m.m[0][3] * m.m[2][2] -
		m.m[2][1] * m.m[0][2] * m.m[1][3] + m.m[2][1] * m.m[0][3] * m.m[1][2];

	result.m[1][0] =
		-m.m[1][0] * m.m[2][2] * m.m[3][3] + m.m[1][0] * m.m[2][3] * m.m[3][2] +
		m.m[2][0] * m.m[1][2] * m.m[3][3] - m.m[2][0] * m.m[1][3] * m.m[3][2] -
		m.m[3][0] * m.m[1][2] * m.m[2][3] + m.m[3][0] * m.m[1][3] * m.m[2][2];

	result.m[1][1] =
		m.m[0][0] * m.m[2][2] * m.m[3][3] - m.m[0][0] * m.m[2][3] * m.m[3][2] -
		m.m[2][0] * m.m[0][2] * m.m[3][3] + m.m[2][0] * m.m[0][3] * m.m[3][2] +
		m.m[3][0] * m.m[0][2] * m.m[2][3] - m.m[3][0] * m.m[0][3] * m.m[2][2];

	result.m[1][2] =
		-m.m[0][0] * m.m[1][2] * m.m[3][3] + m.m[0][0] * m.m[1][3] * m.m[3][2] +
		m.m[1][0] * m.m[0][2] * m.m[3][3] - m.m[1][0] * m.m[0][3] * m.m[3][2] -
		m.m[3][0] * m.m[0][2] * m.m[1][3] + m.m[3][0] * m.m[0][3] * m.m[1][2];

	result.m[1][3] =
		m.m[0][0] * m.m[1][2] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2] -
		m.m[1][0] * m.m[0][2] * m.m[2][3] + m.m[1][0] * m.m[0][3] * m.m[2][2] +
		m.m[2][0] * m.m[0][2] * m.m[1][3] - m.m[2][0] * m.m[0][3] * m.m[1][2];

	result.m[2][0] =
		m.m[1][0] * m.m[2][1] * m.m[3][3] - m.m[1][0] * m.m[2][3] * m.m[3][1] -
		m.m[2][0] * m.m[1][1] * m.m[3][3] + m.m[2][0] * m.m[1][3] * m.m[3][1] +
		m.m[3][0] * m.m[1][1] * m.m[2][3] - m.m[3][0] * m.m[1][3] * m.m[2][1];

	result.m[2][1] =
		-m.m[0][0] * m.m[2][1] * m.m[3][3] + m.m[0][0] * m.m[2][3] * m.m[3][1] +
		m.m[2][0] * m.m[0][1] * m.m[3][3] - m.m[2][0] * m.m[0][3] * m.m[3][1] -
		m.m[3][0] * m.m[0][1] * m.m[2][3] + m.m[3][0] * m.m[0][3] * m.m[2][1];

	result.m[2][2] =
		m.m[0][0] * m.m[1][1] * m.m[3][3] - m.m[0][0] * m.m[1][3] * m.m[3][1] -
		m.m[1][0] * m.m[0][1] * m.m[3][3] + m.m[1][0] * m.m[0][3] * m.m[3][1] +
		m.m[3][0] * m.m[0][1] * m.m[1][3] - m.m[3][0] * m.m[0][3] * m.m[1][1];

	result.m[2][3] =
		-m.m[0][0] * m.m[1][1] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1] +
		m.m[1][0] * m.m[0][1] * m.m[2][3] - m.m[1][0] * m.m[0][3] * m.m[2][1] -
		m.m[2][0] * m.m[0][1] * m.m[1][3] + m.m[2][0] * m.m[0][3] * m.m[1][1];

	result.m[3][0] =
		-m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[1][0] * m.m[2][2] * m.m[3][1] +
		m.m[2][0] * m.m[1][1] * m.m[3][2] - m.m[2][0] * m.m[1][2] * m.m[3][1] -
		m.m[3][0] * m.m[1][1] * m.m[2][2] + m.m[3][0] * m.m[1][2] * m.m[2][1];

	result.m[3][1] =
		m.m[0][0] * m.m[2][1] * m.m[3][2] - m.m[0][0] * m.m[2][2] * m.m[3][1] -
		m.m[2][0] * m.m[0][1] * m.m[3][2] + m.m[2][0] * m.m[0][2] * m.m[3][1] +
		m.m[3][0] * m.m[0][1] * m.m[2][2] - m.m[3][0] * m.m[0][2] * m.m[2][1];

	result.m[3][2] =
		-m.m[0][0] * m.m[1][1] * m.m[3][2] + m.m[0][0] * m.m[1][2] * m.m[3][1] +
		m.m[1][0] * m.m[0][1] * m.m[3][2] - m.m[1][0] * m.m[0][2] * m.m[3][1] -
		m.m[3][0] * m.m[0][1] * m.m[1][2] + m.m[3][0] * m.m[0][2] * m.m[1][1];

	result.m[3][3] =
		m.m[0][0] * m.m[1][1] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1] -
		m.m[1][0] * m.m[0][1] * m.m[2][2] + m.m[1][0] * m.m[0][2] * m.m[2][1] +
		m.m[2][0] * m.m[0][1] * m.m[1][2] - m.m[2][0] * m.m[0][2] * m.m[1][1];

	det = m.m[0][0] * result.m[0][0] + m.m[0][1] * result.m[1][0] +
		m.m[0][2] * result.m[2][0] + m.m[0][3] * result.m[3][0];

	if (det == 0)
		return Matrix4x4{}; // またはエラー処理

	det = 1.0f / det;

	for (i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			result.m[i][j] = result.m[i][j] * det;

	return result;
}
// 透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
	float nearClip, float farClip) {
	Matrix4x4 result = {};

	float f = 1.0f / std::tan(fovY / 2.0f);

	result.m[0][0] = f / aspectRatio;
	result.m[1][1] = f;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = -(nearClip * farClip) / (farClip - nearClip);
	return result;
}
// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
	float bottom, float nearClip, float farClip) {
	Matrix4x4 m = {};

	m.m[0][0] = 2.0f / (right - left);
	m.m[1][1] = 2.0f / (top - bottom);
	m.m[2][2] = 1.0f / (farClip - nearClip);
	m.m[3][0] = -(right + left) / (right - left);
	m.m[3][1] = -(top + bottom) / (top - bottom);
	m.m[3][2] = -nearClip / (farClip - nearClip);
	m.m[3][3] = 1.0f;

	return m;
}
// ビューポート変換行列
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height,
	float minDepth, float maxDepth) {
	Matrix4x4 m = {};

	// 行0：X方向スケーリングと移動
	m.m[0][0] = width / 2.0f;
	m.m[1][1] = -height / 2.0f;
	m.m[2][2] = maxDepth - minDepth;
	m.m[3][0] = left + width / 2.0f;
	m.m[3][1] = top + height / 2.0f;
	m.m[3][2] = minDepth;
	m.m[3][3] = 1.0f;

	return m;
}
// 正規化関数
Vector3 Normalize(const Vector3& v) {
	float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length == 0.0f)
		return { 0.0f, 0.0f, 0.0f };
	return { v.x / length, v.y / length, v.z / length };
}
#pragma endregion

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	// 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリ以下ぶ出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
		time.wYear, time.wMonth, time.wDay, time.wHour,
		time.wMinute);
	HANDLE dumpFileHandle =
		CreateFile(filePath, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processId(このexeのId)とクラッシュ(例外)の発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	// Dumpを出力。MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
		MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	// 他に関連づけられているSEH例外ハンドラがあれば実行。

	return EXCEPTION_EXECUTE_HANDLER;
}

void Log(std::ostream& os, const std::string& message) {
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded =
		MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]),
			static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]),
		static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded =
		WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
			NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
		result.data(), sizeNeeded, NULL, NULL);
	return result;
}

// 関数作成ヒープですか？ 02_03
ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {

	// 頂点リソース用のヒープの設定02_03
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // Uploadheapを使う
	// 頂点リソースの設定02_03
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース。テクスチャの場合はまた別の設定をする02_03
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes; // リソースのサイズ　02_03
	// バッファの場合はこれらは１にする決まり02_03
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり02_03
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 実際に頂点リソースを作る02_03
	ID3D12Resource* vertexResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;
}

// テクスチャデータを読む
ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device,
	D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	UINT numDescriptors,
	bool shaderVisivle) {
	// ディスクリプタヒープの生成02_02
	ID3D12DescriptorHeap* DescriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc{};
	DescriptorHeapDesc.Type = heapType;
	DescriptorHeapDesc.NumDescriptors = numDescriptors;
	DescriptorHeapDesc.Flags = shaderVisivle
		? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		: D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = device->CreateDescriptorHeap(&DescriptorHeapDesc,
		IID_PPV_ARGS(&DescriptorHeap));
	// ディスクリプタヒープが作れなかったので起動できない
	assert(SUCCEEDED(hr)); // 1
	return DescriptorHeap;
}

// クリエイトテクスチャ03_00
ID3D12Resource* CreateTextureResource(ID3D12Device* device,
	const DirectX::TexMetadata& metadata) {
	// 1.metadataをもとにResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);           // Textureの幅
	resourceDesc.Height = UINT(metadata.height);         // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipdmapの数
	resourceDesc.DepthOrArraySize =
		UINT16(metadata.arraySize);        // 奥行き　or 配列Textureの配列数
	resourceDesc.Format = metadata.format; // TextureのFormat
	resourceDesc.SampleDesc.Count = 1;     // サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(
		metadata.dimension); // Textureの次元数　普段使っているのは二次元
	// 2.利用するHeapの設定。非常に特殊な運用。02_04exで一般的なケース版がある
	D3D12_HEAP_PROPERTIES heapProperties{};

	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う//03_00EX

	// 3.Resourceを生成する
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,      // Heapの固定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし
		&resourceDesc,        // Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST, // 初回のResourceState.Textureは基本読むだけ//03_00EX
		nullptr,                        // Clear最適地。使わないのでnullptr
		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}

// データを転送するUploadTextureData関数を作る03_00EX
[[nodiscard]] // 03_00EX
ID3D12Resource*
UploadTextureData(ID3D12Resource* texture,
	const DirectX::ScratchImage& mipImages, ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(),
		mipImages.GetImageCount(), mipImages.GetMetadata(),
		subresources);

	uint64_t intermediateSize = GetRequiredIntermediateSize(
		texture, 0, static_cast<UINT>(subresources.size()));
	ID3D12Resource* intermediate = CreateBufferResource(device, intermediateSize);

	UpdateSubresources(commandList, texture, intermediate, 0, 0,
		static_cast<UINT>(subresources.size()),
		subresources.data());

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);

	return intermediate;
}

// ミップマップです03_00
DirectX::ScratchImage LoadTexture(const std::string& filePath) {
	// テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(
		filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミップマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
		image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
		0, mipImages);
	assert(SUCCEEDED(hr));

	// ミップマップ付きのデータを返す
	return mipImages;
}

//ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device,
//	int32_t width,
//	int32_t height) {
//	// 生成するResourceの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//	resourceDesc.Width = width;
//	resourceDesc.Height = height;
//	resourceDesc.MipLevels = 1;
//	resourceDesc.DepthOrArraySize = 1;
//	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//	resourceDesc.SampleDesc.Count = 1;
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
//
//	// 利用するheapの設定
//	D3D12_HEAP_PROPERTIES heapProperties{};
//	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
//
//	// 深度値のクリア設定
//	D3D12_CLEAR_VALUE depthClearValue{};
//	depthClearValue.DepthStencil.Depth = 1.0f;
//	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//
//	// Resourceの設定
//	ID3D12Resource* resource = nullptr;
//	HRESULT hr = device->CreateCommittedResource(
//		&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
//		D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
//		IID_PPV_ARGS(&resource));
//	assert(SUCCEEDED(hr));
//	return resource;
//}

// 球の頂点生成関数_05_00_OTHER新しい書き方
void GenerateSphereVertices(VertexData* vertices, int kSubdivision,
	float radius) {
	// 経度(360)
	const float kLonEvery = static_cast<float>(M_PI * 2.0f) / kSubdivision;
	// 緯度(180)
	const float kLatEvery = static_cast<float>(M_PI) / kSubdivision;

	for (int latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -static_cast<float>(M_PI) / 2.0f + kLatEvery * latIndex;
		float nextLat = lat + kLatEvery;

		for (int lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			float lon = kLonEvery * lonIndex;
			float nextLon = lon + kLonEvery;

			// verA
			VertexData vertA;
			vertA.position = { cosf(lat) * cosf(lon), sinf(lat), cosf(lat) * sinf(lon),
							  1.0f };
			vertA.texcoord = { static_cast<float>(lonIndex) / kSubdivision,
							  1.0f - static_cast<float>(latIndex) / kSubdivision };
			vertA.normal = { vertA.position.x, vertA.position.y, vertA.position.z };

			// verB
			VertexData vertB;
			vertB.position = { cosf(nextLat) * cosf(lon), sinf(nextLat),
							  cosf(nextLat) * sinf(lon), 1.0f };
			vertB.texcoord = { static_cast<float>(lonIndex) / kSubdivision,
							  1.0f - static_cast<float>(latIndex + 1) / kSubdivision };
			vertB.normal = { vertB.position.x, vertB.position.y, vertB.position.z };

			// vertC
			VertexData vertC;
			vertC.position = { cosf(lat) * cosf(nextLon), sinf(lat),
							  cosf(lat) * sinf(nextLon), 1.0f };
			vertC.texcoord = { static_cast<float>(lonIndex + 1) / kSubdivision,
							  1.0f - static_cast<float>(latIndex) / kSubdivision };
			vertC.normal = { vertC.position.x, vertC.position.y, vertC.position.z };

			// vertD
			VertexData vertD;
			vertD.position = { cosf(nextLat) * cosf(nextLon), sinf(nextLat),
							  cosf(nextLat) * sinf(nextLon), 1.0f };
			vertD.texcoord = { static_cast<float>(lonIndex + 1) / kSubdivision,
							  1.0f - static_cast<float>(latIndex + 1) / kSubdivision };
			vertD.normal = { vertD.position.x, vertD.position.y, vertD.position.z };

			// 初期位置//
			uint32_t startIndex = (latIndex * kSubdivision + lonIndex) * 6;

			vertices[startIndex + 0] = vertA;
			vertices[startIndex + 1] = vertB;
			vertices[startIndex + 2] = vertC;
			vertices[startIndex + 3] = vertC;
			vertices[startIndex + 4] = vertD;
			vertices[startIndex + 5] = vertB;
		}
	}
}



/// CG_02_06
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath,
	const std::string& filename) {

	// 1.中で必要となる変数の宣言
	MaterialData materialData; // 構築するMaterialData

	// 2.ファイルを開く
	std::string line; // ファイルから読んだ１行を格納するもの
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める

	// 3.実際にファイルを読み、MaterialDataを構築していく
	while (std::getline(file, line)) {

		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {

			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;

		}
	}
	// 4.materialDataを返す
	return materialData;
}


ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {



	// 1 ---中で必要となる変数の宣言 ---
	ModelData modelData; //構築するMeadelData
	std::vector<Vector4>positions; //位置
	std::vector<Vector3>normals; //法線
	std::vector<Vector2>texcoords; //テクスチャ座標
	std::string line; //ファイルから読んだ1行を格納するもの

	// 2 ---ファイルを開く---
	std::ifstream file(directoryPath + "/" + filename); //ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める

	// 3 ---ファイルを読み、ModelDataを構築---
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier; // 先頭の識別子を読む

		// ---頂点情報を読む---
		if (identifier == "v") {

			Vector4 position;
			s >> position.x >> position.y >> position.z;

			// 左手座標にする
			position.x *= -1.0f;

			position.w = 1.0f; // 位置はw=1.0f
			positions.push_back(position);

		} else if (identifier == "vt") {

			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;

			texcoord.y = 1.0f - texcoord.y;

			texcoords.push_back(texcoord);

		} else if (identifier == "vn") {

			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;

			// 左手座標にする
			normal.x *= -1.0f;

			normals.push_back(normal);

		} else if (identifier == "f") {

			VertexData triangle[3]; // 三つの頂点を保存

			// --面は三角形限定。その他は未対応--
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {

				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の要素へのindexは「位置/テクスチャ座標/法線」で格納されているので、分割してindexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];

				for (int32_t element = 0; element < 3; ++element) {

					std::string index;

					std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);

				}

				//要素へのindexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];

				// X軸を反転して左手座標系に
				triangle[faceVertex] = { position, texcoord, normal };

			}

			// 逆順にして格納（2 → 1 → 0）
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") {
			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層mtlは存在させるので、ディレクトリ名とファイル名を渡す。
			modelData.material =
				LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData; // 生成したModelDataを返す
}



////////////////////
// 関数の生成ここまで//
////////////////////

// CompileShader関数02_00
IDxcBlob* CompileShader(
	// CompilerするSHaderファイルへのパス02_00
	const std::wstring& filepath,
	// Compilerに使用するprofile02_00
	const wchar_t* profile,
	// 初期化で生成したものを3つ02_00
	IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler, std::ostream& os) {

	// ここの中身を書いていく02_00
	// 1.hlslファイルを読み込む02_00
	// これからシェーダーをコンパイルする旨をログに出す02_00
	Log(os, ConvertString(std::format(L"Begin CompileShader,path:{},profike:{}\n",
		filepath, profile)));
	// hlslファイルを読む02_00
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filepath.c_str(), nullptr, &shaderSource);
	// 読めなかったら止める02_00
	assert(SUCCEEDED(hr));
	// 読み込んだファイルの内容を設定する02_00
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding =
		DXC_CP_UTF8; // UTF8の文字コードであることを通知02_00
	// 2.Compileする
	LPCWSTR arguments[] = {
		filepath.c_str(), // コンパイル対象のhlslファイル名02_00
		L"-E",
		L"main", // エントリーポイントの指定。基本的にmain以外にはしない02_00
		L"-T",
		profile, // shaderProfileの設定02_00
		L"-Zi",
		L"-Qembed_debug", // デバック用の設定を埋め込む02_00
		L"-Od",           /// 最適化を外しておく02_00
		L"-Zpr",           // メモリレイアウトは行優先02_00
		//L"-I", L"resources/shaders"
		L"-HV",
		L"2021"

	};
	// 実際にShaderをコンパイルする02_00
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(

		&shaderSourceBuffer,        // 読み込んだファイル02_00
		arguments,                  // コンパイルオプション02_00
		_countof(arguments),        // コンパイルオプションの数02_00
		includeHandler,             // includeが含まれた諸々02_00
		IID_PPV_ARGS(&shaderResult) // コンパイル結果02_00
	);
	// コンパイルエラーではなくdxcが起動できないなど致命的な状況02_00
	assert(SUCCEEDED(hr));
	// 3.警告、エラーが出ていないか確認する02_00
	// 警告.エラーが出ていたらログに出して止める02_00
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(os, shaderError->GetStringPointer());
		// 警告、エラーダメ絶対02_00
		assert(false);
	}
	// 4.Compile結果を受け取って返す02_00
	// コンパイル結果から実行用のバイナリ部分を取得02_00
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
		nullptr);
	assert(SUCCEEDED(hr));
	// 成功したログを出す02_00
	Log(os,
		ConvertString(std::format(L"Compile Succeeded, path:{}, profike:{}\n ",
			filepath, profile)));
	// もう使わないリソースを解放02_00
	shaderSource->Release();
	shaderResult->Release();
	// 実行用のバイナリを返却02_00
	return shaderBlob;
}

// CG2_05_01_page_5
D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap,
	uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
		descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE
GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap,
	uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
		descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}
/////
// main関数/////-------------------------------------------------------------------------------------------------
//  Windwsアプリでの円とリポウント(main関数)

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    CoInitializeEx(0, COINIT_MULTITHREADED);

    // 誰も補足しなかった場合(Unhandled),補足する関数を登録
    SetUnhandledExceptionFilter(ExportDump);

    // ログディレクトリ作成
    std::filesystem::create_directory("logs");

    // 現在時刻を取得してログファイルを作成
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
        nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    std::chrono::zoned_time loacalTime{ std::chrono::current_zone(), nowSeconds };
    std::string dateString = std::format("{:%Y%m%d_%H%M%S}", loacalTime);
    std::string logFilePath = std::string("logs/") + dateString + ".log";
    std::ofstream logStream(logFilePath);
#pragma region 基盤システムの初期化
    // WinApp の初期化
    WinApp* winApp = new WinApp();
    winApp->Initialize();

    // DirectXCommon の初期化（デバイス・スワップチェーン・RTV/DSV・ImGui 初期化など）
    DirectXCommon* dxCommon = new DirectXCommon();
    dxCommon->Initialize(winApp);

    // ★スプライト共通部の初期化
    SpriteCommon* spriteCommon = new SpriteCommon();
    spriteCommon->Initialize(dxCommon);

    // DirectX リソース取得（DirectXCommon から借りる）
    ID3D12Device* device = dxCommon->GetDevice();
    ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

    // ウィンドウハンドル（必要なら使用）
    HWND hwnd = winApp->GetHwnd();

    // Inputクラスの生成＆初期化
    Input input;
    input.Initialize(winApp);

    Sprite* sprite = new Sprite();
    sprite->Initialize(spriteCommon);

#pragma endregion
    // ★ SRV ヒープは DirectXCommon が持っているものを借りる前提
    //    ※ DirectXCommon に GetSrvDescriptorHeap() がない場合は追加してください
    ID3D12DescriptorHeap* srvDescriptorHeap = dxCommon->GetSrvDescriptorHeap();

    // ===== DXC 初期化（これは DirectXCommon には入れていない前提で main 側に残す） =====
    HRESULT hr = S_OK;
    IDxcUtils* dxcUtils = nullptr;
    IDxcCompiler3* dxcCompiler = nullptr;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    assert(SUCCEEDED(hr));

    IDxcIncludeHandler* includHandler = nullptr;
    hr = dxcUtils->CreateDefaultIncludeHandler(&includHandler);
    assert(SUCCEEDED(hr));

    // ==== ルートシグネチャ（3Dモデル用） ====
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    D3D12_ROOT_PARAMETER rootParameters[4]{};

    // [0] Pixel CBV : Material(b0)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    // [1] Vertex CBV : WVP(b0)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;

    // [2] Pixel SRV(Texture) : t0
    D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    // [3] Pixel CBV : DirectionalLight(b1)
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(
        &descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob
    );
    if (FAILED(hr)) {
        Log(logStream,
            reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    ID3D12RootSignature* rootSignature = nullptr;
    hr = device->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)
    );
    assert(SUCCEEDED(hr));

    // ===== Sprite/Particle 用 RootSignature =====
    D3D12_ROOT_PARAMETER sprParams[3]{};

    // [0] Pixel CBV : Material(b0)
    sprParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    sprParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    sprParams[0].Descriptor.ShaderRegister = 0;

    // [1] Vertex SRV(StructuredBuffer) : t0
    D3D12_DESCRIPTOR_RANGE instRange{};
    instRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    instRange.BaseShaderRegister = 0;
    instRange.NumDescriptors = 1;
    instRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    sprParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    sprParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    sprParams[1].DescriptorTable.pDescriptorRanges = &instRange;
    sprParams[1].DescriptorTable.NumDescriptorRanges = 1;

    // [2] Pixel SRV(Texture) : t0
    D3D12_DESCRIPTOR_RANGE texRange{};
    texRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    texRange.BaseShaderRegister = 0;
    texRange.NumDescriptors = 1;
    texRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    sprParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    sprParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    sprParams[2].DescriptorTable.pDescriptorRanges = &texRange;
    sprParams[2].DescriptorTable.NumDescriptorRanges = 1;

    D3D12_ROOT_SIGNATURE_DESC sprRootDesc{};
    sprRootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    sprRootDesc.pParameters = sprParams;
    sprRootDesc.NumParameters = _countof(sprParams);
    sprRootDesc.pStaticSamplers = staticSamplers;
    sprRootDesc.NumStaticSamplers = _countof(staticSamplers);

    ID3DBlob* sprSigBlob = nullptr;
    ID3DBlob* sprErr = nullptr;
    hr = D3D12SerializeRootSignature(
        &sprRootDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &sprSigBlob,
        &sprErr
    );
    if (FAILED(hr)) {
        Log(logStream,
            reinterpret_cast<char*>(sprErr->GetBufferPointer()));
        assert(false);
    }

    ID3D12RootSignature* spriteRootSignature = nullptr;
    hr = device->CreateRootSignature(
        0,
        sprSigBlob->GetBufferPointer(),
        sprSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&spriteRootSignature)
    );
    assert(SUCCEEDED(hr));

    // ===== テクスチャ読み込み & SRV =====
    DirectX::ScratchImage mipImages = LoadTexture("resources/obj/axis/uvChecker.png");
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    ID3D12Resource* textureResource = CreateTextureResource(device, metadata);
    ID3D12Resource* intermediateResource =
        UploadTextureData(textureResource, mipImages, device, commandList);

    ModelData modelData = LoadObjFile("resources/obj/fence", "fence.obj");

    DirectX::ScratchImage mipImages2 = LoadTexture(modelData.material.textureFilePath);
    const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
    ID3D12Resource* textureResource2 = CreateTextureResource(device, metadata2);
    ID3D12Resource* intermediateResource2 =
        UploadTextureData(textureResource2, mipImages2, device, commandList);

    // ディスクリプタサイズ（SRV）
    const uint32_t descriptorSizeSRV =
        device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // SRV ハンドル計算
    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU =
        GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU =
        GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);

    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 =
        GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 =
        GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);

    // SRV 設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
    srvDesc2.Format = metadata2.format;
    srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

    device->CreateShaderResourceView(
        textureResource, &srvDesc, textureSrvHandleCPU);
    device->CreateShaderResourceView(
        textureResource2, &srvDesc2, textureSrvHandleCPU2);

    // ===== InputLayout / Blend / Rasterizer など（元のまま）=====
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3]{};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // ===== シェーダコンパイル =====
    IDxcBlob* vertexShaderBlob =
        CompileShader(L"resources/shaders/Object3d.VS.hlsl", L"vs_6_0",
            dxcUtils, dxcCompiler, includHandler, logStream);
    IDxcBlob* pixelShaderBlob =
        CompileShader(L"resources/shaders/Object3d.PS.hlsl", L"ps_6_0",
            dxcUtils, dxcCompiler, includHandler, logStream);

    IDxcBlob* particleVS =
        CompileShader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0",
            dxcUtils, dxcCompiler, includHandler, logStream);
    IDxcBlob* particlePS =
        CompileShader(L"resources/shaders/Particle.PS.hlsl", L"ps_6_0",
            dxcUtils, dxcCompiler, includHandler, logStream);

    // ===== 3D モデル用 PSO 基本形 =====
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature;
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
    graphicsPipelineStateDesc.VS = {
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize()
    };
    graphicsPipelineStateDesc.PS = {
        pixelShaderBlob->GetBufferPointer(),
        pixelShaderBlob->GetBufferSize()
    };
    graphicsPipelineStateDesc.BlendState = blendDesc;
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    graphicsPipelineStateDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    ID3D12PipelineState* graphicsPinelineState = nullptr;
    hr = device->CreateGraphicsPipelineState(
        &graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPinelineState));
    assert(SUCCEEDED(hr));

    // ===== 各種ブレンド PSO（Normal / Add / Subtract / Multiply / Screen / None）=====
    ID3D12PipelineState* psoNormal = nullptr;
    ID3D12PipelineState* psoAdd = nullptr;
    ID3D12PipelineState* psoSubtract = nullptr;
    ID3D12PipelineState* psoMultiply = nullptr;
    ID3D12PipelineState* psoScreen = nullptr;
    ID3D12PipelineState* psoNone = nullptr;

    // Normal
    {
        D3D12_BLEND_DESC b = {};
        b.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        b.RenderTarget[0].BlendEnable = TRUE;
        b.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        b.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        b.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        b.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        b.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        b.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        graphicsPipelineStateDesc.BlendState = b;
        hr = device->CreateGraphicsPipelineState(
            &graphicsPipelineStateDesc, IID_PPV_ARGS(&psoNormal));
        assert(SUCCEEDED(hr));
    }
    // Add
    {
        D3D12_BLEND_DESC b = {};
        b.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        b.RenderTarget[0].BlendEnable = TRUE;
        b.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        b.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        b.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        b.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        b.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        b.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        graphicsPipelineStateDesc.BlendState = b;
        hr = device->CreateGraphicsPipelineState(
            &graphicsPipelineStateDesc, IID_PPV_ARGS(&psoAdd));
        assert(SUCCEEDED(hr));
    }
    // Subtract
    {
        D3D12_BLEND_DESC b = {};
        b.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        b.RenderTarget[0].BlendEnable = TRUE;
        b.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        b.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        b.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        b.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        b.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
        b.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
        graphicsPipelineStateDesc.BlendState = b;
        hr = device->CreateGraphicsPipelineState(
            &graphicsPipelineStateDesc, IID_PPV_ARGS(&psoSubtract));
        assert(SUCCEEDED(hr));
    }
    // Multiply
    {
        D3D12_BLEND_DESC b = {};
        b.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        b.RenderTarget[0].BlendEnable = TRUE;
        b.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        b.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
        b.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
        b.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        b.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
        b.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
        graphicsPipelineStateDesc.BlendState = b;
        hr = device->CreateGraphicsPipelineState(
            &graphicsPipelineStateDesc, IID_PPV_ARGS(&psoMultiply));
        assert(SUCCEEDED(hr));
    }
    // Screen
    {
        D3D12_BLEND_DESC b = {};
        b.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        b.RenderTarget[0].BlendEnable = TRUE;
        b.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        b.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        b.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        b.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        b.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        b.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        graphicsPipelineStateDesc.BlendState = b;
        hr = device->CreateGraphicsPipelineState(
            &graphicsPipelineStateDesc, IID_PPV_ARGS(&psoScreen));
        assert(SUCCEEDED(hr));
    }
    // None
    {
        D3D12_BLEND_DESC b = {};
        b.RenderTarget[0].BlendEnable = FALSE;
        b.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        graphicsPipelineStateDesc.BlendState = b;
        hr = device->CreateGraphicsPipelineState(
            &graphicsPipelineStateDesc, IID_PPV_ARGS(&psoNone));
        assert(SUCCEEDED(hr));
    }

    // ===== Sprite/Particle 用 PSO =====
    D3D12_GRAPHICS_PIPELINE_STATE_DESC sprPsoDesc = graphicsPipelineStateDesc;
    sprPsoDesc.pRootSignature = spriteRootSignature;
    sprPsoDesc.VS = { particleVS->GetBufferPointer(), particleVS->GetBufferSize() };
    sprPsoDesc.PS = { particlePS->GetBufferPointer(), particlePS->GetBufferSize() };
    sprPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    ID3D12PipelineState* psoSpriteParticles = nullptr;
    hr = device->CreateGraphicsPipelineState(
        &sprPsoDesc, IID_PPV_ARGS(&psoSpriteParticles));
    assert(SUCCEEDED(hr));

    // ===== モデル頂点バッファ =====
    ID3D12Resource* vertexResource =
        CreateBufferResource(device, sizeof(VertexData) * modelData.vertices.size());
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes =
        UINT(sizeof(VertexData) * modelData.vertices.size());
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    std::memcpy(vertexData, modelData.vertices.data(),
        sizeof(VertexData) * modelData.vertices.size());

    // ===== マテリアル CBV =====
    ID3D12Resource* materialResource =
        CreateBufferResource(device, sizeof(Material));
    Material* materialData = nullptr;
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData->uvTransform = MakeIdentity4x4();
    materialData->enableLighting = true;

    // ===== WVP CBV =====
    ID3D12Resource* wvpResource =
        CreateBufferResource(device, sizeof(TransformationMatrix));
    TransformationMatrix* wvpData = nullptr;
    wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
    Matrix4x4 identity = MakeIdentity4x4();

    // ===== Sprite用 頂点・インデックス =====
    ID3D12Resource* vertexResourceSprite =
        CreateBufferResource(device, sizeof(VertexData) * 4);
    VertexData* vertexDataSprite = nullptr;
    vertexResourceSprite->Map(
        0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

    vertexDataSprite[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
    vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
    vertexDataSprite[1].position = { -0.5f,  0.5f, 0.0f, 1.0f };
    vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
    vertexDataSprite[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
    vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
    vertexDataSprite[3].position = { 0.5f,  0.5f, 0.0f, 1.0f };
    vertexDataSprite[3].texcoord = { 1.0f, 0.0f };

    vertexDataSprite[0].normal = { 0.0f, 0.0f, -1.0f };
    vertexDataSprite[1].normal = { 0.0f, 0.0f, -1.0f };
    vertexDataSprite[2].normal = { 0.0f, 0.0f, -1.0f };
    vertexDataSprite[3].normal = { 0.0f, 0.0f, -1.0f };

    D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
    vertexBufferViewSprite.BufferLocation =
        vertexResourceSprite->GetGPUVirtualAddress();
    vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;
    vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

    ID3D12Resource* indexResourceSprite =
        CreateBufferResource(device, sizeof(uint32_t) * 6);
    uint32_t* indexDataSprite = nullptr;
    indexResourceSprite->Map(0, nullptr,
        reinterpret_cast<void**>(&indexDataSprite));
    indexDataSprite[0] = 0;
    indexDataSprite[1] = 1;
    indexDataSprite[2] = 2;
    indexDataSprite[3] = 2;
    indexDataSprite[4] = 1;
    indexDataSprite[5] = 3;

    D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
    indexBufferViewSprite.BufferLocation =
        indexResourceSprite->GetGPUVirtualAddress();
    indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
    indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

    // ===== Sprite マテリアル =====
    ID3D12Resource* materialResourceSprite =
        CreateBufferResource(device, sizeof(Material));
    Material* materialDataSprite = nullptr;
    materialResourceSprite->Map(0, nullptr,
        reinterpret_cast<void**>(&materialDataSprite));
    materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialDataSprite->uvTransform = MakeIdentity4x4();
    materialDataSprite->enableLighting = false;

    // ===== Sprite用 TransformMatrix CBV =====
    ID3D12Resource* transformationMatrixResourceSprite =
        CreateBufferResource(device, sizeof(TransformationMatrix));
    TransformationMatrix* transformationMatrixDataSprite = nullptr;
    transformationMatrixResourceSprite->Map(
        0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));

    // ===== パーティクル用インスタンシング =====
    const uint32_t kNumInstance = 10;
    ID3D12Resource* instancingResource =
        CreateBufferResource(device, sizeof(TransformationMatrix) * kNumInstance);
    TransformationMatrix* instancingData = nullptr;
    instancingResource->Map(
        0, nullptr, reinterpret_cast<void**>(&instancingData));
    for (uint32_t i = 0; i < kNumInstance; ++i) {
        instancingData[i].WVP = MakeIdentity4x4();
        instancingData[i].World = MakeIdentity4x4();
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
    instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    instancingSrvDesc.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instancingSrvDesc.Buffer.FirstElement = 0;
    instancingSrvDesc.Buffer.NumElements = kNumInstance;
    instancingSrvDesc.Buffer.StructureByteStride = sizeof(TransformationMatrix);
    instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvCPU =
        GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 3);
    D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvGPU =
        GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 3);
    device->CreateShaderResourceView(
        instancingResource, &instancingSrvDesc, instancingSrvCPU);

    // ===== 平行光源 CBV =====
    ID3D12Resource* directionalLightResource =
        CreateBufferResource(device, sizeof(DirectionalLight));
    DirectionalLight* directionalLightData = nullptr;
    directionalLightResource->Map(
        0, nullptr, reinterpret_cast<void**>(&directionalLightData));
    directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLightData->direction = Normalize({ 0.0f, -1.0f, 0.0f });
    directionalLightData->intensity = 1.0f;

    // ===== 各種 Transform 初期値 =====
    Transform transformSprite{
        {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
    Transform transform{
        {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
    Transform cameraTransform{
        {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -5.0f}
    };
    Transform uvTransformSprite{
        {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
    };

    bool useMonstarBall = true;

    std::random_device seedGenerator;
    std::mt19937 randomEngine(seedGenerator());

    float randPosRange = 0.15f;
    float randVelRange = 1.0f;
    float randVelZScale = 0.0f;

    float layoutStartX = -1.4f, layoutStartY = -0.8f, layoutStartZ = 0.0f;
    float layoutStepX = 0.22f, layoutStepY = 0.11f, layoutStepZ = 0.05f;
    float spriteScaleXY = 0.90f;

    // パーティクル配列
    Particle particles[kNumInstance];
    for (uint32_t i = 0; i < kNumInstance; ++i) {
        particles[i].transform.scale = { 1.0f, 1.0f, 1.0f };
        particles[i].transform.rotate = { 0.0f, 0.0f, 0.0f };
        particles[i].transform.translate = {
            -1.8f + 0.4f * i,
            -1.0f + 0.2f * i,
            0.25f * i
        };
        particles[i].velocity = { 0.0f, 0.0f, 0.0f };
    }

    MSG msg{};

    // メインループ
    while (true) {
        if (winApp->ProcessMessage()) {
            break;
        }

        // ImGui フレーム開始（ImGui の初期化は DirectXCommon 側で行っている前提）
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 入力更新
        input.Update();

        // ==== カメラ操作（元のまま）====
        const bool uiWantsMouse = ImGui::GetIO().WantCaptureMouse;

        if (!uiWantsMouse && input.MouseDown(Input::MouseLeft)) {
            const float sensitivity = 0.0025f;
            cameraTransform.rotate.y -= input.MouseDeltaX() * sensitivity;
            cameraTransform.rotate.x += input.MouseDeltaY() * sensitivity;
            const float kPitchLimit = 1.55334306f;
            cameraTransform.rotate.x =
                std::clamp(cameraTransform.rotate.x, -kPitchLimit, kPitchLimit);
        }

        Vector3 move{ 0,0,0 };
        if (input.PushKey(DIK_W)) move.z += 1.0f;
        if (input.PushKey(DIK_S)) move.z -= 1.0f;
        if (input.PushKey(DIK_D)) move.x += 1.0f;
        if (input.PushKey(DIK_A)) move.x -= 1.0f;

        if (float len = std::sqrt(move.x * move.x + move.z * move.z); len > 0.0f) {
            move.x /= len; move.z /= len;
        }

        float speed = input.PushKey(DIK_LSHIFT) ? 6.0f : 2.5f;
        const float dt = 1.0f / 60.0f;

        if (move.x != 0.0f || move.z != 0.0f) {
            const float yaw = cameraTransform.rotate.y;
            Vector3 forward = { std::sin(yaw), 0.0f, std::cos(yaw) };
            Vector3 right = { std::cos(yaw), 0.0f,-std::sin(yaw) };
            Vector3 delta = {
                right.x * move.x + forward.x * move.z,
                0.0f,
                right.z * move.x + forward.z * move.z
            };
            cameraTransform.translate.x += delta.x * speed * dt;
            cameraTransform.translate.y += delta.y * speed * dt;
            cameraTransform.translate.z += delta.z * speed * dt;
        }

        if (input.TriggerKey(DIK_0)) {
            OutputDebugStringA("Hit 0\n");
        }

        // ==== ImGui（元の UI ロジックをそのまま）====
        ImGui::ShowDemoWindow();

        ImGui::Begin("Materialcolor");
        ImGui::SliderFloat3("translate", &transform.translate.x, 0.1f, 5.0f);
        ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 5.0f);
        ImGui::SliderAngle("RotateX", &transform.rotate.x, -180.0f, 180.0f);
        ImGui::SliderAngle("RotateY", &transform.rotate.y, -180.0f, 180.0f);
        ImGui::SliderAngle("RotateZ", &transform.rotate.z, -180.0f, 180.0f);
        ImGui::SliderFloat3("Translate", &transform.translate.x, -5.0f, 5.0f);

        ImGui::SeparatorText("Particle Layout");
        ImGui::DragFloat("layoutStartX", &layoutStartX, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat("layoutStartY", &layoutStartY, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat("layoutStartZ", &layoutStartZ, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat("layoutStepX", &layoutStepX, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("layoutStepY", &layoutStepY, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("layoutStepZ", &layoutStepZ, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("SpriteScale", &spriteScaleXY, 0.01f, 0.1f, 3.0f);

        ImGui::SeparatorText("Random Init");
        ImGui::DragFloat("PosJitter (+/-)", &randPosRange, 0.005f, 0.0f, 1.0f);
        ImGui::DragFloat("VelRange (+/-)", &randVelRange, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("VelZ Scale", &randVelZScale, 0.01f, 0.0f, 2.0f);

        if (ImGui::Button("Apply Layout")) {
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                particles[i].transform.translate = {
                    layoutStartX + layoutStepX * i,
                    layoutStartY + layoutStepY * i,
                    layoutStartZ + layoutStepZ * i
                };
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].velocity = { 0.0f, 0.0f, 0.0f };
            }
        }

        static bool play = false;
        if (ImGui::Button(play ? "Stop" : "Start")) {
            play = !play;
            std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

            for (uint32_t i = 0; i < kNumInstance; ++i) {
                float jx = randPosRange * distribution(randomEngine);
                float jy = randPosRange * distribution(randomEngine);
                float jz = randPosRange * distribution(randomEngine);

                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].transform.rotate = { 0.0f, 0.0f, 0.0f };
                particles[i].transform.translate = {
                    layoutStartX + layoutStepX * i + jx,
                    layoutStartY + layoutStepY * i + jy,
                    layoutStartZ + layoutStepZ * i + jz
                };

                particles[i].velocity = {
                    randVelRange * distribution(randomEngine),
                    randVelRange * distribution(randomEngine),
                    randVelRange * randVelZScale * distribution(randomEngine)
                };
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            play = false;
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].transform.rotate = { 0.0f, 0.0f, 0.0f };
                particles[i].transform.translate = {
                    layoutStartX + layoutStepX * i,
                    layoutStartY + layoutStepY * i,
                    layoutStartZ + layoutStepZ * i
                };
                particles[i].velocity = { 0.0f, 0.0f, 0.0f };
            }
        }

        ImGui::SeparatorText("Particle Layout 2");
        static bool autoApply = false;
        ImGui::Checkbox("Auto Apply (when not playing)", &autoApply);
        if (ImGui::Button("Apply Layout 2")) {
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].transform.translate = {
                    layoutStartX + layoutStepX * i,
                    layoutStartY + layoutStepY * i,
                    layoutStartZ + layoutStepZ * i
                };
            }
        }
        if (autoApply && !play) {
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].transform.translate = {
                    layoutStartX + layoutStepX * i,
                    layoutStartY + layoutStepY * i,
                    layoutStartZ + layoutStepZ * i
                };
            }
        }

        ImGui::Text("useMonstarBall");
        ImGui::Checkbox("useMonstarBall", &useMonstarBall);
        ImGui::Text("Lighting Direction");
        ImGui::SliderFloat("x", &directionalLightData->direction.x, -10.0f, 10.0f);
        ImGui::SliderFloat("y", &directionalLightData->direction.y, -10.0f, 10.0f);
        ImGui::SliderFloat("z", &directionalLightData->direction.z, -10.0f, 10.0f);
        ImGui::Text("Color");
        ImGui::ColorEdit4("Color", &materialData->color.x);
        ImGui::Text("UVTransform");
        ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
        ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

        ImGui::Text("Material Blend Mode");
        static int currentBlendMode = 0;
        const char* blendModeNames[] = {
            "Normal","Add","Subtract","Multiply","Screen","None"
        };
        ImGui::Combo("BlendMode", &currentBlendMode, blendModeNames,
            IM_ARRAYSIZE(blendModeNames));

        ImGui::End();

        directionalLightData->direction =
            Normalize(directionalLightData->direction);

        // ==== カメラ行列など計算 ====
        Matrix4x4 worldMatrix =
            MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
        Matrix4x4 cameraMatrix =
            MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate,
                cameraTransform.translate);
        Matrix4x4 viewMatrix = Inverse(cameraMatrix);
        Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
            0.45f,
            float(WinApp::kClientWidth) / float(WinApp::kClientHeight),
            0.1f, 100.0f);
        Matrix4x4 worldViewProjectionMatrix =
            Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

        // パーティクル行列更新
        const float kDeltaTime = 1.0f / 60.0f;
        for (uint32_t i = 0; i < kNumInstance; ++i) {
            if (play) {
                particles[i].transform.translate.x += particles[i].velocity.x * kDeltaTime;
                particles[i].transform.translate.y += particles[i].velocity.y * kDeltaTime;
                particles[i].transform.translate.z += particles[i].velocity.z * kDeltaTime;
            }
            Matrix4x4 w = MakeAffineMatrix(
                particles[i].transform.scale,
                particles[i].transform.rotate,
                particles[i].transform.translate);
            Matrix4x4 wvp = Multiply(w, Multiply(viewMatrix, projectionMatrix));
            instancingData[i].WVP = wvp;
            instancingData[i].World = w;
        }

        ImGui::Render();

        // ====== ここから描画（DirectXCommon による PreDraw / PostDraw を使用）======
        dxCommon->PreDraw();

        ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap };
        commandList->SetDescriptorHeaps(1, heaps);

        // 3Dモデル描画
        commandList->SetGraphicsRootSignature(rootSignature);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // マテリアル CBV
        commandList->SetGraphicsRootConstantBufferView(
            0, materialResource->GetGPUVirtualAddress());
        // WVP CBV
        wvpData->WVP = worldViewProjectionMatrix;
        wvpData->World = worldMatrix;
        commandList->SetGraphicsRootConstantBufferView(
            1, wvpResource->GetGPUVirtualAddress());
        // ライト CBV
        commandList->SetGraphicsRootConstantBufferView(
            3, directionalLightResource->GetGPUVirtualAddress());

        switch (currentBlendMode) {
        case 0: commandList->SetPipelineState(psoNormal); break;
        case 1: commandList->SetPipelineState(psoAdd); break;
        case 2: commandList->SetPipelineState(psoSubtract); break;
        case 3: commandList->SetPipelineState(psoMultiply); break;
        case 4: commandList->SetPipelineState(psoScreen); break;
        case 5: commandList->SetPipelineState(psoNone); break;
        default: commandList->SetPipelineState(psoNormal); break;
        }

        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
        commandList->DrawInstanced(
            UINT(modelData.vertices.size()), 1, 0, 0);

        // パーティクルスプライト描画
        commandList->SetGraphicsRootSignature(spriteRootSignature);
        commandList->SetPipelineState(psoSpriteParticles);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
        commandList->IASetIndexBuffer(&indexBufferViewSprite);
        commandList->SetGraphicsRootConstantBufferView(
            0, materialResourceSprite->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(1, instancingSrvGPU);
        commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
        commandList->DrawIndexedInstanced(6, kNumInstance, 0, 0, 0);

        // ImGui 描画（Init/Shutdown は DirectXCommon 内）
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

        dxCommon->PostDraw();
    }

    // ==== 後始末 ====
    psoSpriteParticles->Release();
    spriteRootSignature->Release();
    particleVS->Release();
    particlePS->Release();
    instancingResource->Release();
    sprSigBlob->Release();
    if (sprErr) { sprErr->Release(); }

    graphicsPinelineState->Release();
    psoNormal->Release();
    psoAdd->Release();
    psoSubtract->Release();
    psoMultiply->Release();
    psoScreen->Release();
    psoNone->Release();
    signatureBlob->Release();
    if (errorBlob) { errorBlob->Release(); }
    rootSignature->Release();
    pixelShaderBlob->Release();
    vertexShaderBlob->Release();

    vertexResource->Release();
    materialResource->Release();
    wvpResource->Release();
    textureResource->Release();
    mipImages.Release();
    intermediateResource->Release();
    textureResource2->Release();
    intermediateResource2->Release();
    materialResourceSprite->Release();
    vertexResourceSprite->Release();
    indexResourceSprite->Release();
    transformationMatrixResourceSprite->Release();
    directionalLightResource->Release();

    // ★ ImGui の Shutdown / DestroyContext は DirectXCommon::Finalize() 内で行う前提

    dxCommon->Finalize();
    winApp->Finalize();
    delete dxCommon;
    delete winApp;
    CoUninitialize();

    // リソースリークチェック
    IDXGIDebug1* debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

    return 0;
}
