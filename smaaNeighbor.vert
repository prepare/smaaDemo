#version 330
#extension GL_ARB_gpu_shader5 : enable
#extension GL_ARB_texture_gather : enable


#define SMAA_RT_METRICS screenSize
#define SMAA_GLSL_3 1
#define SMAA_PRESET_ULTRA 1

uniform vec4 screenSize;

#include "utils.h"

#define SMAA_INCLUDE_PS 0
#define SMAA_INCLUDE_VS 1

#include "smaa.h"


out vec2 texcoord;
out vec4 offset;


void main(void)
{
    vec2 pos = triangleVertex(gl_VertexID, texcoord);
    texcoord = flipTexCoord(texcoord);

	offset = vec4(0.0, 0.0, 0.0, 0.0);
	SMAANeighborhoodBlendingVS(texcoord, offset);

	gl_Position = vec4(pos, 1.0, 1.0);
}