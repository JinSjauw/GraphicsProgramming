#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;


out vec2 uv;
out vec3 normal;
out vec3 worldPosition;

uniform sampler2D mainTex;

uniform mat4 world, view, projection;

void main()
{
    vec3 pos = aPos;
    //object space pffset

    vec4 worldPos = world * vec4(aPos, 1.0);
    //world space offset
    worldPos.y += texture(mainTex, vUV).r * 100.0;

    gl_Position = projection * view * world * vec4(aPos, 1.0);
    uv = vUV;
    normal = vNormal;

    worldPosition = mat3(world) * aPos;
}