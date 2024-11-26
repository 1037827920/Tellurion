#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/glad.h>
#include "shader.h"
#include "WindowFactory.h"

#include <vector>
#include <string>
#include <iostream>

using std::vector;
using std::string;
using std::cout;
using std::endl;

class SkyBox {
public:
    // 构造函数
    SkyBox(GLFWWindowFactory* window);

    // 绘制天空盒
    void draw();

private:
    // 渲染数据
    unsigned int VAO, VBO;
    // 纹理ID
    unsigned int textureID;
    // 窗口指针
    GLFWWindowFactory* window;
    // 着色器
    Shader shader;

    // 加载纹理
    void loadTexture(vector<string> faces);
    // 初始化渲染数据
    void setupVertices();
};

#endif