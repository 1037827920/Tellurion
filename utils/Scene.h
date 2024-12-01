#ifndef SCENE_H
#define SCENE_H

// 定义了Scene类，用来加载场景中的模型

#include "windowFactory.h"
#include <glad/glad.h>
#include "shader.h"
#include <vector>
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "model.h"

using std::vector;

class Scene {
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
        // 光的方向
        glm::vec3 direction;
        // 环境光系数
        glm::vec3 ambient;
        // 漫反射系数
        glm::vec3 diffuse;
        // 镜面反射系数
        glm::vec3 specular;
        // 光的颜色
        glm::vec3 lightColor;
        // 光空间矩阵
        glm::mat4 lightSpaceMatrix;
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
    // 定向光数组
    vector<DirectionalLight> directionalLights;

    /// @brief 构造函数，初始化窗口和加载配置文件
    /// @param window  opengl窗口
    Scene(GLFWWindowFactory* window);

    /// @brief 绘制函数，用于渲染场景
    void draw();

    ~Scene();
private:
    static const unsigned int SHADOW_WIDTH = 1024;
    static const unsigned int SHADOW_HEIGHT = 1024;
    static const unsigned int SCR_WIDTH = 800;
    static const unsigned int SCR_HEIGHT = 600;
    // 阴影贴图能够覆盖的最近距离
    static constexpr float NEAR_PLANE = 1.0f;
    // 阴影贴图能够覆盖的最远距离
    static constexpr float FAR_PLANE = 90.0f;
    // 光源宽度，影响阴影的柔和度，较大的光源宽度会导致阴影边缘更加柔和
    static constexpr float lightWidth = 0.132f;
    // PCF采样半径
    static constexpr float PCFSampleRadius = 0.588f;

    // 方向光阴影渲染着色器
    Shader directionLightShadowShader;
    // 点光源阴影渲染着色器
    // Shader pointLightShadowShader;

    vector<unsigned int> texture;
    GLFWWindowFactory* window;
    // 定向光帧缓冲对象
    unsigned int directionLightDepthMapFBO;
    // 定向光深度贴图
    vector<unsigned int> directionLightDepthMaps;

    // 模型信息
    vector<ModelInfo> modelInfos;
    // 定向光数量
    int numDirectionalLights;
    // 点光源数组
    vector<PointLight> pointLights;


    /// @brief 加载场景配置文件 
    /// @param fileName 文件名
    /// @return 模型信息
    vector<ModelInfo> loadScene(const std::string& fileName);
    /// @brief 加载方向光配置文件
    /// @param fileName 文件名
    /// @return 返回方向光信息
    vector<DirectionalLight> loadDirectionalLights(const std::string& fileName);
    /// @brief 加载点光源配置文件
    /// @param fileName 文件名
    /// @return 返回点光源信息
    vector<PointLight> loadPointLights(const std::string& fileName);
    void loadDirectionLightDepthMap();
    void renderSceneToDepthMap();
    /// @brief 渲染场景
    /// @param shader 使用的着色器
    /// @param isActiveTexture 是否激活纹理，一般是开启的，在渲染深度贴图时不开启（也就是从光源的视角渲染场景时
    void renderScene(Shader& shader, bool isActiveTexture);
    // void loadDepthMap();
    /// @brief 处理输入，移动定向光
    void processInputMoveDirLight();
};

#endif // SCENE_H