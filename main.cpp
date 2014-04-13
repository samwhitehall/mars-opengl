#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GL/glfw.h>

#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
using namespace std;

#include "util.h"

/* definition macros */
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600


/* global variables */

static struct model terrain;
static struct model base;

static struct scene main_scene;
static struct camera main_camera;

static GLdouble last_known_time;

static GLboolean free_roam_mode;

/* horizontally rotate the camera (around its up vector)
 *  +ve turns left from eye perspective, i.e. anticlockwise from above */
static void camera_rotate(GLfloat angle_x, GLfloat angle_y) {
    main_camera.angles.x += angle_x;
    main_camera.angles.y += angle_y;
}

static void camera_translate(glm::vec3 direction, GLfloat magnitude) {
    glm::vec3 vector = direction * magnitude;
    main_camera.position += vector;
}

static void camera_move(GLfloat forwards_mag, GLfloat right_mag) {
    glm::vec3 forwards_dir = glm::vec3(sinf(main_camera.angles.x),
                                      0,
                                      cosf(main_camera.angles.x));
    glm::vec3 right_dir = glm::vec3(-forwards_dir.z,
                                         0,
                                         forwards_dir.x);
    
    camera_translate(forwards_dir, forwards_mag);
    camera_translate(right_dir, right_mag);
}

/* add a stage to the camera's tour */
static void camera_add_stage(glm::vec3 to_point,
                             GLfloat to_angle,
                             GLfloat duration) {
    int i = main_camera.num_stages;
    if (i < MAX_CAMERA_ACTIONS) {
        if (i == 0) {/* first: start is initial camera position */
            main_camera.tour[i].start = main_camera.position;
            main_camera.tour[i].start_angle = main_camera.angles.x;
        }
        else { /* o/w, use end of previous camera action */
            main_camera.tour[i].start = main_camera.tour[i-1].end;
            main_camera.tour[i].start_angle = main_camera.tour[i-1].end_angle;
        }
        
        main_camera.tour[i].end = to_point;
        main_camera.tour[i].end_angle = to_angle;
        main_camera.tour[i].duration = duration;
        
        main_camera.num_stages += 1;
    }
    else {
        printf("Too many camera actions");
    }
}

static void camera_recalculate_view_matrix() {
        glm::vec3 lookat;
        lookat.x = sinf(main_camera.angles.x) * cosf(main_camera.angles.y);
        lookat.y = sinf(main_camera.angles.y);
        lookat.z = cosf(main_camera.angles.x) * cosf(main_camera.angles.y);
    
        main_scene.view_matrix = glm::lookAt(main_camera.position,
                                             main_camera.position + lookat,
                                             main_camera.up);
}

/* populate fields of main_camera's initial position */
static void camera_make(glm::vec3 position,
                        glm::vec2 angles,
                        glm::vec3 up
                        ) {
    main_camera.stopped = GL_TRUE;
    main_camera.angles = angles;
    main_camera.up = up;
    main_camera.rate = 1.0;
    camera_translate(position, 1.0);
        
    main_scene.projection_matrix = glm::perspective(45.0f,
                                                    1.0f*SCREEN_WIDTH/SCREEN_HEIGHT, 0.1f,
                                                    20.0f);
}

/* change the rate of the camera motion (compared to how it was programmed)... but not < 0 */
static void camera_rate(GLfloat delta) {
    printf("%f\n", main_camera.rate);
    if (main_camera.rate + delta <= 0.2) {
        return;
    }
    
    main_camera.rate += delta;
    
    int i;
    for (i = 0; i < main_camera.num_stages; i++) {
        main_camera.tour[i].duration /= main_camera.rate;
        main_camera.tour[i].time_elapsed /= main_camera.rate;        
    }
}

/* set details about the material of a model (i.e. ambient light level */
static void model_set_material(struct model *model,
                                glm::vec3 ambient) {
    model->uniforms.ambient = glGetUniformLocation(model->program, "material.ambient");
    model->material.ambient = ambient;
}

/* set the position in world space of a model */
static void model_set_location(struct model *model,
                                glm::vec3 position) {
    model->position = position;
}

/* draw a model to the screen */
static void model_render(struct model *obj_model) {
    glUseProgram(obj_model->program);
    
    /* texture stuff */
    if (terrain.texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, terrain.texture);
        glUniform1i(terrain.uniforms.texture, /*GL_TEXTURE*/0);
    }
    
    /* mvp matrix uniform */
    glm::mat4 model_mat = glm::translate(glm::mat4(1.0), obj_model->position);
    glm::mat3 model_inv = glm::transpose(glm::inverse(glm::mat3(model_mat)));
    
    glUniformMatrix4fv(obj_model->uniforms.model,
                       1,
                       GL_FALSE,
                       glm::value_ptr(model_mat));
    glUniformMatrix3fv(obj_model->uniforms.model_inv,
                       1,
                       GL_FALSE,
                       glm::value_ptr(model_inv));
    
    glUniformMatrix4fv(obj_model->uniforms.view,
                       1,
                       GL_FALSE,
                       glm::value_ptr(main_scene.view_matrix));
    glUniformMatrix4fv(obj_model->uniforms.projection,
                       1,
                       GL_FALSE,
                       glm::value_ptr(main_scene.projection_matrix));
    
    /* lighting uniforms */
    GLuint light_idx;
    for(light_idx = 0; light_idx < main_scene.num_lights; light_idx++) {
        glUniform4fv(obj_model->lights[light_idx].position,
                     1,
                     glm::value_ptr(main_scene.lights[light_idx].position));
        glUniform3fv(obj_model->lights[light_idx].diffuse,
                     1,
                     glm::value_ptr(main_scene.lights[light_idx].diffuse));
        glUniform3fv(obj_model->lights[light_idx].attenuation,
                     1,
                     glm::value_ptr(main_scene.lights[light_idx].attenuation));
    }
    
    /* material uniforms */
    glUniform3fv(obj_model->uniforms.ambient, 1, glm::value_ptr(obj_model->material.ambient));
    
    /* draw elements */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj_model->element_buffer);
    glDrawElements(
                   GL_TRIANGLES,       /* drawing mode */
                   (GLuint)obj_model->num_drawn_vertices,      /* count */
                   GL_UNSIGNED_SHORT,       /* type */
                   (void*)0                 /* element array buffer offset */
                   );
    
}

/* let there be (a) light */
static void light_make(glm::vec4 position,
                       glm::vec3 diffuse,
                       glm::vec3 attenuation) {
    if(main_scene.num_lights < MAX_LIGHTS) {
        main_scene.lights[main_scene.num_lights].position = position;
        main_scene.lights[main_scene.num_lights].diffuse = diffuse;
        main_scene.lights[main_scene.num_lights].attenuation = attenuation;
        
        main_scene.num_lights += 1;
    }
    else {
        fprintf(stderr, "Too many lights");
    }
}

/* camera movement handler; called on every "tick" of the timer */
static void timer_camera(GLdouble delta) {
    if(main_camera.stopped) {
        camera_recalculate_view_matrix();
        return;
    }
    
    // if this stage has finished
    if (main_camera.tour[main_camera.current_stage].time_elapsed
        > main_camera.tour[main_camera.current_stage].duration) {
        
        // place it at the end point (otherwise it may overshoot)
        main_camera.position = main_camera.tour[main_camera.current_stage].end;
        
        if (main_camera.current_stage == main_camera.num_stages - 1) {
            // if it's the last stage
            main_camera.stopped = GL_TRUE;
            return;
        }
        else {
            // otherwise, progress to next stage
            main_camera.current_stage += 1;
        }
    }
    
    int i = main_camera.current_stage;
    glm::vec3 delta_vector = main_camera.tour[i].end - main_camera.tour[i].start;
    delta_vector *= (main_camera.tour[i].time_elapsed / main_camera.tour[i].duration);
    main_camera.position = main_camera.tour[i].start + delta_vector;
    

    GLfloat delta_angle = main_camera.tour[i].end_angle - main_camera.tour[i].start_angle;
    delta_angle *= (main_camera.tour[i].time_elapsed / main_camera.tour[i].duration);
    
    main_camera.angles.x = main_camera.tour[i].start_angle + delta_angle;
    
    camera_recalculate_view_matrix();
    
    main_camera.tour[i].time_elapsed += delta;
}

/* earthquake movement handler; called on every "tick" */
static void timer_earthquake(GLdouble delta) {
    GLfloat amplitude_change_rate = 1.01;
    GLfloat time_periodicity_ratio = 500.0;
    GLfloat amplitude_basis = 0.005;
    
    if (terrain.earthquake_on) {
        terrain.earthquake_elapsed += delta;
        
        if(terrain.earthquake_elapsed > terrain.earthquake_duration) {
            terrain.earthquake_on = GL_FALSE;
            return;
        }
        
        if (terrain.earthquake_elapsed < terrain.earthquake_duration / 2)
            terrain.earthquake_amplitude *= amplitude_change_rate;
        else
            terrain.earthquake_amplitude /= amplitude_change_rate;

        GLfloat displacement_1 = terrain.earthquake_amplitude * amplitude_basis * sinf(time_periodicity_ratio * terrain.earthquake_elapsed * delta);
        GLfloat displacement_2 = terrain.earthquake_amplitude * amplitude_basis * sinf(time_periodicity_ratio * terrain.earthquake_elapsed * delta);
        
        
        model_set_location(&terrain, terrain.position + glm::vec3(displacement_1, displacement_2, 0.0));
    }
}

/* initialise everything: lights, camera, models */
static int init_resources() {
    // lighting
    light_make(glm::vec4(-0.451442, 3.999998, -3.918742, 0.0),
               glm::vec3(1.0, 0.5, 0.5),
               glm::vec3(20.0, 5.0, 0.0));
    
    // camera
    camera_make(glm::vec3(-5.328159, 0.400000, -7.339204),
                glm::vec2(1.0, 0.0),
                glm::vec3(0.0, 1.0, 0.0));
    
    // manually set up camera motion
    camera_add_stage(glm::vec3(-3.158986, 0.600000, -5.130444), 1.0, 3.0);
    camera_add_stage(glm::vec3(-0.939203, 0.800000, -2.144298), 3.0, 5.0);
    camera_add_stage(glm::vec3(2.036759, 1.200000, -5.792756), -0.9, 5.0);
    camera_add_stage(glm::vec3(2.339581, 1.200000, -8.596780), -0.460386, 3.0);
    camera_add_stage(glm::vec3(-5.328159, 0.400000, -7.339204), 1.0, 3.0);
    
    int error = make_model(&terrain, "terrain_tex.obj", "vert.glsl", "frag.glsl", "terrain_texture.tga");

    model_set_material(&terrain, glm::vec3(0.15));
    model_set_location(&terrain, glm::vec3(0.0, 0.0, -4.0));
    
    terrain.earthquake_duration = 3.14;
    terrain.earthquake_elapsed = 0.0;
    terrain.earthquake_on = GL_FALSE;
    terrain.earthquake_amplitude = 1.0;
    
    return error;
}

/* dispatch key presses to the relevant function */
void GLFWCALL handle_keypresses(int key, int action) {
    if(action == GLFW_RELEASE) {    
        /* Q/Esc.: quit */
        if(key == GLFW_KEY_ESC || key == 'Q') {
            glfwTerminate();
            exit(EXIT_SUCCESS);
        }
        
        /* T: begin camera tour */
        if(key == 'T') {
            if(main_camera.stopped) {
                int i;
                for (i = 0; i < main_camera.num_stages; i++) {
                    main_camera.tour[i].time_elapsed = 0.0;
                    main_camera.tour[i].duration *= main_camera.rate;
                }
                main_camera.rate = 1.0;
                main_camera.current_stage = 0;
                main_camera.position = main_camera.tour[0].start;
                main_camera.stopped = GL_FALSE;
            }
        }
        
        /* F: toggle free roam mode */
        if (key == 'F') {
            free_roam_mode = !free_roam_mode;
            printf("Free roam mode %s\n\n", (free_roam_mode)?"enabled":"disabled");
        }
        
        if (key == 'E') {
            terrain.earthquake_elapsed = 0.0;
            terrain.earthquake_on = GL_TRUE;
            terrain.earthquake_amplitude = 1.0;
        }
        
        /* <up>/<down> Alter speed of tour */
        if (key == GLFW_KEY_UP) {
            camera_rate(0.1);
        }
        
        if (key == GLFW_KEY_DOWN) {
            camera_rate(-0.1);
        }
        
        if (free_roam_mode) {
            switch (key) {
                case 'W':
                    camera_move(0.5, 0.0);
                    break;
                case 'S':
                    camera_move(-0.5, 0.0);
                    break;
                case 'A':
                    camera_move(0.0, -0.5);
                    break;
                case 'D':
                    camera_move(0.0, 0.5);
                    break;
                case GLFW_KEY_LEFT:
                    camera_rotate(0.1, 0.0);
                    break;
                case GLFW_KEY_RIGHT:
                    camera_rotate(-0.1, 0.0);
                    break;
                case GLFW_KEY_UP:
                    camera_translate(main_camera.up, 0.1);
                    break;
                case GLFW_KEY_DOWN:
                    camera_translate(main_camera.up, -0.1);
                default:
                    break;
            }
            
            fprintf(stdout, "pos: glm::vec3(%f, %f, %f) \nangles: glm::vec2(%f, %f)\n\n",
                    main_camera.position.x, main_camera.position.y, main_camera.position.z,
                    main_camera.angles.x, main_camera.angles.y);
        }
    }
}

/* called on Esc/Q/q; terminate the glfw window */
int GLFWCALL close_window(void) {
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

/* here be renderin' */
static void render(void) {    
    /* clear buffer */
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    /* timer stuff (camera/model animation) */
    GLdouble current_time = glfwGetTime();
    GLdouble delta = current_time - last_known_time;
    last_known_time = current_time;
    timer_camera(delta);
    timer_earthquake(delta);
    
    model_render(&terrain);
    model_render(&base);
}

int main(int argc, char** argv) {
    int running = GL_TRUE;
    
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}
    
    // set hints to open a GL3.2 context (otherwise it will do 2.1 by default on Mac)
    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
	glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
	if (!glfwOpenWindow(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 0, 16, 0, GLFW_WINDOW)) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
    
    glfwSetWindowTitle("Mars");
    glfwSetKeyCallback(handle_keypresses);
    glfwSetWindowCloseCallback(close_window);
    
    glewExperimental = GL_TRUE;
	glewInit();
	glGetError();
    
	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    if(!init_resources()) {
        fprintf(stdout, "Failed to load resources");
        return 1;
    }
    
	while (running) {
		render();
        glfwSwapBuffers();
	}
    
	glfwTerminate();
	exit(EXIT_SUCCESS);
}