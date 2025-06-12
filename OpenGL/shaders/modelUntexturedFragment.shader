#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normals;
in vec4 FragPos;

uniform vec4 defaultColor = vec4(0, 0, 0, 1);
uniform vec3 cameraPosition;
uniform vec3 lightDirection;

vec4 lerp(vec4 a, vec4 b, float t) {
    return a + (b - a) * t;
}

vec3 lerp(vec3 a, vec3 b, float t) {
    return a + (b - a) * t;
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void main()
{
  
    vec4 diffuse = defaultColor;

    float light = max(dot(-lightDirection, Normals), 0.0);

    vec3 viewDirection = normalize(FragPos.rgb - cameraPosition);
    vec3 refl = reflect(lightDirection, Normals);
    
    float spec = pow(max(-dot( refl, viewDirection), 0.0), 256);

    vec3 specular = spec * vec3(1, 1, 1);
    
    float distance = length(FragPos.xyz - cameraPosition);
    float fog = pow(clamp((distance - 900) / 1000, 0, 1), 2);

    vec3 topColor = vec3(68.0f / 255.0f, 118.0f / 255.0f, 189.0f / 255.0f);
    vec3 bottomColor = vec3(188.0f / 255.0f, 214.0f / 255.0f, 231.0f / 255.0f);
    
    vec3 fogColor = lerp(bottomColor, topColor, max(viewDirection.y, 0.0));

    vec4 result = lerp(diffuse * max(light, 0.2) + vec4(specular, 0), vec4(fogColor, 1.0), fog);

    FragColor = result;
}