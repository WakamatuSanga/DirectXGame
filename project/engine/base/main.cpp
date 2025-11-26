#define _USE_MATH_DEFINES

// --- Windows / 標準ライブラリ ---
#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <strsafe.h>
#include <vector>
#include <random>
#include <sstream>
#include <iostream>
#include <format>
#include <algorithm>
#include <cmath>

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
#include "externals/DirectXTex/d3dx12.h"

// --- ImGui ---
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

// --- 自作ヘッダ ---
#include "Input.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "Matrix4x4.h"
#include "Logger.h"
#include "StringUtility.h"
#include "TextureManager.h"

// ------------- 型定義 / 構造体など -------------

using namespace MatrixMath;

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
    float   alpha;
    bool    active;
};

struct Material {
    Vector4   color;
    int32_t   enableLighting;
    float     padding[3];
    Matrix4x4 uvTransform;
};

struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
};

struct DirectionalLight {
    Vector4 color;
    Vector3 direction;
    float   intensity;
};

// CG2_06_02
struct MaterialData {
    std::string textureFilePath;
};

struct ModelData {
    std::vector<VertexData> vertices;
    MaterialData            material;
};

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

WaveType      waveType = WAVE_SINE;
AnimationType animationType = ANIM_NONE;
float         waveTime = 0.0f;

// パーティクルインスタンス数
const uint32_t kNumInstance = 10;

// ---------- ユーティリティ関数 ----------

// 正規化
Vector3 Normalize(const Vector3& v) {
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length == 0.0f) {
        return { 0.0f,0.0f,0.0f };
    }
    return { v.x / length, v.y / length, v.z / length };
}

// クラッシュ時に dmp を出す
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t filePath[MAX_PATH] = { 0 };
    CreateDirectory(L"./Dumps", nullptr);
    StringCchPrintfW(
        filePath, MAX_PATH,
        L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
        time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute);

    HANDLE dumpFileHandle = CreateFile(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        0, CREATE_ALWAYS, 0, 0);

    DWORD processId = GetCurrentProcessId();
    DWORD threadId = GetCurrentThreadId();

    MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{};
    minidumpInformation.ThreadId = threadId;
    minidumpInformation.ExceptionPointers = exception;
    minidumpInformation.ClientPointers = TRUE;

    MiniDumpWriteDump(
        GetCurrentProcess(),
        processId,
        dumpFileHandle,
        MiniDumpNormal,
        &minidumpInformation,
        nullptr, nullptr);

    return EXCEPTION_EXECUTE_HANDLER;
}

// ログ（ファイル + デバッグ出力）
void LogToStream(std::ostream& os, const std::string& message) {
    os << message << std::endl;
    Logger::Log(message);  // OutputDebugStringA + std::cout
}

// 頂点/定数バッファ用リソース生成
ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));
    return resource;
}

// mtl 読み込み
MaterialData LoadMaterialTemplateFile(
    const std::string& directoryPath,
    const std::string& filename) {

    MaterialData materialData{};
    std::string line;
    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "map_Kd") {
            std::string textureFilename;
            s >> textureFilename;
            materialData.textureFilePath = directoryPath + "/" + textureFilename;
        }
    }
    return materialData;
}

// obj 読み込み
ModelData LoadObjFile(
    const std::string& directoryPath,
    const std::string& filename) {

    ModelData modelData;
    std::vector<Vector4> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::string line;

    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "v") {
            Vector4 p{};
            s >> p.x >> p.y >> p.z;
            p.x *= -1.0f;  // 左手系へ
            p.w = 1.0f;
            positions.push_back(p);
        } else if (identifier == "vt") {
            Vector2 uv{};
            s >> uv.x >> uv.y;
            uv.y = 1.0f - uv.y;
            texcoords.push_back(uv);
        } else if (identifier == "vn") {
            Vector3 n{};
            s >> n.x >> n.y >> n.z;
            n.x *= -1.0f;
            normals.push_back(n);
        } else if (identifier == "f") {
            VertexData triangle[3]{};

            for (int i = 0; i < 3; ++i) {
                std::string vertexDefinition;
                s >> vertexDefinition;
                std::istringstream v(vertexDefinition);
                uint32_t idx[3]{};

                for (int e = 0; e < 3; ++e) {
                    std::string indexStr;
                    std::getline(v, indexStr, '/');
                    idx[e] = static_cast<uint32_t>(std::stoi(indexStr));
                }

                Vector4 p = positions[idx[0] - 1];
                Vector2 t = texcoords[idx[1] - 1];
                Vector3 n = normals[idx[2] - 1];

                triangle[i] = { p, t, n };
            }

            // 2,1,0 の順に追加
            modelData.vertices.push_back(triangle[2]);
            modelData.vertices.push_back(triangle[1]);
            modelData.vertices.push_back(triangle[0]);
        } else if (identifier == "mtllib") {
            std::string materialFilename;
            s >> materialFilename;
            modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
        }
    }

    return modelData;
}

// シェーダコンパイル
IDxcBlob* CompileShader(
    const std::wstring& filepath,
    const wchar_t* profile,
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* includeHandler,
    std::ostream& os) {

    using StringUtility::ConvertString;

    // ログ：コンパイル開始
    {
        std::wstring wmsg =
            std::format(L"Begin CompileShader, path:{}, profile:{}", filepath, profile);
        LogToStream(os, ConvertString(wmsg));
    }

    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hr = dxcUtils->LoadFile(filepath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hr));

    DxcBuffer shaderSourceBuffer{};
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;

    LPCWSTR arguments[] = {
        filepath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zi",
        L"-Qembed_debug",
        L"-Od",
        L"-Zpr",
        L"-HV", L"2021",
    };

    IDxcResult* shaderResult = nullptr;
    hr = dxcCompiler->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler,
        IID_PPV_ARGS(&shaderResult));
    assert(SUCCEEDED(hr));

    // 警告・エラー
    IDxcBlobUtf8* shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError && shaderError->GetStringLength() != 0) {
        LogToStream(os, shaderError->GetStringPointer());
        assert(false);
    }

    IDxcBlob* shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));

    {
        std::wstring wmsg =
            std::format(L"Compile Succeeded, path:{}, profile:{}", filepath, profile);
        LogToStream(os, ConvertString(wmsg));
    }

    shaderSource->Release();
    shaderResult->Release();
    if (shaderError) { shaderError->Release(); }

    return shaderBlob;
}

// ------------------------
// WinMain
// ------------------------

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    CoInitializeEx(0, COINIT_MULTITHREADED);
    SetUnhandledExceptionFilter(ExportDump);

    // ログディレクトリ作成
    std::filesystem::create_directory("logs");

    // ログファイルオープン
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    auto nowSeconds =
        std::chrono::time_point_cast<std::chrono::seconds>(now);
    std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
    std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
    std::string logFilePath = std::string("logs/") + dateString + ".log";
    std::ofstream logStream(logFilePath);

#pragma region 基盤初期化
    // WinApp
    WinApp* winApp = new WinApp();
    winApp->Initialize();

    // DirectXCommon (シングルトン前提)
    DirectXCommon* dxCommon = new DirectXCommon();
    dxCommon->Initialize(winApp);

    // SpriteCommon（今はまだ使い始めの段階）
    SpriteCommon* spriteCommon = new SpriteCommon();
    spriteCommon->Initialize(dxCommon);

    // TextureManager
    TextureManager* texManager = TextureManager::GetInstance();
    texManager->Initialize(dxCommon);

    // デバイス & コマンドリスト & SRVヒープ
    ID3D12Device* device = dxCommon->GetDevice();
    ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();
    ID3D12DescriptorHeap* srvDescriptorHeap = dxCommon->GetSrvDescriptorHeap();

    // 入力
    Input input;
    input.Initialize(winApp);

    // 仮：Spriteインスタンス（後で使う前提）
    Sprite* sprite = new Sprite();
    sprite->Initialize(spriteCommon);
#pragma endregion

    // ===== DXC 初期化 =====
    HRESULT hr = S_OK;
    IDxcUtils* dxcUtils = nullptr;
    IDxcCompiler3* dxcCompiler = nullptr;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    assert(SUCCEEDED(hr));

    IDxcIncludeHandler* includeHandler = nullptr;
    hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
    assert(SUCCEEDED(hr));

    // ===== 3Dモデル用 RootSignature =====
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
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(
        &descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            LogToStream(logStream,
                reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        }
        assert(false);
    }

    ID3D12RootSignature* rootSignature = nullptr;
    hr = device->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature));
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
        &sprErr);
    if (FAILED(hr)) {
        if (sprErr) {
            LogToStream(logStream,
                reinterpret_cast<char*>(sprErr->GetBufferPointer()));
        }
        assert(false);
    }

    ID3D12RootSignature* spriteRootSignature = nullptr;
    hr = device->CreateRootSignature(
        0,
        sprSigBlob->GetBufferPointer(),
        sprSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&spriteRootSignature));
    assert(SUCCEEDED(hr));

    // ===== モデル読み込み =====
    ModelData modelData = LoadObjFile("resources/obj/fence", "fence.obj");

    // ===== テクスチャを TextureManager で読み込み =====
    texManager->LoadTexture("resources/obj/axis/uvChecker.png");
    texManager->LoadTexture(modelData.material.textureFilePath);

    uint32_t texIndexUvChecker =
        texManager->GetTextureIndexByFilePath("resources/obj/axis/uvChecker.png");
    uint32_t texIndexFence =
        texManager->GetTextureIndexByFilePath(modelData.material.textureFilePath);

    // どのテクスチャをモデルに貼るか（ImGui から切り替え用）
    int currentModelTexture = 0;  // 0: uvChecker, 1: fence

    // ===== InputLayout / Blend / Rasterizer =====
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
            dxcUtils, dxcCompiler, includeHandler, logStream);
    IDxcBlob* pixelShaderBlob =
        CompileShader(L"resources/shaders/Object3d.PS.hlsl", L"ps_6_0",
            dxcUtils, dxcCompiler, includeHandler, logStream);

    IDxcBlob* particleVS =
        CompileShader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0",
            dxcUtils, dxcCompiler, includeHandler, logStream);
    IDxcBlob* particlePS =
        CompileShader(L"resources/shaders/Particle.PS.hlsl", L"ps_6_0",
            dxcUtils, dxcCompiler, includeHandler, logStream);

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
    depthStencilDesc.DepthEnable = TRUE;
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

    ID3D12PipelineState* graphicsPipelineState = nullptr;
    hr = device->CreateGraphicsPipelineState(
        &graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));

    // ===== 各種ブレンド PSO =====
    ID3D12PipelineState* psoNormal = nullptr;
    ID3D12PipelineState* psoAdd = nullptr;
    ID3D12PipelineState* psoSubtract = nullptr;
    ID3D12PipelineState* psoMultiply = nullptr;
    ID3D12PipelineState* psoScreen = nullptr;
    ID3D12PipelineState* psoNone = nullptr;

    // Normal
    {
        D3D12_BLEND_DESC b{};
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
        D3D12_BLEND_DESC b{};
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
        D3D12_BLEND_DESC b{};
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
        D3D12_BLEND_DESC b{};
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
        D3D12_BLEND_DESC b{};
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
        D3D12_BLEND_DESC b{};
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

    // ===== モデル用頂点バッファ =====
    ID3D12Resource* vertexResource =
        CreateBufferResource(device, sizeof(VertexData) * modelData.vertices.size());
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes =
        UINT(sizeof(VertexData) * modelData.vertices.size());
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    std::memcpy(
        vertexData,
        modelData.vertices.data(),
        sizeof(VertexData) * modelData.vertices.size());

    // ===== マテリアル CBV =====
    ID3D12Resource* materialResource =
        CreateBufferResource(device, sizeof(Material));
    Material* materialData = nullptr;
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = { 1.0f,1.0f,1.0f,1.0f };
    materialData->uvTransform = MakeIdentity4x4();
    materialData->enableLighting = true;

    // ===== WVP CBV =====
    ID3D12Resource* wvpResource =
        CreateBufferResource(device, sizeof(TransformationMatrix));
    TransformationMatrix* wvpData = nullptr;
    wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

    // ===== Sprite用 頂点・インデックス =====
    ID3D12Resource* vertexResourceSprite =
        CreateBufferResource(device, sizeof(VertexData) * 4);
    VertexData* vertexDataSprite = nullptr;
    vertexResourceSprite->Map(0, nullptr,
        reinterpret_cast<void**>(&vertexDataSprite));

    vertexDataSprite[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
    vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
    vertexDataSprite[1].position = { -0.5f,  0.5f, 0.0f, 1.0f };
    vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
    vertexDataSprite[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
    vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
    vertexDataSprite[3].position = { 0.5f,  0.5f, 0.0f, 1.0f };
    vertexDataSprite[3].texcoord = { 1.0f, 0.0f };

    for (int i = 0; i < 4; ++i) {
        vertexDataSprite[i].normal = { 0.0f, 0.0f,-1.0f };
    }

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

    // ===== Sprite マテリアル CBV =====
    ID3D12Resource* materialResourceSprite =
        CreateBufferResource(device, sizeof(Material));
    Material* materialDataSprite = nullptr;
    materialResourceSprite->Map(0, nullptr,
        reinterpret_cast<void**>(&materialDataSprite));
    materialDataSprite->color = { 1.0f,1.0f,1.0f,1.0f };
    materialDataSprite->uvTransform = MakeIdentity4x4();
    materialDataSprite->enableLighting = false;

    // ===== Sprite TransformMatrix CBV =====
    ID3D12Resource* transformationMatrixResourceSprite =
        CreateBufferResource(device, sizeof(TransformationMatrix));
    TransformationMatrix* transformationMatrixDataSprite = nullptr;
    transformationMatrixResourceSprite->Map(
        0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));

    // ===== パーティクル用インスタンシング =====
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
    instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instancingSrvDesc.Buffer.FirstElement = 0;
    instancingSrvDesc.Buffer.NumElements = kNumInstance;
    instancingSrvDesc.Buffer.StructureByteStride = sizeof(TransformationMatrix);
    instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    // SRV index: 0=ImGui, 1,2=TextureManager, 3=Instancing とする
    const uint32_t kInstancingSrvIndex = 3;
    D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvCPU =
        dxCommon->GetSRVCPUDescriptorHandle(kInstancingSrvIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvGPU =
        dxCommon->GetSRVGPUDescriptorHandle(kInstancingSrvIndex);
    device->CreateShaderResourceView(
        instancingResource, &instancingSrvDesc, instancingSrvCPU);

    // ===== 平行光源 CBV =====
    ID3D12Resource* directionalLightResource =
        CreateBufferResource(device, sizeof(DirectionalLight));
    DirectionalLight* directionalLightData = nullptr;
    directionalLightResource->Map(
        0, nullptr, reinterpret_cast<void**>(&directionalLightData));
    directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
    directionalLightData->direction = Normalize({ 0.0f,-1.0f,0.0f });
    directionalLightData->intensity = 1.0f;

    // ===== 各種 Transform 初期値 =====
    Transform transformSprite{
        { 1.0f,1.0f,1.0f },
        { 0.0f,0.0f,0.0f },
        { 0.0f,0.0f,0.0f },
    };
    Transform transform{
        { 1.0f,1.0f,1.0f },
        { 0.0f,0.0f,0.0f },
        { 0.0f,0.0f,0.0f },
    };
    Transform cameraTransform{
        { 1.0f,1.0f,1.0f },
        { 0.0f,0.0f,0.0f },
        { 0.0f,0.0f,-5.0f },
    };
    Transform uvTransformSprite{
        { 1.0f,1.0f,1.0f },
        { 0.0f,0.0f,0.0f },
        { 0.0f,0.0f,0.0f },
    };

    bool useMonstarBall = true;

    std::random_device seedGenerator;
    std::mt19937 randomEngine(seedGenerator());

    float randPosRange = 0.15f;
    float randVelRange = 1.0f;
    float randVelZScale = 0.0f;

    float layoutStartX = -1.4f;
    float layoutStartY = -0.8f;
    float layoutStartZ = 0.0f;
    float layoutStepX = 0.22f;
    float layoutStepY = 0.11f;
    float layoutStepZ = 0.05f;
    float spriteScaleXY = 0.90f;

    Particle particles[kNumInstance];
    for (uint32_t i = 0; i < kNumInstance; ++i) {
        particles[i].transform.scale = { 1.0f,1.0f,1.0f };
        particles[i].transform.rotate = { 0.0f,0.0f,0.0f };
        particles[i].transform.translate = {
            -1.8f + 0.4f * i,
            -1.0f + 0.2f * i,
             0.25f * i
        };
        particles[i].velocity = { 0.0f,0.0f,0.0f };
    }

    MSG msg{};

    // ----------------
    // メインループ
    // ----------------
    while (true) {
        if (winApp->ProcessMessage()) {
            break;
        }

        // ImGui フレーム開始
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 入力更新
        input.Update();

        // ==== カメラ操作 ====
        const bool uiWantsMouse = ImGui::GetIO().WantCaptureMouse;
        if (!uiWantsMouse && input.MouseDown(Input::MouseLeft)) {
            const float sensitivity = 0.0025f;
            cameraTransform.rotate.y += input.MouseDeltaX() * sensitivity;
            cameraTransform.rotate.x += input.MouseDeltaY() * sensitivity;
            const float kPitchLimit = 1.55334306f;
            cameraTransform.rotate.x =
                std::clamp(cameraTransform.rotate.x, -kPitchLimit, kPitchLimit);
        }

        Vector3 move{ 0.0f,0.0f,0.0f };
        if (input.PushKey(DIK_W)) move.z += 1.0f;
        if (input.PushKey(DIK_S)) move.z -= 1.0f;
        if (input.PushKey(DIK_D)) move.x += 1.0f;
        if (input.PushKey(DIK_A)) move.x -= 1.0f;

        if (float len = std::sqrt(move.x * move.x + move.z * move.z); len > 0.0f) {
            move.x /= len;
            move.z /= len;
        }

        float speed = input.PushKey(DIK_LSHIFT) ? 6.0f : 2.5f;
        const float dt = 1.0f / 60.0f;

        if (move.x != 0.0f || move.z != 0.0f) {
            const float yaw = cameraTransform.rotate.y;
            Vector3 forward = { std::sin(yaw), 0.0f, std::cos(yaw) };
            Vector3 right = { std::cos(yaw), 0.0f,-std::sin(yaw) };
            Vector3 delta{
                right.x * move.x + forward.x * move.z,
                0.0f,
                right.z * move.x + forward.z * move.z
            };
            cameraTransform.translate.x += delta.x * speed * dt;
            cameraTransform.translate.y += delta.y * speed * dt;
            cameraTransform.translate.z += delta.z * speed * dt;
        }

        if (input.TriggerKey(DIK_0)) {
            Logger::Log("Hit 0");
        }

        // ==== ImGui ====
        ImGui::ShowDemoWindow();

        ImGui::Begin("Material / Particle / Texture");

        ImGui::SeparatorText("Model Transform");
        ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 5.0f);
        ImGui::SliderAngle("RotateX", &transform.rotate.x, -180.0f, 180.0f);
        ImGui::SliderAngle("RotateY", &transform.rotate.y, -180.0f, 180.0f);
        ImGui::SliderAngle("RotateZ", &transform.rotate.z, -180.0f, 180.0f);
        ImGui::SliderFloat3("Translate", &transform.translate.x, -5.0f, 5.0f);

        ImGui::SeparatorText("Model Texture");
        const char* modelTextureNames[] = {
            "uvChecker",
            "FenceTexture",
        };
        ImGui::Combo("ModelTex",
            &currentModelTexture,
            modelTextureNames,
            IM_ARRAYSIZE(modelTextureNames));

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
                    layoutStartZ + layoutStepZ * i,
                };
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].velocity = { 0.0f,0.0f,0.0f };
            }
        }

        static bool play = false;
        if (ImGui::Button(play ? "Stop" : "Start")) {
            play = !play;
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                float jx = randPosRange * dist(randomEngine);
                float jy = randPosRange * dist(randomEngine);
                float jz = randPosRange * dist(randomEngine);

                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].transform.rotate = { 0.0f,0.0f,0.0f };
                particles[i].transform.translate = {
                    layoutStartX + layoutStepX * i + jx,
                    layoutStartY + layoutStepY * i + jy,
                    layoutStartZ + layoutStepZ * i + jz,
                };

                particles[i].velocity = {
                    randVelRange * dist(randomEngine),
                    randVelRange * dist(randomEngine),
                    randVelRange * randVelZScale * dist(randomEngine),
                };
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            play = false;
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].transform.rotate = { 0.0f,0.0f,0.0f };
                particles[i].transform.translate = {
                    layoutStartX + layoutStepX * i,
                    layoutStartY + layoutStepY * i,
                    layoutStartZ + layoutStepZ * i,
                };
                particles[i].velocity = { 0.0f,0.0f,0.0f };
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
                    layoutStartZ + layoutStepZ * i,
                };
            }
        }
        if (autoApply && !play) {
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].transform.translate = {
                    layoutStartX + layoutStepX * i,
                    layoutStartY + layoutStepY * i,
                    layoutStartZ + layoutStepZ * i,
                };
            }
        }

        ImGui::SeparatorText("Lighting / Material");
        ImGui::Checkbox("useMonstarBall", &useMonstarBall);
        ImGui::Text("Lighting Direction");
        ImGui::SliderFloat("lx", &directionalLightData->direction.x, -10.0f, 10.0f);
        ImGui::SliderFloat("ly", &directionalLightData->direction.y, -10.0f, 10.0f);
        ImGui::SliderFloat("lz", &directionalLightData->direction.z, -10.0f, 10.0f);
        ImGui::Text("Color");
        ImGui::ColorEdit4("Color", &materialData->color.x);

        ImGui::SeparatorText("UV Transform");
        ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
        ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

        ImGui::SeparatorText("Material Blend Mode");
        static int currentBlendMode = 0;
        const char* blendModeNames[] = {
            "Normal","Add","Subtract","Multiply","Screen","None"
        };
        ImGui::Combo("BlendMode", &currentBlendMode,
            blendModeNames, IM_ARRAYSIZE(blendModeNames));

        ImGui::End();

        directionalLightData->direction =
            Normalize(directionalLightData->direction);

        // ==== UV行列の更新（スプライトの切り取り用）====
        {
            // ImGui からいじっている uvTransformSprite を 4x4 行列に変換
            Vector3 uvScale = { uvTransformSprite.scale.x,  uvTransformSprite.scale.y,  1.0f };
            Vector3 uvRotate = { 0.0f, 0.0f, uvTransformSprite.rotate.z };
            Vector3 uvTrans = { uvTransformSprite.translate.x, uvTransformSprite.translate.y, 0.0f };

            // UV 変換用のアフィン行列を作ってマテリアルに書き戻す
            materialDataSprite->uvTransform = MakeAffine(uvScale, uvRotate, uvTrans);
        }

        // ==== 行列計算 ====
        Matrix4x4 worldMatrix =
            MakeAffine(transform.scale, transform.rotate, transform.translate);

        Matrix4x4 cameraMatrix =
            MakeAffine(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
        Matrix4x4 viewMatrix = Inverse(cameraMatrix);
        Matrix4x4 projectionMatrix = PerspectiveFov(
            0.45f,
            float(WinApp::kClientWidth) / float(WinApp::kClientHeight),
            0.1f, 100.0f);
        Matrix4x4 worldViewProjectionMatrix =
            Multipty(worldMatrix, Multipty(viewMatrix, projectionMatrix));

        // パーティクル行列更新
        const float kDeltaTime = 1.0f / 60.0f;
        for (uint32_t i = 0; i < kNumInstance; ++i) {
            if (play) {
                particles[i].transform.translate.x += particles[i].velocity.x * kDeltaTime;
                particles[i].transform.translate.y += particles[i].velocity.y * kDeltaTime;
                particles[i].transform.translate.z += particles[i].velocity.z * kDeltaTime;
            }
            Matrix4x4 w = MakeAffine(
                particles[i].transform.scale,
                particles[i].transform.rotate,
                particles[i].transform.translate);
            Matrix4x4 wvp = Multipty(w, Multipty(viewMatrix, projectionMatrix));
            instancingData[i].WVP = wvp;
            instancingData[i].World = w;
        }

        ImGui::Render();

        // ==== 描画 ====
        dxCommon->PreDraw();

        ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap };
        commandList->SetDescriptorHeaps(1, heaps);

        // --- 3D モデル描画 ---
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
        case 0: commandList->SetPipelineState(psoNormal);   break;
        case 1: commandList->SetPipelineState(psoAdd);      break;
        case 2: commandList->SetPipelineState(psoSubtract); break;
        case 3: commandList->SetPipelineState(psoMultiply); break;
        case 4: commandList->SetPipelineState(psoScreen);   break;
        case 5: commandList->SetPipelineState(psoNone);     break;
        default: commandList->SetPipelineState(psoNormal);  break;
        }

        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

        // 使用テクスチャを TextureManager から取得
        uint32_t useTexIndex =
            (currentModelTexture == 0) ? texIndexUvChecker : texIndexFence;
        D3D12_GPU_DESCRIPTOR_HANDLE modelTexHandle =
            texManager->GetSrvHandleGPU(useTexIndex);
        commandList->SetGraphicsRootDescriptorTable(2, modelTexHandle);

        commandList->DrawInstanced(
            UINT(modelData.vertices.size()), 1, 0, 0);

        // --- パーティクルスプライト描画 ---
        commandList->SetGraphicsRootSignature(spriteRootSignature);
        commandList->SetPipelineState(psoSpriteParticles);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
        commandList->IASetIndexBuffer(&indexBufferViewSprite);
        commandList->SetGraphicsRootConstantBufferView(
            0, materialResourceSprite->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(1, instancingSrvGPU);

        // スプライトには uvChecker を貼る
        D3D12_GPU_DESCRIPTOR_HANDLE spriteTexHandle =
            texManager->GetSrvHandleGPU(texIndexUvChecker);
        commandList->SetGraphicsRootDescriptorTable(2, spriteTexHandle);

        commandList->DrawIndexedInstanced(6, kNumInstance, 0, 0, 0);

        // ImGui 描画
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

        dxCommon->PostDraw();
    }

    // ==== テクスチャ中間リソース解放 ====
    texManager->ReleaseIntermediateResources();
    texManager->Finalize();

    // ==== 後始末 ====
    psoSpriteParticles->Release();
    spriteRootSignature->Release();
    particleVS->Release();
    particlePS->Release();
    instancingResource->Release();
    sprSigBlob->Release();
    if (sprErr) { sprErr->Release(); }

    graphicsPipelineState->Release();
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

    materialResourceSprite->Release();
    vertexResourceSprite->Release();
    indexResourceSprite->Release();
    transformationMatrixResourceSprite->Release();
    directionalLightResource->Release();

    // DirectX / WinApp の終了
    dxCommon->Finalize();
    winApp->Finalize();

    delete sprite;
    delete spriteCommon;
    delete dxCommon;
    delete winApp;

    CoUninitialize();

    // リソースリークチェック
    IDXGIDebug1* debug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
        debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
        debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
        debug->Release();
    }

    return 0;
}
