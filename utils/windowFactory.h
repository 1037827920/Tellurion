#ifndef WINDOW_FACTORY_H
#define WINDOW_FACTORY_H
#include <glad/glad.h> // gald前面不能包含任何opengl头文件
#include <GLFW/glfw3.h>
#include <functional>
#include <iostream>
#include "QuaternionCamera.h"

using std::cout;
using std::endl;

class GLFWWindowFactory {
public:
    static bool blinn;
    static bool blinnKeyPressed; // 添加一个标志位
    // 默认构造函数
    GLFWWindowFactory() {}
    // 构造函数，初始化窗口
    GLFWWindowFactory(int width, int height, const char* title) {
        // 初始化glfw
        glfwInit();
        // 设置opengl版本
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        // 使用核心模式：确保不使用任何被弃用的功能
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // 创建glfw窗口
        this->window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (this->window == NULL) {
            cout << "Failed to create GLFW window" << endl;
            glfwTerminate();
            exit(-1);
        }
        // 设置当前窗口的上下文
        glfwMakeContextCurrent(this->window);
        // 设置窗口大小改变的回调函数
        glfwSetFramebufferSizeCallback(this->window, framebuffer_size_callback);
        // 加载所有opengl函数指针
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            cout << "Failed to initialize GLAD" << endl;
        }
        // 再次设置当前窗口的上下文，确保当前上下文仍然是刚刚创建的窗口，是一个安全措施
        glfwMakeContextCurrent(this->window);
        // 设置窗口大小改变的回调函数
        glfwSetFramebufferSizeCallback(this->window, framebuffer_size_callback);
        // 设置鼠标移动的回调函数
        glfwSetCursorPosCallback(this->window, mouse_callback);
        // 设置鼠标滚轮滚动的回调函数
        glfwSetScrollCallback(this->window, scroll_callback);
        // 设置鼠标按钮的回调函数
        glfwSetMouseButtonCallback(this->window, mouse_button_callback);
        // 默认不捕获鼠标
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    }

    // 获取窗口对象
    GLFWwindow* getWindow() {
        return this->window;
    }

    // 运行窗口，传入一个自定义的更新函数
    void run(std::function<void()> updateFunc) {
        // 启用深度测试，opengl将在绘制每个像素之前比较其深度值，以确定该像素是否应该被绘制
        glEnable(GL_DEPTH_TEST);

        // 循环渲染
        while (!glfwWindowShouldClose(this->window)) { // 检查是否应该关闭窗口
            // 清空屏幕所用的颜色
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            // 清空颜色缓冲，主要目的是为每一帧的渲染准备一个干净的画布
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            float currentFrame = glfwGetTime();
            this->deltaTime = currentFrame - this->lastFrame;
            this->lastFrame = currentFrame;
            this->timeElapsed += this->deltaTime;
            this->frameCount++;

            // 处理输入
            GLFWWindowFactory::process_input(this->window);

            // 初始化投影矩阵和视图矩阵
            this->projection =
                glm::perspective(glm::radians(camera.Zoom),
                    (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
            this->view = this->camera.GetViewMatrix();

            // 执行更新函数
            updateFunc();

            // 交换缓冲区
            glfwSwapBuffers(this->window);
            // 处理所有待处理事件，去poll所有事件，看看哪个没处理的
            glfwPollEvents();
        }

        // 终止GLFW，清理GLFW分配的资源
        glfwTerminate();
    }

    // 窗口大小改变的回调函数
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        // 确保视口与新窗口尺寸匹配，注意在视网膜显示器上，宽度和高度会显著大于指定值
        glViewport(0, 0, width, height);
    }

    // 鼠标移动的回调函数
    static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        // 反转y坐标，因为y坐标的范围是从上到下，我们需要从下到上
        float yoffset = lastY - ypos;

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }

    // 鼠标滚轮的回调函数
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
        camera.ProcessMouseScroll(static_cast<float>(yoffset));
    }

    // 鼠标按钮的回调函数
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            // 切换鼠标模式
            int cursorMode = glfwGetInputMode(window, GLFW_CURSOR);
            if (cursorMode == GLFW_CURSOR_NORMAL) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                // 重置第一次鼠标移动的标志
                firstMouse = true;
            }
        }
    }

    // 处理输入
    static void process_input(GLFWwindow* window) {
        // 按下ESC键时进入if块
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            // 关闭窗口
            glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
            camera.ProcessKeyboard(PITCH_UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            camera.ProcessKeyboard(PITCH_DOWN, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            camera.ProcessKeyboard(YAW_LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            camera.ProcessKeyboard(YAW_RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
            camera.ProcessKeyboard(ROLL_LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
            camera.ProcessKeyboard(ROLL_RIGHT, deltaTime);

        // 当按下键1时，切换Blinn-Phong着色模式
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            if (!blinnKeyPressed) {
                blinn = !blinn;
                blinnKeyPressed = true;
            }
        }
        else {
            blinnKeyPressed = false;
        }
    }

    // 获取投影矩阵
    const glm::mat4 getProjectionMatrix() {
        return this->projection;
    }

    // 获取视图矩阵
    glm::mat4 getViewMatrix() {
        return this->view;
    }

public:
    // 投影矩阵
    glm::mat4 projection;
    // 视图矩阵
    glm::mat4 view;
    // 摄像机
    static Camera camera;
    // 屏幕宽度
    static const unsigned int SCR_WIDTH = 800;
    // 屏幕高度
    static const unsigned int SCR_HEIGHT = 600;
    // 窗口对象
    GLFWwindow* window;
private:

    // 经过的时间
    float timeElapsed;
    // 帧计数
    int frameCount;

    // 上一次鼠标的X坐标
    static float lastX;
    // 上一次鼠标的Y坐标
    static float lastY;
    // 是否第一次鼠标移动
    static bool firstMouse;

    // 时间间隔
    static float deltaTime;
    // 上一帧的时间
    static float lastFrame;
};

#endif