#include "ModelManager.h"
#include <filesystem>

std::unique_ptr<ModelManager> ModelManager::instance_ = nullptr;

ModelManager* ModelManager::GetInstance()
{
    if (!instance_) {
        instance_.reset(new ModelManager());
    }
    return instance_.get();
}

void ModelManager::Initialize(DirectXCommon* dxCommon)
{
    modelCommon_ = std::make_unique<ModelCommon>();
    modelCommon_->Initialize(dxCommon);
}

void ModelManager::Finalize()
{
    models_.clear();
}

void ModelManager::LoadModel(const std::string& filePath)
{
    if (models_.contains(filePath)) { return; }

    // ファイルが存在しない場合はクラッシュを防ぐためにスキップ
    if (!std::filesystem::exists(filePath)) { return; }

    std::string directoryPath;
    std::string filename;
    size_t pos = filePath.find_last_of('/');
    if (pos != std::string::npos) {
        directoryPath = filePath.substr(0, pos);
        filename = filePath.substr(pos + 1);
    } else {
        directoryPath = "";
        filename = filePath;
    }

    std::unique_ptr<Model> model = std::make_unique<Model>();
    model->Initialize(modelCommon_.get(), directoryPath, filename);

    models_.insert(std::make_pair(filePath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath)
{
    if (models_.contains(filePath)) {
        return models_.at(filePath).get();
    }
    return nullptr;
}

Model* ModelManager::CreateSphere(const std::string& keyName, uint32_t subdivision)
{
    if (models_.contains(keyName)) {
        return models_.at(keyName).get();
    }

    std::unique_ptr<Model> model = std::make_unique<Model>();
    Model::ModelData data = Model::CreateSphereData(subdivision);
    model->Initialize(modelCommon_.get(), data);

    models_.insert(std::make_pair(keyName, std::move(model)));
    return models_.at(keyName).get();
}