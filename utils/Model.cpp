#include "Model.h"
// #define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void Model::draw(Shader& shader, vector<unsigned int> directionLightDepthMaps, bool isActiveTexture, vector<unsigned int> d_d2_filter_maps, bool is_d_d2, bool isLightMap, unsigned int lightMap) {
    // 遍历所有网格，并调用它们各自的draw函数
    for (unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i].draw(shader, directionLightDepthMaps, isActiveTexture, d_d2_filter_maps, is_d_d2, isLightMap, lightMap);
    }
}

void Model::loadModel(string path, vector<vertex_t>& lightVertices, vector<unsigned int>& lightIndices) {
    // 读取文件，将模型数据存储在scene中
    Assimp::Importer importer;
    // 预处理参数
    // - aiProcess_Triangulate：如果模型不是三角形，则将其转换为三角形
    // - aiProcess_FlipUVs：翻转纹理坐标的y轴（opengl中大部分的图像的y轴都是反的）
    // - aiProcess_CalcTangentSpace：计算切线和副切线
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    // 检查是否导入成功
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
        return;
    }

    // 获取模型文件所在的目录
    this->directory = path.substr(0, path.find_last_of('/'));

    // 递归处理场景中的每个节点
    // 每个节点包含了一系列的网格索引
    // 每个索引指向场景对象中的那个特定网格
    this->processNode(scene->mRootNode, scene, lightVertices, lightIndices);
}

void Model::processNode(aiNode* node, const aiScene* scene, vector<vertex_t>& lightVertices, vector<unsigned int>& lightIndices) {
    // 处理节点的所有网格(如果有的话)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* meshes = scene->mMeshes[node->mMeshes[i]];
        this->meshes.push_back(this->processMesh(meshes, scene, lightVertices, lightIndices));
    }

    // 对它的子节点重复这一过程
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        this->processNode(node->mChildren[i], scene, lightVertices, lightIndices);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, vector<vertex_t>& lightVertices, vector<unsigned int>& lightIndices) {
    // 顶点数据
    vector<Vertex> vertices;
    // 索引数据
    vector<unsigned int> indices;
    // 纹理数据
    vector<Texture> textures;

    // 遍历网格的所有顶点，取出位置、法线、纹理坐标
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // 处理网格的顶点
        Vertex vertex;
        glm::vec3 vector;
        vertex_t lightVertex;
        // 顶点位置
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        lightVertex.p[0] = vector.x;
        lightVertex.p[1] = vector.y;
        lightVertex.p[2] = vector.z;
        // 顶点法线
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        // 纹理坐标
        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            lightVertex.t[0] = vec.x;
            lightVertex.t[1] = vec.y;
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            lightVertex.t[0] = 0.0f;
            lightVertex.t[1] = 0.0f;
        }
        // 切线
        vector.x = mesh->mTangents[i].x;
        vector.y = mesh->mTangents[i].y;
        vector.z = mesh->mTangents[i].z;
        // DEBUG    
        // cout << "tangent: " << vector.x << " " << vector.y << " " << vector.z << endl;
        vertex.Tangent = vector;
        // 副切线
        vector.x = mesh->mBitangents[i].x;
        vector.y = mesh->mBitangents[i].y;
        vector.z = mesh->mBitangents[i].z;
        // DEBUG
        // cout << "bitangent: " << vector.x << " " << vector.y << " " << vector.z << endl;
        vertex.Bitangent = vector;

        vertices.push_back(vertex);
        lightVertices.push_back(lightVertex);
    }

    // 处理网格的索引(服了，一开始把这步操作写在处理顶点的循环里面了，怪不得导入某些模型内存oom了)
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
            lightIndices.push_back(face.mIndices[j]);
        }
    }

    // 处理网格的材质
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 1. 处理漫反射贴图
        std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. 处理镜面光贴图
        std::vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. 处理法线贴图
        std::vector<Texture> normalMaps = this->loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    }
    return Mesh(vertices, indices, textures);
}


unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);
unsigned int TextureFromFile(const char* path, const string& directory, bool gamma) {
    std::filesystem::path dirPath(directory);
    std::filesystem::path filePath(path);

    string fullPath = (dirPath / filePath).string();
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    std::cout << fullPath << std::endl;
    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 4)
            format = GL_RGBA;
        else if (nrComponents == 3)
            format = GL_RGB;
        else
            format = GL_RED;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName) {
    vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        // 从aiMaterial中获取纹理
        mat->GetTexture(type, i, &str);
        // 用来检查纹理之前是否已经加载过了
        bool skip = false;
        for (unsigned int j = 0; j < this->textures_loaded.size(); j++) {
            if (std::strcmp(this->textures_loaded[j].path.data(), str.C_Str()) == 0) {
                textures.push_back(this->textures_loaded[j]);
                skip = true;
                break;
            }
        }

        // 如果纹理之前没有加载过，加载它
        if (!skip) {
            Texture texture;
            aiColor3D color(0.f, 0.f, 0.f);
            // 从aiMaterial中获取环境光系数Ka
            mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
            texture.ambient = glm::vec3(color.r, color.g, color.b);
            // 从aiMaterial中获取漫反射系数Kd
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            texture.diffuse = glm::vec3(color.r, color.g, color.b);
            // 从aiMaterial中获取镜面反射系数Ks
            mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
            // DEBUG
            // cout << "new ambient: " << color.r << " " << color.g << " " << color.b << endl;
            // cout << "new diffuse: " << color.r << " " << color.g << " " << color.b << endl;
            // cout << "new specular: " << color.r << " " << color.g << " " << color.b << endl;
            texture.specular = glm::vec3(color.r, color.g, color.b);
            // 自定义高光系数Ns
            texture.shininess = 108.0f;

            // 从aiMaterial中获取纹理
            texture.id = TextureFromFile(str.C_Str(), this->directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            this->textures_loaded.push_back(texture);
        }
    }

    return textures;
}