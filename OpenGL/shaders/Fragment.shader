#version 330 core
out vec4 FragColor;

in vec3 color;
in vec2 uv;
in mat3 tbn;
in vec4 FragPos;

uniform sampler2D mainTex;
uniform sampler2D normalTex;
uniform sampler2D gradientTex;

uniform vec3 lightDirection;
uniform vec3 cameraPosition;

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
    //Normal map
    vec3 normal = texture(normalTex, uv).rgb;
    normal = normalize(normal * 2.0f - 1.0f);
    normal = tbn * normal;

    float lightValue = max(dot(lightDirection, normal), 0.0);

    //Specular data
    vec3 viewDirection = normalize(FragPos.rgb - cameraPosition);
    vec3 reflectedLightDirection = reflect(lightDirection, normal);

    //Lighting

    vec3 cellShading = texture(gradientTex,  vec2(0.0f, -min(lightValue + .1f, 1.0))).rgb;

    float specular = pow(max(-dot( reflectedLightDirection, viewDirection), 0.0), 256);

    vec4 result = vec4(color, 1.0f) * texture( mainTex, uv);
    result.rgb = result.rgb * cellShading;

    //Fog
    float distance = length(FragPos.xyz - cameraPosition);
    float fog = pow(clamp((distance - 900) / 1000, 0, 1), 2);

    vec3 topColor = vec3(68.0f / 255.0f, 118.0f / 255.0f, 189.0f / 255.0f);
    vec3 bottomColor = vec3(188.0f / 255.0f, 214.0f / 255.0f, 231.0f / 255.0f);
    
    vec3 fogColor = lerp(bottomColor, topColor, max(viewDirection.y, 0.0));

    FragColor =  lerp(result + specular, vec4(fogColor, 1), fog);
}