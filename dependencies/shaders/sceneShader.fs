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
// 阴影计算算法选择
uniform int shadowMapType;

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
    // 阴影方差与均值贴图
    sampler2D d_d2_filter;
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
// 光源宽度
uniform float lightWidth;
// PCF采样半径
uniform float PCFSampleRadius;
// 近裁剪面
uniform float near_plane;
// 远裁剪面
uniform float far_plane;

#define NR_POINT_LIGHTS 4
uniform int numPointLights;
uniform PointLight pointLights[NR_POINT_LIGHTS];

uniform bool useLightMap;
uniform sampler2D lightMap;

// 存储了从光源视角看当前fragment位置的深度值，这个深度值是从阴影贴图中采样得到的，用于判断当前fragment是否在阴影中
float closestDepth;
// 存储了从摄像机是将看当前fragment位置的深度值，这个深度值计算是在摄像机移动时计算的
float currentDepth;
// PCF采样邻域大小
#define PCF_RADIUS 6
// 块半径
#define BLOCK_RADIUS 5

// 计算定向光贡献
vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir);
// 计算点光源贡献
vec3 CalcPointLight(PointLight light,vec3 normal,vec3 fragPos,vec3 viewDir);
// 使用SM计算阴影
float SM(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D shadowMap);
// 使用PCF计算阴影
float PCF(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D shadowMap);
// 使用PCSS计算阴影
float PCSS(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D shadowMap);
// 找到阴影贴图中遮挡当前片段的遮挡者，并计算遮挡者的平均深度值（阴影软化效果
// uv: 当前片段在阴影贴图中的纹理坐标
// zReceiver: 当前片段在光源视角看到的深度值
// shadowMap: 阴影贴图
// bias: 阴影偏移量
float findBlocker(vec2 uv,float zReceiver,sampler2D shadowMap,float bias);
// 使用VSM计算阴影
float VSM(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D d_d2_filter);

vec2 d_d2;
float depth;

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
    
    if (useLightMap)
    {
        FragColor = vec4(texture(lightMap, TexCoords).rgb, gl_FrontFacing ? 1.0 : 0.0);
        return;
    }

    // 计算所有方向光的贡献
    vec3 result=vec3(0.);
    for(int i=0;i<numDirectionalLights;i++)
    result+=CalcDirLight(directionalLights[i],norm,viewDir);
    
    FragColor=vec4(result,1.);
    
    // DEBUG：测试阴影贴图
    // vec4 FragPosLightSpace=directionalLights[0].lightSpaceMatrix*vec4(FragPos,1.);
    // float temp=VSM(FragPosLightSpace, norm, viewDir, directionalLights[0].d_d2_filter);
    // FragColor=vec4(vec3(1.-temp),1.);
    // DEBUG：VSM，显示光源视角的深度值
    // FragColor=vec4(vec3(d_d2.x),1.);
}

vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir){
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
    float shadow;
    if(shadowMapType==0){
        shadow=SM(FragPosLightSpace,normal,lightDir,light.shadowMap);
    }
    else if(shadowMapType==1){
        shadow=PCF(FragPosLightSpace,normal,lightDir,light.shadowMap);
    }
    else if(shadowMapType==2){
        shadow=PCSS(FragPosLightSpace,normal,lightDir,light.shadowMap);
    }
    else if(shadowMapType==3){
        shadow=VSM(FragPosLightSpace,normal,lightDir,light.d_d2_filter);
    }
    
    return(ambient+(1.-shadow)*(diffuse+specular));
}

vec3 CalcPointLight(PointLight light,vec3 normal,vec3 fragPos,vec3 viewDir){
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

float SM(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D shadowMap){
    // 转换为标准齐次坐标 z[-1, 1]
    vec3 projCoords=fragPosLightSpace.xyz/fragPosLightSpace.w;
    // xyz: [-1, 1] -> [0, 1]
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
    /// 常规做法：如果currentDepth大于closetDepth，说明当前fragment被某个物体遮挡住了，在阴影之中
    float shadow=currentDepth-bias>closestDepth?1.:0.;
    
    return shadow;
}

float PCF(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D shadowMap){
    // 转换为标准齐次坐标 z[-1, 1]
    vec3 projCoords=fragPosLightSpace.xyz/fragPosLightSpace.w;
    // xyz: [-1, 1] -> [0, 1]
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
    /// PCF:
    float shadow=0.;
    // 计算每个纹素的大小
    vec2 texelSize=1./textureSize(shadowMap,0);
    // 遍历3x3的邻域
    for(int x=-PCF_RADIUS;x<=PCF_RADIUS;++x)
    {
        for(int y=-PCF_RADIUS;y<=PCF_RADIUS;++y)
        {
            // 从阴影贴图中采样深度值
            float pcfDepth=texture(shadowMap,projCoords.xy+vec2(x,y)*texelSize).r;
            // 如果当前片段的深度值大于采样的深度值，则在阴影中
            shadow+=currentDepth-bias>pcfDepth?1.:0.;
        }
    }
    // 计算平均阴影值
    float total=2*PCF_RADIUS+1;
    shadow/=(total*total);
    
    return shadow;
}

float PCSS(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D shadowMap){
    // 转换为标准齐次坐标 z[-1, 1]
    vec3 projCoords=fragPosLightSpace.xyz/fragPosLightSpace.w;
    // xyz: [-1, 1] -> [0, 1]
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
    /// PCSS:
    // 计算平均遮挡物体的深度值
    float avgDepth=findBlocker(projCoords.xy,currentDepth,shadowMap,bias);
    // 如果没有遮挡物体，则直接返回0.0(不在阴影中)
    if(avgDepth==-1.){
        return 0.;
    }
    // 半影大小
    float penumbra=(currentDepth-avgDepth)/avgDepth*lightWidth;
    // 采样半径
    float filterRadius=penumbra*near_plane/currentDepth;
    // PCF
    filterRadius*=PCFSampleRadius;
    float shadow=0.;
    // 计算每个纹素的大小
    vec2 texelSize=1./textureSize(shadowMap,0);
    // 遍历邻域
    for(int x=-PCF_RADIUS;x<=PCF_RADIUS;++x)
    {
        for(int y=-PCF_RADIUS;y<=PCF_RADIUS;++y)
        {
            // 从阴影贴图中采样深度值
            float shadowMapDepth=texture(shadowMap,projCoords.xy+filterRadius*vec2(x,y)*texelSize).r;
            // 如果当前片段的深度值大于采样的深度值，则在阴影中
            shadow+=currentDepth-bias>shadowMapDepth?1.:0.;
        }
    }
    // 计算平均阴影值
    float total=2*PCF_RADIUS+1;
    shadow/=(total*total);
    
    return shadow;
}

float findBlocker(vec2 uv,float zReceiver,sampler2D shadowMap,float bias){
    // 遮挡者计数
    int blockers=0;
    // 遮挡者深度值累加
    float ret=0.;
    
    // 计算每个纹素的大小
    vec2 texelSize=1./textureSize(shadowMap,0);
    // 遍历以当前片段为中心的BLOCK_RADIUS*2+1的区域
    for(int x=-BLOCK_RADIUS;x<=BLOCK_RADIUS;++x){
        for(int y=-BLOCK_RADIUS;y<=BLOCK_RADIUS;++y){
            // 从阴影贴图中采样深度值
            float shadowMapDepth=texture(shadowMap,uv+vec2(x,y)*texelSize).r;
            // 如果当前片段的深度值大于采样的深度值，则认为是遮挡者
            if(zReceiver-bias>shadowMapDepth){
                // 累加遮挡者的深度值
                ret+=shadowMapDepth;
                // 遮挡者计数+1
                ++blockers;
            }
        }
    }
    
    // 如果没有找到遮挡者，则返回-1
    if(blockers==0)
    return-1.;
    
    // 返回遮挡者的平均深度值
    return ret/blockers;
}

float VSM(vec4 fragPosLightSpace,vec3 normal,vec3 lightDir,sampler2D d_d2_filter){
    vec3 projCoords=fragPosLightSpace.xyz/fragPosLightSpace.w;
    // [-1, 1] => [0, 1]
    projCoords=projCoords*.5+.5;
    if(projCoords.z>1.||projCoords.z<0.)
    return 0.;
    
    depth=projCoords.z;
    
    // 从模糊后的纹理中获得深度值均值和方差
    d_d2=texture(d_d2_filter,projCoords.xy).rg;
    float var=d_d2.y-d_d2.x*d_d2.x;// E(X-EX)^2 = EX^2-E^2X
    
    // 偏移量，解决阴影失真的问题, 根据表面朝向光线的角度更改偏移量
    float bias=max(.05*(1.-dot(normal,lightDir)),.005);
    // float bias=.005;
    float visibility;
    if(depth-bias<d_d2.x){
        visibility=1.;// 没有阴影
    }
    else{
        // 使用切比雪夫不等式计算阴影
        float t_minus_mu=depth-d_d2.x;
        visibility=var/(var+t_minus_mu*t_minus_mu);
    }
    return 1.-visibility;
}
