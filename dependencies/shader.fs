#version 330 core
/// 输出
// 输出颜色
out vec4 FragColor;

/// 输入
// 纹理坐标
in vec2 TexCoords;
// 法线
in vec3 Normal;
// 片段位置
in vec3 FragPos;
// TBN矩阵
in mat3 TBN;

// 材质结构体
struct Material{
    // 环境光系数
    vec3 ambient;
    // 漫反射系数
    vec3 diffuse;
    // 镜面反射系数
    vec3 specular;
    // 漫反射贴图
    sampler2D diffuseMap;
    // 是否使用法线贴图
    bool sampleNormalMap;
    // 法线贴图（凹凸贴图）
    sampler2D normalMap;
    // 是否使用镜面反射贴图
    bool sampleSpecularMap;
    // 镜面反射贴图
    sampler2D specularMap;
    // 反射光泽度
    float shininess;
    
};
// 材质
uniform Material material0;

uniform vec3 viewPos;
uniform bool blinn;

struct DirLight{
    vec3 direction;
    vec3 lightColor;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight{
    vec3 position;
    vec3 lightColor;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
#define MAX_DIRECTIONAL_LIGHTS 4
uniform int numDirectionalLights;
uniform DirLight directionalLights[MAX_DIRECTIONAL_LIGHTS];// MAX_DIRECTIONAL_LIGHTS is a predefined maximum

#define NR_POINT_LIGHTS 4
uniform int numPointLights;
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir);
vec3 CalcPointLight(PointLight light,vec3 normal,vec3 fragPos,vec3 viewDir);
void main()
{
    vec3 sampledNormal=Normal;
    // 判断是否进行法线贴图
    if(material0.sampleNormalMap){
        // 从法线贴图采样法线
        vec3 normalMap=texture(material0.normalMap,TexCoords).rgb;
        sampledNormal=normalize(normalMap*2.-1.);
        sampledNormal=normalize(TBN*sampledNormal);
    }
    // DEBUG
    // FragColor = vec4(sampledNormal * 0.5 + 0.5, 1.0);
    
    // Use this normal for lighting calculations
    vec3 norm=normalize(sampledNormal);
    vec3 viewDir=normalize(viewPos-FragPos);
    
    // phase 1: Directional lighting
    vec3 result=vec3(0.);
    for(int i=0;i<numDirectionalLights;i++)
    result+=CalcDirLight(directionalLights[i],norm,viewDir);
    
    // phase 2: Point lights
    for(int i=0;i<numPointLights;i++)
    result+=CalcPointLight(pointLights[i],norm,FragPos,viewDir);
    // phase 3: Spot light
    //result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    
    FragColor=vec4(result,1.);
}

vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir)
{
    vec3 lightDir=normalize(-light.direction);
    // diffuse shading
    float diff=max(dot(normal,lightDir),0.);
    // specular shading
    vec3 halfVector=normalize(lightDir+viewDir);
    float spec=pow(max(dot(normal,halfVector),0.),material0.shininess);
    if(!blinn){
        vec3 reflectDir=reflect(-lightDir,normal);
        spec=pow(max(dot(reflectDir,viewDir),0.),material0.shininess);
    }
    // combine results
    vec3 ambient=light.ambient*light.lightColor*vec3(texture(material0.diffuseMap,TexCoords));
    vec3 diffuse=light.diffuse*light.lightColor*diff*vec3(texture(material0.diffuseMap,TexCoords));
    vec3 specular;
    if(material0.sampleSpecularMap)
    specular=light.specular*light.lightColor*spec*vec3(texture(material0.specularMap,TexCoords));
    else
    specular=light.specular*light.lightColor*spec*vec3(texture(material0.diffuseMap,TexCoords));
    
    return(ambient+diffuse+specular);
}

vec3 CalcPointLight(PointLight light,vec3 normal,vec3 fragPos,vec3 viewDir)
{
    vec3 lightDir=normalize(light.position-fragPos);
    // diffuse shading
    float diff=max(dot(normal,lightDir),0.);
    // specular shading
    vec3 halfVector=normalize(lightDir+viewDir);
    float spec=pow(max(dot(normal,halfVector),0.),material0.shininess);
    if(!blinn){
        vec3 reflectDir=reflect(-lightDir,normal);
        spec=pow(max(dot(reflectDir,viewDir),0.),material0.shininess);
    }
    // attenuation
    float distance=length(light.position-fragPos);
    float attenuation=1./(light.constant+light.linear*distance+
        light.quadratic*(distance*distance));
        // combine results
        vec3 ambient=light.ambient*light.lightColor*vec3(texture(material0.diffuseMap,TexCoords));
        vec3 diffuse=light.diffuse*light.lightColor*diff*vec3(texture(material0.diffuseMap,TexCoords));
        vec3 specular;
        if(material0.sampleSpecularMap)
        specular=light.specular*light.lightColor*spec*vec3(texture(material0.specularMap,TexCoords));
        else
        specular=light.specular*light.lightColor*spec*vec3(texture(material0.diffuseMap,TexCoords));
        ambient*=attenuation;
        diffuse*=attenuation;
        specular*=attenuation;
        return(ambient+diffuse+specular);
    }
    