#ifndef SCENE_H
#define SCENE_H

// 定义了Scene类，用来加载场景中的模型

#include <glad/glad.h>
#include "shader.h"
#include <vector>
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdlib.h>

#include "windowFactory.h"
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
    // 定向光数组
    vector<DirectionalLight> directionalLights;

    /// @brief 构造函数，初始化窗口和加载配置文件
    /// @param window  opengl窗口
    Scene(GLFWWindowFactory* window);

    /// @brief 绘制函数，用于渲染场景
    void draw();

    ~Scene();
private:
    // 屏幕的宽度和
    static const unsigned int SCR_WIDTH = 800;
    // 屏幕的高度
    static const unsigned int SCR_HEIGHT = 600;
    // 阴影贴图的宽度
    static const unsigned int SHADOW_WIDTH = 1024;
    // 阴影贴图的高度
    static const unsigned int SHADOW_HEIGHT = 1024;
    // 阴影贴图能够覆盖的最近距离
    static constexpr float NEAR_PLANE = 2.0f;
    // 阴影贴图能够覆盖的最远距离
    static constexpr float FAR_PLANE = 120.0f;
    // 光源宽度，影响阴影的柔和度，较大的光源宽度会导致阴影边缘更加柔和
    static constexpr float lightWidth = 0.132f;
    // PCF采样半径
    static constexpr float PCFSampleRadius = 0.588f;
    // 阴影算法类型
    // 0: SM
    // 1: PCF
    // 2: PCSS
    // 3: VSM
    static const unsigned int SHADOW_ALGORITHM = 1;
    // 光照贴图的宽度
    unsigned int LIGHT_MAP_WIDTH = 1024;
    // 光照贴图的高度
    unsigned int LIGHT_MAP_HEIGHT = 1024;
    // 是否使用光线烘焙
    const bool BAKE = false;


    // 场景渲染着色器
    Shader shader;
    // 方向光阴影渲染着色器
    Shader directionLightShadowShader;
    // 均值和方差计算着色器
    Shader d_d2_filter_shader;
    // 光照贴图着色器
    Shader lightMapShader;

    GLFWWindowFactory* window;
    // 定向光帧缓冲对象
    vector<unsigned int> directionLightDepthMapFBOs;
    // 定向光深度贴图
    vector<unsigned int> directionLightDepthMaps;
    // 定向光深度的方差和均值贴图
    vector<unsigned int> directionLightDepthMeanVarMaps;
    vector<unsigned int> d_d2_filter_FBO;
    vector<unsigned int> d_d2_filter_maps;

    // 模型信息
    vector<ModelInfo> modelInfos;
    // 定向光数量
    int numDirectionalLights;
    // 点光源数组
    vector<PointLight> pointLights;

    // 屏幕的渲染数据
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    // 光照贴图
    unsigned int lightMap;
    // 顶点数据
    vector<vertex_t> vertices;
    // 索引数据
    vector<unsigned int> indices;

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
    /// @brief 加载定向光深度贴图
    void loadDirectionLightDepthMap();
    /// @brief 加载光照贴图
    void loadLightMap();
    void renderSceneToDepthMap();
    /// @brief 设置场景的统一变量
    void setupSceneUniform();
    /// @brief 渲染场景
    /// @param shader 使用的着色器
    /// @param isActiveTexture 是否激活纹理，一般是开启的，在渲染深度贴图时不开启（也就是从光源的视角渲染场景时
    void renderScene(Shader& shader, bool isActiveTexture);
    /// @brief 处理输入，移动定向光
    void processInputMoveDirLight();
    /// @brief 渲染整个屏幕，一般用于图像后期处理
    void renderQuad();
    /// @brief 光照贴图烘培函数
    int bakeLightMap();
};

#endif // SCENE_H