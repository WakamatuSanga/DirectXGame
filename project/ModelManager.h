#pragma once
#include <map>
#include <string>
#include <memory>
#include "Model.h"
#include "ModelCommon.h"

class ModelManager
{
private:
	// シングルトンインスタンス
	static ModelManager* instance;

	// コンストラクタ等を隠蔽
	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

public:
	// シングルトン取得
	static ModelManager* GetInstance();

	// 初期化
	void Initialize(DirectXCommon* dxCommon);

	// 終了
	void Finalize();

	// モデルファイルの読み込み
	// filePath : "resources/obj/fence/fence.obj" のようなパス
	void LoadModel(const std::string& filePath);

	// 読み込み済みモデルの検索・取得
	Model* FindModel(const std::string& filePath);

private:
	// モデルデータ (キー:ファイルパス, 値:Modelのスマートポインタ)
	std::map<std::string, std::unique_ptr<Model>> models_;

	// モデル共通部 (このクラスが所有権を持つ)
	ModelCommon* modelCommon_ = nullptr;
};