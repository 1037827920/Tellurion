#include "utils/WindowFactory.h"
#include "utils/TellurionModel.h"

int main() {
    // 创建一个窗口Factory对象
    GLFWWindowFactory myWindow(800, 600, "This is Title");
    // 创建一个矩形模型对象
    Tellurion rectangle(&myWindow);

    // 运行窗口，传入一个lambda表达式，用于自定义渲染逻辑
    myWindow.run([&]() {
        // 绘制矩形
        rectangle.draw();
        });

    return 0;
}