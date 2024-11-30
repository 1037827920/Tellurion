#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    sampler2D normalMap;
    sampler2D specularMap;
    float shininess;

    bool sampleNormalMap;
};

uniform Material material;

/// 材质
// 漫反射贴图
uniform sampler2D texture_diffuse0;


uniform vec3 viewPos;
uniform bool blinn;

struct DirLight {
    vec3 direction;
    vec3 lightColor;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

struct PointLight {    
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
uniform DirLight directionalLights[MAX_DIRECTIONAL_LIGHTS];  // MAX_DIRECTIONAL_LIGHTS is a predefined maximum

#define NR_POINT_LIGHTS 4  
uniform int numPointLights;
uniform PointLight pointLights[NR_POINT_LIGHTS];


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);  
void main()
{
    vec3 sampledNormal = Normal;
    if(material.sampleNormalMap){
        //code to implement normal mapping
        // 从法线贴图采样法线
        vec3 normalMap = texture(material.normalMap, TexCoords).rgb;
        sampledNormal = normalize(normalMap * 2.0 - 1.0);
    }

    // Use this normal for lighting calculations
    vec3 norm = normalize(sampledNormal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // phase 1: Directional lighting
    vec3 result = vec3(0.0);
    for(int i = 0; i < numDirectionalLights; i++)
        result += CalcDirLight(directionalLights[i], norm, viewDir);
    
    // phase 2: Point lights
    for(int i = 0; i < numPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);    
    // phase 3: Spot light
    //result += CalcSpotLight(spotLight, norm, FragPos, viewDir);    
    
    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfVector = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfVector), 0.0), material.shininess);
    if(!blinn){
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(reflectDir, viewDir), 0.0), material.shininess);
    }
    // combine results
    vec3 ambient  = light.ambient * light.lightColor  * vec3(texture(texture_diffuse0, TexCoords));
    vec3 diffuse  = light.diffuse * light.lightColor * diff * vec3(texture(texture_diffuse0, TexCoords));
    vec3 specular = light.specular * light.lightColor * spec * vec3(texture(texture_diffuse0, TexCoords));

    
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfVector = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfVector), 0.0), material.shininess);
    if(!blinn){
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(reflectDir, viewDir), 0.0), material.shininess);
    }
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * light.lightColor * vec3(texture(texture_diffuse0, TexCoords));
    vec3 diffuse  = light.diffuse  * light.lightColor * diff * vec3(texture(texture_diffuse0, TexCoords));
    vec3 specular = light.specular * light.lightColor * spec * vec3(texture(texture_diffuse0, TexCoords));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 
