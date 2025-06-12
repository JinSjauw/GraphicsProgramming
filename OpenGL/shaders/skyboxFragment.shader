#version 330 core
out vec4 FragColor;

in vec4 worldPosition;

uniform vec3 lightDirection;
uniform vec3 cameraPosition;

vec3 lerpv3(vec3 a, vec3 b, float t)
{
    return a + (b - a) * t;   
}

void main()
{
    vec3 topColor = vec3(68.0f / 255.0f, 118.0f / 255.0f, 189.0f / 255.0f);
    vec3 bottomColor = vec3(188.0f / 255.0f, 214.0f / 255.0f, 231.0f / 255.0f);

    vec3 sunColor = vec3(250.0f / 255, 0 / 255.0, 250 / 255.0);

    vec3 viewDirection = normalize(worldPosition.rgb - cameraPosition);

    float sun = max(pow(dot( -viewDirection, lightDirection), 128), 0.0f);

    FragColor =  vec4(lerpv3(bottomColor, topColor, max(viewDirection.y, 0.0)) + sun * sunColor, 1);
}