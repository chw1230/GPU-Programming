#version 330 core
out vec4 FragColor;

in vec2 texCoord;
uniform sampler2D texture1; // texture sampler 1번
uniform sampler2D texture2; // texture sampler 2번 -> 겹치게 하기 위해서!!!!!
uniform float mix_ratio; // mix 값

void main()
{
    FragColor = mix(texture(texture1, texCoord), texture(texture2, texCoord), mix_ratio);
}