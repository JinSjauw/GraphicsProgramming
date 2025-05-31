#version 330 core
out vec4 FragColor;

in vec3 color;
in vec2 uv;
in mat3 tbn;
in vec3 worldPosition;

uniform sampler2D mainTex;
uniform sampler2D normalTex;
uniform sampler2D gradientTex;

uniform vec3 lightPosition;
uniform vec3 cameraPosition;

void main()
{
    //Normal map
    vec3 normal = texture(normalTex, uv).rgb;
    normal = normalize(normal * 2.0f - 1.0f);
    normal = tbn * normal;

    vec3 lightDirection = normalize(worldPosition - lightPosition);

    //Specular data
    vec3 viewDirection = normalize(worldPosition - cameraPosition);
    vec3 reflectedLightDirection = normalize(reflect(lightDirection, normal));

    //Lighting
    
    float lightValue = max( -dot(normal, lightDirection), 0.0f);

    vec3 cellShading = texture(gradientTex,  vec2(0.0f, -min(lightValue + .1f, 1.0))).rgb;

    float specular = pow(max(-dot( reflectedLightDirection, viewDirection), 0.0), 256);

    vec4 result = vec4(color, 1.0f) * texture( mainTex, uv);
    //result.rgb = result.rgb *  min(lightValue + .1, 1.0) + specular;

    result.rgb = result.rgb * cellShading;

    FragColor =  result;
    //FragColor = texture(gradientTex, vec2(0.0f, lightValue));
}