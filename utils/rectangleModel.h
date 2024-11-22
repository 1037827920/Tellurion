#pragma once

#include "windowFactory.h"
#include <glad/glad.h>
#include "shader.h"
#include <vector>
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "objModel.h"

class RectangleModel
{ 
    struct DirectionalLight{
        glm::vec3 direction;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 lightColor; 
    };
    struct ModelInfo {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        std::string path;
        Model* model = nullptr; 
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
public:
    RectangleModel( GLFWWindowFactory* window);

    void draw();
    ~RectangleModel();
    std::vector<ModelInfo> loadScene(const std::string& fileName);
    std::vector<DirectionalLight> loadDirectionalLights(const std::string& fileName);
    std::vector<PointLight> loadPointLights(const std::string& fileName);

private:

    Shader shader;
    Shader lightShader;
    std::vector<unsigned int> texture;
    GLFWWindowFactory* window;


    std::vector<ModelInfo> modelInfos;
    std::vector<DirectionalLight> directionalLights;
    std::vector<PointLight> pointLights;

};
