#include "mlgi_gui.h"
#include "mlgi_include.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#endif

#define APPLICATION_NAME "Microscopy Lightweight Graphical Interface v1.2.2 | Unstable build 240406"

static void device_init(struct device *dev)
{
    GLint status;
    static const GLchar *vertex_shader =
        NK_SHADER_VERSION
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 TexCoord;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main() {\n"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";
    static const GLchar *fragment_shader =
        NK_SHADER_VERSION
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main(){\n"
        "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    nk_buffer_init_default(&dev->cmds);
    dev->prog = glCreateProgram();
    dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
    glCompileShader(dev->vert_shdr);
    glCompileShader(dev->frag_shdr);
    glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glAttachShader(dev->prog, dev->vert_shdr);
    glAttachShader(dev->prog, dev->frag_shdr);
    glLinkProgram(dev->prog);
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = glGetAttribLocation(dev->prog, "Color");

    {
        /* buffer setup */
        GLsizei vs = sizeof(struct nk_glfw_vertex);
        size_t vp = offsetof(struct nk_glfw_vertex, position);
        size_t vt = offsetof(struct nk_glfw_vertex, uv);
        size_t vc = offsetof(struct nk_glfw_vertex, col);

        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void device_upload_atlas(struct device *dev, const void *image, int width, int height)
{
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
}

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

static void text_input(GLFWwindow *win, unsigned int codepoint)
{nk_input_unicode((struct nk_context*)glfwGetWindowUserPointer(win), codepoint);}
static void scroll_input(GLFWwindow *win, double _, double yoff)
{nk_input_scroll((struct nk_context*)glfwGetWindowUserPointer(win), nk_vec2(0, (float)yoff));}


static void GLFW_init(GLFWwindow* &win, struct nk_context &ctx, int &width, int &height){
    if (!glfwInit()) {
        fprintf(stdout, "[GFLW] failed to init!\n");
        exit(1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APPLICATION_NAME, NULL, NULL);
    glfwMakeContextCurrent(win);
    glfwSetWindowUserPointer(win, &ctx);
    glfwSetCharCallback(win, text_input);
    glfwSetScrollCallback(win, scroll_input);
    glfwGetWindowSize(win, &width, &height);

    /* OpenGL */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK) {
        printf("Failed to setup GLEW\n");
        exit(1);
    }
}

static void device_shutdown(struct device *dev){
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

static void close_all(struct nk_font_atlas &atlas, struct nk_context &ctx, struct device &device){
    nk_font_atlas_clear(&atlas);
    nk_free(&ctx);
    device_shutdown(&device);
    glfwTerminate();
}

static void pump_input(struct nk_context *ctx, GLFWwindow *win){
    double x, y;
    nk_input_begin(ctx);
    glfwPollEvents();

    nk_input_key(ctx, NK_KEY_DEL, glfwGetKey(win, GLFW_KEY_DELETE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_ENTER, glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TAB, glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_BACKSPACE, glfwGetKey(win, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_UP, glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_DOWN, glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS);

    if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        nk_input_key(ctx, NK_KEY_COPY, glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_PASTE, glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_CUT, glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_CUT, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_SHIFT, 1);
    } else {
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
        nk_input_key(ctx, NK_KEY_SHIFT, 0);
    }

    glfwGetCursorPos(win, &x, &y);
    nk_input_motion(ctx, (int)x, (int)y);
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    nk_input_end(ctx);
}

static void device_draw(struct device *dev, struct nk_context *ctx, int width, int height, enum nk_anti_aliasing AA){
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)width;
    ortho[1][1] /= (GLfloat)height;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        const nk_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_MEMORY, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_ELEMENT_MEMORY, NULL, GL_STREAM_DRAW);

        /* load draw vertices & elements directly into vertex + element buffer */
        vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };
            NK_MEMSET(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_glfw_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_glfw_vertex);
            config.null = dev->null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            {struct nk_buffer vbuf, ebuf;
            nk_buffer_init_fixed(&vbuf, vertices, MAX_VERTEX_MEMORY);
            nk_buffer_init_fixed(&ebuf, elements, MAX_ELEMENT_MEMORY);
            nk_convert(ctx, &dev->cmds, &vbuf, &ebuf, &config);}
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

      /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x),
                (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h))),
                (GLint)(cmd->clip_rect.w),
                (GLint)(cmd->clip_rect.h));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(ctx);
    }

    /* default OpenGL state */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

static void die(const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

static struct nk_image load_image(const char *filename){
    //printf("Loading image at: %s\n", filename);
    int x,y,n;	
    GLuint tex;	
    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);	
    if (!data){return nk_image_id(0);}	

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);	
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);	
    return nk_image_id((int)tex);
}

static GLFWwindow *win;
struct nk_context ctx;
struct device device;
struct nk_font_atlas atlas;

int width = 0, height = 0;

struct nk_image displayImage;
int imageSaved = true;

void gui_init(){
    GLFW_init(win, ctx, width, height);
    device_init(&device);

    GLFWimage images;
    images.pixels = stbi_load("cache/perm/icon.png", &images.width, &images.height, 0, 4); //rgba channels 
    glfwSetWindowIcon(win, 1, &images); 
    stbi_image_free(images.pixels);
    
    const void *image;
    struct nk_font *font;
    int w, h;

    nk_font_atlas_init_default(&atlas);
    nk_font_atlas_begin(&atlas);
    font = nk_font_atlas_add_default(&atlas, 13, 0);
    image = nk_font_atlas_bake(&atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    device_upload_atlas(&device, image, w, h);
    nk_font_atlas_end(&atlas, nk_handle_id((int)device.font_tex), &device.null);
    nk_init_default(&ctx, &font->handle);
}

void gui_loop(_network *network, _gui *gui){
    pump_input(&ctx, win);

    if (nk_begin(&ctx, "Connection", nk_rect(0, 0, 400, 52),
        NK_WINDOW_BORDER)) {
        // fixed widget pixel width
        nk_layout_row_begin(&ctx, NK_STATIC, 30, 3);
        {   
            nk_layout_row_push(&ctx, 100);
            nk_label(&ctx, "IP Address:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 130);
            nk_edit_string_zero_terminated(&ctx, NK_EDIT_FIELD, network->input.addrBuf, sizeof(network->input.addrBuf) - 1, nk_filter_default);
            nk_layout_row_push(&ctx, 100);
            char* charBuf;
            switch (network->output.connectStatus){
            case connected:
                charBuf = (char*)"Disconnect";
                break;
            case connecting:
                charBuf = (char*)"Connecting";
                network->input.connectCmd = doNothing;
                break;
            default:
                charBuf = (char*)"Connect";
                break;
            }
            if (nk_button_label(&ctx, charBuf)) {
                if(network->output.connectStatus == connected){
                    network->input.connectCmd = doDisconnect;
                }
                if(network->output.connectStatus == disconnected){
                    network->input.connectCmd = doConnect;
                }
            }
        }
        nk_layout_row_end(&ctx);
    }
    nk_end(&ctx);

    if (nk_begin(&ctx, "Camera", nk_rect(0, 52, 400, 220),
        NK_WINDOW_BORDER)) {
        nk_layout_row_begin(&ctx, NK_STATIC, 20, 2);
        {
            nk_layout_row_push(&ctx, 180);
            if(nk_button_label(&ctx, gui->output.getStream?"Stream on ":"Stream off")){
                if(!gui->output.timeLapseOn){
                    gui->output.getStream = !gui->output.getStream;
                }
            }
            nk_layout_row_push(&ctx, 180);
            if(nk_button_label(&ctx, "Get still")){
                gui->output.getStream = false;
                if(!gui->output.stillProgress){
                    gui->output.stillProgress = 1;
                }
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 30, 1);
        {
            nk_layout_row_push(&ctx, 364);
            if(nk_button_label(&ctx, "Save frame")){
                gui->output.saveFrame = true;
                if(!imageSaved){
                    imageSaved = true;
                    std::time_t t = std::time(0);
                    std::tm* now = std::localtime(&t);
                    char temp[48];
                    sprintf(temp, "images/still/%d-%d-%d-%d-%d-%d%s", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, imgFormat[gui->output.imgFmt]);
                    copyFile(network->output.cachePath, temp);
                }
            }
            else{
                gui->output.saveFrame = false;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 2);
        {   
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Average frame interval:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 180);
            char frameTemp[64];
            sprintf(frameTemp, "%d ms", network->output.IMGtick);
            nk_label(&ctx, frameTemp, NK_TEXT_LEFT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 2);
        {   
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Last frame interval:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 180);
            char frameTemp[64];
            sprintf(frameTemp, "%d ms", network->output.lastIMGtick);
            nk_label(&ctx, frameTemp, NK_TEXT_LEFT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 2);
        {   
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Timelapse interval:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 180);
            gui->output.timeLapse_t = nk_combo(&ctx, timelapse_interval, NK_LEN(timelapse_interval), gui->output.timeLapse_t, 20, nk_vec2(180,1000));
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 10, 1);
        {   
            nk_layout_row_push(&ctx, 360);
            nk_label(&ctx, "Note: Use at least 2s interval for QHD or higher", NK_TEXT_LEFT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 2);
        {   
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Start timelapse", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 180);
            if(nk_button_label(&ctx, gui->output.timeLapseOn?" Stop timelapse":"Start timelapse")){
                gui->output.timeLapseOn = !gui->output.timeLapseOn;
                if(gui->output.timeLapseOn){
                    gui->output.getStream = true;
                }
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 2);
        {   
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Image format:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 180);
            gui->output.imgFmt = nk_combo(&ctx, imgFormat, NK_LEN(imgFormat), gui->output.imgFmt, 20, nk_vec2(180, height-300));
        }
        nk_layout_row_end(&ctx);
    }
    nk_end(&ctx);
    
    if (nk_begin(&ctx, "Options", nk_rect(0, 272, 400, height-272),
        NK_WINDOW_BORDER)) {
        // Options
        
        nk_layout_row_begin(&ctx, NK_STATIC, 20, 2);
        {   
            //printf("Resolution: %d\n", network->input.frameSize);
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Resolution:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 180);
            int temp = network->input.frameSize;
            network->input.frameSize = nk_combo(&ctx, framesizes, NK_LEN(framesizes), network->input.frameSize, 20, nk_vec2(180, height-272-50));
            if(network->input.frameSize!=temp){
                network->input.set_dcw = 1;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 4);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Brightness:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "-2", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            nk_slider_int(&ctx, -2, &network->input.brightness, 2, 1);
            nk_layout_row_push(&ctx, 5);
            nk_label(&ctx, "2", NK_TEXT_RIGHT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 4);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Contrast:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "-2", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            nk_slider_int(&ctx, -2, &network->input.contrast, 2, 1);
            nk_layout_row_push(&ctx, 5);
            nk_label(&ctx, "2", NK_TEXT_RIGHT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 4);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Saturation:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "-2", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            nk_slider_int(&ctx, -2, &network->input.saturation, 2, 1);
            nk_layout_row_push(&ctx, 5);
            nk_label(&ctx, "2", NK_TEXT_RIGHT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 2);
        {   
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Special effect:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 180);
            network->input.special_effect = nk_combo(&ctx, effects, NK_LEN(effects), network->input.special_effect, 20, nk_vec2(180, height-272-50-80));
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "White balance:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.whitebal?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.whitebal=!network->input.whitebal;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Auto white balance(AWB):", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.awb_gain?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.awb_gain=!network->input.awb_gain;
                if(!network->input.awb_gain){
                    network->input.wb_mode = 0;
                }
            }
        }
        nk_layout_row_end(&ctx);
        
        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {   
            if(network->input.awb_gain){
                nk_layout_row_push(&ctx, 30);
                nk_label(&ctx, ">", NK_TEXT_CENTERED);
                nk_layout_row_push(&ctx, 150);
                nk_label(&ctx, "AWB mode:", NK_TEXT_LEFT);
                nk_layout_row_push(&ctx, 180);
                network->input.wb_mode = nk_combo(&ctx, awbMode, NK_LEN(awbMode), network->input.wb_mode, 20, nk_vec2(180, height-272-50-160));
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Exposure control:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.exposure_ctrl?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.exposure_ctrl=!network->input.exposure_ctrl;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Auto exposure control:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.aec2?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.aec2=!network->input.aec2;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 4);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Auto exposure level:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "-2", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            nk_slider_int(&ctx, -2, &network->input.ae_level, 2, 1);
            nk_layout_row_push(&ctx, 5);
            nk_label(&ctx, "2", NK_TEXT_RIGHT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 4);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "AEC value:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "0", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            nk_slider_int(&ctx, 0, &network->input.aec_value, 1200, 5);
            nk_layout_row_push(&ctx, 5);
            nk_label(&ctx, "1200", NK_TEXT_RIGHT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 165);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[4];
            itoa(network->input.aec_value, buf, 10);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Gain control:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.gain_ctrl?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.gain_ctrl=!network->input.gain_ctrl;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 4);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Auto gain control:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "0", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            nk_slider_int(&ctx, 0, &network->input.agc_gain, 30, 1);
            nk_layout_row_push(&ctx, 5);
            nk_label(&ctx, "30", NK_TEXT_RIGHT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 165);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[4];
            itoa(network->input.agc_gain, buf, 10);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 4);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Gain ceiling:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "0", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            nk_slider_int(&ctx, 0, &network->input.gainceiling, 6, 1);
            nk_layout_row_push(&ctx, 5);
            nk_label(&ctx, "6", NK_TEXT_RIGHT);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Black pixel correction:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.bpc?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.bpc=!network->input.bpc;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "White pixel correction:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.wpc?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.wpc=!network->input.wpc;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Raw gamma mode:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.raw_gma?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.raw_gma=!network->input.raw_gma;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Lens correction:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.lenc?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.lenc=!network->input.lenc;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Horizontal mirror:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.hmirror?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.hmirror=!network->input.hmirror;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Vertical flip:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.vflip?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.vflip=!network->input.vflip;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 1);
        {
            nk_layout_row_push(&ctx, 350);
            nk_label(&ctx, "DCW will scale image to conform to set resolution", NK_TEXT_LEFT);
        }

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {   
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 165);
            nk_label(&ctx, "DCW (Downsize EN):", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.set_dcw?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.set_dcw=!network->input.set_dcw;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Color bar:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, network->input.colorbar?"TOGGLED ON ":"TOGGLED OFF")){
                network->input.colorbar=!network->input.colorbar;
            }
        }
        nk_layout_row_end(&ctx);
    }
    nk_end(&ctx);

    if (nk_begin(&ctx, "Auxiliary", nk_rect(width-500, 0, 100, height),
        NK_WINDOW_BORDER)) {
        
    }
    nk_end(&ctx);
    
    if (nk_begin(&ctx, "Photos", nk_rect(400, 0, width-500, height),
        NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT)) {
        struct nk_command_buffer *canvas = nk_window_get_canvas(&ctx);
        const struct nk_color grid_color = nk_rgba(255, 255, 255, 255);
        if(gui->output.IMGcounter != network->output.IMGcounter){
            imageSaved = false;
            displayImage = load_image(network->output.cachePath);
            if(displayImage.handle.id == 0){printf("Error loading image\n");}
            gui->output.IMGcounter = network->output.IMGcounter;
        }
        if(displayImage.handle.id != 0){
            int h = rs_table[network->input.frameSize].h;
            int w = rs_table[network->input.frameSize].w;
            struct nk_rect window = nk_rect(400, 0, width-500, height);
            struct nk_rect displayWindow;
            {
                if(((float)h/(float)w)>((float)window.h/(float)window.w)){
                    displayWindow.h = window.h;
                    displayWindow.w = (float)w/(float)h*(float)window.h;
                    displayWindow.x = (float)(window.w-displayWindow.w)/2+window.x;
                    displayWindow.y = 0;
                }
                else{
                    displayWindow.w = window.w;
                    displayWindow.h = (float)h/(float)w*(float)window.w;
                    displayWindow.x = 400;
                    displayWindow.y = (float)(window.h-displayWindow.h)/2+window.y;
                }
            }
            nk_draw_image(canvas, displayWindow, &displayImage, grid_color);
        }
    }
    nk_end(&ctx);

    glfwGetWindowSize(win, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    device_draw(&device, &ctx, width, height, NK_ANTI_ALIASING_ON);
    glfwSwapBuffers(win);
}

int gui_shouldClose(){
    return glfwWindowShouldClose(win);
}

void gui_terminate(){
    close_all(atlas, ctx, device);
}