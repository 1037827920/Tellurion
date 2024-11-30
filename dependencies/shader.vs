#version 330 core
/// 输入
// 顶点坐标
layout(location=0)in vec3 aPos;
// 法线
layout(location=1)in vec3 aNormal;
// 纹理坐标
layout(location=2)in vec2 aTexCoords;
// 切线
layout(location=3)in vec3 aTangent;
// 副切线
layout(location=4)in vec3 aBitangent;

/// 输出
// 法线
out vec3 Normal;
// 纹理坐标
out vec2 TexCoords;
// 片段位置
out vec3 FragPos;
// 切线空间
out mat3 TBN;

/// uniform
// 模型矩阵
uniform mat4 model;
// 视图矩阵
uniform mat4 view;
// 投影矩阵
uniform mat4 projection;

void main()
{
    gl_Position=projection*view*model*vec4(aPos,1.);
    
    Normal=mat3(transpose(inverse(model)))*aNormal;
    FragPos=vec3(model*vec4(aPos,1.f));
    TexCoords=vec2(aTexCoords.x,aTexCoords.y);
    
    // 计算TBN矩阵
    vec3 T=normalize(vec3(model*vec4(aTangent,0.)));
    vec3 B=normalize(vec3(model*vec4(aBitangent,0.)));
    vec3 N=normalize(vec3(model*vec4(aNormal,0.)));
    TBN=mat3(T,B,N);
}