#ifndef MODEL_H
#define MODEL_H

#include "shader.h"
#include "Mesh.h"
#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

using std::vector;
using std::string;
using std::cout;
using std::endl;

class Model {
public:
    // 已经加载的纹理
    vector<Texture> textures_loaded;
    // 网格数据
    vector<Mesh> meshes;
    // 目录
    string directory;

    // 构造函数
    Model(string const &path) {
        loadModel(path);
    }

    // 绘制函数
    void draw(Shader& shader);

private:

    // 加载模型
    void loadModel(string path);
    // 处理节点
    void processNode(aiNode* node, const aiScene* scene);
    // 处理网格
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    // 加载材质纹理
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
};

#endif // MODEL_H