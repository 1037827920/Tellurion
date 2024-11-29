
#include "TellurionModel.h"
#include <iostream>
#include "yaml-cpp/yaml.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/// @brief 构造函数，初始化窗口和加载配置文件
/// @param window  opengl窗口
Tellurion::Tellurion(GLFWWindowFactory* window) :window(window) {
    // 加载方向光配置
    directionalLights = loadDirectionalLights("config/directionalLights.yaml");
    // 加载点光源配置
    pointLights = loadPointLights("config/pointLights.yaml");
    // 加载场景配置
    this->modelInfos = loadScene("config/scene.yaml");
    // 加载深度贴图
    loadDirectionLightDepthMap();

    // 为每个模型信息加载模型
    for (auto& modelInfo : modelInfos) {
        // 新建模型文件夹路径
        string folderPath = modelInfo.path.substr(0, modelInfo.path.find_last_of("/"));
        // 新建材质文件路径
        string materialPath = modelInfo.path.substr(0, modelInfo.path.find_last_of(".")) + ".mtl";
        // 新建材质
        Material material;
        // 加载材质
        loadMaterial(folderPath, materialPath, material);
        // 加载模型
        modelInfo.model = new Model(modelInfo.path);
        modelInfo.material = material;
    }

    // 初始化着色器
    this->shader = Shader("shader.vs", "shader.fs");
    // 初始化方向光阴影着色器
    this->directionLightShadowShader = Shader("directionLightShadowShader.vs", "directionLightShadowShader.fs");
    // 初始化点光源阴影着色器
    // 初始化调试深度贴图着色器
    this->debugDepthQuad = Shader("debug_depth_quad.vs", "debug_depth_quad.fs");
}

/// @brief 析构函数，释放资源
Tellurion::~Tellurion() {
}

/// @brief 绘制函数，用于渲染场景
void Tellurion::draw() {
    // 渲染深度贴图
    // renderSceneToDepthMap();

    // 切换回默认视口
    glViewport(0, 0, this->SCR_WIDTH, this->SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // -- 场景着色器配置 -- 
    this->shader.use();
    // 将阴影矩阵传递给着色器
    this->shader.setMat4("lightSpaceMatrix", this->lightSpaceMatrix);
    // 传递方向光数量给着色器
    this->shader.setInt("numDirectionalLights", directionalLights.size());
    // 传递每个方向光的属性给着色器
    for (auto i = 0; i < directionalLights.size(); i++) {
        std::string number = std::to_string(i);
        this->shader.setVec3("directionalLights[" + number + "].direction", directionalLights[i].direction);
        // this->shader.setVec3("directionalLights[" + number + "].ambient", directionalLights[i].ambient);
        this->shader.setVec3("directionalLights[" + number + "].diffuse", directionalLights[i].diffuse);
        this->shader.setVec3("directionalLights[" + number + "].specular", directionalLights[i].specular);
        this->shader.setVec3("directionalLights[" + number + "].lightColor", directionalLights[i].lightColor);
    }
    // 传递点光源数量给着色器
    this->shader.setInt("numPointLights", pointLights.size());
    // 传递每个点光源的属性给着色器
    for (auto i = 0; i < pointLights.size(); i++) {
        std::string number = std::to_string(i);
        this->shader.setVec3("pointLights[" + number + "].position", pointLights[i].position);
        this->shader.setVec3("pointLights[" + number + "].ambient", pointLights[i].ambient);
        this->shader.setVec3("pointLights[" + number + "].diffuse", pointLights[i].diffuse);
        this->shader.setVec3("pointLights[" + number + "].specular", pointLights[i].specular);
        this->shader.setFloat("pointLights[" + number + "].constant", pointLights[i].constant);
        this->shader.setFloat("pointLights[" + number + "].linear", pointLights[i].linear);
        this->shader.setFloat("pointLights[" + number + "].quadratic", pointLights[i].quadratic);
        this->shader.setVec3("pointLights[" + number + "].lightColor", pointLights[i].lightColor);
    }
    // 当按下键1时，切换Blinn-Phong着色模式(将blinn传递给着色器)
    if (window->blinn) {
        this->shader.setInt("blinn", 1);
    }
    else {
        this->shader.setInt("blinn", 0);
    }
    // 传递投影矩阵和视图矩阵给着色器
    this->shader.setMat4("projection", window->getProjectionMatrix());
    this->shader.setMat4("view", window->getViewMatrix());
    auto camera = window->camera;
    // 传递摄像机位置给着色器
    this->shader.setVec3("viewPos", camera.Position);

    // 渲染场景
    renderScene(false);
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

void Tellurion::loadDirectionLightDepthMap() {
    // 创建帧缓冲对象
    glGenFramebuffers(1, &this->directionLightDepthMapFBO);
    // 创建深度贴图
    glGenTextures(1, &this->directionLightDepthMap);
    // 绑定深度纹理
    glBindTexture(GL_TEXTURE_2D, this->directionLightDepthMap);
    // 只关注深度值，设置为GL_DEPTH_COMPONENT
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    // 设置纹理过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // 设置纹理环绕方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // 绑定深度贴图到帧缓冲对象
    glBindFramebuffer(GL_FRAMEBUFFER, this->directionLightDepthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->directionLightDepthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    // 解绑帧缓冲对象
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// void Tellurion::loadDepthMap() {
//     // 创建帧缓冲对象
//     glGenFramebuffers(1, &this->directionLightDepthMapFBO);
//     // 创建深度贴图
//     glGenTextures(1, &this->directionLightDepthMap);
//     // 绑定深度纹理
//     glBindTexture(GL_TEXTURE_CUBE_MAP, directionLightDepthMap);
//     // 生成立方体贴图的每个面
//     for (unsigned int i = 0; i < 6; ++i) {
//         // 只关注深度值，设置为GL_DEPTH_COMPONENT
//         glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
//     }
//     // 设置纹理过滤方式
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     // 设置纹理环绕方式
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//     // 绑定深度贴图到帧缓冲对象
//     glBindFramebuffer(GL_FRAMEBUFFER, this->directionLightDepthMapFBO);
//     glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->directionLightDepthMap, 0);
//     // 不需要颜色缓冲
//     glDrawBuffer(GL_NONE);
//     glReadBuffer(GL_NONE);
//     // 解绑帧缓冲对象
//     glBindFramebuffer(GL_FRAMEBUFFER, 0);
// }

void Tellurion::renderSceneToDepthMap() {
    // 投影矩阵
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, NEAR_PLANE, FAR_PLANE);

    // 对每个方向光生成阴影贴图（先糊一下代码，假设目前只有一个方向光）
    // 视图矩阵
    glm::mat4 lightView = glm::lookAt(directionalLights[0].direction * 10.0f, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    // 计算阴影矩阵
    this->lightSpaceMatrix = lightProjection * lightView;

    // 使用着色器
    this->directionLightShadowShader.use();
    // 传递模型矩阵给着色器
    this->directionLightShadowShader.setMat4("model", glm::mat4(1.0f));
    // 传递阴影矩阵给着色器
    this->directionLightShadowShader.setMat4("lightSpaceMatrix", this->lightSpaceMatrix);

    // 切换视口
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, this->directionLightDepthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // 渲染场景
    renderScene();

    // 解绑帧缓冲对象
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Tellurion::renderScene(bool isRenderDepthMap) {
    // 使用着色器
    this->shader.use();

    // 绘制每个模型
    for (const auto& modelInfo : modelInfos) {
        // 获取当前时间（s）
        float currentTime = glfwGetTime();
        // 根据时间计算旋转角度，5.0f是速度因子
        float angle = currentTime * 10.0f;

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
            // model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        // 缩放模型
        model = glm::scale(model, modelInfo.scale);
        // 传递模型矩阵给着色器
        this->shader.setMat4("model", model);

        // 传递材质给着色器
        this->shader.setFloat("material.shininess", modelInfo.material.shininess);
        this->shader.setVec3("material.ambient", modelInfo.material.ambient);
        this->shader.setVec3("material.diffuse", modelInfo.material.diffuse);
        this->shader.setVec3("material.specular", modelInfo.material.specular);
        // 设置地球贴图
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, modelInfo.material.texture);
        shader.setInt("material.texture", 0);
        // 设置法线贴图
        if (modelInfo.material.normalMap != 0) {
            shader.setBool("material.sampleNormalMap", true);
            // 实现法线贴图
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, modelInfo.material.normalMap);
            this->shader.setInt("material.normalMap", 1);
        }
        else {
            this->shader.setBool("material.sampleNormalMap", false);
        }
        // 设置specularMap
        if (modelInfo.material.specularMap != 0) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, modelInfo.material.specularMap);
            shader.setInt("material.specularMap", 2);
        }
        // 设置深度贴图
        if (isRenderDepthMap) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, directionLightDepthMap);
            shader.setInt("directionalLightShadowMaps", 3);
        }

        // 绘制模型
        modelInfo.model->draw(this->shader, this->directionLightDepthMap);
    }
}

void Tellurion::loadMaterial(const std::string& folderPath, const std::string& mtlPath, Material& material) {
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << mtlPath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "newmtl") {
            ss >> material.name;
        }
        else if (token == "Ka") {
            ss >> material.ambient.r >> material.ambient.g >> material.ambient.b;
        }
        else if (token == "Kd") {
            ss >> material.diffuse.r >> material.diffuse.g >> material.diffuse.b;
        }
        else if (token == "Ks") {
            ss >> material.specular.r >> material.specular.g >> material.specular.b;
        }
        else if (token == "map_Kd") {
            std::string texturePath;
            ss >> texturePath;

            texturePath = folderPath + "/" + texturePath;
            material.texture = loadTexture(texturePath);
        }
        else if (token == "map_Ks") {
            std::string texturePath;
            ss >> texturePath;

            texturePath = folderPath + "/" + texturePath;
            material.specularMap = loadTexture(texturePath);
        }
        else if (token == "map_Bump") { // 法线贴图
            std::string texturePath;
            ss >> texturePath;

            texturePath = folderPath + "/" + texturePath;
            material.normalMap = loadTexture(texturePath);
        }
    }

    // 控制物体表面的高光效果，越大产生的高光越小但更锐利
    material.shininess = 108.0f;

    file.close();
}

GLuint Tellurion::loadTexture(const std::string& texturePath) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 4)
            format = GL_RGBA;
        else if (nrChannels == 3)
            format = GL_RGB;
        else
            format = GL_RED;  // or handle other formats as needed

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);  // Use format variable here

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        return texture;
    }
    else {
        std::cout << "Texture failed to load at path: " << texturePath << std::endl;
        stbi_image_free(data);
        return 0;
    }
}