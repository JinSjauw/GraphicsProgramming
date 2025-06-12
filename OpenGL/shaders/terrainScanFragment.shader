#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform mat4 inverseView;
uniform mat4 inverseProjection;


float near = 0.1; 
float far = 500.0; 
float distanceBetweenLines = 100;
float scanLineThickness = 1;

vec3 originTerrainScan = vec3(1500, 150, 1300);

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

vec3 lerp( vec3 a, vec3 b, float t)
{
    return a + (b - a) * t;
}

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;

    //Getting the depth
    float depth = texture(depthTexture, TexCoords).r;
    float linearDepth = LinearizeDepth(depth) / far; // normalize for viewing

    if(linearDepth > .9999)
    {
        FragColor = vec4(color, 1.0);  
        return;
    }

    //Calculating view space position
    vec4 remappedScreenPosition = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePosition = inverseProjection * remappedScreenPosition;
    viewSpacePosition /= viewSpacePosition.w;

    //World space position
    vec4 worldSpacePosition = inverseView * viewSpacePosition;

    //Creating the scanLines

    float distanceFromOrigin = distance(originTerrainScan, worldSpacePosition.xyz);
    float depthCategories = fract(distanceFromOrigin / distanceBetweenLines);

    float depthLinesMask = 1.0 - step((scanLineThickness / distanceBetweenLines), depthCategories);

    vec3 result = lerp(color, vec3(depthLinesMask), depthLinesMask);

    FragColor = vec4(result, 1.0);

    //float lines = fract(linearDepth / distanceBetweenLines);

    //vec3 scanLines = depthLinesMask > .97f ? vec3(1.0) : color;

    //FragColor = vec4(scanLines, 1.0);

    //Debug worldPosition

    // float stripes = step(.2, fract(worldSpacePosition.x * .01));
    // float checker = step(.2, fract(worldSpacePosition.x * .01)) * step(.2, fract(worldSpacePosition.z * .01));

    // FragColor = vec4(vec3(checker), 1.0);

    // vec3 worldPosColor = vec3(worldSpacePosition.xyz) * .1;

    // worldPosColor = clamp(worldPosColor, 0.0, 1.0);

    // FragColor = vec4(worldPosColor, 1.0);
}
