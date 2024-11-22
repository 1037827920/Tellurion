#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <vector>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    PITCH_UP,
    PITCH_DOWN,
    ROLL_LEFT,
    ROLL_RIGHT,
    YAW_LEFT,
    YAW_RIGHT
};
struct MovementInfo {
    glm::vec3* direction;
    float velocityFactor;
};
const float SPEED = 12.0f;
const float SENSITIVITY = 0.2f;
const float ZOOM = 45.0f;

const float YAW = 0.0f;
const float PITCH = 0.0f;
const float ROLL = 0.0f;

class Camera {
public:
    glm::vec3 Position;
    glm::quat Orientation;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    std::map<Camera_Movement, MovementInfo> translationMapping;
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);

    // Declare pitch, yaw, and roll as member variables of your camera class
    float pitch = 0.0f; // initial pitch
    float yaw = 0.0f;   // initial yaw
    float roll = 0.0f;  // initial roll

    // Your initial Front, Right, and Up vectors
    glm::vec3 initialFront = glm::vec3(0.0f, 0.0f, -1.0f); // Example initial front direction
    glm::vec3 initialRight = glm::vec3(1.0f, 0.0f, 0.0f); // Example initial right direction
    glm::vec3 initialUp = glm::vec3(0.0f, 1.0f, 0.0f); // Example initial up direction


    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
        Position = position;
        glm::quat qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0));
        glm::quat qYaw = glm::angleAxis(glm::radians(yaw), glm::vec3(0, 1, 0));
        Orientation = qPitch * qYaw;

        translationMapping = {
            {FORWARD, {&Front,1}},
            {BACKWARD, {&Front,-1}},
            {LEFT, {&Right,-1}},
            {RIGHT, {&Right,1}},
            {UP, {&Up,1}},
            {DOWN, {&Up,-1}},
        };
    }

    // 获取视图矩阵
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // 处理键盘输入
    void ProcessKeyboard(Camera_Movement movement, float deltaTime) {
        auto velocity = MovementSpeed * deltaTime;
        if (translationMapping.find(movement) != translationMapping.end()) {
            auto movementInfo = translationMapping[movement];
            auto direction = *movementInfo.direction;
            auto velocityFactor = movementInfo.velocityFactor;

            auto movement = direction * (velocity * velocityFactor);
            Position += movement;
            return;
        }

        if (movement == PITCH_UP) {
            auto rotation = glm::angleAxis(glm::radians(-velocity), Right);
            Front = glm::normalize(Front * rotation);
            Up = glm::normalize(glm::cross(Right, Front));
        }
        if (movement == PITCH_DOWN) {
            auto rotation = glm::angleAxis(glm::radians(velocity), Right);
            Front = glm::normalize(Front * rotation);
            Up = glm::normalize(glm::cross(Right, Front));
        }
        if (movement == ROLL_LEFT) {
            auto rotation = glm::angleAxis(glm::radians(-velocity), Front);
            Up = rotation * Up;
            Right = rotation * Right;
        }
        if (movement == ROLL_RIGHT) {
            auto rotation = glm::angleAxis(glm::radians(velocity), Front);
            Up = rotation * Up;
            Right = rotation * Right;
        }
        if (movement == YAW_LEFT) {
            auto rotation = glm::angleAxis(glm::radians(-velocity), Up);
            Front = glm::normalize(Front * rotation);
            Right = glm::normalize(glm::cross(Front, Up));
        }
        if (movement == YAW_RIGHT) {
            auto rotation = glm::angleAxis(glm::radians(velocity), Up);
            Front = glm::normalize(Front * rotation);
            Right = glm::normalize(glm::cross(Front, Up));
        }

    }

    // 处理鼠标移动
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;


        glm::quat qPitch = glm::angleAxis(glm::radians(yoffset), Right);
        glm::quat qYaw = glm::angleAxis(glm::radians(-xoffset), Up);

        glm::quat orientation = qYaw * qPitch;

        Front = glm::normalize(orientation * Front);
        Up = glm::normalize(orientation * Up);
        Right = glm::normalize(glm::cross(Front, Up));
        // Orientation = glm::normalize(Orientation);

        // updateAxes();
    }

    // 更新轴向
    void updateAxes() {
        Right = glm::vec3(1, 0, 0) * Orientation;
        Front = glm::vec3(0, 0, -1) * Orientation;
        Up = glm::normalize(glm::cross(Right, Front));
    }

    // 处理鼠标滚轮
    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

};

#endif
