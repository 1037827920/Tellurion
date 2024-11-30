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
    void draw(Shader& shader) {
        // 绑定纹理
        unsigned int diffuseNr = 0;
        unsigned int specularNr = 0;
        unsigned int normalNr = 0;

        for (unsigned int i = 0; i < textures.size(); i++) {
            // 激活纹理单元
            glActiveTexture(GL_TEXTURE0 + i);
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
            glBindTexture(GL_TEXTURE_2D, textures[i].id);

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

        // 恢复默认纹理单元
        glActiveTexture(GL_TEXTURE0);

        // 绘制网格
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
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
        // 法线
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // 纹理坐标
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
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