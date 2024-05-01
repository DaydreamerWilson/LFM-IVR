#ifndef mlgi_gui
#define mlgi_gui

// Defining graphic libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Defining gui widgets libraries

#define NK_PRIVATE
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"

#define NK_SHADER_VERSION "#version 150\n"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

struct device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
};

struct nk_glfw_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

static void text_input(GLFWwindow *win, unsigned int codepoint);
static void scroll_input(GLFWwindow *win, double _, double yoff);

static void GLFW_init(GLFWwindow* &win, struct nk_context &ctx, int &width, int &height);
static void device_init(struct device *dev);
static void device_upload_atlas(struct device *dev, const void *image, int width, int height);

static void die(const char *fmt, ...);
static struct nk_image load_image(const char *filename);

static void pump_input(struct nk_context *ctx, GLFWwindow *win);
static void device_draw(struct device *dev, struct nk_context *ctx, int width, int height, enum nk_anti_aliasing AA);
static void device_shutdown(struct device *dev);
static void close_all(struct nk_font_atlas &atlas, struct nk_context &ctx, struct device &device);

#include "mlgi_include.h"

void copyFile(char *src, char *dst);
void gui_init();
void gui_loop(_network *network, _gui *gui);
int gui_shouldClose();
void gui_terminate();

#endif