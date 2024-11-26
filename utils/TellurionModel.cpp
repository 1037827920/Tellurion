
#include "TellurionModel.h"
#include <iostream>
#include "yaml-cpp/yaml.h"

/// @brief 构造函数，初始化窗口和加载配置文件
/// @param window  opengl窗口
Tellurion::Tellurion(GLFWWindowFactory* window) :window(window) {
    // 加载方向光配置
    directionalLights = loadDirectionalLights("config/directionalLights.yaml");
    // 加载点光源配置
    pointLights = loadPointLights("config/pointLights.yaml");
    // 加载场景配置
    modelInfos = loadScene("config/scene.yaml");

    // 为每个模型信息加载模型
    for (auto& modelInfo : modelInfos) {
        modelInfo.model = new Model(modelInfo.path);
    }

    // 初始化着色器
    shader = Shader("shader.vs", "shader.fs");
}

/// @brief 析构函数，释放资源
Tellurion::~Tellurion() {
}

/// @brief 绘制函数，用于渲染场景
void Tellurion::draw() {
    // 使用着色器
    shader.use();

    // 传递方向光数量给着色器
    shader.setInt("numDirectionalLights", directionalLights.size());
    // 传递每个方向光的属性给着色器
    for (auto i = 0; i < directionalLights.size(); i++) {
        std::string number = std::to_string(i);
        shader.setVec3("directionalLights[" + number + "].direction", directionalLights[i].direction);
        shader.setVec3("directionalLights[" + number + "].ambient", directionalLights[i].ambient);
        shader.setVec3("directionalLights[" + number + "].diffuse", directionalLights[i].diffuse);
        shader.setVec3("directionalLights[" + number + "].specular", directionalLights[i].specular);
        shader.setVec3("directionalLights[" + number + "].lightColor", directionalLights[i].lightColor);
    }

    // 传递点光源数量给着色器
    shader.setInt("numPointLights", pointLights.size());
    // 传递每个点光源的属性给着色器
    for (auto i = 0; i < pointLights.size(); i++) {
        std::string number = std::to_string(i);
        shader.setVec3("pointLights[" + number + "].position", pointLights[i].position);
        shader.setVec3("pointLights[" + number + "].ambient", pointLights[i].ambient);
        shader.setVec3("pointLights[" + number + "].diffuse", pointLights[i].diffuse);
        shader.setVec3("pointLights[" + number + "].specular", pointLights[i].specular);
        shader.setFloat("pointLights[" + number + "].constant", pointLights[i].constant);
        shader.setFloat("pointLights[" + number + "].linear", pointLights[i].linear);
        shader.setFloat("pointLights[" + number + "].quadratic", pointLights[i].quadratic);
        shader.setVec3("pointLights[" + number + "].lightColor", pointLights[i].lightColor);
    }

    // 绘制每个模型
    for (const auto& modelInfo : modelInfos) {
        // 获取当前时间（s）
        float currentTime = glfwGetTime();
        // 根据时间计算旋转角度，5.0f是速度因子
        float angle = currentTime * 5.0f;

        // 初始化模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        // 平移模型
        model = glm::translate(model, modelInfo.position);

        // 静态旋转
        model = glm::rotate(model, glm::radians(modelInfo.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(modelInfo.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(modelInfo.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        if (modelInfo.path.find("sphere.obj") != std::string::npos) {
            // 添加倾斜23°26'，因为支架的模型本来就是倾斜的，所以不用再倾斜，只需要调整球体即可
            float tiltAngle = 23.433f;
            model = glm::rotate(model, glm::radians(tiltAngle), glm::vec3(0.0f, 0.0f, 1.0f));

            // 动态旋转（绕y轴旋转
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));   
        }

        // 缩放模型
        model = glm::scale(model, modelInfo.scale);

        // 传递模型矩阵给着色器
        shader.setMat4("model", model);
        // 绘制模型
        modelInfo.model->draw(shader);
    }

    // 传递投影矩阵和视图矩阵给着色器
    shader.setMat4("projection", window->getProjectionMatrix());
    shader.setMat4("view", window->getViewMatrix());
    auto camera = window->camera;
    // 传递摄像机位置给着色器
    shader.setVec3("viewPos", camera.Position);

    // 当按下键1时，切换Blinn-Phong着色模式
    if (window->blinn) {
        shader.setInt("blinn", 1);
    }
    else {
        shader.setInt("blinn", 0);
    }
}

/// @brief 加载场景配置文件
/// @param fileName 文件名
/// @return 模型信息
std::vector<Tellurion::ModelInfo> Tellurion::loadScene(const std::string& fileName) {
    std::vector<ModelInfo> models;
    try {
        YAML::Node scene = YAML::LoadFile(fileName);
        if (scene["models"]) {
            for (size_t i = 0; i < scene["models"].size(); ++i) {
                ModelInfo info;
                info.path = scene["models"][i]["path"].as<std::string>();
                info.position.x = scene["models"][i]["position"]["x"].as<float>();
                info.position.y = scene["models"][i]["position"]["y"].as<float>();
                info.position.z = scene["models"][i]["position"]["z"].as<float>();
                info.rotation.x = scene["models"][i]["rotation"]["x"].as<float>();
                info.rotation.y = scene["models"][i]["rotation"]["y"].as<float>();
                info.rotation.z = scene["models"][i]["rotation"]["z"].as<float>();
                info.scale.x = scene["models"][i]["scale"]["x"].as<float>();
                info.scale.y = scene["models"][i]["scale"]["y"].as<float>();
                info.scale.z = scene["models"][i]["scale"]["z"].as<float>();
                models.push_back(info);
                // 打印模型信息
                std::cout << info.path << std::endl;
                std::cout << info.position.x << " " << info.position.y << " " << info.position.z << std::endl;
                std::cout << info.rotation.x << " " << info.rotation.y << " " << info.rotation.z << std::endl;
                std::cout << info.scale.x << " " << info.scale.y << " " << info.scale.z << std::endl;
            }
        }
    } catch (const YAML::BadFile& e) {
        std::cerr << "Error: Unable to open file " << fileName << std::endl;
    } catch (const YAML::ParserException& e) {
        std::cerr << "Error: Parsing failed: " << e.what() << std::endl;
    }

    return models;
}

/// @brief 加载方向光配置文件
/// @param fileName 文件名
/// @return 返回方向光信息
std::vector<Tellurion::DirectionalLight> Tellurion::loadDirectionalLights(const std::string& fileName) {
    std::vector<DirectionalLight> directionalLights;
    try {
        YAML::Node scene = YAML::LoadFile(fileName);
        if (scene["directionalLights"]) {
            for (size_t i = 0; i < scene["directionalLights"].size(); ++i) {
                DirectionalLight light;
                light.direction.x = scene["directionalLights"][i]["direction"]["x"].as<float>();
                light.direction.y = scene["directionalLights"][i]["direction"]["y"].as<float>();
                light.direction.z = scene["directionalLights"][i]["direction"]["z"].as<float>();
                light.ambient.x = scene["directionalLights"][i]["ambient"]["x"].as<float>();
                light.ambient.y = scene["directionalLights"][i]["ambient"]["y"].as<float>();
                light.ambient.z = scene["directionalLights"][i]["ambient"]["z"].as<float>();
                light.diffuse.x = scene["directionalLights"][i]["diffuse"]["x"].as<float>();
                light.diffuse.y = scene["directionalLights"][i]["diffuse"]["y"].as<float>();
                light.diffuse.z = scene["directionalLights"][i]["diffuse"]["z"].as<float>();
                light.specular.x = scene["directionalLights"][i]["specular"]["x"].as<float>();
                light.specular.y = scene["directionalLights"][i]["specular"]["y"].as<float>();
                light.specular.z = scene["directionalLights"][i]["specular"]["z"].as<float>();
                light.lightColor.x = scene["directionalLights"][i]["lightColor"]["x"].as<float>();
                light.lightColor.y = scene["directionalLights"][i]["lightColor"]["y"].as<float>();
                light.lightColor.z = scene["directionalLights"][i]["lightColor"]["z"].as<float>();
                directionalLights.push_back(light);
            }
        }
    } catch (const YAML::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return directionalLights;
}

/// @brief 加载点光源配置文件
/// @param fileName 文件名
/// @return 返回点光源信息
std::vector<Tellurion::PointLight> Tellurion::loadPointLights(const std::string& fileName) {
    std::vector<PointLight> pointLights;
    try {
        YAML::Node scene = YAML::LoadFile(fileName);
        if (scene["pointLights"]) {
            for (size_t i = 0; i < scene["pointLights"].size(); ++i) {
                PointLight light;
                light.position.x = scene["pointLights"][i]["position"]["x"].as<float>();
                light.position.y = scene["pointLights"][i]["position"]["y"].as<float>();
                light.position.z = scene["pointLights"][i]["position"]["z"].as<float>();
                light.constant = scene["pointLights"][i]["constant"].as<float>();
                light.linear = scene["pointLights"][i]["linear"].as<float>();
                light.quadratic = scene["pointLights"][i]["quadratic"].as<float>();
                light.ambient.x = scene["pointLights"][i]["ambient"]["x"].as<float>();
                light.ambient.y = scene["pointLights"][i]["ambient"]["y"].as<float>();
                light.ambient.z = scene["pointLights"][i]["ambient"]["z"].as<float>();
                light.diffuse.x = scene["pointLights"][i]["diffuse"]["x"].as<float>();
                light.diffuse.y = scene["pointLights"][i]["diffuse"]["y"].as<float>();
                light.diffuse.z = scene["pointLights"][i]["diffuse"]["z"].as<float>();
                light.specular.x = scene["pointLights"][i]["specular"]["x"].as<float>();
                light.specular.y = scene["pointLights"][i]["specular"]["y"].as<float>();
                light.specular.z = scene["pointLights"][i]["specular"]["z"].as<float>();
                light.lightColor.x = scene["pointLights"][i]["lightColor"]["x"].as<float>();
                light.lightColor.y = scene["pointLights"][i]["lightColor"]["y"].as<float>();
                light.lightColor.z = scene["pointLights"][i]["lightColor"]["z"].as<float>();
                pointLights.push_back(light);
            }
        }
    } catch (const YAML::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return pointLights;
}