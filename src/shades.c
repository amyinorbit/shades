//===--------------------------------------------------------------------------------------------===
// shades.c - Small shader runner/tester
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2022 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include "gl.h"

#define WIDTH   1024
#define HEIGHT  800
#define SCALE   2
#define NAME    "Shades"

#define MAX_TEXTURES 4

typedef struct {
    GLuint          prog;
    const char      *path;
    
    struct {
        GLuint      vtx_pos;
    } attr;
    
    struct {
        GLuint      pvm;
        GLuint      tex0;
        GLuint      tex1;
        GLuint      tex2;
        GLuint      tex3;
        GLuint      tex_res;
        GLuint      res;
        GLuint      time;
        GLuint      scale;
    } uniform;
} shader_info_t;

typedef struct {
    GLuint          tex;
    vect2_t         size;
    const char      *path;
} texture_info_t;

typedef struct {
    vect2_t         pos;
    vect2_t         tex0;
} vertex_t;

typedef struct {
    shader_info_t   shader;
    texture_info_t  textures[MAX_TEXTURES];
    
    float           scale;
    vect2_t         size;
    
    GLuint          vao;
    GLuint          vbo;
    GLuint          ebo;
    
    vect2_t         vert[4];
    GLuint          indices[6];
    
    
    float           proj[16];
} shades_data_t;

static const char *vert_shader =
    "#version 400\n"
    "uniform mat4   u_pvm;\n"
    "in vec2        in_vtx_pos;\n"
    "void main() {\n"
    "    gl_Position = vec4(in_vtx_pos, 0.0, 1.0);\n"
    "}\n";

static const char *frag_defines =
    "#version 400\n"
    "uniform sampler2D  u_tex0;\n"
    "uniform sampler2D  u_tex1;\n"
    "uniform sampler2D  u_tex2;\n"
    "uniform sampler2D  u_tex3;\n"
    "uniform vec2       u_tex_res[4];\n"
    "uniform vec2       u_res;\n"
    "uniform float      u_time;\n"
    "uniform float      u_scale;\n"
    "\n"
    "out vec4           out_color;\n"  
    "\n";

static const char *frag_shader =
    "void main() {\n"
    "    vec2 coord = vec2(gl_FragCoord.x, u_res.y-gl_FragCoord.y);\n"
    "    out_color = main_image(coord / u_scale);\n"
    "}\n";



static void glfw_error(int code, const char *message) {
    fprintf(stderr, "glfw error [%d]: %s\n", code, message);
}

static GLuint reload_shader(GLuint prog, const char *path) {
    if(prog) {
        glDeleteProgram(prog);
        prog = 0;
    }
        
    char *source = load_source(path);
    if(!source) {
        fprintf(stderr, "could not open shader source `%s`\n", path);
        return 0;
    }
    fprintf(stderr, "loaded fragment shader source `%s`\n", path);
    GLuint vert = gl_load_shader(GL_VERTEX_SHADER, vert_shader, NULL);
    if(!vert) return 0;
    GLuint frag = gl_load_shader(GL_FRAGMENT_SHADER, frag_defines, source, frag_shader, NULL);
    if(!frag) return 0;
    
    prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    glDeleteShader(vert);
    glDeleteShader(frag);

    if(!gl_check_program(prog)) {
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

static GLuint reload_texture(GLuint tex, const char *path, vect2_t *size) {
    if(tex) {
        glDeleteTextures(1, &tex);
        tex = 0;
    }
    
    int w, h;
    tex = gl_load_tex(path, &w, &h);
    if(size) *size = VECT2(w, h);
    return tex;
}

static void setup(shades_data_t *data) {
    data->vert[0] = VECT2(-1, -1);
    data->vert[1] = VECT2(1, -1);
    data->vert[2] = VECT2(1, 1);
    data->vert[3] = VECT2(-1, 1);
    
    gl_ortho(data->proj, 0, 0, data->size.x, data->size.y);
    
    glGenVertexArrays(1, &data->vao);
    glBindVertexArray(data->vao);
    
    glGenBuffers(1, &data->vbo);
    glGenBuffers(1, &data->ebo);
    
    static const GLuint indices[] = {0, 1, 2, 0, 2, 3};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data->vert), data->vert, GL_STATIC_DRAW);
}

static void fetch_shader_info(shades_data_t *data) {
    glUseProgram(data->shader.prog);
    glBindVertexArray(data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
    
    data->shader.attr.vtx_pos = glGetAttribLocation(data->shader.prog, "in_vtx_pos");
    data->shader.uniform.pvm = glGetUniformLocation(data->shader.prog, "u_pvm");
    data->shader.uniform.tex0 = glGetUniformLocation(data->shader.prog, "u_tex0");
    data->shader.uniform.tex1 = glGetUniformLocation(data->shader.prog, "u_tex1");
    data->shader.uniform.tex2 = glGetUniformLocation(data->shader.prog, "u_tex2");
    data->shader.uniform.tex3 = glGetUniformLocation(data->shader.prog, "u_tex3");
    data->shader.uniform.tex_res = glGetUniformLocation(data->shader.prog, "u_tex_res");
    data->shader.uniform.res = glGetUniformLocation(data->shader.prog, "u_res");
    data->shader.uniform.time = glGetUniformLocation(data->shader.prog, "u_time");
    data->shader.uniform.scale = glGetUniformLocation(data->shader.prog, "u_scale");
    
    glEnableVertexAttribArray(data->shader.attr.vtx_pos);
    glVertexAttribPointer(data->shader.attr.vtx_pos, 2, GL_FLOAT, GL_FALSE, sizeof(vect2_t), (void*)0);
}

static void run_loop(const shades_data_t *data) {
    glBindVertexArray(data->vao);
    glUseProgram(data->shader.prog);
    
    glUniformMatrix4fv(data->shader.uniform.pvm, 1, GL_TRUE, data->proj);
    glUniform1i(data->shader.uniform.tex0, 0);
    glUniform1i(data->shader.uniform.tex1, 1);
    glUniform1i(data->shader.uniform.tex2, 2);
    glUniform1i(data->shader.uniform.tex3, 3);
    
    glUniform2fv(data->shader.uniform.res, 1, (const float *)&data->size);
    glUniform1f(data->shader.uniform.time, (float)glfwGetTime());
    glUniform1f(data->shader.uniform.scale, data->scale);
    
    // printf("size: %.0fx%.0f (%.0fX)\n", data->size.x, data->size.y, data->scale);
    
    vect2_t tex_res[MAX_TEXTURES];
    
    for(int i = 0; i < MAX_TEXTURES; ++i) {
        glActiveTexture(GL_TEXTURE0+i);
        glBindTexture(GL_TEXTURE_2D, data->textures[i].tex);
        tex_res[i] = data->textures[i].size;
    }
    
    glUniform2fv(data->shader.uniform.tex_res, MAX_TEXTURES, (const float *)tex_res);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

static void usage(const char *prog, FILE *out, bool detailed) {
    fprintf(out, "Usage: %s [-h] [-s <size>] <shader.glsl> [<texture.png>...]\n", prog);
    if(!detailed) return;
    
#ifdef __APPLE__
#define CMD_CHAR "⌘"
#else
#define CMD_CHAR "^"
#endif
    
    fprintf(out,
    "\n"
    "Shortcuts\n"
    "  %sR      reload currently loaded shaders and textures.\n"
    "  %s+      increase the zoom level by 1.\n"
    "  %s-      decrease the zoom level by 1.\n"
    "\n"
    "Examples\n"
    "  Run the `crt.glsl' fragment shader, with `image.png'\n"
    "  in u_tex0 and `mask.png' in u_tex1. Start with a\n"
    "  1024x800-point window.\n"
    "\n"
    "  $ %s crt.glsl -s 1024x800 image.png mask.png\n"
    "\n"
    "Options\n"
    " -s <size> specify a starting window size in points.\n"
    " -h        shows this help screen and exists.\n",
    CMD_CHAR,CMD_CHAR,CMD_CHAR,prog);
    
}

// static void resize_callback(GLFWwindow* window, int width, int height) {
//     shades_data_t *data = glfwGetWindowUserPointer(window);
//     data->size = VECT2(width, height);
// }

static void framebuffer_callback(GLFWwindow *window, int width, int height) {
    shades_data_t *data = glfwGetWindowUserPointer(window);
    data->size = VECT2(width, height);
    glViewport(0, 0, width, height);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action != GLFW_PRESS) return;
    if(!(mods & (GLFW_MOD_SUPER|GLFW_MOD_CONTROL))) return;
    
    (void)scancode;
    
    shades_data_t *data = glfwGetWindowUserPointer(window);
    switch(key) {
    case GLFW_KEY_R:
        data->shader.prog = reload_shader(data->shader.prog, data->shader.path);
        fetch_shader_info(data);
        for(int i = 0; i < MAX_TEXTURES; ++i) {
            const char *path = data->textures[i].path;
            if(!path) continue;
            data->textures[i].tex = reload_texture(data->textures[i].tex, path, &data->textures[i].size);
        }
        break;
        
    case GLFW_KEY_EQUAL:
        data->scale += 1.f;
        break;
        
    case GLFW_KEY_MINUS:
        data->scale -= 1.f;
        if(data->scale < 1.f) data->scale = 1.f;
        break;
    default: break;
    }
}

static void exit_usage(const char *prog, const char *message) {
    fprintf(stderr, "error: %s\n", message);
    usage(prog, stderr, false);
    exit(EXIT_FAILURE);
}

static bool parse_size(char *arg, double *width, double *height) {
    char *sep = strchr(arg, 'x');
    
    if(!sep) return false;
    assert(width && height);
    
    *sep = '\0';
    const char *w = arg;
    const char *h = sep+1;
    *width = atof(w);
    *height = atof(h);
    return true;
}

int main(int argc, char *args[]) {
    (void)argc;
    (void)args;
    
    if(argc < 2) exit_usage(args[0], "wrong number of arguments");
    
    
    double width = NAN;
    double height = NAN;
    const char *shader_path = NULL;
    const char *tex_path[MAX_TEXTURES] = {NULL};
    
    int num_tex = 0;
    
    opterr = 0;
    int c = '\0';
    
    while((c = getopt(argc, args, "s:h")) != -1) {
        switch(c) {
            case 's':
                if(!parse_size(optarg, &width, &height)) {
                    exit_usage(args[0], "invalid window size format");
                }
                break;
                
            case 'h':
                usage(args[0], stdout, true);
                exit(EXIT_SUCCESS);
                break;
        
            case '?':
                exit_usage(args[0], "unknown argument");
                break;
            default:
                break;
        }
    }
    
    if(isnan(width) && isnan(height)) {
        width = WIDTH;
        height = HEIGHT;
    } else if(isnan(width)) {
        exit_usage(args[0], "height specified without width");
    } else if(isnan(height)) {
        exit_usage(args[0], "width specified without height");
    }
    
    int count = argc - optind;
    num_tex = count - 1;
    
    if(count < 1) {
        exit_usage(args[0], "missing shader path");
    }
    
    if(count > 1 + MAX_TEXTURES) {
        exit_usage(args[0], "too many texture channels");
    }
    
    shader_path = args[optind];
    
    for(int i = 0; i < num_tex; ++i) {
        tex_path[i] = args[optind+1+i];
    }
    
    // Create our window
    if(!glfwInit()) die("could not initialise window system");
    glfwSetErrorCallback(glfw_error);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    GLFWwindow *window = glfwCreateWindow(width, height, NAME, NULL, NULL);
    if(!window) die("could not create application window");
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, 200, 200, GLFW_DONT_CARE, GLFW_DONT_CARE);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    CHECK_GL();
    
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    
    shades_data_t data = {
        .shader = {.prog = reload_shader(0, shader_path), .path = shader_path},
        .size = VECT2(w, h),
        .scale = (float)w/(float)height,
    };
    
    
    for(int i = 0; i < num_tex && i < MAX_TEXTURES; ++i) {
        const char *path = tex_path[i];
        data.textures[i].path = path;
        glActiveTexture(GL_TEXTURE0+i);
        data.textures[i].tex = reload_texture(0, path, &data.textures[i].size);
    }
    
    setup(&data);
    fetch_shader_info(&data);
    glfwSetWindowUserPointer(window, &data);
    // glfwSetWindowSizeCallback(window, resize_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_callback);
    
    
    // Main Loop
    while(!glfwWindowShouldClose(window)) {
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glViewport(0, 0, WIDTH*SCALE, HEIGHT*SCALE);
        
        run_loop(&data);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // end window loop
    glfwDestroyWindow(window);
}
