#version 330 core

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
// uniform mat4 uBones[100];

layout(location = 0) in vec3  aPosition;
// layout(location = 1) in vec3  aNormal;
// layout(location = 2) in vec2  aTexCoords;
// layout(location = 3) in ivec4 aBoneIds;
// layout(location = 4) in vec4  aBoneWeights;

// out VS_OUT
// {
//     vec3 normal;
//     vec2 texCoords;
// } vs_out;_

void main()
{
    // mat4 skinMatrix =
    //         aBoneWeights.x * uBones[aBoneIds.x]+
    //         aBoneWeights.y * uBones[aBoneIds.y]+
    //         aBoneWeights.z * uBones[aBoneIds.z]+
    //         aBoneWeights.w * uBones[aBoneIds.w];
    // vec4 skinnedPosition = skinMatrix * vec4(aPosition, 1.0);
    // vec3 skinnedNormal = mat3(skinMatrix) * aNormal;
    // vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    // gl_Position = uProjection * uView * worldPosition;
    gl_Position = vec4(aPosition, 1.0);

    // vs_out.normal = mat3(uModel) * aNormal;
    // vs_out.texCoords = aTexCoords;
}