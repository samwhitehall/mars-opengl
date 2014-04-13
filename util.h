#define MAX_LIGHTS 8
#define MAX_CAMERA_ACTIONS 5

/* structure definitions */
struct LightSource {
    glm::vec4 position;
    glm::vec3 diffuse;
    glm::vec3 attenuation; // attentuation coefficients: [x=A, y=B, z=C] -> An^2 + Bn + C
};

struct model {
    GLuint vao;
    GLuint vertex_buffer;
    GLuint normal_buffer;
    GLuint element_buffer;
    GLuint texcoord_buffer;
    
    GLuint texture;
    
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    
    GLulong num_drawn_vertices;
    
    GLboolean earthquake_on;
    GLfloat earthquake_duration;
    GLfloat earthquake_elapsed;
    GLfloat earthquake_amplitude;
    
    struct {
        GLint model;
        GLint view;
        GLint projection;
        GLint model_inv;
        
        GLint ambient;
        
        GLint texture;
    } uniforms;
    
    struct {
        glm::vec3 ambient;
    } material;
    
    struct {
        GLint position;
        GLint normal;
        GLint texcoord;
    } attributes;
    
    struct light {
        struct LightSource *source;
        GLint position;
        GLint diffuse;
        GLint attenuation;
    };
    
    struct light lights[MAX_LIGHTS];
    
    glm::vec3 position;
};

struct scene {
    struct LightSource lights[MAX_LIGHTS];
    GLuint num_lights;
    
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
};

struct camera {    
    glm::vec3 position;
    glm::vec3 focus;
    glm::vec3 up;
    
    glm::vec2 angles;
    
    GLboolean stopped;
    
    GLfloat rate;
    
    struct stage {
        glm::vec3 start;
        glm::vec3 end;
        GLfloat start_angle;
        GLfloat end_angle;
        GLdouble duration;    /* seconds */
        GLdouble time_elapsed;  /* time so far in action */
    };
    
    GLushort num_stages;
    GLushort current_stage;
    
    struct stage tour[MAX_CAMERA_ACTIONS];
};

/* function prototypes */
int make_model(struct model *resources,
               const char *obj_path,
               const char *vertex_shader_path,
               const char *fragment_shader_path,
               const char *texture_path
               );