#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalize(normalMat * aNormal);

    vec3 T = normalize(normalMat * aTangent);
    vec3 B = normalize(normalMat * aBitangent);
    vec3 N = Normal;
    TBN = mat3(T, B, N);

    TexCoords = aTexCoords;

    gl_Position = projection * view * worldPos;
}
