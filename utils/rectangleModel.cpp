
#include "rectangleModel.h"
#include <iostream>
#include "yaml-cpp/yaml.h"

RectangleModel::RectangleModel(GLFWWindowFactory* window) :window(window) {
    directionalLights = loadDirectionalLights("config/directionalLights.yaml");
    pointLights = loadPointLights("config/pointLights.yaml");
    modelInfos = loadScene("config/scene.yaml");

    for (auto& modelInfo : modelInfos) {
        modelInfo.model = new Model(modelInfo.path);
    }
    shader = Shader("shader.vs", "shader.fs");


}

RectangleModel::~RectangleModel() {
}

void RectangleModel::draw() {
    shader.use();
    // pass projection matrix to shader (note that in this case it could change every frame)

    shader.setInt("numDirectionalLights", directionalLights.size());
    for (auto i = 0; i < directionalLights.size(); i++) {
        std::string number = std::to_string(i);
        shader.setVec3("directionalLights[" + number + "].direction", directionalLights[i].direction);
        shader.setVec3("directionalLights[" + number + "].ambient", directionalLights[i].ambient);
        shader.setVec3("directionalLights[" + number + "].diffuse", directionalLights[i].diffuse);
        shader.setVec3("directionalLights[" + number + "].specular", directionalLights[i].specular);
        shader.setVec3("directionalLights[" + number + "].lightColor", directionalLights[i].lightColor);
    }

    shader.setInt("numPointLights", pointLights.size());
    for (auto i = 0; i < pointLights.size(); i++) {
        std::string number = std::to_string(i);
        shader.setVec3("pointLights[" + number + "].position", pointLights[i].position);
        shader.setVec3("pointLights[" + number + "].ambient", pointLights[i].ambient);
        shader.setVec3("pointLights[" + number + "].diffuse", pointLights[i].diffuse);
        shader.setVec3("pointLights[" + number + "].specular", pointLights[i].specular);
        shader.setFloat("pointLights[" + number + "].constant", pointLights[i].constant);
        shader.setFloat("pointLights[" + number + "].linear", pointLights[i].linear);
        shader.setFloat("pointLights[" + number + "].quadratic", pointLights[i].quadratic);
        shader.setVec3("pointLights[" + number + "].lightColor", pointLights[i].lightColor);
    }



    for (const auto& modelInfo : modelInfos) {
        float currentTime = glfwGetTime();  // Get current time in seconds
        float angle = currentTime * 5.0f;  // Calculate angle based on time, 50.0f is the speed factor

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, modelInfo.position);

        // Static rotations
        model = glm::rotate(model, glm::radians(modelInfo.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(modelInfo.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(modelInfo.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        // Dynamic rotation (e.g., around y-axis)
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

        model = glm::scale(model, modelInfo.scale);

        shader.setMat4("model", model);
        modelInfo.model->draw(shader);
    }

    shader.setMat4("projection", window->getProjectionMatrix());
    shader.setMat4("view", window->getViewMatrix());
    auto camera = window->camera;
    shader.setVec3("viewPos", camera.Position);

    //set blinn to !blinn when key 1 is hit
    if (window->blinn) {
        shader.setInt("blinn", 1);
    }
    else {
        shader.setInt("blinn", 0);
    }
}


std::vector<RectangleModel::ModelInfo> RectangleModel::loadScene(const std::string& fileName) {
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

std::vector<RectangleModel::DirectionalLight> RectangleModel::loadDirectionalLights(const std::string& fileName) {
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
                directionalLights.push_back(light);
            }
        }
    } catch (const YAML::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return directionalLights;
}

std::vector<RectangleModel::PointLight> RectangleModel::loadPointLights(const std::string& fileName) {
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