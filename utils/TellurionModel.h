#pragma once

#include "windowFactory.h"
#include <glad/glad.h>
#include "shader.h"
#include <vector>
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// #include "objModel.h"
#include "model.h"

class Tellurion {
    /// 材质结构体
    struct Material {
        string name;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        GLuint texture;
        GLuint normalMap;
        GLuint specularMap;
        float shininess;
    };
    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 lightColor;
    };
    struct PointLight {
        std::string path;
        Model* model = nullptr;

        glm::vec3 position;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 lightColor;
        float constant;
        float linear;
        float quadratic;
    };
    struct ModelInfo {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        std::string path;
        Model* model = nullptr;
        Material material;
    };
public:

    // 场景渲染着色器
    Shader shader;

    Tellurion(GLFWWindowFactory* window);

    void draw();
    ~Tellurion();
    std::vector<ModelInfo> loadScene(const std::string& fileName);
    std::vector<DirectionalLight> loadDirectionalLights(const std::string& fileName);
    std::vector<PointLight> loadPointLights(const std::string& fileName);

private:
    static const unsigned int SHADOW_WIDTH = 1024;
    static const unsigned int SHADOW_HEIGHT = 1024;
    static const unsigned int SCR_WIDTH = 800;
    static const unsigned int SCR_HEIGHT = 600;
    static constexpr float NEAR_PLANE = 1.0f;
    static constexpr float FAR_PLANE = 25.0f;

    // 方向光阴影渲染着色器
    Shader directionLightShadowShader;
    // 点光源阴影渲染着色器
    // Shader pointLightShadowShader;

    std::vector<unsigned int> texture;
    GLFWWindowFactory* window;
    // 方向光帧缓冲对象
    unsigned int directionLightDepthMapFBO;
    // 方向光深度贴图
    unsigned int directionLightDepthMap;
    // 方向光阴影矩阵
    glm::mat4 lightSpaceMatrix;

    std::vector<ModelInfo> modelInfos;
    std::vector<DirectionalLight> directionalLights;
    std::vector<PointLight> pointLights;

    // 加载方向光深度贴图
    void loadDirectionLightDepthMap();
    // void loadDepthMap();
    void renderSceneToDepthMap();
    void renderScene(bool isRenderDepthMap = false);
    void renderQuad();
    // 加载材质
    void loadMaterial(const std::string& folderPath, const string& mtlPath, Material& material);
    // 加载纹理
    GLuint loadTexture(const string& texturePath);
};
