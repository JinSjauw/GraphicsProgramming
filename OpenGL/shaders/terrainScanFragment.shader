#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform mat4 inverseView;
uniform mat4 inverseProjection;
uniform vec2 screenResolution;

float near = 0.1; 
float far = 100.0; 
float distanceBetweenLines = 50;
float scanLineThickness = 1;
vec3 scanLineColor = vec3(0.9, .9, 1.2);

//Sobel outline
float outlineThickness = 1;
float outlineDepthMultiplier = 1;
float outlineDepthBias = 10;;

vec4 outlineColor = vec4(0, 1, 0, 1);

//Scanner shape
float areaRadius = 600;
vec3 edgeColor = vec3(.1, .5, 1);
float edgeGradientFallOff = .6;
float edgeGradientSize = .15;

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

float lerp( float a, float b, float t)
{
    return a + (b - a) * t;
}

vec4 CalculateWorldSpacePosition(vec2 screenPosition, float depth)
{
    //Calculating view space position
    vec4 remappedScreenPosition = vec4(screenPosition * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePosition = inverseProjection * remappedScreenPosition;
    viewSpacePosition /= viewSpacePosition.w;

    //World space position
    vec4 worldSpacePosition = inverseView * viewSpacePosition;

    return worldSpacePosition;
}

vec3 Blend_Darken(vec3 baseColor, vec3 blendColor, float alpha)
{
    vec3 minimumValue = min(blendColor, baseColor);
    vec3 finalValue = lerp(baseColor, minimumValue, alpha);

    return finalValue;
}

float SobelSampleDepth(vec2 uv, vec2 pixelOffset, float depth)
{
    vec2 horizontalOffset = vec2(pixelOffset.x, 0);
    vec2 verticalOffset = vec2(0, pixelOffset.y);

    // float pixelCenter = distance(CalculateWorldSpacePosition(uv, depth).xyz,                    originTerrainScan);
    // float pixelLeft = distance(CalculateWorldSpacePosition(uv - horizontalOffset, depth).xyz, originTerrainScan);
    // float pixelRight = distance(CalculateWorldSpacePosition(uv + horizontalOffset, depth).xyz, originTerrainScan);
    // float pixelUp = distance(CalculateWorldSpacePosition(uv + verticalOffset, depth).xyz,   originTerrainScan);
    // float pixelDown = distance(CalculateWorldSpacePosition(uv - verticalOffset, depth).xyz,   originTerrainScan);

    // pixelCenter = clamp((pixelCenter - minDistance) / maxDistance, 0, 1);
    // pixelLeft = clamp((pixelLeft - minDistance) / maxDistance, 0, 1);
    // pixelRight = clamp((pixelRight - minDistance) / maxDistance, 0, 1);
    // pixelUp = clamp((pixelUp - minDistance) / maxDistance, 0, 1);
    // pixelDown = clamp((pixelDown - minDistance) / maxDistance, 0, 1);

    //Sobel outline whole effect
    float pixelCenter = LinearizeDepth(texture(depthTexture, TexCoords).r);
    float pixelLeft = LinearizeDepth(texture(depthTexture, TexCoords - horizontalOffset).r);
    float pixelRight = LinearizeDepth(texture(depthTexture, TexCoords + horizontalOffset).r);
    float pixelUp = LinearizeDepth(texture(depthTexture, TexCoords + verticalOffset).r);
    float pixelDown = LinearizeDepth(texture(depthTexture, TexCoords - verticalOffset).r);

    return abs(pixelLeft - pixelCenter) + 
           abs(pixelRight - pixelCenter) +
           abs(pixelUp - pixelCenter) +
           abs(pixelDown - pixelCenter);

    //Sobel outline scanline MASK
    // //Distances of each pixel to the origin of the terrain scan effect.
    // float centerDistanceToOrigin = distance(CalculateWorldSpacePosition(uv, depth).xyz,                    originTerrainScan);
    // float leftDistanceToOrigin   = distance(CalculateWorldSpacePosition(uv - horizontalOffset, depth).xyz, originTerrainScan);
    // float rightDistanceToOrigin  = distance(CalculateWorldSpacePosition(uv + horizontalOffset, depth).xyz, originTerrainScan);
    // float upDistanceToOrigin     = distance(CalculateWorldSpacePosition(uv + verticalOffset, depth).xyz,   originTerrainScan);
    // float downDistanceToOrigin   = distance(CalculateWorldSpacePosition(uv - verticalOffset, depth).xyz,   originTerrainScan);


    // //Comparing intervals to get the "lines" effect.
    // float center = floor(centerDistanceToOrigin / distanceBetweenLines);
    // float left = floor(leftDistanceToOrigin / distanceBetweenLines);
    // float right = floor(rightDistanceToOrigin / distanceBetweenLines);
    // float up = floor(upDistanceToOrigin / distanceBetweenLines);
    // float down = floor(downDistanceToOrigin / distanceBetweenLines);

    // return (center - left) + (center - right) + (center - up) + (center - down);
}

void main()
{
    vec3 originalColor = texture(screenTexture, TexCoords).rgb;

    //Getting the depth
    float depth = texture(depthTexture, TexCoords).r;
    float linearDepth = LinearizeDepth(depth) / far; // normalize for viewing

    if(linearDepth > .9999)
    {
        FragColor = vec4(originalColor, 1.0);  
        return;
    }

    // //Calculating view space position
    // vec4 remappedScreenPosition = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    // vec4 viewSpacePosition = inverseProjection * remappedScreenPosition;
    // viewSpacePosition /= viewSpacePosition.w;

    // //World space position
    // vec4 worldSpacePosition = inverseView * viewSpacePosition;

    vec4 worldSpacePosition = CalculateWorldSpacePosition(TexCoords, depth);

    //Creating the scan Lines
    float distanceFromOrigin = distance(originTerrainScan, worldSpacePosition.xyz);
    float depthCategories = fract(distanceFromOrigin / distanceBetweenLines);

    //Create Line interval mask
    float depthLinesMask = 1.0 - step((scanLineThickness / distanceBetweenLines), depthCategories);

    //apply to scene color
    //vec3 scanLinesColor = lerp(originalColor, scanLineColor, depthLinesMask);

    //Get Sobel Outline
    vec2 pixelOffset = vec2(1.0 / screenResolution.x, 1.0 / screenResolution.y) * outlineThickness;
    float sobelDepth = SobelSampleDepth(TexCoords, pixelOffset, depth) / outlineThickness;
    
    sobelDepth = pow(abs(clamp(sobelDepth, 0, 1) * outlineDepthMultiplier), outlineDepthBias);
    
    //Add sobel outline effect to line interval mask
    //vec3 scanLinesResult = lerp(scanLinesColor, outlineColor.rgb, sobelDepth * depthLinesMask);

    //Create circular area mask
    float areaMask = 1.0 - step(areaRadius, distanceFromOrigin);
    
    //Create circular edge gradient
    float edgeWidth = areaRadius * edgeGradientSize;
    float edgeGradientMask = 1.0 - pow(clamp((areaRadius - distanceFromOrigin) / edgeWidth, 0.0, 1.0), edgeGradientFallOff);

    float darkeningEdgeWidth = areaRadius * .4;
    float darkeningEdgeMask = 1.0 - pow(clamp((areaRadius - distanceFromOrigin) / darkeningEdgeWidth, 0.0, 1.0), 4.5);
    vec3 darkenedColor = Blend_Darken(originalColor, vec3(0, 0, 0), 1);

    //Add edge gradient & darkening effect
    vec3 darkeningEffect = lerp(originalColor, darkenedColor, darkeningEdgeMask);
    vec3 edgeGradient = lerp(darkeningEffect, edgeColor, edgeGradientMask);

    //Add scanlines
    vec3 scanLinesColor = lerp(edgeGradient, scanLineColor, depthLinesMask);
    vec3 scanLinesResult = lerp(scanLinesColor, outlineColor.rgb, sobelDepth * depthLinesMask);

    vec3 result = lerp(originalColor, scanLinesResult, areaMask);

    FragColor = vec4(result, 1.0);
}
