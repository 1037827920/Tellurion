#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <fstream>  
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include "shader.h"

using std::string;
using std::vector;
using std::ifstream;
using std::cerr;
using std::cout;
using std::endl;
using std::getline;
using std::stringstream;
using std::istringstream;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    string type;
    string path;
};

class Model {
public:
    struct face {
        int vertexIndex[3];
        int textureIndex[3];
        int normalIndex[3];
    };
    vector<Texture> textures_loaded;
    vector<Vertex> vertices;
    vector<face> faces;

    string directory;

    Model(string const& path) {
        // 加载模型
        loadModel(path);
        // 设置顶点缓冲区
        setupBuffer();
    }

    // 绘制模型
    void draw(Shader& shader, unsigned int directionLightDepthMap);

private:
    unsigned int VAO, VBO, EBO;

    // 从文件中加载OBJ模型，并将结果网格存储在网格向量中
    void loadModel(string const& path);
    void setupBuffer();

    // 辅助函数，分割字符串
    vector<string> split(const string& s, char delimiter);
    // 处理从.obj文件中读取的面数据，并将其转换为顶点数据存储在vertices容器中
    void processFaceVertex(const string& token,
        const vector<glm::vec3>& tempPositions,
        const vector<glm::vec2>& tempTexCoords,
        const vector<glm::vec3>& tempNormals,
        vector<Vertex>& vertices);
};

#endif // MODEL_H