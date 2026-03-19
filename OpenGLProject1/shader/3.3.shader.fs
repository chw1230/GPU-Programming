#version 330 core
out vec4 FragColor;
in vec3 ourColor;
uniform float ourBright;
void main(){
    FragColor = ourBright * vec4(ourColor, 1.0f);
};