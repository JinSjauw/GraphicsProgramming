#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform mat4 inverseView;
uniform mat4 inverseProjection;
uniform vec2 screenResolution;
uniform vec3 originTerrainScan;
uniform float effectVisibility = 0;
uniform float areaRadius = 100;
uniform float firstLineRadius = 100;

float near = 0.1; 
float far = 100.0; 

//Sobel outline
float outlineThickness = 2;
float outlineDepthMultiplier = 1;
float outlineDepthBias = 1;;

vec4 outlineColor = vec4(0, 0.8, 1, 1);

//Scanner 
float distanceBetweenLines = 15;
float scanLineThickness = 1.5;
vec3 scanLineColor = vec3(0, 0.6, 1);

//Edge Effects
vec3 edgeColor = vec3(.1, .5, 1);
float edgeGradientFallOff = 1;
float edgeGradientSize = .25;

vec3 edgeDarkeningColor = vec3(0, 0, 0);
float edgeDarkeningFallOff = 3;
float edgeDarkeningSize = .35;

vec3 borderEdgeColor = vec3(0, .1, .5);
float borderEdgeSize = .015;
float borderEdgeFallOff = .8;

vec3 pushingLineColor = vec3(1, 1, 1);
vec3 trailingLineColor = vec3(0, 0.9, 1);

//Convert to [-1, 1]
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0;
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
}

void main()
{
    vec3 originalColor = texture(screenTexture, TexCoords).rgb;

    //Getting the depth
    float depth = texture(depthTexture, TexCoords).r;
    float linearDepth = LinearizeDepth(depth) / far;

    //Make it so the skybox doesn't get included'
    if(linearDepth > .9999)
    {
        FragColor = vec4(originalColor, 1.0);  
        return;
    }

    float aberrationAmount = 20;

    // Chromatic aberration offsets
    vec2 redOffset = vec2(aberrationAmount, 0.7) / screenResolution;
    vec2 greenOffset = vec2(0.0, aberrationAmount) / screenResolution;
    vec2 blueOffset = vec2(-aberrationAmount, 0.7f) / screenResolution;

    // Sample each color channel with offset
    float r = texture(screenTexture, TexCoords + redOffset).r;
    float g = texture(screenTexture, TexCoords + greenOffset).g;
    float b = texture(screenTexture, TexCoords + blueOffset).b;

    vec3 aberratedColor = vec3(r, g, b);

    //Calculating World Space
    vec4 worldSpacePosition = CalculateWorldSpacePosition(TexCoords, depth);

    //Creating the scan line mask
    float distanceFromOrigin = distance(originTerrainScan, worldSpacePosition.xyz);
    float depthCategories = fract(distanceFromOrigin / distanceBetweenLines);

    //Create Line interval mask
    float depthLinesMask = 1.0 - step((scanLineThickness / distanceBetweenLines), depthCategories);

    //Get Sobel Outline sum
    vec2 pixelOffset = vec2(1.0 / screenResolution.x, 1.0 / screenResolution.y) * outlineThickness;
    float sobelDepth = SobelSampleDepth(TexCoords, pixelOffset, depth) / outlineThickness;
    
    sobelDepth = pow(abs(clamp(sobelDepth, 0, 1) * outlineDepthMultiplier), outlineDepthBias);

    //Create circular area mask
    float areaMask = 1.0 - step(areaRadius, distanceFromOrigin);
    
    //Create circular edge gradient
    float edgeWidth = areaRadius * edgeGradientSize;
    float edgeGradientMask = 1.0 - pow(clamp((areaRadius - distanceFromOrigin) / edgeWidth, 0.0, 1.0), edgeGradientFallOff);

    //Create darkening effect for under the edge gradient
    float darkeningEdgeWidth = areaRadius * edgeDarkeningSize;
    float darkeningEdgeMask = 1.0 - pow(clamp((areaRadius - distanceFromOrigin) / darkeningEdgeWidth, 0.0, 1.0), edgeDarkeningFallOff);
    vec3 darkenedColor = Blend_Darken(originalColor, edgeDarkeningColor, 1);
    
    //Add edge gradient & darkening effect + abberation on the edge gradient
    vec3 darkeningEffect = lerp(originalColor, darkenedColor, darkeningEdgeMask);
    vec3 aberratedEdgeGradient = lerp(darkeningEffect, edgeColor + aberratedColor, edgeGradientMask);

    //Create a more obvious border on the edge of the effect
    float borderEdgeWidth = areaRadius * borderEdgeSize;
    float borderEdgeGradientMask = 1.0 - pow(clamp((areaRadius - distanceFromOrigin) / borderEdgeWidth, 0.0, 1.0), borderEdgeFallOff);

    vec3 borderEdgeGradient = lerp(aberratedEdgeGradient, borderEdgeColor, borderEdgeGradientMask);

    //Add scanlines
    vec3 scanLinesColor = lerp(borderEdgeGradient, scanLineColor, depthLinesMask);
    vec3 scanLinesResult = lerp(scanLinesColor, outlineColor.rgb, sobelDepth);

    //Color first line closest to the edge differently
    float firstLineMask = step((ceil(firstLineRadius / distanceBetweenLines) - 2) * distanceBetweenLines, distanceFromOrigin);
    vec3 firstScanLineColor = lerp(scanLinesResult, trailingLineColor, firstLineMask * depthLinesMask);

    //Color the line behind the first line differently
    float secondLineMask = step((ceil(firstLineRadius / distanceBetweenLines - 1)) * distanceBetweenLines, distanceFromOrigin);
    vec3 secondScanLineColor = lerp(firstScanLineColor, pushingLineColor, secondLineMask * depthLinesMask);

    //Make sure to apply the normal scanLine color
    float thirdLineMask = step((ceil(firstLineRadius / distanceBetweenLines)) * distanceBetweenLines, distanceFromOrigin);
    vec3 thirdScanLineColor = lerp(secondScanLineColor, scanLineColor, thirdLineMask * depthLinesMask);

    //Have the first white scanline stay at the end
    float whiteLineMask = step((ceil(areaRadius / distanceBetweenLines - 1)) * distanceBetweenLines, distanceFromOrigin);
    vec3 whiteScanLineColor = lerp(thirdScanLineColor, pushingLineColor, whiteLineMask * depthLinesMask);

    vec3 result = lerp(originalColor, whiteScanLineColor, areaMask);

    FragColor = vec4(lerp(result, originalColor, effectVisibility), 1);
}
