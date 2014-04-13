#version 150r
r
struct LightSourcer
{r
    vec4 position;r
    vec3 diffuse;r
    vec3 attenuation;  // [x=A, y=B, z=C] -> An^2 + Bn + Cr
};r
const int num_lights = 8;r
uniform LightSource light[num_lights];r
r
struct Materialr
{r
    vec3 ambient;r
};r
uniform Material material;r
r
uniform mat4 model;r
uniform mat4 view;r
uniform mat4 projection;r
uniform sampler2D tex;r
r
uniform mat3 model_inv;r
r
in vec4 out_Position;r
in vec3 out_Normal;r
in vec2 out_TexCoord;r
r
out vec4 fragmentColour;r
r
void main() {r
    vec3 total_lighting = material.ambient;r
r
    for(int i = 0; i < num_lights; i++) {r
        vec3 normal_direction = normalize(model_inv * out_Normal);r
        vec3 material_diffuse = vec3(1.0, 0.8, 0.8);r
        r
        vec3 light_direction;r
        float attenuation;r
    r
        // OPTIMISATION: remove if branching?r
        // DIRECTIONAL lightingr
        if(light[i].position.w == 0.0) {r
            attenuation = 1.0;r
            light_direction = normalize(vec3(light[i].position));r
        }r
        r
        // POINT/SPOT lightingr
        else {r
            vec3 vertexToSource = vec3(light[i].position - model * out_Position);r
            float dist = length(vertexToSource);r
            r
            attenuation = 1.0 / (light[i].attenuation.x * dist * dist + light[i].attenuation.y * dist + light[i].attenuation.z);r
            light_direction = normalize(vec3(vertexToSource));r
        }r
r
        vec3 diffuse = attenuation r
                        * light[i].diffuser
                        * material_diffuser
                        * max(0.0, dot(normal_direction, light_direction));r
            r
        total_lighting = clamp(total_lighting + diffuse, 0.0, 1.0);r
    }r
    r
    fragmentColour = texture(tex, out_TexCoord);r
    fragmentColour = vec4(total_lighting, 1.0);r
}r
