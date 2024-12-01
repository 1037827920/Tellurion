#include "utils/WindowFactory.h"
#include "utils/Scene.h"
#include "utils/SkyBox.h"

int main() {
    // 创建一个窗口Factory对象
    GLFWWindowFactory myWindow(800, 600, "地球仪");
    // 创建一个地球仪模型对象
    Scene tellurion(&myWindow);
    // 创建一个天空盒对象
    SkyBox skyBox(&myWindow);

    // 运行窗口，传入一个lambda表达式，用于自定义渲染逻辑
    myWindow.run([&]() {
        // 绘制地球仪
        tellurion.draw();
        // 绘制天空盒
        skyBox.draw();
        });
    return 0;
}