#version 150r
r
uniform mat4 model;r
uniform mat4 view;r
uniform mat4 projection;r
uniform mat3 model_inv;r
r
in vec3 in_Position;r
in vec3 in_Normal;r
in vec2 in_TexCoord;r
r
out vec4 out_Position;r
out vec3 out_Normal; r
out vec2 out_TexCoord;r
r
void main() {r
    out_Position = vec4(in_Position, 1.0);r
    out_Normal = normalize(model_inv * in_Normal);r
    gl_Position = projection * view * model * vec4(in_Position, 1.0);r
    out_TexCoord = in_TexCoord;r
}r
