#pragma once

#include <xaudio2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl.h>
#include <string>
#include <map>

// XAudio2 と MediaFoundation のライブラリをリンク
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

class Audio {
public:
    // 音声データ構造体
    struct SoundData {
        WAVEFORMATEX wfex;
        BYTE* pBuffer;
        unsigned int bufferSize;
    };

public: // メンバ関数
    static Audio* GetInstance();

    void Initialize();
    void Finalize();

    // =============== ここに追加しています ===============
    // 音声ファイルの読み込み (WAV, MP3 など対応)
    void LoadAudio(const std::string& filename);

    // 音声の再生
    void PlayAudio(const std::string& filename);
    // ====================================================

private: // プライベート関数
    Audio() = default;
    ~Audio() = default;
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    // 音声データの解放
    void UnloadAudio(SoundData* soundData);

private: // メンバ変数
    static Audio* instance_;

    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    IXAudio2MasteringVoice* masterVoice_ = nullptr;

    std::map<std::string, SoundData> soundDatas_;
};