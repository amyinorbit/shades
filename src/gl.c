#include "gl.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

_Noreturn void die(const char *msg) {
    fprintf(stderr, "fatal error: %s\n", msg);
    abort();
}

bool gl_check_shader(GLuint sh) {
    GLint is_compiled = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &is_compiled);
    if(is_compiled == GL_TRUE) return true;

    GLint length = 0;
    glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &length);

    char log[length + 1];
    glGetShaderInfoLog(sh, length, &length, log);
    log[length] = '\0';
    fprintf(stderr, "shader compile error: %s", log);
    return false;
}

bool gl_check_program(GLuint sh) {
    GLint is_compiled = 0;
    glGetProgramiv(sh, GL_LINK_STATUS, &is_compiled);
    if(is_compiled == GL_TRUE) return true;

    GLint length = 0;
    glGetProgramiv(sh, GL_INFO_LOG_LENGTH, &length);

    char log[length + 1];
    glGetProgramInfoLog(sh, length, &length, log);
    log[length] = '\0';
    fprintf(stderr, "shader link error: %s", log);
    return false;
}

GLuint gl_create_program(const char *vertex, const char *fragment) {
    assert(vertex);
    assert(fragment);

    GLuint vert = gl_load_shader(GL_VERTEX_SHADER, vertex);
    if(!vert) return 0;

    GLuint frag = gl_load_shader(GL_FRAGMENT_SHADER, fragment);
    if(!frag) return 0;

    GLuint prog = glCreateProgram();
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

char *load_source(const char *path) {
    assert(path);
    
    FILE *f = fopen(path, "rb");
    if(!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *source = calloc(len+1, sizeof(char));
    
    size_t r = fread(source, 1, len, f);
    if(r != len) {
        free(source);
        source = NULL;
    } else {
        source[len] = '\0';
    }
    
    fclose(f);
    return source;
}

char *load_sourcef(const char *fmt, ...) {
    va_list args, args_copy;
    
    va_start(args, fmt);
    va_copy(args_copy, args);
    
    size_t len = vsnprintf(NULL, 0, fmt, args_copy);
    
    char *path = calloc(len + 1, sizeof(char));
    vsnprintf(path, len+1, fmt, args);
    
    char *source = load_source(path);
    
    free(path);
    va_end(args);
    va_end(args_copy);
    return source;
}

GLuint gl_load_shader(GLenum type, ...) {
    assert(type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER);
    
    GLsizei num_sources = 0;
    
    va_list ap1, ap2;
    va_start(ap1, type);
    va_copy(ap2, ap1);
    
    for(const char *s = va_arg(ap2, const char *); s != NULL; s = va_arg(ap2, const char *)) {
        num_sources += 1;
    }
    va_end(ap2);
    
    const char *sources[num_sources];
    GLint lengths[num_sources];
    
    for(GLsizei i = 0; i < num_sources; ++i) {
        sources[i] = va_arg(ap1, const char *);
        lengths[i] = strlen(sources[i]);
    }
    va_end(ap1);
    
    
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, num_sources, sources, lengths);
    glCompileShader(sh);
    if(!gl_check_shader(sh)) {
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

GLuint gl_create_tex(unsigned width, unsigned height) {
    assert(width > 0);
    assert(height > 0);
    
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    return tex;
}

GLuint gl_load_tex(const char *path, int *w, int *h) {
    // stbi_set_flip_vertically_on_load(true);
    int components = 0;
    uint8_t *data = stbi_load(path, w, h, &components, 0);
    if(!data) {
        fprintf(stderr, "unable to load image `%s`\n", path);
        return 0;
    }
    if(components != 4 && components != 3) {
        fprintf(stderr, "image `%s` does not have the right format\n", path);
        free(data);
        return 0;
    }

    GLuint tex = gl_create_tex(*w, *h);
    glBindTexture(GL_TEXTURE_2D, tex);
    if(components == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *w, *h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, *w, *h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    
    free(data);

    fprintf(stderr, "loaded texture `%s` (%dx%d)\n", path, *w, *h);
    return tex;
}

void gl_ortho(float proj[16], float x, float y, float width, float height) {
    assert(proj);
    // float x_max = (x+width) -1;
    // float y_max = (y+height) -1;
    float z_near = 1.0;
    float z_far = -1.0;
    
    float left = x;
    float right = x + width;
    float top = y;
    float bottom = y + height;

    proj[0] = 2.f / (right-left);
    proj[1] = 0.f;
    proj[2] = 0.f;
    proj[3] = - (right + left) / (right - left);

    proj[4] = 0.f;
    proj[5] = 2.f / (top-bottom);
    proj[6] = 0.f;
    proj[7] = - (top + bottom) / (top - bottom);

    proj[8] = 0.f;
    proj[9] = 0.f;
    proj[10] = -2.f / (z_far-z_near);
    proj[11] = (z_near+z_far)/(z_near-z_far);

    proj[12] = 0.f;
    proj[13] = 0.f;
    proj[14] = 0.f;
    proj[15] = 1.f;
}

void check_gl(const char *where, int line) {
    GLenum error = glGetError();
    if(error == GL_NO_ERROR) return;
    fprintf(stderr, "%s() OpenGL error code 0x%04x line %d\n", where, error, line);
}
