#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "shader.h"

using std::string;
using std::vector;

// 顶点数据
struct Vertex {
    // 顶点位置
    glm::vec3 Position;
    // 法线
    glm::vec3 Normal;
    // 纹理坐标
    glm::vec2 TexCoords;
    // 切线
    glm::vec3 Tangent;
    // 副切线
    glm::vec3 Bitangent;
};

// 纹理
struct Texture {
    // 纹理ID
    unsigned int id;
    // 纹理类型
    std::string type;
    // 纹理文件路径
    std::string path;
    // 纹理环境光系数
    glm::vec3 ambient;
    // 纹理漫反射系数
    glm::vec3 diffuse;
    // 纹理镜面反射系数
    glm::vec3 specular;
    // 纹理高光系数
    float shininess;
};

// 网格
class Mesh {
public:
    // 网格数据
    // 顶点数据
    vector<Vertex> vertices;
    // 索引数据
    vector<unsigned int> indices;
    // 纹理数据
    vector<Texture> textures;

    // 构造函数
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures) {
        // 设置数据
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        setupMesh();
    }

    // 绘制函数
    void draw(Shader& shader, vector<unsigned int> directionLightDepthMaps, bool isActiveTexture, vector<unsigned int> d_d2_filter_maps, bool is_d_d2, bool isLightMap, unsigned int lightMap) {
        // 是否激活纹理
        if (isActiveTexture) {
            unsigned int diffuseNr = 0;
            unsigned int specularNr = 0;
            unsigned int normalNr = 0;
            unsigned int i = 0;
            for (; i < textures.size(); i++) {
                // 激活纹理单元
                glActiveTexture(GL_TEXTURE0 + i);
                // 绑定纹理单元
                glBindTexture(GL_TEXTURE_2D, textures[i].id);

                /// 将纹理传递给着色器
                // 获取纹理序号
                string number;
                string name = textures[i].type;
                string shaderUniformName;
                if (name == "texture_diffuse") {
                    number = std::to_string(diffuseNr++);
                    shaderUniformName = "material" + number + ".diffuseMap";
                }
                else if (name == "texture_specular") {
                    number = std::to_string(specularNr++);
                    shaderUniformName = "material" + number + ".specularMap";
                }
                else if (name == "texture_normal") {
                    number = std::to_string(normalNr++);
                    shaderUniformName = "material" + number + ".normalMap";
                }

                // 将纹理传递给着色器
                shader.setInt(shaderUniformName.c_str(), i);

                // 传递环境光系数给着色器
                shader.setVec3("material" + number + ".ambient", textures[i].ambient);
                // 传递漫反射系数给着色器
                shader.setVec3("material" + number + ".diffuse", textures[i].diffuse);
                // 传递镜面反射系数给着色器
                shader.setVec3("material" + number + ".specular", textures[i].specular);
                // 传递高光系数给着色器
                shader.setFloat("material" + number + ".shininess", textures[i].shininess);
                // 设置采用法线贴图
                shader.setBool("material" + number + ".sampleNormalMap", name == "texture_normal");
                // 设置采用镜面光贴图
                shader.setBool("material" + number + ".sampleSpecularMap", name == "texture_specular");
            }

            int j = 0;
            if (!is_d_d2) {
                // 设置定向光深度贴图
                for (; j < directionLightDepthMaps.size(); j++) {
                    glActiveTexture(GL_TEXTURE0 + i + j);
                    glBindTexture(GL_TEXTURE_2D, directionLightDepthMaps[j]);
                    shader.setInt("directionalLights[" + std::to_string(j) + "].shadowMap", i + j);
                }
            }
            else {
                // 设置定向光均值和方差贴图
                for (; j * 2 + 1 < d_d2_filter_maps.size(); j++) {
                    glActiveTexture(GL_TEXTURE0 + i + j);
                    glBindTexture(GL_TEXTURE_2D, d_d2_filter_maps[j * 2 + 1]);
                    shader.setInt("directionalLights[" + std::to_string(j) + "].d_d2_filter", i + j);
                }
            }

            if (isLightMap) {
                // 设置光照贴图
                glActiveTexture(GL_TEXTURE0 + i + j);
                glBindTexture(GL_TEXTURE_2D, lightMap);
                shader.setInt("lightMap", i + j);
            }
        }

        // 绘制网格
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // 恢复默认纹理单元
        glActiveTexture(GL_TEXTURE0);
        // 解绑VAO
        glBindVertexArray(0);
    }

private:
    // 渲染数据
    unsigned int VAO, VBO, EBO;

    // 初始化渲染数据
    void setupMesh() {
        // 生成VAO，VBO，EBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // 绑定VAO
        glBindVertexArray(VAO);
        // 绑定VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // 将顶点数据复制到VBO
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // 绑定EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // 将索引数据复制到EBO
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // 顶点位置
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // 纹理坐标
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // 法线
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // 切线
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // 副切线
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

        // 解绑VAO
        glBindVertexArray(0);
    }
};

#endif // MESH_H