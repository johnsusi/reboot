#version 330 core

// in VS_OUT
// {
//     vec3 normal;
//     vec2 texCoords;
// } fs_in;

// uniform sampler2D uTexture;

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0); //texture(uTexture, fs_in.texCoords);
}