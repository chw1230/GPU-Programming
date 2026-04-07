#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // 카메라(View/Projection)와 큐브의 위치(Model) 행렬을 곱해 3D 공간의 정점을 2D 모니터 화면 좌표로 변환
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}