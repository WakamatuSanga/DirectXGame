#include "ModelManager.h"

ModelManager* ModelManager::instance = nullptr;

ModelManager* ModelManager::GetInstance()
{
	if (instance == nullptr) {
		instance = new ModelManager();
	}
	return instance;
}

void ModelManager::Initialize(DirectXCommon* dxCommon)
{
	// モデル共通部の生成
	modelCommon_ = new ModelCommon();
	modelCommon_->Initialize(dxCommon);
}

void ModelManager::Finalize()
{
	// 共通部の解放
	delete modelCommon_;
	modelCommon_ = nullptr;

	// マップのクリア（unique_ptrなので中身も自動解放される）
	models_.clear();

	delete instance;
	instance = nullptr;
}

void ModelManager::LoadModel(const std::string& filePath)
{
	// 読み込み済みモデルを検索
	if (models_.contains(filePath)) {
		// 読み込み済みなら早期リターン
		return;
	}

	// 読み込まれていない場合、新しく生成して登録する

	// 1. ファイルパスから「ディレクトリパス」と「ファイル名」に分割する
	// 例: "resources/obj/fence/fence.obj"
	// -> directoryPath: "resources/obj/fence"
	// -> filename: "fence.obj"

	std::string directoryPath;
	std::string filename;

	// 最後のスラッシュを探す
	size_t pos = filePath.find_last_of('/');
	if (pos != std::string::npos) {
		directoryPath = filePath.substr(0, pos);
		filename = filePath.substr(pos + 1);
	} else {
		// スラッシュが無い場合はそのまま
		directoryPath = "";
		filename = filePath;
	}

	// 2. モデル生成と初期化
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(modelCommon_, directoryPath, filename);

	// 3. マップに登録 (所有権を移動)
	// キーはフルパス(filePath)にする
	models_.insert(std::make_pair(filePath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath)
{
	// 読み込み済みならポインタを返す
	if (models_.contains(filePath)) {
		return models_.at(filePath).get();
	}

	// 読み込まれていなければ nullptr
	return nullptr;
}