#include "SkyBox.h"
#include "stb_image.h"

// public

/// @brief 构造函数
/// @param face_paths 纹理路径 
SkyBox::SkyBox(GLFWWindowFactory* window) : window(window) {
    vector<string> face_paths{
        "assets/skybox/right.jpg",
        "assets/skybox/left.jpg",
        "assets/skybox/top.jpg",
        "assets/skybox/bottom.jpg",
        "assets/skybox/front.jpg",
        "assets/skybox/back.jpg"
    };
    // 加载纹理
    loadTexture(face_paths);
    // 初始化渲染数据
    setupVertices();
    // 初始化着色器
    this->shader = Shader("shaders/skyboxShader.vs", "shaders/skyboxShader.fs");
}

/// @brief 绘制天空盒
/// @param shader 天空盒着色器
void SkyBox::draw() {
    // 设置深度测试的比较函数
    // Gl_LEQUAL表示深度值小于或等于深度缓冲区值的像素能够通过深度测试
    glDepthFunc(GL_LEQUAL);
    // 使用着色器
    this->shader.use();

    // 设置模型矩阵
    auto model = glm::mat4(1.0f);
    // 缩放矩阵，设置缩放倍数
    model = glm::scale(model, glm::vec3(200.0f));
    this->shader.setMat4("model", model);
    // 设置视图矩阵
    // 移除视图矩阵的位移部分，只保留旋转部分
    glm::mat4 view = glm::mat4(glm::mat3(window->getViewMatrix()));
    this->shader.setMat4("view", view);
    // 设置投影矩阵
    this->shader.setMat4("projection", this->window->getProjectionMatrix());

    // 在上下文中绑定VAO
    glBindVertexArray(this->VAO);
    // 激活纹理
    glActiveTexture(GL_TEXTURE0);
    // 绑定纹理
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);
    // 设置uniform变量
    this->shader.setInt("skybox", 0);

    // 绘制
    glDrawArrays(GL_TRIANGLES, 0, 36);
    // 解绑VAO
    glBindVertexArray(0);
    // 将深度测试的比较函数设置回默认值
    glDepthFunc(GL_LESS);
}

// private

/// @brief 加载纹理
/// @param faces 纹理路径
void SkyBox::loadTexture(vector<string> faces) {
    // 生成一个纹理
    glGenTextures(1, &this->textureID);
    // 在上下文中绑定该纹理
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            cout << "Cubemap texture failed to load at path: " << faces[i] << endl;
            stbi_image_free(data);
        }
    }
    // 设置环绕和过滤方式
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void SkyBox::setupVertices() {
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // 生成VAO
    glGenVertexArrays(1, &this->VAO);
    // 生成VBO
    glGenBuffers(1, &this->VBO);
    // 将VAO绑定到当前上下文
    glBindVertexArray(this->VAO);
    // 将VBO绑定到GL_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    // 将顶点数据复制到VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    // 设置顶点属性指针
    // 位置属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}