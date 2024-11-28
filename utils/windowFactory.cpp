#include "WindowFactory.h"

Camera GLFWWindowFactory::camera = Camera(glm::vec3(0.0f, 0.0f, 25.0f));

// 初始化鼠标的最后X位置为屏幕宽度的一半
float GLFWWindowFactory::lastX = GLFWWindowFactory::SCR_WIDTH / 2.0f;
// 初始化鼠标的最后Y位置为屏幕高度的一半
float GLFWWindowFactory::lastY = GLFWWindowFactory::SCR_HEIGHT / 2.0f;
// 标记是否为第一次鼠标输入
bool GLFWWindowFactory::firstMouse = true;
// 初始化帧间隔时间
float GLFWWindowFactory::deltaTime = 0.0f;
// 初始化上一帧的时间
float GLFWWindowFactory::lastFrame = 0.0f;
bool GLFWWindowFactory::blinn = false;
bool GLFWWindowFactory::blinnKeyPressed = false;