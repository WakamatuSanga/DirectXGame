#include "DirectXCommon.h"

#include <cassert>
#include <vector>
#include <string>

#include "Logger.h"
#include "StringUtility.h"

// DirectXTex / d3dx12
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

// ライブラリリンク
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

using namespace Microsoft::WRL;

// ======================================
// 深度ステンシル用テクスチャ生成ヘルパ
// ======================================
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
    assert(winApp);
    this->winApp = winApp;

    CreateDevice();               // デバイス生成
    InitializeCommand();          // コマンド関連
    CreateSwapChain();            // スワップチェーン
    CreateDepthBuffer();          // 深度バッファ
    CreateDescriptorHeaps();      // デスクリプタヒープ
    InitializeRenderTargetView(); // RTV
    InitializeDepthStencilView(); // DSV
    CreateFence();                // フェンス
    InitializeViewport();         // ビューポート
    InitializeScissorRect();      // シザー矩形
    CreateDXCCompiler();          // DXC コンパイラ
    InitializeImGui();            // ImGui

    // ★ ここでの dxcUtils_ / dxcCompiler_ / dxcIncludeHandler_ の
    //    二重初期化は削除。
    //    DXC は CreateDXCCompiler() で一元管理します。
}

// --------------------
// 終了処理
// --------------------
void DirectXCommon::Finalize()
{
    // GPU がコマンドを全部処理するまで待つ
    if (commandQueue && fence) {
        ++fenceValue;
        HRESULT hr = commandQueue->Signal(fence.Get(), fenceValue);
        if (SUCCEEDED(hr) && fence->GetCompletedValue() < fenceValue) {
            hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
            if (SUCCEEDED(hr)) {
                WaitForSingleObject(fenceEvent, INFINITE);
            }
        }
    }

    if (fenceEvent) {
        CloseHandle(fenceEvent);
        fenceEvent = nullptr;
    }

    // ComPtr が自動で Release してくれるので何もしなくてOK
}

// --------------------
// 描画前処理
// --------------------
void DirectXCommon::PreDraw()
{
    UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    // PRESENT → RENDER_TARGET
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // RTV / DSV 設定
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHandles[backBufferIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
        dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 画面クリア
    const float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // 深度クリア
    commandList->ClearDepthStencilView(
        dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // SRV ヒープをセット
    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);

    // ビューポート / シザー設定
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}

// --------------------
// 描画後処理
// --------------------
void DirectXCommon::PostDraw()
{
    UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    // RENDER_TARGET → PRESENT
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // コマンドリストを閉じて実行
    HRESULT hr = commandList->Close();
    assert(SUCCEEDED(hr));
    ID3D12CommandList* commandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(1, commandLists);

    // 画面の入れ替え
    hr = swapChain->Present(1, 0);
    assert(SUCCEEDED(hr));

    // Fence 更新 & 待ち
    ++fenceValue;
    hr = commandQueue->Signal(fence.Get(), fenceValue);
    assert(SUCCEEDED(hr));

    if (fence->GetCompletedValue() < fenceValue) {
        hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
        assert(SUCCEEDED(hr));
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    // 次フレーム用リセット
    hr = commandAllocator->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList->Reset(commandAllocator.Get(), nullptr);
    assert(SUCCEEDED(hr));
}

// --------------------
// デバイス生成
// --------------------
void DirectXCommon::CreateDevice()
{
    HRESULT hr = S_OK;

    hr = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
    assert(SUCCEEDED(hr));

    ComPtr<IDXGIAdapter4> useAdapter;
    for (UINT i = 0;
        dxgiFactory->EnumAdapterByGpuPreference(
            i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(useAdapter.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND;
        ++i)
    {
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));

        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            break;
        }
        useAdapter.Reset();
    }
    assert(useAdapter);

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };

    device.Reset();
    for (auto fl : featureLevels) {
        hr = D3D12CreateDevice(useAdapter.Get(), fl,
            IID_PPV_ARGS(device.GetAddressOf()));
        if (SUCCEEDED(hr)) break;
    }
    assert(device);

#ifdef _DEBUG
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
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    HRESULT hr = device->CreateCommandQueue(
        &commandQueueDesc,
        IID_PPV_ARGS(commandQueue.GetAddressOf()));
    assert(SUCCEEDED(hr));

    hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(commandAllocator.GetAddressOf()));
    assert(SUCCEEDED(hr));

    hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(commandList.GetAddressOf()));
    assert(SUCCEEDED(hr));

    // ★ ここは Close しない！
    //   → 初期化中（テクスチャ転送など）でもそのまま commandList を使えるようにする。
    // hr = commandList->Close();
    // assert(SUCCEEDED(hr));
}

// --------------------
// スワップチェーン生成
// --------------------
void DirectXCommon::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = WinApp::kClientWidth;
    swapChainDesc.Height = WinApp::kClientHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue.Get(),
        winApp->GetHwnd(),
        &swapChainDesc,
        nullptr, nullptr,
        reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
    assert(SUCCEEDED(hr));

    // バックバッファ 2 枚取得だけここでやる
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(swapChainResources[0].GetAddressOf()));
    assert(SUCCEEDED(hr));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(swapChainResources[1].GetAddressOf()));
    assert(SUCCEEDED(hr));
}

// --------------------
// 深度バッファ生成
// --------------------
void DirectXCommon::CreateDepthBuffer()
{
    depthBuffer = CreateDepthStencilTextureResource(
        device.Get(),
        WinApp::kClientWidth,
        WinApp::kClientHeight);
}

// --------------------
// 各種デスクリプタヒープ生成
// --------------------
void DirectXCommon::CreateDescriptorHeaps()
{
    HRESULT hr = S_OK;

    // RTV
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = 2;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = device->CreateDescriptorHeap(&desc,
            IID_PPV_ARGS(&rtvDescriptorHeap));
        assert(SUCCEEDED(hr));
    }

    // DSV
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = device->CreateDescriptorHeap(&desc,
            IID_PPV_ARGS(&dsvDescriptorHeap));
        assert(SUCCEEDED(hr));
    }

    // SRV/CBV/UAV
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 128;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = device->CreateDescriptorHeap(&desc,
            IID_PPV_ARGS(&srvDescriptorHeap));
        assert(SUCCEEDED(hr));
    }

    descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

// --------------------
// RTV 初期化
// --------------------
void DirectXCommon::InitializeRenderTargetView()
{
    HRESULT hr = S_OK;

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle =
        rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // 1 枚目
    rtvHandles[0] = rtvStartHandle;
    device->CreateRenderTargetView(
        swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);

    // 2 枚目
    rtvHandles[1].ptr = rtvHandles[0].ptr + descriptorSizeRTV;
    device->CreateRenderTargetView(
        swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);
}

// --------------------
// DSV 初期化
// --------------------
void DirectXCommon::InitializeDepthStencilView()
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    device->CreateDepthStencilView(
        depthBuffer.Get(),
        &dsvDesc,
        dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

// --------------------
// フェンス生成
// --------------------
void DirectXCommon::CreateFence()
{
    fenceValue = 0;
    HRESULT hr = device->CreateFence(
        fenceValue,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence));
    assert(SUCCEEDED(hr));

    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fenceEvent != nullptr);
}

// --------------------
// ビューポート
// --------------------
void DirectXCommon::InitializeViewport()
{
    viewport.Width = static_cast<float>(WinApp::kClientWidth);
    viewport.Height = static_cast<float>(WinApp::kClientHeight);
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
}

// --------------------
// シザー矩形
// --------------------
void DirectXCommon::InitializeScissorRect()
{
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = WinApp::kClientWidth;
    scissorRect.bottom = WinApp::kClientHeight;
}

// --------------------
// DXC コンパイラ生成
// --------------------
void DirectXCommon::CreateDXCCompiler()
{
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    assert(SUCCEEDED(hr));

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    assert(SUCCEEDED(hr));

    hr = dxcUtils->CreateDefaultIncludeHandler(&dxcIncludeHandler);
    assert(SUCCEEDED(hr));
}

// --------------------
// ImGui 初期化
// --------------------
void DirectXCommon::InitializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsClassic();

    ImGui_ImplWin32_Init(winApp->GetHwnd());

    ImGui_ImplDX12_Init(
        device.Get(),
        2,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        srvDescriptorHeap.Get(),
        srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

// ============================================================
// ここから スライドで「自分で考えよう」となっている各種関数
// ============================================================

// シェーダーコンパイル
ComPtr<IDxcBlob> DirectXCommon::CompileShader(
    const std::wstring& filePath,
    const wchar_t* profile)
{
    assert(dxcUtils);
    assert(dxcCompiler);
    assert(dxcIncludeHandler);

    // ファイル読み込み
    ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &sourceBlob);
    assert(SUCCEEDED(hr));

    DxcBuffer sourceBuffer{};
    sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
    sourceBuffer.Size = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_UTF8;

    // コンパイル引数
    std::vector<LPCWSTR> arguments;
    arguments.push_back(filePath.c_str());
    arguments.push_back(L"-E");
    arguments.push_back(L"main");
    arguments.push_back(L"-T");
    arguments.push_back(profile);
#ifdef _DEBUG
    arguments.push_back(L"-Zi");
    arguments.push_back(L"-Qembed_debug");
    arguments.push_back(L"-Od");
#else
    arguments.push_back(L"-O3");
#endif
    arguments.push_back(L"-Zpr"); // 行優先行列

    ComPtr<IDxcResult> result;
    hr = dxcCompiler->Compile(
        &sourceBuffer,
        arguments.data(),
        static_cast<UINT>(arguments.size()),
        dxcIncludeHandler.Get(),
        IID_PPV_ARGS(&result));
    assert(SUCCEEDED(hr));

    // エラーチェック
    ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() != 0) {
        Logger::Log(errors->GetStringPointer());
        assert(false); // 開発中は止める
    }

    // コンパイル結果を取得
    ComPtr<IDxcBlob> shaderBlob;
    hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));

    return shaderBlob;
}

// バッファリソース生成（アップロードヒープ）
ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes)
{
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> resource;
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(resource.GetAddressOf()));
    assert(SUCCEEDED(hr));

    return resource;
}

// テクスチャリソース生成（Default ヒープ）
ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(
    const DirectX::TexMetadata& metadata)
{
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
    resourceDesc.Width = static_cast<UINT>(metadata.width);
    resourceDesc.Height = static_cast<UINT>(metadata.height);
    resourceDesc.DepthOrArraySize = static_cast<UINT16>(
        (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D) ? metadata.depth : metadata.arraySize);
    resourceDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
    resourceDesc.Format = metadata.format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> texture;
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, // 転送待ち
        nullptr,
        IID_PPV_ARGS(texture.GetAddressOf()));
    assert(SUCCEEDED(hr));

    return texture;
}

// テクスチャデータ転送
void DirectXCommon::UploadTextureData(
    const ComPtr<ID3D12Resource>& texture,
    const DirectX::ScratchImage& mipImages)
{
    assert(texture);

    // ScratchImage → D3D12_SUBRESOURCE_DATA 列へ変換
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    subresources.reserve(mipImages.GetImageCount());

    const DirectX::Image* images = mipImages.GetImages();
    for (size_t i = 0; i < mipImages.GetImageCount(); ++i) {
        D3D12_SUBRESOURCE_DATA src{};
        src.pData = images[i].pixels;
        src.RowPitch = images[i].rowPitch;
        src.SlicePitch = images[i].slicePitch;
        subresources.push_back(src);
    }

    // アップロード用バッファ
    UINT64 uploadBufferSize =
        GetRequiredIntermediateSize(texture.Get(), 0, static_cast<UINT>(subresources.size()));
    ComPtr<ID3D12Resource> intermediate = CreateBufferResource(uploadBufferSize);

    // コピー
    UpdateSubresources(
        commandList.Get(),
        texture.Get(),
        intermediate.Get(),
        0, 0,
        static_cast<UINT>(subresources.size()),
        subresources.data());

    // シェーダから読める状態へ
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);
}

// テクスチャファイル読み込み（static）
DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string& filePath)
{
    using namespace DirectX;

    std::wstring filePathW = StringUtility::ConvertString(filePath);

    ScratchImage image;
    HRESULT hr = LoadFromWICFile(
        filePathW.c_str(),
        WIC_FLAGS_FORCE_SRGB,
        nullptr,
        image);
    assert(SUCCEEDED(hr));
    return image;
}

// --------------------
// SRV CPU / GPU ハンドル
// --------------------
D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(
    const ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
    uint32_t descriptorSize,
    uint32_t index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(descriptorSize) * index;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(
    const ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
    uint32_t descriptorSize,
    uint32_t index)
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle =
        descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(descriptorSize) * index;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index)
{
    return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
{
    return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}
