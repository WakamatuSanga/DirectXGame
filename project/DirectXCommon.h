#pragma once

// DirectX12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>

#include <array>
#include <cstdint>

#include "WinApp.h"

// DirectX 基盤クラス
class DirectXCommon {
public:
    DirectXCommon() = default;
    ~DirectXCommon() = default;

    // DirectXの初期化
    void Initialize(WinApp* winApp);

    // DirectXの終了処理
    void Finalize();

    // 描画前処理（毎フレームの先頭で呼ぶ）
    void PreDraw();

    // 描画後処理（毎フレームの最後で呼ぶ）
    void PostDraw();

    // デバイス取得（外部の描画処理で使う用）
    ID3D12Device* GetDevice() const { return device.Get(); }

    // コマンドリスト取得（外部の描画処理で使う用）
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }

    // SRVの指定番号のCPUディスクリプタハンドルを取得する
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

    // SRVの指定番号のGPUディスクリプタハンドルを取得する
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

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

    // シザリング矩形の初期化
    void InitializeScissorRect();

    // DXCコンパイラの生成
    void CreateDXCCompiler();

    // ImGuiの初期化
    void InitializeImGui();

    // 指定番号のCPUディスクリプタハンドルを取得する（内部用）
    static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
        uint32_t descriptorSize,
        uint32_t index);

    // 指定番号のGPUディスクリプタハンドルを取得する（内部用）
    static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
        uint32_t descriptorSize,
        uint32_t index);

private:
    // WindowsAPIクラスへのポインタ（スワップチェーン生成などで使う）
    WinApp* winApp = nullptr;

    // DirectX12デバイス
    Microsoft::WRL::ComPtr<ID3D12Device> device;

    // DXGIファクトリ
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;

    // コマンド関連
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>        commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

    // スワップチェーン
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
    // バックバッファ（裏表2つ分）
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

    // 深度バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;

    // 各種デスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

    // デスクリプタサイズ
    uint32_t descriptorSizeRTV = 0;
    uint32_t descriptorSizeSRV = 0;
    uint32_t descriptorSizeDSV = 0;

    // フェンス関連
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    uint64_t fenceValue = 0;
    HANDLE   fenceEvent = nullptr;

    // ビューポートとシザリング矩形
    D3D12_VIEWPORT viewport{};
    D3D12_RECT     scissorRect{};

    // DXCコンパイラ関連
    Microsoft::WRL::ComPtr<IDxcUtils>          dxcUtils;
    Microsoft::WRL::ComPtr<IDxcCompiler3>      dxcCompiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> dxcIncludeHandler;

    // レンダーターゲットビュー用ハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2]{};
};
