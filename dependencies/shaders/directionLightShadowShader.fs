#version 330 core

out vec4 FragColor;

void main()
{
    // 让片段着色器自己计算深度值
    // gl_FragDepth = gl_FragCoord.z;
    
    // VSM
    float depth = gl_FragCoord.z;
    FragColor.r=depth;
    FragColor.g=depth*depth;
}