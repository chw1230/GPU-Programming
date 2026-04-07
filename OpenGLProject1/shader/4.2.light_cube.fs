#version 330 core
out vec4 FragColor;
uniform vec3 LightColor;

void main()
{
    // 큐브 자체가 빛을 내는 광원
    FragColor = vec4(LightColor, 1.0); 
}