#version 450

#define FXAA_PC 1
#define FXAA_GLSL_130 1

in vec2 pos;

out vec2 texcoord;

void main(void)
{
    texcoord = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 1.0, 1.0);
}