#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;

float near = 0.1; 
float far = 100.0; 

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;
    float depth = texture(depthTexture, TexCoords).r;
    float linearDepth = LinearizeDepth(depth) / far; // normalize for viewing

    FragColor = vec4(vec3(linearDepth), 1.0);

    //FragColor = vec4(vec3(1.0 - texture(mainTex, texCoords)), 1.0);
    //float depth = texture(depthTexture, TexCoords).r;

    //FragColor = vec4(vec3(depth, depth, depth), 1.0);
    // float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    // FragColor = vec4(average, average, average, 1.0);
}
