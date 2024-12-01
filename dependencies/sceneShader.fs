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
// 片段位置的光空间
// in vec4 FragPosLightSpace;

/// uniform
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
    // 光空间矩阵
    mat4 lightSpaceMatrix;
    // 阴影贴图
    sampler2D shadowMap;
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
// 定向光数量
uniform int numDirectionalLights;
// 定向光数组
uniform DirLight directionalLights[MAX_DIRECTIONAL_LIGHTS];

#define NR_POINT_LIGHTS 4
uniform int numPointLights;
uniform PointLight pointLights[NR_POINT_LIGHTS];

// 计算定向光贡献
vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir);
// 计算点光源贡献
vec3 CalcPointLight(PointLight light,vec3 normal,vec3 fragPos,vec3 viewDir);
// 存储了从光源视角看当前fragment位置的深度值，这个深度值是从阴影贴图中采样得到的，用于判断当前fragment是否在阴影中
float closestDepth;
// 存储了从摄像机是将看当前fragment位置的深度值，这个深度值计算是在摄像机移动时计算的
float currentDepth;
// 计算阴影
float ShadowCalculation(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D shadowMap);

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
    
    // 1. 计算所有方向光的贡献
    vec3 result=vec3(0.);
    for(int i=0;i<numDirectionalLights;i++)
    result+=CalcDirLight(directionalLights[i],norm,viewDir);
    
    // 2. 计算所有点光源的贡献
    // for(int i=0;i<numPointLights;i++)
    // result+=CalcPointLight(pointLights[i],norm,FragPos,viewDir);
    // 3. Spot light
    //result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    
    FragColor=vec4(result,1.);
    
    // DEBUG：测试阴影贴图
    // float temp=ShadowCalculation(FragPosLightSpace);
    // FragColor=vec4(vec3(1.-temp),1.);
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
    
    // 计算阴影
    vec4 FragPosLightSpace=light.lightSpaceMatrix*vec4(FragPos,1.);
    float shadow=ShadowCalculation(FragPosLightSpace,normal,lightDir,light.shadowMap);
    return(ambient+(1.-shadow)*(diffuse+specular));
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
    float attenuation=1./(light.constant+light.linear*distance+light.quadratic*(distance*distance));
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

float ShadowCalculation(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D shadowMap)
{
    // perform perspective divide
    vec3 projCoords=fragPosLightSpace.xyz/fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords=projCoords*.5+.5;
    // 只要投影向量的z坐标大于1.0或小于0.0，就把shadow设置为1.0(即超出了光源视锥体的最远处，这样最远处也不会处在阴影中，导致采样过多不真实)
    if(projCoords.z>1.||projCoords.z<0.)
    return 0.;
    
    // 从光源视角看到的深度值（从阴影贴图获取
    closestDepth=texture(shadowMap,projCoords.xy).r;
    // 从摄像机视角看到的深度值
    currentDepth=projCoords.z;
    // 偏移量，解决阴影失真的问题, 根据表面朝向光线的角度更改偏移量
    float bias=max(.05*(1.-dot(normal,lightDir)),.005);
    // 如果currentDepth大于closetDepth，说明当前fragment被某个物体遮挡住了，在阴影之中
    float shadow=currentDepth-bias>closestDepth?1.:0.;
    
    return shadow;
}