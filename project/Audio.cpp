#include "Audio.h"
#include "StringUtility.h" // パス変換用
#include <cassert>
#include <vector>

Audio* Audio::instance_ = nullptr;

Audio* Audio::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = new Audio();
    }
    return instance_;
}

void Audio::Initialize() {
    HRESULT result;

    // XAudioエンジンのインスタンスを生成
    result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(result));

    // マスターボイスを生成
    result = xAudio2_->CreateMasteringVoice(&masterVoice_);
    assert(SUCCEEDED(result));

    // Windows Media Foundationの初期化
    result = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    assert(SUCCEEDED(result));
}

void Audio::Finalize() {
    // 読み込んだ全ての音声データを解放する
    for (auto& pair : soundDatas_) {
        UnloadAudio(&pair.second);
    }
    soundDatas_.clear();

    // Windows Media Foundationの終了
    MFShutdown();

    if (xAudio2_) {
        xAudio2_.Reset();
    }

    delete instance_;
    instance_ = nullptr;
}

// ====================================================================
// WAVだけでなく、MP3などもMFを使って自動デコードして読み込む
// ====================================================================
void Audio::LoadAudio(const std::string& filename) {
    if (soundDatas_.contains(filename)) {
        return;
    }

    // Media Foundation API を使うため、ファイルパスをワイド文字列に変換
    std::wstring wFilePath = StringUtility::ConvertString(filename);

    // SourceReaderの作成 (ここでMP3でもWAVでも自動判別して開いてくれる)
    Microsoft::WRL::ComPtr<IMFSourceReader> pReader;
    HRESULT hr = MFCreateSourceReaderFromURL(wFilePath.c_str(), nullptr, &pReader);
    assert(SUCCEEDED(hr));

    // デコード後のフォーマット（PCM）を指定する
    Microsoft::WRL::ComPtr<IMFMediaType> pAudioType;
    hr = MFCreateMediaType(&pAudioType);
    assert(SUCCEEDED(hr));
    hr = pAudioType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    assert(SUCCEEDED(hr));
    hr = pAudioType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    assert(SUCCEEDED(hr));

    // SourceReaderにフォーマットを設定
    hr = pReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, pAudioType.Get());
    assert(SUCCEEDED(hr));

    // 実際の出力メディアタイプを取得し、WAVEFORMATEXに変換
    Microsoft::WRL::ComPtr<IMFMediaType> pUncompressedAudioType;
    hr = pReader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &pUncompressedAudioType);
    assert(SUCCEEDED(hr));

    WAVEFORMATEX* pWfex = nullptr;
    UINT32 wfexSize = 0;
    hr = MFCreateWaveFormatExFromMFMediaType(pUncompressedAudioType.Get(), &pWfex, &wfexSize);
    assert(SUCCEEDED(hr));

    // 音声データをすべて読み込む
    std::vector<BYTE> audioData;
    while (true) {
        Microsoft::WRL::ComPtr<IMFSample> pSample;
        DWORD dwFlags = 0;
        hr = pReader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr, &dwFlags, nullptr, &pSample);
        assert(SUCCEEDED(hr));

        // ファイルの終端に達したらループを抜ける
        if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
            break;
        }

        if (pSample) {
            Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;
            hr = pSample->ConvertToContiguousBuffer(&pBuffer);
            assert(SUCCEEDED(hr));

            BYTE* pData = nullptr;
            DWORD cbData = 0;
            hr = pBuffer->Lock(&pData, nullptr, &cbData);
            assert(SUCCEEDED(hr));

            // 読み込んだデータをバッファに追加
            size_t currentSize = audioData.size();
            audioData.resize(currentSize + cbData);
            memcpy(audioData.data() + currentSize, pData, cbData);

            pBuffer->Unlock();
        }
    }

    // データを格納する構造体を準備
    SoundData soundData{};
    soundData.wfex = *pWfex;

    // APIで確保されたWAVEFORMATEXのメモリを解放
    CoTaskMemFree(pWfex);

    // 最終的なサイズでメモリを確保し、ベクターからデータをコピー
    soundData.bufferSize = static_cast<unsigned int>(audioData.size());
    soundData.pBuffer = new BYTE[soundData.bufferSize];
    memcpy(soundData.pBuffer, audioData.data(), soundData.bufferSize);

    // コンテナに登録
    soundDatas_.insert(std::make_pair(filename, soundData));
}

void Audio::UnloadAudio(SoundData* soundData) {
    delete[] soundData->pBuffer;
    soundData->pBuffer = nullptr;
    soundData->bufferSize = 0;
    soundData->wfex = {};
}

// ====================================================================
// 再生処理
// ====================================================================
void Audio::PlayAudio(const std::string& filename) {
    // 読み込み済みか確認
    assert(soundDatas_.contains(filename));

    const SoundData& soundData = soundDatas_.at(filename);
    HRESULT result;

    // 波形フォーマットを元にSourceVoiceの生成
    IXAudio2SourceVoice* pSourceVoice = nullptr;
    result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
    assert(SUCCEEDED(result));

    // 再生する波形データの設定
    XAUDIO2_BUFFER buf{};
    buf.pAudioData = soundData.pBuffer;
    buf.AudioBytes = soundData.bufferSize;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    // 波形データの再生
    result = pSourceVoice->SubmitSourceBuffer(&buf);
    assert(SUCCEEDED(result));

    result = pSourceVoice->Start();
    assert(SUCCEEDED(result));
}