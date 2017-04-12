#version 450

#include "shaderDefines.h"

layout(binding = TEXUNIT_COLOR) uniform sampler2D colorTex;

in vec2 texcoord;

layout (location = 0) out vec4 outColor;

void main(void)
{
    outColor = texture2D(colorTex, texcoord);
}
