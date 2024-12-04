
#include "Scene.h"
#include <iostream>
#include "yaml-cpp/yaml.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// 导入库
#define LIGHTMAPPER_IMPLEMENTATION
#define LM_DEBUG_INTERPOLATION
#include "lightmapper.h"

Scene::Scene(GLFWWindowFactory* window) :window(window) {
    // 加载定向光配置
    this->directionalLights = loadDirectionalLights("config/directionalLights.yaml");
    this->numDirectionalLights = this->directionalLights.size();
    // 加载点光源配置
    pointLights = loadPointLights("config/pointLights.yaml");
    // 加载场景配置
    this->modelInfos = loadScene("config/scene.yaml");

    /// 阴影深度贴图处理
    // 给directionLightDepthMapFBOs分配大小
    this->directionLightDepthMapFBOs.resize(this->numDirectionalLights);
    // 给directionLightDepthMaps分配大小
    this->directionLightDepthMaps.resize(this->numDirectionalLights);
    // 给directionLightDepthVarianceMaps分配大小
    this->directionLightDepthMeanVarMaps.resize(this->numDirectionalLights);
    this->d_d2_filter_FBO.resize(this->numDirectionalLights * 2);
    this->d_d2_filter_maps.resize(this->numDirectionalLights * 2);
    // 加载深度贴图
    loadDirectionLightDepthMap();

    /// 光照贴图处理
    // 加载光照贴图
    loadLightMap();

    // 为每个模型信息加载模型
    for (auto& modelInfo : modelInfos) {

        modelInfo.model = new Model(modelInfo.path, vertices, indices);
    }

    // 初始化着色器
    this->shader = Shader("shaders/sceneShader.vs", "shaders/sceneShader.fs");
    // 初始化方向光阴影着色器
    this->directionLightShadowShader = Shader("shaders/directionLightShadowShader.vs", "shaders/directionLightShadowShader.fs");
    // 初始化均值方差计算着色器
    this->d_d2_filter_shader = Shader("shaders/vsmShader.vs", "shaders/vsmShader.fs");
    // 初始化光照贴图着色器
    this->lightMapShader = Shader("shaders/lightMapShader.vs", "shaders/lightMapShader.fs");
}

Scene::~Scene() {
}

void Scene::draw() {
    // 处理输入
    processInputMoveDirLight();
    if (BAKE) {
        static int baking = 0; // 添加一个标志
        if (glfwGetKey(this->window->window, GLFW_KEY_SPACE) == GLFW_PRESS && !baking) {
            baking = 1; // 设置标志
            cout << "baking" << endl;
            bakeLightMap();
        }
        if (glfwGetKey(this->window->window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
            baking = 0; // 重置标志
        }
    }

    // 渲染深度贴图
    renderSceneToDepthMap();

    this->shader.use();
    if (BAKE) {
        // 使用光照贴图
        this->shader.setBool("useLightMap", true);
    }
    else {
        // 不使用光照贴图
        this->shader.setBool("useLightMap", false);
    }

    // 设置场景着色器uniform变量
    setupSceneUniform();

    // 切换回默认视口
    glViewport(0, 0, this->SCR_WIDTH, this->SCR_HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 渲染场景
    renderScene(this->shader, true);
}


std::vector<Scene::ModelInfo> Scene::loadScene(const std::string& fileName) {
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

std::vector<Scene::DirectionalLight> Scene::loadDirectionalLights(const std::string& fileName) {
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
                light.lightSpaceMatrix = glm::mat4(1.0f);
                directionalLights.push_back(light);
            }
        }
    } catch (const YAML::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return directionalLights;
}

std::vector<Scene::PointLight> Scene::loadPointLights(const std::string& fileName) {
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

void Scene::loadDirectionLightDepthMap() {
    for (int i = 0; i < this->numDirectionalLights; ++i) {
        // 创建帧缓冲对象
        glGenFramebuffers(1, &this->directionLightDepthMapFBOs[i]);
        // 深度贴图
        // 创建深度贴图
        glGenTextures(1, &this->directionLightDepthMaps[i]);
        // 绑定深度纹理
        glBindTexture(GL_TEXTURE_2D, this->directionLightDepthMaps[i]);
        // 只关注深度值，设置为GL_DEPTH_COMPONENT
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        // 设置纹理过滤方式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // 设置纹理环绕方式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // 存储深度贴图边框颜色（防止出现采样过多，这样超出深度贴图的坐标就不会一直在阴影中）
        float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        // 绑定深度贴图到帧缓冲对象
        glBindFramebuffer(GL_FRAMEBUFFER, this->directionLightDepthMapFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->directionLightDepthMaps[i], 0);

        if (SHADOW_ALGORITHM == 3) {
            // 深度的均值和方差贴图
            // 创建深度贴图
            glGenTextures(1, &this->directionLightDepthMeanVarMaps[i]);
            // 绑定深度纹理
            glBindTexture(GL_TEXTURE_2D, this->directionLightDepthMeanVarMaps[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
            // 设置纹理过滤方式
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // 存储深度贴图边框颜色（防止出现采样过多，这样超出深度贴图的坐标就不会一直在阴影中）
            float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            // 绑定到 DepthMap 中
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->directionLightDepthMeanVarMaps[i], 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
            }
            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, drawBuffers);
        }
        else {
            // 如果不需要颜色附件，则禁用颜色输出
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }
    }

    // 解绑帧缓冲对象
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (SHADOW_ALGORITHM == 3) {
        for (int i = 0; i < this->numDirectionalLights; ++i) {
            glGenFramebuffers(1, &this->d_d2_filter_FBO[i * 2]);
            glGenTextures(1, &this->d_d2_filter_maps[i * 2]);
            glBindTexture(GL_TEXTURE_2D, this->d_d2_filter_maps[i * 2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glBindFramebuffer(GL_FRAMEBUFFER, this->d_d2_filter_FBO[i * 2]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->d_d2_filter_maps[i * 2], 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
            }

            glGenFramebuffers(1, &this->d_d2_filter_FBO[i * 2 + 1]);
            glGenTextures(1, &this->d_d2_filter_maps[i * 2 + 1]);
            glBindTexture(GL_TEXTURE_2D, this->d_d2_filter_maps[i * 2 + 1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glBindFramebuffer(GL_FRAMEBUFFER, this->d_d2_filter_FBO[i * 2 + 1]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->d_d2_filter_maps[i * 2 + 1], 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Scene::renderSceneToDepthMap() {
    // 解决悬浮(pater panning)的阴影失真问题
    // 告诉opengl剔除正面
    glCullFace(GL_FRONT);
    // 投影矩阵
    // 阴影贴图覆盖的实际范围(正交投影)
    float edge = 120.0f;
    glm::mat4 lightProjection = glm::ortho(-edge, edge, -edge, edge, NEAR_PLANE, FAR_PLANE);
    // 透视投影
    // 对每个方向光生成阴影贴图
    // 视图矩阵
    glm::mat4 lightView;
    // 计算阴影矩阵
    for (int i = 0; i < this->numDirectionalLights; ++i) {
        lightView = glm::lookAt(-directionalLights[i].direction * 1.0f, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        this->directionalLights[i].lightSpaceMatrix = lightProjection * lightView;
        // DEBUG
        // cout << "lightSpaceMatrix: " << lightSpaceMatrix[0][0] << " " << lightSpaceMatrix[0][1] << " " << lightSpaceMatrix[0][2] << " " << lightSpaceMatrix[0][3] << endl;
        // cout << "lightSpaceMatrix: " << lightSpaceMatrix[1][0] << " " << lightSpaceMatrix[1][1] << " " << lightSpaceMatrix[1][2] << " " << lightSpaceMatrix[1][3] << endl;
        // cout << "lightSpaceMatrix: " << lightSpaceMatrix[2][0] << " " << lightSpaceMatrix[2][1] << " " << lightSpaceMatrix[2][2] << " " << lightSpaceMatrix[2][3] << endl;
        // cout << "lightSpaceMatrix: " << lightSpaceMatrix[3][0] << " " << lightSpaceMatrix[3][1] << " " << lightSpaceMatrix[3][2] << " " << lightSpaceMatrix[3][3] << endl;

        // 使用着色器
        this->directionLightShadowShader.use();
        // 传递阴影矩阵给着色器
        this->directionLightShadowShader.setMat4("lightSpaceMatrix", this->directionalLights[i].lightSpaceMatrix);

        // 切换视口
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

        // 绑定帧缓冲
        glBindFramebuffer(GL_FRAMEBUFFER, this->directionLightDepthMapFBOs[i]);

        if (SHADOW_ALGORITHM == 3) {
            glClearColor(1.0f, 1.0f, 0.0f, 1.0f); // 注意这里的初始化, 1.0f 深度最大值
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        }
        else {
            glClear(GL_DEPTH_BUFFER_BIT);
        }

        // 渲染场景
        renderScene(this->directionLightShadowShader, false);

        if (SHADOW_ALGORITHM == 3) {
            // 绑定均值和方差帧缓冲对象 pass2
            glBindFramebuffer(GL_FRAMEBUFFER, this->d_d2_filter_FBO[i * 2]);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // 使用均值和方差计算着色器
            this->d_d2_filter_shader.use();
            this->d_d2_filter_shader.setBool("vertical", false);
            this->d_d2_filter_shader.setInt("d_d2", 0);
            // 激活深度贴图
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, this->directionLightDepthMeanVarMaps[i]);
            renderQuad();

            // 绑定均值和方差帧缓冲对象 pass3
            glBindFramebuffer(GL_FRAMEBUFFER, this->d_d2_filter_FBO[i * 2 + 1]);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // 使用均值和方差计算着色器
            this->d_d2_filter_shader.use();
            this->d_d2_filter_shader.setBool("vertical", true);
            this->d_d2_filter_shader.setInt("d_d2", 0);
            // 激活深度贴图
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, this->d_d2_filter_maps[i * 2]);
            renderQuad();
        }
    }

    // 解绑帧缓冲对象
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // 恢复剔除背面
    glCullFace(GL_BACK);
}

void Scene::renderScene(Shader& shader, bool isActiveTexture) {
    shader.use();
    // 绘制每个模型
    for (const auto& modelInfo : modelInfos) {
        // 获取当前时间（s）
        float currentTime = glfwGetTime();
        // 根据时间计算旋转角度，10.0f是速度因子
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
            if (!BAKE)
                model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        // 缩放模型
        model = glm::scale(model, modelInfo.scale);
        // 传递模型矩阵给着色器
        shader.setMat4("model", model);

        // 绘制模型
        modelInfo.model->draw(shader, this->directionLightDepthMaps, isActiveTexture, this->d_d2_filter_maps, SHADOW_ALGORITHM == 3, BAKE, lightMap);
    }
}

void Scene::processInputMoveDirLight() {
    // 定义方向变化的步长
    float step = 0.01f;

    // 监听按键事件
    if (glfwGetKey(this->window->window, GLFW_KEY_UP) == GLFW_PRESS) {
        directionalLights[0].direction.y -= step;
    }
    if (glfwGetKey(this->window->window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        directionalLights[0].direction.y += step;
    }
    if (glfwGetKey(this->window->window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        directionalLights[0].direction.z -= step;
    }
    if (glfwGetKey(this->window->window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        directionalLights[0].direction.z += step;
    }

    // 限制y和z坐标的范围
    if (directionalLights[0].direction.y < -2.0f) {
        directionalLights[0].direction.y = -2.0f;
    }
    if (directionalLights[0].direction.y > 2.0f) {
        directionalLights[0].direction.y = 2.0f;
    }
    if (directionalLights[0].direction.z < -3.0f) {
        directionalLights[0].direction.z = -3.0f;
    }
    if (directionalLights[0].direction.z > 3.0f) {
        directionalLights[0].direction.z = 3.0f;
    }

    // 监听按键事件
    // if (glfwGetKey(this->window->window, GLFW_KEY_I) == GLFW_PRESS) {
    //     directionalLights[1].direction.y -= step;
    // }
    // if (glfwGetKey(this->window->window, GLFW_KEY_K) == GLFW_PRESS) {
    //     directionalLights[1].direction.y += step;
    // }
    // if (glfwGetKey(this->window->window, GLFW_KEY_J) == GLFW_PRESS) {
    //     directionalLights[1].direction.x -= step;
    // }
    // if (glfwGetKey(this->window->window, GLFW_KEY_L) == GLFW_PRESS) {
    //     directionalLights[1].direction.x += step;
    // }

    // 限制y和z坐标的范围
    // if (directionalLights[1].direction.y < -2.0f) {
    //     directionalLights[1].direction.y = -2.0f;
    // }
    // if (directionalLights[1].direction.y > 2.0f) {
    //     directionalLights[1].direction.y = 2.0f;
    // }
    // if (directionalLights[1].direction.x < -3.0f) {
    //     directionalLights[1].direction.x = -3.0f;
    // }
    // if (directionalLights[1].direction.x > 3.0f) {
    //     directionalLights[1].direction.x = 3.0f;
    // }
}

void Scene::renderQuad() {
    if (this->quadVAO == 0) {
        float quadVertices[] = {
            // 位置             // 纹理坐标
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, // 左下角
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f, // 右下角
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f, // 左上角
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f, // 右上角
        };
        // 生成VAO
        glGenVertexArrays(1, &this->quadVAO);
        // 生成VBO
        glGenBuffers(1, &this->quadVBO);
        // 将VAO绑定到当前上下文
        glBindVertexArray(this->quadVAO);
        // 将VBO绑定到GL_ARRAY_BUFFER
        glBindBuffer(GL_ARRAY_BUFFER, this->quadVBO);
        // 将顶点数据复制到VBO
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        // 设置顶点属性指针
        // 位置属性
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        // 纹理坐标属性
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        // 解绑VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // 解绑VAO
        glBindVertexArray(0);
    }

    // 绘制四边形
    glBindVertexArray(this->quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Scene::setupSceneUniform() {
    // -- 场景着色器配置 -- 
    this->shader.use();
    // 传递方向光数量给着色器
    this->shader.setInt("numDirectionalLights", this->numDirectionalLights);
    // 传递每个方向光的属性给着色器
    for (auto i = 0; i < this->numDirectionalLights; i++) {
        std::string number = std::to_string(i);
        this->shader.setVec3("directionalLights[" + number + "].direction", this->directionalLights[i].direction);
        this->shader.setVec3("directionalLights[" + number + "].ambient", this->directionalLights[i].ambient);
        this->shader.setVec3("directionalLights[" + number + "].diffuse", this->directionalLights[i].diffuse);
        this->shader.setVec3("directionalLights[" + number + "].specular", this->directionalLights[i].specular);
        this->shader.setVec3("directionalLights[" + number + "].lightColor", this->directionalLights[i].lightColor);
        // 将阴影矩阵传递给着色器
        this->shader.setMat4("directionalLights[" + number + "].lightSpaceMatrix", this->directionalLights[i].lightSpaceMatrix);
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
    // 传递光源宽度给着色器
    this->shader.setFloat("lightWidth", this->lightWidth);
    // 将PCF采样半径传递给着色器
    this->shader.setFloat("PCFSampleRadius", this->PCFSampleRadius);
    // 设置阴影映射算法类型
    this->shader.setInt("shadowMapType", SHADOW_ALGORITHM);
    // 将近平面和远平面传递给着色器
    this->shader.setFloat("near_plane", NEAR_PLANE);
    this->shader.setFloat("far_plane", FAR_PLANE);
}

void Scene::loadLightMap() {
    // 生成光照贴图
    glGenTextures(1, &this->lightMap);
    // 绑定光照贴图
    glBindTexture(GL_TEXTURE_2D, this->lightMap);
    // 设置光照贴图环绕和过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    unsigned char emissive[] = { 0, 0, 0, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, emissive);
}

int Scene::bakeLightMap() {
    // lmCrate用于创建一个光照映射的上下文
    lm_context* ctx = lmCreate(
        512,               // 表示渲染质量或分辨率，通常越大越精细，但也会增加计算开销
        0.001f, 10.0f,   // 定义了渲染的近裁剪面和远裁剪面
        1.0f, 1.0f, 1.0f, // 定义环境光颜色
        5, 0.0001f,         // 定义了层次选择性插值的设置，用于加速计算，2代表迭代次数，0.01f是阈值，用于决定何时进行插值
        0.0f);            // 用于影响从摄像机到表面距离的计算，帮助光照贴图计算中权衡质量和性能

    // 初始化失败
    if (!ctx) {
        fprintf(stderr, "Error: Could not initialize lightmapper.\n");
        return 0;
    }

    // 分配内存用于存储光照贴图数据
    float* data = (float*)calloc(LIGHT_MAP_WIDTH * LIGHT_MAP_HEIGHT * 4, sizeof(float));
    // 设置目标光照贴图
    lmSetTargetLightmap(ctx, data, LIGHT_MAP_WIDTH, LIGHT_MAP_HEIGHT, 4);

    // 设置几何数据
    // 输出scene->vertices大小
    printf("verticeCount: %d\n", this->vertices.size());
    // 输出scene->indexCount大小
    printf("indexCount: %d\n", this->indices.size());
    lmSetGeometry(ctx, NULL,                                                                 // no transformation in this example
        LM_FLOAT, (unsigned char*)(vertices.data()) + offsetof(vertex_t, p), sizeof(vertex_t),
        LM_NONE, NULL, 0, // 不使用插值法线
        LM_FLOAT, (unsigned char*)(vertices.data()) + offsetof(vertex_t, t), sizeof(vertex_t),
        indices.size(), LM_UNSIGNED_SHORT, indices.data());

    int vp[4];
    float view[16], projection[16];
    double lastUpdateTime = 0.0;
    while (lmBegin(ctx, vp, view, projection)) {
        // 渲染到光照贴图帧缓冲区
        glViewport(vp[0], vp[1], vp[2], vp[3]);

        // 将视图矩阵传递给着色器
        this->shader.use();
        this->shader.setMat4("view", glm::make_mat4(view));
        // 将投影矩阵传递给着色器
        this->shader.setMat4("projection", glm::make_mat4(projection));

        // 渲染场景
        renderScene(this->shader, false);

        // 每秒显示进度
        double time = glfwGetTime();
        if (time - lastUpdateTime > 1.0) {
            lastUpdateTime = time;
            printf("\r%6.2f%%", lmProgress(ctx) * 100.0f);
            fflush(stdout);
        }

        lmEnd(ctx);
    }
    printf("\rFinished baking %d triangles.\n", indices.size() / 3);

    // 销毁光照贴图上下文
    lmDestroy(ctx);

    // 后处理纹理
    float* temp = (float*)calloc(LIGHT_MAP_WIDTH * LIGHT_MAP_HEIGHT * 4, sizeof(float));
    for (int i = 0; i < 16; i++) {
        lmImageDilate(data, temp, LIGHT_MAP_WIDTH, LIGHT_MAP_HEIGHT, 4);
        lmImageDilate(temp, data, LIGHT_MAP_WIDTH, LIGHT_MAP_HEIGHT, 4);
    }
    lmImageSmooth(data, temp, LIGHT_MAP_WIDTH, LIGHT_MAP_HEIGHT, 4);
    lmImageSmooth(data, temp, LIGHT_MAP_WIDTH, LIGHT_MAP_HEIGHT, 4);

    lmImageDilate(temp, data, LIGHT_MAP_WIDTH, LIGHT_MAP_HEIGHT, 4);
    lmImagePower(data, LIGHT_MAP_WIDTH, LIGHT_MAP_HEIGHT, 4, 1.0f / 2.2f, 0x7); // 伽马矫正颜色通道
    free(temp);

    // 保存结果到文件
    if (lmImageSaveTGAf("result.tga", data, LIGHT_MAP_WIDTH, LIGHT_MAP_HEIGHT, 4, 1.0f))
        printf("Saved result.tga\n");

    // 上传结果到opengl纹理
    glBindTexture(GL_TEXTURE_2D, lightMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LIGHT_MAP_WIDTH, LIGHT_MAP_HEIGHT, 0, GL_RGBA, GL_FLOAT, data);
    free(data);

    return 1;
}