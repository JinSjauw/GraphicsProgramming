#version 330 core
out vec4 FragColor;

in vec2 uv;
in vec3 worldPosition;

uniform sampler2D mainTex;
uniform sampler2D normalTex;

uniform sampler2D dirt, sand, grass, rock, snow;

uniform vec3 lightDirection;
uniform vec3 cameraPosition;

vec3 lerp( vec3 a, vec3 b, float t)
{
    return a + (b - a) * t;
}

void main()
{
    //Normal map
    vec3 normal = texture(normalTex, uv).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal.gb = normal.bg;
    normal.r = -normal.r;
    normal.b = -normal.b;


    //Specular data
    vec3 viewDirection = normalize(worldPosition.rgb - cameraPosition);
    //vec3 reflectedLightDirection = normalize(reflect(lightDirection, normal));

    //Lighting
    
    float lightValue = max( -dot(normal, lightDirection), 0.0f);

    //vec3 cellShading = texture(gradientTex,  vec2(0.0f, -min(lightValue + .1f, 1.0))).rgb;

    //float specular = pow(max(-dot( reflectedLightDirection, viewDirection), 0.0), 256);

    //Paint based on height
    float y = worldPosition.y;

    float dirtToSand = clamp((y - 20) / 10, -1, 1) * .5 + .5;
    float sandToGrass = clamp((y - 40) / 10, -1, 1) * .5 + .5;
    float grassToRock = clamp((y - 90) / 10, -1, 1) * .5 + .5;
    float rockToSnow = clamp((y - 125) / 10, -1, 1) * .5 + .5;

    float distance = length(worldPosition.xyz - cameraPosition);
    float uvLerp = clamp((distance - 250) / 150, -1, 1) * 0.5f + 0.5f;

    vec3 dirtColorClose = texture(dirt, uv * 100).rgb;
    vec3 sandColorClose = texture(sand, uv * 100).rgb;
    vec3 grassColorClose = texture(grass, uv * 100).rgb;
    vec3 rockColorClose = texture(rock, uv * 100).rgb;
    vec3 snowColorClose = texture(snow, uv * 100).rgb;

    vec3 dirtColorFar = texture(dirt, uv * 10).rgb;
    vec3 sandColorFar = texture(sand, uv * 10).rgb;
    vec3 grassColorFar = texture(grass, uv * 10).rgb;
    vec3 rockColorFar = texture(rock, uv * 10).rgb;
    vec3 snowColorFar = texture(snow, uv * 10).rgb;

    vec3 dirtColor = lerp(dirtColorClose, dirtColorFar, uvLerp);
    vec3 sandColor = lerp(sandColorClose, sandColorFar, uvLerp);
    vec3 grassColor = lerp(grassColorClose, grassColorFar, uvLerp);
    vec3 rockColor = lerp(rockColorClose, rockColorFar, uvLerp);
    vec3 snowColor = lerp(snowColorClose, snowColorFar, uvLerp);

    vec3 diffuse = lerp(lerp(lerp(lerp(dirtColor, sandColor, dirtToSand), grassColor, sandToGrass), rockColor, grassToRock), snowColor, rockToSnow);

    float fog = pow(clamp((distance - 250) / 1000, 0, 1), 2);

    vec3 topColor = vec3(68.0f / 255.0f, 118.0f / 255.0f, 189.0f / 255.0f);
    vec3 bottomColor = vec3(188.0f / 255.0f, 214.0f / 255.0f, 231.0f / 255.0f);
    
    vec3 fogColor = lerp(bottomColor, topColor, max(viewDirection.y, 0.0));

    vec4 result = vec4(lerp(diffuse * min(lightValue + 0.1, 1.0), fogColor, fog), 1); //+ specular;

    FragColor =  result;
    //FragColor = texture(gradientTex, vec2(0.0f, lightValue));
}