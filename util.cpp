#include <GL/glew.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/glfw.h>

#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

#include <glm/glm.hpp>

#include "util.h"

using namespace std;

/*
 * Boring, non-OpenGL-related utility functions
 */

void *file_contents(const char *filename, GLint *length)
{
    FILE *f = fopen(filename, "r");
    void *buffer;
    
    if (!f) {
        fprintf(stderr, "Unable to open %s for reading\n", filename);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    *length = (GLint)ftell(f);
    fseek(f, 0, SEEK_SET);
    
    buffer = malloc(*length+1);
    *length = (GLint)fread(buffer, 1, *length, f);
    fclose(f);
    ((char*)buffer)[*length] = '\0';
    
    return buffer;
}

// load Wavefront (.obj) files into vertices/normals for OpenGL
// derived from code found here: http://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Load_OBJ
// but with texture (UV) co-ordinate handling & comments added
void load_obj(const char* filename,
              vector<glm::vec3> &vertices,
              vector<glm::vec2> &tex_coords,
              vector<glm::vec3> &normals,
              vector<GLushort> &elements,
              GLboolean has_texture) {
    
    // .obj file is nice to parse: process line-by-line:
    // v ... space-separated vertex co-ordinates
    // f ... space-separated 1-indexed vertices making up a face (should be triangle... not checked though!)
    
    // load file as input stream; fail if it doesn't work
    ifstream in(filename, ios::in);
    if (!in) { cerr << "Cannot open " << filename << endl; exit(1); }
    
    // process line by line...
    string line;
    vector<glm::vec2> tex_vertices;
    vector<GLushort> tex_elements;
    while (getline(in, line)) {
        
        // vertex co-ordinate
        if (line.substr(0,2) == "v ") {
            // stream of remainder of line (x,y,z co-ordinates) by spaces
            istringstream s(line.substr(2));
            glm::vec3 v;
            
            // pack vector co-ordinate with these values
            s >> v.x;
            s >> v.y;
            s >> v.z;
            
            // add to the end of the vertices vector
            vertices.push_back(v);
        }
        
        // texture co-ordinate
        if (line.substr(0,3) == "vt ") {
            // vs have finished:
            tex_coords.resize(vertices.size(), glm::vec2(0.0, 0.0));
            
            // stream of remainder of line (x,y co-ordinates) by spaces
            istringstream s(line.substr(3));
            glm::vec2 t;
            
            // pack vector co-ordinates with these values
            s >> t.x;
            s >> t.y;
            
            // add to the end of tex vertices vector
            tex_vertices.push_back(t);
        }
        
        // face definition
        else if (line.substr(0,2) == "f ") {
            // stream of remainder of line (vertex indicies of face triangle) by spaces
            stringstream s(line.substr(2));
            
            string face_str;
            while (s >> face_str) {
                istringstream nums(face_str);
                
                GLushort mesh_elem;
                GLushort tex_elem;
                
                nums >> mesh_elem;
                if(has_texture) {
                    nums.ignore();  // ignore slash
                    nums >> tex_elem;
                    tex_elem--;
                }
                
                mesh_elem--;
                
                elements.push_back(mesh_elem);
                
                if (has_texture)
                    tex_coords[mesh_elem] = tex_vertices[tex_elem];
            }
        }
        else {
            /* ignoring this line */
        }
    }
    
    // calculate normals
    normals.resize(vertices.size(), glm::vec3(0.0, 0.0, 0.0));
    for (GLuint i = 0; i < elements.size(); i+=3) {
        GLushort ia = elements[i];
        GLushort ib = elements[i+1];
        GLushort ic = elements[i+2];
        glm::vec3 normal = glm::normalize(glm::cross(glm::vec3(vertices[ib]) - glm::vec3(vertices[ia]),
                                                     glm::vec3(vertices[ic]) - glm::vec3(vertices[ia])));
        normals[ia] = normals[ib] = normals[ic] = normal;
    }
}

/* helper function to generate, bind and populate a buffer */
static GLuint make_buffer(GLenum target,
                          const void *buffer_data,
                          unsigned long buffer_size) {
    GLuint buffer;
    
    glGenBuffers(1, &buffer); /* generate 1 buffer object name */
    glBindBuffer(target, buffer); /* bind buffer to specified binding point */
    glBufferData(target, (GLsizei)buffer_size, buffer_data, GL_STATIC_DRAW);
    /* create and initialise new data store for object bound to target */
    
    return buffer;
}

/* print out log */
static void show_info_log(
                          GLuint object,
                          PFNGLGETSHADERIVPROC glGet__iv,
                          PFNGLGETSHADERINFOLOGPROC glGet__InfoLog
                          ) {
    GLint log_length;
    char *log;
    
    glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
    log = (char *)malloc(log_length);
    glGet__InfoLog(object, log_length, NULL, log);
    
    fprintf(stderr, "%s", log);
    free(log);
}

/* loads and compiles a shader */
static GLuint make_shader(GLenum type,
                          const char *filename) {
    GLint length;
    GLuint shader;
    GLint shader_ok;
    
    /* load shader from file */
    GLchar *source = (GLchar *)file_contents(filename, &length);
    
    
    if(!source) /* file loading error */
        return 0;
    
    /* create & compile shader */
    shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar**)&source, &length);
    free(source);
    glCompileShader(shader);
    
    /* check shader compilation */
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
    if(!shader_ok) {
        fprintf(stderr, "Failed to compile %s: \n", filename);
        show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

/* attach & link shaders to a program */
static GLuint make_program(GLuint vertex_shader,
                           GLuint fragment_shader) {
    GLint program_ok;
    GLuint program = glCreateProgram();
    
    /* vertex shader */
    glAttachShader(program, vertex_shader);
    
    /* fragment shader */
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    /* check shaders correctly linked */
    glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
    if (!program_ok) {
        fprintf(stderr, "Failed to link shader program:\n");
        show_info_log(program, glGetProgramiv, glGetProgramInfoLog);
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

/* generate all necessary resources */
int make_model(struct model *resources,
                       const char *obj_path,
                       const char *vertex_shader_path,
                       const char *fragment_shader_path,
                       const char *texture_path
                       ) {
    /* texture */
    if(texture_path) {
        glGenTextures(1, &(resources->texture));
        glBindTexture(GL_TEXTURE_2D, resources->texture);
        glfwLoadTexture2D( texture_path, 0);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    /* vertex array object */
    glGenVertexArrays(1, &(resources->vao));
    glBindVertexArray(resources->vao);
    
    vector<glm::vec3> mesh_vertices;
    vector<glm::vec2> tex_coords;
    vector<glm::vec3> mesh_normals;
    vector<GLushort> mesh_elements;
    
    load_obj(obj_path,
             mesh_vertices,
             tex_coords,
             mesh_normals,
             mesh_elements,
             texture_path != NULL);
    
    resources->vertex_buffer = make_buffer(GL_ARRAY_BUFFER,
                                           &mesh_vertices[0],
                                           sizeof(glm::vec3) * mesh_vertices.size());
    
    resources->normal_buffer = make_buffer(GL_ARRAY_BUFFER,
                                           &mesh_normals[0],
                                           sizeof(glm::vec3) * mesh_normals.size());
    
    resources->num_drawn_vertices = mesh_elements.size();
    
    resources->element_buffer = make_buffer(GL_ELEMENT_ARRAY_BUFFER,
                                            &mesh_elements[0],
                                            sizeof(GLushort) * resources->num_drawn_vertices);
    if(texture_path) {
        resources->texcoord_buffer = make_buffer(GL_ARRAY_BUFFER,
                                                 &tex_coords[0],
                                                 sizeof(glm::vec2) * tex_coords.size());
    }
    
    /* make vertex shaders */
    resources->vertex_shader = make_shader(GL_VERTEX_SHADER, vertex_shader_path);
    if (resources->vertex_shader == 0)
        return 0;
    
    /* make fragment shader */
    resources->fragment_shader = make_shader(GL_FRAGMENT_SHADER, fragment_shader_path);
    
    if (resources->fragment_shader == 0)
        return 0;
    
    /* make program */
    resources->program = make_program(resources->vertex_shader,
                                      resources->fragment_shader);
    if(resources->program == 0)
        return 0;
    
    /* setup shader uniforms (MVP matrix & texture) */
    resources->uniforms.model = glGetUniformLocation(resources->program, "model");
    if(resources->uniforms.model == -1)
        return 0;
    resources->uniforms.view = glGetUniformLocation(resources->program, "view");
    if(resources->uniforms.view == -1)
        return 0;
    resources->uniforms.projection = glGetUniformLocation(resources->program, "projection");
    if(resources->uniforms.projection == -1)
        return 0;
    resources->uniforms.model_inv = glGetUniformLocation(resources->program, "model_inv");
    if(resources->uniforms.model_inv == -1)
        return 0;
    
    if (texture_path) {
        resources->uniforms.texture = glGetUniformLocation(resources->program, "tex");
        if(resources->uniforms.texture == -1)
            return 0;
    }
    
    /* setup lighting uniforms: all lights on this scene */
    int light_idx;
    for(light_idx = 0; light_idx < MAX_LIGHTS; light_idx++) {
        /* generate uniform name strings */
        char position_uniform_name[50];
        char diffuse_uniform_name[50];
        char attenuation_uniform_name[50];
        snprintf(position_uniform_name, 50, "light[%d].position", light_idx);
        snprintf(diffuse_uniform_name, 50, "light[%d].diffuse", light_idx);
        snprintf(attenuation_uniform_name, 50, "light[%d].attenuation", light_idx);
        
        /* set appropriate uniforms */
        resources->lights[light_idx].position = glGetUniformLocation(resources->program,
                                                                     position_uniform_name);
        resources->lights[light_idx].diffuse = glGetUniformLocation(resources->program,
                                                                    diffuse_uniform_name);
        resources->lights[light_idx].attenuation = glGetUniformLocation(resources->program,
                                                                        attenuation_uniform_name);
    }
    
    /* setup shader attributes */
    resources->attributes.position = glGetAttribLocation(resources->program, "in_Position");
    resources->attributes.normal = glGetAttribLocation(resources->program, "in_Normal");
    
    if(texture_path)
        resources->attributes.texcoord = glGetAttribLocation(resources->program, "in_TexCoord");
    
    /* add out colour variable to fragment shader */
    glBindFragDataLocation(resources->program, 0, "fragment_Colour");
    
    /* set vertex attributes (positions) */
    glBindBuffer(GL_ARRAY_BUFFER, resources->vertex_buffer);
    glEnableVertexAttribArray(resources->attributes.position);
    glVertexAttribPointer(
                          resources->attributes.position,  /* attribute */
                          3,                                /* size */
                          GL_FLOAT,                         /* type */
                          GL_FALSE,                         /* normalized? */
                          0,                                /* stride */
                          (void*)0                          /* array buffer offset */
                          );
    
    /* set vertex attributes (normals) */
    glBindBuffer(GL_ARRAY_BUFFER, resources->normal_buffer);
    glEnableVertexAttribArray(resources->attributes.normal);
    glVertexAttribPointer(
                          resources->attributes.normal,  /* attribute */
                          3,                                /* size */
                          GL_FLOAT,                         /* type */
                          GL_FALSE,                         /* normalized? */
                          0,                                /* stride */
                          (void*)0                          /* array buffer offset */
                          );
    
    if (texture_path) {
        /* set texture coordinates */
        glBindBuffer(GL_ARRAY_BUFFER, resources->texcoord_buffer);
        glEnableVertexAttribArray(resources->attributes.texcoord);
        glVertexAttribPointer(
                              resources->attributes.texcoord,  /* attribute */
                              2,                                /* size */
                              GL_FLOAT,                         /* type */
                              GL_FALSE,                         /* normalized? */
                              0,                                /* stride */
                              (void*)0                          /* array buffer offset */
                              );
    }
    return 1;
}