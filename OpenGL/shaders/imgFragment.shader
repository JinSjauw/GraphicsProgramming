#version 330 core
out vec4 FragColor;

in vec2 texCoords;
uniform sampler2D mainTex;

void main()
{
    FragColor =  texture(mainTex, texCoords);
}