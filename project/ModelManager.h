#pragma once
#include <map>
#include <string>
#include <memory>
#include "Model.h"
#include "ModelCommon.h"

class ModelManager
{
    friend struct std::default_delete<ModelManager>;

private:
    static std::unique_ptr<ModelManager> instance_;

    ModelManager() = default;
    ~ModelManager() = default;
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

public:
    static ModelManager* GetInstance();

    void Initialize(DirectXCommon* dxCommon);
    void Finalize();

    void LoadModel(const std::string& filePath);
    Model* FindModel(const std::string& filePath);

private:
    std::map<std::string, std::unique_ptr<Model>> models_;

    std::unique_ptr<ModelCommon> modelCommon_;
};