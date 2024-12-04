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
// 定义顶点结构体，包含位置和纹理坐标
typedef struct {
    float p[3]; // 位置
    float t[2]; // 纹理坐标
} vertex_t;

class Model {
public:
    // 已经加载的纹理
    vector<Texture> textures_loaded;
    // 网格数据
    vector<Mesh> meshes;
    // 目录
    string directory;

    // 构造函数
    Model(string const& path, vector<vertex_t>& lightVertices, vector<unsigned int>& lightIndices) {
        loadModel(path, lightVertices, lightIndices);
    }

    // 绘制函数
    void draw(Shader& shader, vector<unsigned int> directionLightDepthMaps, bool isActiveTexture, vector<unsigned int> d_d2_filter_maps, bool is_d_d2, bool isLightMap, unsigned int lightMap);

private:

    // 加载模型
    void loadModel(string path, vector<vertex_t>& lightVertices, vector<unsigned int>& lightIndices);
    // 处理节点
    void processNode(aiNode* node, const aiScene* scene, vector<vertex_t>& lightVertices, vector<unsigned int>& lightIndices);
    // 处理网格
    Mesh processMesh(aiMesh* mesh, const aiScene* scene, vector<vertex_t>& lightVertices, vector<unsigned int>& lightIndices);
    // 加载材质纹理
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
};

#endif // MODEL_H