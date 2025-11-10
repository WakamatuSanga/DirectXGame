#include "DirectXCommon.h"

#include <cassert>
#include <vector>

// ライブラリリンク
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

using namespace Microsoft::WRL;


// 深度ステンシル用テクスチャを作るヘルパー関数
static ID3D12Resource* CreateDepthStencilTextureResource(
    ID3D12Device* device,
    int32_t width,
    int32_t height)
{
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.MipLevels = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ID3D12Resource* resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
        IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));
    return resource;
}






// --------------------
// 初期化
// --------------------
void DirectXCommon::Initialize(WinApp* winApp)
{
    // NULL検出
    assert(winApp);
    // メンバ変数に記録
    this->winApp = winApp;

    // ここから下は資料の「DirectXの初期化 全体像」に対応
    CreateDevice();               // デバイスの生成
    InitializeCommand();          // コマンド関連の初期化
    CreateSwapChain();            // スワップチェーンの生成
    CreateDepthBuffer();          // 深度バッファの生成
    CreateDescriptorHeaps();      // 各種デスクリプタヒープの生成
    InitializeRenderTargetView(); // レンダーターゲットビューの初期化
    InitializeDepthStencilView(); // 深度ステンシルビューの初期化
    CreateFence();                // フェンスの生成
    InitializeViewport();         // ビューポート矩形の初期化
    InitializeScissorRect();      // シザリング矩形の初期化
    CreateDXCCompiler();          // DXCコンパイラの生成
    InitializeImGui();            // ImGuiの初期化
}

// --------------------
// 終了処理
// --------------------
void DirectXCommon::Finalize()
{
    // 必要ならGPU待ち合わせなどをここで行う
    // （main.cpp の終了処理に書いていたDirectX片をここに移す）
}

// --------------------
// 描画前処理
// --------------------
void DirectXCommon::PreDraw()
{
    // バックバッファの番号取得
    UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    // リソースバリアで書き込み可能に変更
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // 描画先のRTVとDSVを指定
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHandles[backBufferIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
        dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 画面全体の色をクリア
    const float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // 画面全体の深度をクリア
    commandList->ClearDepthStencilView(
        dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // SRV用のディスクリプタヒープを指定
    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);

    // ビューポート領域・シザー矩形の設定
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // ※ パイプライン状態やルートシグネチャの設定はここではしない
}


// --------------------
// 描画後処理
// --------------------
void DirectXCommon::PostDraw()
{
    // バックバッファの番号取得
    UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    // リソースバリアで表示状態に変更
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // グラフィックスコマンドをクローズ
    HRESULT hr = commandList->Close();
    assert(SUCCEEDED(hr));

    // GPUコマンドの実行
    ID3D12CommandList* commandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(1, commandLists);

    // 画面の交換（Present）
    hr = swapChain->Present(1, 0);
    assert(SUCCEEDED(hr));

    // Fenceの値を更新 & シグナル送信
    ++fenceValue;
    hr = commandQueue->Signal(fence.Get(), fenceValue);
    assert(SUCCEEDED(hr));

    // コマンド完了待ち
    if (fence->GetCompletedValue() < fenceValue) {
        hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
        assert(SUCCEEDED(hr));
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    // 次のフレーム用にコマンドアロケータとコマンドリストをリセット
    hr = commandAllocator->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList->Reset(commandAllocator.Get(), nullptr);
    assert(SUCCEEDED(hr));
}


// --------------------
// デバイスの生成
// --------------------
void DirectXCommon::CreateDevice()
{
    HRESULT hr = S_OK;

    // DXGIファクトリーの生成
    //  ※ メンバ変数 dxgiFactory に直接作る
    hr = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
    assert(SUCCEEDED(hr));

    // 使用するアダプタを列挙して選択
    //  ※ アダプタは一時的に使うだけなのでローカル ComPtr でOK
    ComPtr<IDXGIAdapter4> useAdapter;

    for (UINT i = 0;
        dxgiFactory->EnumAdapterByGpuPreference(
            i,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(useAdapter.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND;
        ++i)
    {
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));

        // ソフトウェアアダプタでなければ採用する
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            break;
        }

        // ソフトウェアアダプタだったので破棄して次を試す
        useAdapter.Reset();
    }

    // 適切なアダプタが見つからなかったらゲーム続行不能
    assert(useAdapter);

    // デバイスの生成
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };

    // 以前の値が残っていないように念のためリセット
    device.Reset();

    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        hr = D3D12CreateDevice(
            useAdapter.Get(),                // 採用したアダプタ
            featureLevels[i],                // 試す FeatureLevel
            IID_PPV_ARGS(device.GetAddressOf())); // メンバ変数 device に作る

        if (SUCCEEDED(hr)) {
            // どれか一つ成功したら抜ける（ログは省略）
            break;
        }
    }

    // デバイスが作れなかったら終了
    assert(device);

#ifdef _DEBUG
    // InfoQueue 設定（やばいエラーで止める）
    ID3D12InfoQueue* infoQueue = nullptr;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        D3D12_MESSAGE_ID denyIds[] = {
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
        };
        D3D12_MESSAGE_SEVERITY severities[] = {
            D3D12_MESSAGE_SEVERITY_INFO,
        };
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;

        infoQueue->PushStorageFilter(&filter);
        infoQueue->Release();
    }
#endif
}


// --------------------
// コマンド関連の初期化
// --------------------
void DirectXCommon::InitializeCommand()
{
    // コマンドキューを生成
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    HRESULT hr = device->CreateCommandQueue(
        &commandQueueDesc,
        IID_PPV_ARGS(commandQueue.GetAddressOf())); // メンバ commandQueue
    assert(SUCCEEDED(hr));

    // コマンドアロケーターを生成
    hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(commandAllocator.GetAddressOf())); // メンバ commandAllocator
    assert(SUCCEEDED(hr));

    // コマンドリストを生成
    hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(),      // メンバ commandAllocator を指定
        nullptr,
        IID_PPV_ARGS(commandList.GetAddressOf())); // メンバ commandList
    assert(SUCCEEDED(hr));

    // 最初の Reset ができるよう、いったん Close しておく
    hr = commandList->Close();
    assert(SUCCEEDED(hr));
}


// --------------------
// スワップチェーンの生成
// --------------------
void DirectXCommon::CreateSwapChain()
{
    // スワップチェーンの設定（画面サイズは WinApp から）
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = WinApp::kClientWidth;   // クライアント幅
    swapChainDesc.Height = WinApp::kClientHeight;  // クライアント高さ
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;         // 色フォーマット
    swapChainDesc.SampleDesc.Count = 1;                      // マルチサンプルなし
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画ターゲット
    swapChainDesc.BufferCount = 2;                      // ダブルバッファ
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;   // フリップ

    // スワップチェーンを生成（ウィンドウハンドルは WinApp から取得）
    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue.Get(),                    // コマンドキュー
        winApp->GetHwnd(),                     // 対象ウィンドウ
        &swapChainDesc,                        // 設定
        nullptr,
        nullptr,
        reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf())); // メンバ swapChain
    assert(SUCCEEDED(hr));

    // ==== ディスクリプタヒープ各種 ====

    // RTV 用ヒープ（バックバッファ 2 枚ぶん）
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = 2;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = device->CreateDescriptorHeap(
            &desc,
            IID_PPV_ARGS(rtvDescriptorHeap.GetAddressOf()));
        assert(SUCCEEDED(hr));
    }

    // DSV 用ヒープ（深度バッファ 1 枚ぶん）
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = device->CreateDescriptorHeap(
            &desc,
            IID_PPV_ARGS(dsvDescriptorHeap.GetAddressOf()));
        assert(SUCCEEDED(hr));
    }

    // SRV 用ヒープ（テクスチャや ImGui 用）
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 128; // 余裕を多めに確保
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = device->CreateDescriptorHeap(
            &desc,
            IID_PPV_ARGS(srvDescriptorHeap.GetAddressOf()));
        assert(SUCCEEDED(hr));
    }

    // ==== バックバッファ取得 ====

    // SwapChain からバックバッファ（2枚）を取得してメンバに保持
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(swapChainResources[0].GetAddressOf()));
    assert(SUCCEEDED(hr));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(swapChainResources[1].GetAddressOf()));
    assert(SUCCEEDED(hr));

    // ==== RTV を 2 枚作成 ====

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // 先頭ハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle =
        rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // RTV のインクリメントサイズ
    UINT rtvIncrementSize =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 1枚目
    rtvHandles[0] = rtvStartHandle;
    device->CreateRenderTargetView(
        swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);

    // 2枚目（1枚目からインクリメントぶんずらす）
    rtvHandles[1].ptr = rtvHandles[0].ptr + rtvIncrementSize;
    device->CreateRenderTargetView(
        swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);
}


// --------------------
// 深度バッファの生成
// --------------------
void DirectXCommon::CreateDepthBuffer()
{
    // 深度バッファ用テクスチャリソースを生成
    // （画面サイズは WinApp のクライアントサイズを使用）
    depthBuffer = CreateDepthStencilTextureResource(
        device.Get(),
        WinApp::kClientWidth,
        WinApp::kClientHeight
    );

    // DSV（Depth Stencil View）の設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    // DSVヒープの先頭に DSV を1つ作成
    device->CreateDepthStencilView(
        depthBuffer.Get(),
        &dsvDesc,
        dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
    );
}

// --------------------
// 各種デスクリプタヒープの生成
// --------------------
void DirectXCommon::CreateDescriptorHeaps()
{
    HRESULT hr = S_OK;

    // --- RTV ヒープ（2個、シェーダからは見えない）---
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = 2;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;

        hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvDescriptorHeap));
        assert(SUCCEEDED(hr));
    }

    // --- DSV ヒープ（1個、シェーダからは見えない）---
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;

        hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dsvDescriptorHeap));
        assert(SUCCEEDED(hr));
    }

    // --- SRV/CBV/UAV ヒープ（128個、シェーダから見える）---
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 128;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask = 0;

        hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvDescriptorHeap));
        assert(SUCCEEDED(hr));
    }

    // --- デスクリプタサイズをメンバに保存 ---
    descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

// --------------------
// レンダーターゲットビューの初期化
// --------------------
void DirectXCommon::InitializeRenderTargetView()
{
    HRESULT hr = S_OK;

    // スワップチェーンからバックバッファのリソースを取得
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(swapChainResources[0].GetAddressOf()));
    assert(SUCCEEDED(hr));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(swapChainResources[1].GetAddressOf()));
    assert(SUCCEEDED(hr));

    // RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // ディスクリプタヒープ先頭ハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle =
        rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // 1つ目のRTV
    rtvHandles[0] = rtvStartHandle;
    device->CreateRenderTargetView(
        swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);

    // 2つ目のRTV（1つ目から descriptorSizeRTV だけずらす）
    rtvHandles[1].ptr = rtvHandles[0].ptr + descriptorSizeRTV;
    device->CreateRenderTargetView(
        swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);
}


// --------------------
// 深度ステンシルビューの初期化
// --------------------
void DirectXCommon::InitializeDepthStencilView()
{
    // DSV の設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    // DSV ヒープの先頭に DSV を作成
    device->CreateDepthStencilView(
        depthBuffer.Get(),
        &dsvDesc,
        dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}


// --------------------
// フェンスの生成
// --------------------
void DirectXCommon::CreateFence()
{
    // フェンス値を初期化
    fenceValue = 0;

    // フェンスを生成
    HRESULT hr = device->CreateFence(
        fenceValue,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence));
    assert(SUCCEEDED(hr));

    // フェンス待ち用イベントを生成
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fenceEvent != nullptr);
}


// --------------------
// ビューポート矩形の初期化
// --------------------
void DirectXCommon::InitializeViewport()
{
    // クライアント領域いっぱいに描画するビューポートを設定
    viewport.Width = static_cast<float>(WinApp::kClientWidth);
    viewport.Height = static_cast<float>(WinApp::kClientHeight);
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
}


// --------------------
// シザリング矩形の初期化
// --------------------
void DirectXCommon::InitializeScissorRect()
{
    // クライアント領域全体をシザー矩形に設定
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = WinApp::kClientWidth;
    scissorRect.bottom = WinApp::kClientHeight;
}


// --------------------
// DXCコンパイラの生成
// --------------------
void DirectXCommon::CreateDXCCompiler()
{
    HRESULT hr = S_OK;

    // DXCユーティリティの生成
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    assert(SUCCEEDED(hr));

    // DXCコンパイラ本体の生成
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    assert(SUCCEEDED(hr));

    // インクルードハンドラの生成
    hr = dxcUtils->CreateDefaultIncludeHandler(&dxcIncludeHandler);
    assert(SUCCEEDED(hr));
}


// --------------------
// ImGuiの初期化
// --------------------
void DirectXCommon::InitializeImGui()
{
    // ImGui コンテキスト生成
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // スタイル設定
    ImGui::StyleColorsClassic();

    // Win32 側の初期化（HWND は WinApp から取得）
    ImGui_ImplWin32_Init(winApp->GetHwnd());

    // DirectX12 側の初期化
    //   第2引数 : スワップチェーンのバッファ数（ダブルバッファなら 2）
    //   第3引数 : RTV のフォーマット（RTV 作成で使ったものと合わせる）
    ImGui_ImplDX12_Init(
        device.Get(),                                     // D3D12 デバイス（メンバ）
        2,                                          // バッファ数（ダブルバッファ）
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,           // RTV のフォーマット
        srvDescriptorHeap.Get(),                         // SRV ヒープ（メンバ）
        srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

// --------------------
// 内部用：CPUディスクリプタハンドル取得
// --------------------
D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(
    const ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
    uint32_t descriptorSize,
    uint32_t index)
{
    // 指定番号のCPUディスクリプタハンドルを取得する
    D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(descriptorSize) * index;
    return handle;
}

// --------------------
// 内部用：GPUディスクリプタハンドル取得
// --------------------
D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(
    const ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
    uint32_t descriptorSize,
    uint32_t index)
{
    // 指定番号のGPUディスクリプタハンドルを取得する
    D3D12_GPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(descriptorSize) * index;
    return handle;
}

// --------------------
// SRV用CPUディスクリプタハンドル取得
// --------------------
D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index)
{
    // SRV用のメンバ変数を渡して内部のstatic関数を呼び出す
    return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

// --------------------
// SRV用GPUディスクリプタハンドル取得
// --------------------
D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
{
    // SRV用のメンバ変数を渡して内部のstatic関数を呼び出す
    return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}
