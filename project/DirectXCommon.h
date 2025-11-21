#pragma once

// DirectX12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>

#include <array>
#include <cstdint>
#include <string>
#include <chrono>
#include <thread>
#include <cstdint>

#include "WinApp.h"
#include "externals/DirectXTex/DirectXTex.h" // TexMetadata / ScratchImage 用

// DirectX 基盤クラス
class DirectXCommon {
public:
    DirectXCommon() = default;
    ~DirectXCommon() = default;

    // DirectX の初期化 / 終了
    void Initialize(WinApp* winApp);
    void Finalize();

    // 毎フレームの前処理 / 後処理
    void PreDraw();
    void PostDraw();

    // 最大SRV数（最大テクスチャ枚数）
    static const uint32_t kMaxSRVCount;

    // --- getter（スライド通り＋少し追加） ---
    ID3D12Device* GetDevice() const { return device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
    IDXGISwapChain4* GetSwapChain() const { return swapChain.Get(); }

    ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvDescriptorHeap.Get(); }
    ID3D12DescriptorHeap* GetRtvDescriptorHeap() const { return rtvDescriptorHeap.Get(); }
    ID3D12DescriptorHeap* GetDsvDescriptorHeap() const { return dsvDescriptorHeap.Get(); }

    UINT GetSrvDescriptorSize() const { return descriptorSizeSRV; }

    // バックバッファ RTV を直接欲しい場合
    D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(uint32_t index) const {
        return rtvHandles[index];
    }

    // SRV 用ディスクリプタハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

    // ============================
    // ここからスライドの追加関数
    // ============================

    // シェーダーのコンパイル
    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
        const std::wstring& filePath,   // HLSL ファイルパス
        const wchar_t* profile);        // "vs_6_0" など

    /// <summary>バッファリソースの生成（アップロードヒープ）</summary>
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

    /// <summary>テクスチャリソースの生成（Default ヒープ）</summary>
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(
        const DirectX::TexMetadata& metadata);

    /// <summary>テクスチャデータの転送（ScratchImage → GPU）</summary>
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
        const Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
        const DirectX::ScratchImage& mipImages);

    /// <summary>テクスチャファイルの読み込み（静的関数）</summary>
    static DirectX::ScratchImage LoadTexture(const std::string& filePath);

    // ★ DXC 関連 Getter（外からも使えるように）
    IDxcUtils* GetDxcUtils() const { return dxcUtils.Get(); }
    IDxcCompiler3* GetDxcCompiler() const { return dxcCompiler.Get(); }
    IDxcIncludeHandler* GetDxcIncludeHandler() const { return dxcIncludeHandler.Get(); }

private:
    // デバイスの生成
    void CreateDevice();
    // コマンド関連の初期化
    void InitializeCommand();
    // スワップチェーンの生成
    void CreateSwapChain();
    // 深度バッファの生成
    void CreateDepthBuffer();
    // 各種デスクリプタヒープの生成
    void CreateDescriptorHeaps();
    // レンダーターゲットビューの初期化
    void InitializeRenderTargetView();
    // 深度ステンシルビューの初期化
    void InitializeDepthStencilView();
    // フェンスの生成
    void CreateFence();
    // ビューポート矩形の初期化
    void InitializeViewport();
    // シザー矩形の初期化
    void InitializeScissorRect();
    // DXC コンパイラの生成
    void CreateDXCCompiler();
    // ImGui の初期化
    void InitializeImGui();

    // 内部用：指定番号の CPU ハンドル取得
    static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
        uint32_t descriptorSize,
        uint32_t index);

    // 内部用：指定番号の GPU ハンドル取得
    static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
        uint32_t descriptorSize,
        uint32_t index);

    // FPS固定初期化
    void InitializeFixFPS();
    // FPS固定更新
    void UpdateFixFPS();

    // 記録時間（FPS固定用）
    std::chrono::steady_clock::time_point reference_;

private:
    // WindowsAPI
    WinApp* winApp = nullptr;

    // デバイス / ファクトリ
    Microsoft::WRL::ComPtr<ID3D12Device>   device;
    Microsoft::WRL::ComPtr<IDXGIFactory7>  dxgiFactory;

    // コマンドまわり
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>        commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

    // スワップチェーンとバックバッファ
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

    // 深度バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;

    // デスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

    // デスクリプタサイズ
    uint32_t descriptorSizeRTV = 0;
    uint32_t descriptorSizeSRV = 0;
    uint32_t descriptorSizeDSV = 0;

    // フェンス
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    uint64_t fenceValue = 0;
    HANDLE   fenceEvent = nullptr;

    // ビューポート / シザー
    D3D12_VIEWPORT viewport{};
    D3D12_RECT     scissorRect{};

    // DXC コンパイラ関連
    Microsoft::WRL::ComPtr<IDxcUtils>          dxcUtils;
    Microsoft::WRL::ComPtr<IDxcCompiler3>      dxcCompiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> dxcIncludeHandler;

    // バックバッファ 2 枚分の RTV
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2]{};
};
