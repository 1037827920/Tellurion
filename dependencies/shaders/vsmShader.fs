#version 330 core

// 纹理坐标
in vec2 TexCoords;

// 输出颜色
out vec4 FragColor;

// 深度纹理
uniform sampler2D d_d2;
// 决定模糊操作的方向，true表示垂直方向模糊，false表示水平方向模糊
uniform bool vertical;

// 模糊半径，定义了采样范围
#define R 5
// 采样总点数
#define TOTAL_SAMPLES 11

void main(){
    // 初始化累积值，存储深度和深度平方的总和
    vec2 d=vec2(0,0);
    // 计算纹素的大小
    vec2 texelSize=1./textureSize(d_d2,0);
    if(vertical){
        // 垂直方向模糊
        // 垂直方向上一个纹素的大小
        float r=texelSize.y;
        for(int i=-R;i<=R;++i){
            // 在垂直方向上采样，并累加深度值和深度平方值
            d+=texture(d_d2,vec2(TexCoords.x,TexCoords.y+i*r)).rg;
        }
    }else{
        // 水平方向模糊
        // 水平方向上一个纹素的大小
        float r=texelSize.x;
        for(int i=-R;i<=R;++i){
            // 在水平方向上采样，并累加深度值和深度平方值
            d+=texture(d_d2,vec2(TexCoords.x+i*r,TexCoords.y)).rg;
        }
    }
    // 计算平均值，将累积的深度值和深度平方值除以采样点总数
    FragColor.rg=d/TOTAL_SAMPLES;
    // DEBUG：原始纹理值
    // FragColor = vec4(texture(d_d2, TexCoords).rgb, 1.0);
}