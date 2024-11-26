#include "objModel.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// 绘制模型
void Model::draw(Shader& shader) {
    // 使用着色器
    shader.use();

    // 设置uniform变量
    shader.setFloat("material.shininess", material.shininess);
    shader.setVec3("material.ambient", material.ambient);
    shader.setVec3("material.diffuse", material.diffuse);
    shader.setVec3("material.specular", material.specular);

    // 设置纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material.texture);
    shader.setInt("material.texture", 0);

    // 设置法线贴图
    if (material.normalMap != 0) {
        shader.setBool("material.sampleNormalMap", true);
        // 实现法线贴图
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, material.normalMap);
        shader.setInt("material.normalMap", 1);
    }
    else {
        shader.setBool("material.sampleNormalMap", false);
    }

    // 设置specularMap
    if (material.specularMap != 0) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, material.specularMap);
        shader.setInt("material.specularMap", 2);
    }

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

void Model::loadMaterial(const std::string& mtlPath, Material& material) {
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << mtlPath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "newmtl") {
            ss >> material.name;
        }
        else if (token == "Ka") {
            ss >> material.ambient.r >> material.ambient.g >> material.ambient.b;
        }
        else if (token == "Kd") {
            ss >> material.diffuse.r >> material.diffuse.g >> material.diffuse.b;
        }
        else if (token == "Ks") {
            ss >> material.specular.r >> material.specular.g >> material.specular.b;
        }
        else if (token == "map_Kd") {
            std::string texturePath;
            ss >> texturePath;

            texturePath = folderPath + "/" + texturePath;
            material.texture = loadTexture(texturePath);
        }
        else if (token == "map_Ks") {
            std::string texturePath;
            ss >> texturePath;

            texturePath = folderPath + "/" + texturePath;
            material.specularMap = loadTexture(texturePath);
        }
        else if (token == "map_Bump") { // 法线贴图
            std::string texturePath;
            ss >> texturePath;

            texturePath = folderPath + "/" + texturePath;
            material.normalMap = loadTexture(texturePath);
        }
    }

    file.close();
}

GLuint Model::loadTexture(const std::string& texturePath) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 4)
            format = GL_RGBA;
        else if (nrChannels == 3)
            format = GL_RGB;
        else
            format = GL_RED;  // or handle other formats as needed

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);  // Use format variable here

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        return texture;
    }
    else {
        std::cout << "Texture failed to load at path: " << texturePath << std::endl;
        stbi_image_free(data);
        return 0;
    }
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

