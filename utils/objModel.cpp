#include "objModel.h"

// 绘制模型
void Model::draw(Shader& shader, unsigned int directionLightDepthMap) {
    // 使用着色器
    shader.use();

    // 绑定VAO
    glBindVertexArray(VAO);

    // 绘制模型
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    // 解绑VAO
    glBindVertexArray(0);
}

// 加载模型
void Model::loadModel(string const& path) {
    // 文件读取
    ifstream file(path);
    if (!file.is_open()) {
        cout << "Error: File Not Found" << endl;
        return;
    }

    string line;
    // 临时存储顶点、法线、纹理坐标
    vector<glm::vec3> tempPositions;
    vector<glm::vec3> tempNormals;
    vector<glm::vec2> tempTexCoords;

    // 逐行读取文件
    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        ss >> token;

        if (token == "v") { // 顶点位置
            glm::vec3 position;
            ss >> position.x >> position.y >> position.z;
            tempPositions.push_back(position);
        }
        else if (token == "vt") { // 纹理坐标
            glm::vec2 texCoord;
            ss >> texCoord.x >> texCoord.y;
            texCoord.y = 1.0f - texCoord.y; // 翻转纹理坐标的y轴
            tempTexCoords.push_back(texCoord);
        }
        else if (token == "vn") { // 法线
            glm::vec3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            tempNormals.push_back(normal);
        }
        else if (token == "f") { // 面
            auto faceTokens = split(line, ' ');
            if (faceTokens.size() >= 4) {
                // 使用第一个顶点和其他顶点组成三角形
                for (size_t i = 2; i < faceTokens.size() - 1; ++i) {
                    processFaceVertex(faceTokens[1], tempPositions, tempTexCoords, tempNormals, vertices);
                    processFaceVertex(faceTokens[i], tempPositions, tempTexCoords, tempNormals, vertices);
                    processFaceVertex(faceTokens[i + 1], tempPositions, tempTexCoords, tempNormals, vertices);
                }
            }
        }
    }
    // 关闭文件
    file.close();
}

void Model::setupBuffer() {
    // 生成VAO
    glGenVertexArrays(1, &VAO);
    // 生成VBO
    glGenBuffers(1, &VBO);
    // 将VAO绑定到当前上下文
    glBindVertexArray(VAO);
    // 将VBO绑定到当前上下文
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // 将顶点数据复制到VBO
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    // 设置顶点属性指针
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);
    // 解绑VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // 解绑VAO
    glBindVertexArray(0);
}

vector<string> Model::split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void Model::processFaceVertex(const string& token,
    const vector<glm::vec3>& tempPositions,
    const vector<glm::vec2>& tempTexCoords,
    const vector<glm::vec3>& tempNormals,
    vector<Vertex>& vertices) {
    // 分割token
    vector<string> subTokens = split(token, '/');
    // 将子token转换为索引，减1是因为索引从1开始，而c++中索引从0开始
    int posIndex = stoi(subTokens[0]) - 1;
    int texIndex = stoi(subTokens[1]) - 1;
    int normIndex = stoi(subTokens[2]) - 1;

    // 创建顶点
    Vertex vertex;
    vertex.Position = tempPositions[posIndex];
    vertex.TexCoords = tempTexCoords[texIndex];
    vertex.Normal = tempNormals[normIndex];
    vertices.push_back(vertex);
}

