// Copyright 2016 Intel Corporation All Rights Reserved
// 
// Intel makes no representations about the suitability of this software for any purpose.
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#version 450

layout(location = 0) in vec4 i_Position;
layout(location = 1) in vec2 i_Texcoord;
layout(location = 2) in vec4 i_PerInstanceData;

layout( push_constant ) uniform Scaling {
  float AspectScale;
} PushConstant;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec2 v_Texcoord;
layout(location = 1) out float v_Distance;

void main() {
    v_Texcoord = i_Texcoord;
    v_Distance = 1.0 - i_PerInstanceData.z;       // Darken with distance

    vec4 position = i_Position;
    position.y *= PushConstant.AspectScale;       // Adjust to screen aspect ration
    position.xy *= pow( v_Distance, 0.5 );        // Scale with distance
    gl_Position = position + i_PerInstanceData;
}
