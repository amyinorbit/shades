//===--------------------------------------------------------------------------------------------===
// shades.c - Small shader runner/tester
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2022 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
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
        GLuint      vtx_tex0;
    } attr;
    
    struct {
        GLuint      pvm;
        GLuint      tex0;
        GLuint      tex1;
        GLuint      tex2;
        GLuint      res;
        GLuint      time;
    } uniform;
} shader_info_t;

typedef struct {
    GLuint          tex;
    const char      *path;
} texture_info_t;

typedef struct {
    vect2_t         pos;
    vect2_t         tex0;
} vertex_t;

typedef struct {
    shader_info_t   shader;
    texture_info_t  textures[MAX_TEXTURES];
    
    vect2_t         size;
    
    GLuint          vao;
    GLuint          vbo;
    GLuint          ebo;
    
    vertex_t        vert[4];
    GLuint          indices[6];
    
    
    float           proj[16];
} shades_data_t;

static const char *vert_shader =
    "#version 400\n"
    "uniform mat4   u_pvm;\n"
    "in vec2        in_vtx_pos;\n"
    "in vec2        in_vtx_tex0;\n"
    "out vec2       tex_coord;\n"
    "void main() {\n"
    "    tex_coord = in_vtx_tex0;\n"
    "    gl_Position = u_pvm * vec4(in_vtx_pos, 0.0, 1.0);\n"
    "}\n";

static const char *frag_defines =
    "#version 400\n"
    "uniform sampler2D  u_tex0;\n"
    "uniform sampler2D  u_tex1;\n"
    "uniform sampler2D  u_tex2;\n"
    "uniform vec2       u_res;\n"
    "uniform float      u_time;\n"
    "\n"
    "in vec2            tex_coord;\n"
    "out vec4           out_color;\n"  
    "\n";

static const char *frag_shader =
    "void main() {\n"
    "    out_color = main_image(tex_coord);\n"
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

static GLuint reload_texture(GLuint tex, const char *path) {
    if(tex) {
        glDeleteTextures(1, &tex);
        tex = 0;
    }
    
    int w, h;
    tex = gl_load_tex(path, &w, &h);
    return tex;
}

static void setup(shades_data_t *data) {
    data->vert[0].pos = VECT2(0, 0);
    data->vert[0].tex0 = VECT2(0, 0);
    
    data->vert[1].pos = VECT2(data->size.x, 0);
    data->vert[1].tex0 = VECT2(1, 0);
    
    data->vert[2].pos = VECT2(data->size.x, data->size.y);
    data->vert[2].tex0 = VECT2(1, 1);
    
    data->vert[3].pos = VECT2(0, data->size.y);
    data->vert[3].tex0 = VECT2(0, 1);
    
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
    
    glUseProgram(data->shader.prog);
    
    data->shader.attr.vtx_pos = glGetAttribLocation(data->shader.prog, "in_vtx_pos");
    data->shader.attr.vtx_tex0 = glGetAttribLocation(data->shader.prog, "in_vtx_tex0");
    
    data->shader.uniform.pvm = glGetUniformLocation(data->shader.prog, "u_pvm");
    data->shader.uniform.tex0 = glGetUniformLocation(data->shader.prog, "u_tex0");
    data->shader.uniform.tex1 = glGetUniformLocation(data->shader.prog, "u_tex1");
    data->shader.uniform.tex2 = glGetUniformLocation(data->shader.prog, "u_tex2");
    data->shader.uniform.res = glGetUniformLocation(data->shader.prog, "u_res");
    data->shader.uniform.time = glGetUniformLocation(data->shader.prog, "u_time");
    
    glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
    glEnableVertexAttribArray(data->shader.attr.vtx_pos);
    glEnableVertexAttribArray(data->shader.attr.vtx_tex0);
    
    glVertexAttribPointer(data->shader.attr.vtx_pos, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void *)offsetof(vertex_t, pos));
    glVertexAttribPointer(data->shader.attr.vtx_tex0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void *)offsetof(vertex_t, tex0));
}

static void run_loop(const shades_data_t *data) {
    glBindVertexArray(data->vao);
    glUseProgram(data->shader.prog);
    
    glUniformMatrix4fv(data->shader.uniform.pvm, 1, GL_TRUE, data->proj);
    glUniform1i(data->shader.uniform.tex0, 0);
    glUniform1i(data->shader.uniform.tex1, 1);
    glUniform1i(data->shader.uniform.tex2, 2);
    
    glUniform2fv(data->shader.uniform.res, 1, (const float *)&data->size);
    glUniform1f(data->shader.uniform.time, (float)glfwGetTime());
    
    for(int i = 0; i < MAX_TEXTURES; ++i) {
        glActiveTexture(GL_TEXTURE0+i);
        glBindTexture(GL_TEXTURE_2D, data->textures[i].tex);
    }
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

static void usage(const char *prog, FILE *out) {
    fprintf(out, "usage: %s shader [images...]\n", prog);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action != GLFW_PRESS) return;
    if(!(mods & (GLFW_MOD_SUPER|GLFW_MOD_CONTROL))) return;
    
    (void)scancode;
    
    shades_data_t *data = glfwGetWindowUserPointer(window);
    switch(key) {
        case GLFW_KEY_R:
        data->shader.prog = reload_shader(data->shader.prog, data->shader.path);
        
        for(int i = 0; i < MAX_TEXTURES; ++i) {
            const char *path = data->textures[i].path;
            if(!path) continue;
            data->textures[i].tex = reload_texture(data->textures[i].tex, path);
        }
        break;
        default: break;
    }
}

int main(int argc, const char **args) {
    (void)argc;
    (void)args;
    
    if(argc < 2) {
        fprintf(stderr, "error: invalid arguments\n");
        usage(args[0], stderr);
        return -1;
    }
    
    if(argc > 2 + MAX_TEXTURES) {
        fprintf(stderr, "error: too many texture channels\n");
        usage(args[0], stderr);
        return -1;
    }
    
    const char *shader_path = args[1];
    
    // Create our window
    if(!glfwInit()) die("could not initialise window system");
    glfwSetErrorCallback(glfw_error);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, NAME, NULL, NULL);
    if(!window) die("could not create application window");
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    CHECK_GL();

    shades_data_t data = {
        .shader = {.prog = reload_shader(0, shader_path), .path = shader_path},
        .size = VECT2(WIDTH, HEIGHT),
    };
    setup(&data);
    glfwSetWindowUserPointer(window, &data);
    glfwSetKeyCallback(window, key_callback);
    
    for(int i = 0; i < argc - 2 && i < MAX_TEXTURES; ++i) {
        const char *path = args[2+i];
        data.textures[i].path = path;
        data.textures[i].tex = reload_texture(0, path);
    }
    
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
