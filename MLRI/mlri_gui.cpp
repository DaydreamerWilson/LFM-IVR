#include "mlri_gui.h"
#include "mlri_include.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

#define APPLICATION_NAME "Microlens Lightwight Reconstruction Interface v1.0.0 | Unstable build 240421"

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

stbi_uc *data;

static struct nk_image load_image(const char *filename, int &x, int &y){
    //printf("Loading image at: %s\n", filename);
    int n;	
    GLuint tex;	
    delete[] data;
    data = stbi_load(filename, &x, &y, &n, STBI_rgb_alpha);
    if (!data){return nk_image_id(0);}

    printf("Loading image: %d %d %d\n", x, y, n);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);	
    glGenerateMipmap(GL_TEXTURE_2D);
    return nk_image_id((int)tex);
}

stbi_uc *overlay;
int overlay_h = 0;
int overlay_w = 0;

enum{DW_MODE_REPLACE, DW_MODE_ADDITION, DW_MODE_SUBTRACTION, DW_MODE_MULTIPLY};

// Warning: This function can result in segementation fault if x, y is out of bound, use drawPixel() if you have no bound check
static int assignPixel(int x, int y, int R, int G, int B, int A, int mode){
    int tR, tG, tB, tA;
    if(mode == DW_MODE_REPLACE){
        tR = R;
        tG = G;
        tB = B;
        tA = A;
    }
    if(mode == DW_MODE_ADDITION){
        tR = (int)overlay[4*(x+overlay_w*y)]+R;
        tG = (int)overlay[4*(x+overlay_w*y)+1]+G;
        tB = (int)overlay[4*(x+overlay_w*y)+2]+B;
        tA = (int)overlay[4*(x+overlay_w*y)+3]+A;
    }
    if(mode == DW_MODE_SUBTRACTION){
        tR = (int)overlay[4*(x+overlay_w*y)]-R;
        tG = (int)overlay[4*(x+overlay_w*y)+1]-G;
        tB = (int)overlay[4*(x+overlay_w*y)+2]-B;
        tA = (int)overlay[4*(x+overlay_w*y)+3]-A;
    }
    if(mode == DW_MODE_MULTIPLY){
        tR = (int)overlay[4*(x+overlay_w*y)]*R;
        tG = (int)overlay[4*(x+overlay_w*y)+1]*G;
        tB = (int)overlay[4*(x+overlay_w*y)+2]*B;
        tA = (int)overlay[4*(x+overlay_w*y)+3]*A;
    }
    // Overflow check
    if(tR>255){tR=255;}
    if(tG>255){tG=255;}
    if(tB>255){tB=255;}
    if(tA>255){tA=255;}
    // Underflow check
    if(tR<0){tR=0;}
    if(tG<0){tG=0;}
    if(tB<0){tB=0;}
    if(tA<0){tA=0;}
    // Assign value
    overlay[4*(x+overlay_w*y)] = tR;
    overlay[4*(x+overlay_w*y)+1] = tG;
    overlay[4*(x+overlay_w*y)+2] = tB;
    overlay[4*(x+overlay_w*y)+3] = tA;
    return true;
}

static char* _getPixel(int x, int y, char *image){
    if(x>=0 && x<overlay_w && y>=0 && y<overlay_h){
        return (char*)image+4*(x+overlay_w*y);
    }
    else{
        return NULL;
    }
}

static int getPixel(int x, int y, int &R, int &G, int &B, int &A){
    if(x>=0 && x<overlay_w && y>=0 && y<overlay_h){
        R = (int)overlay[4*(x+overlay_w*y)];
        G = (int)overlay[4*(x+overlay_w*y)+1];
        B = (int)overlay[4*(x+overlay_w*y)+2];
        A = (int)overlay[4*(x+overlay_w*y)+3];
        return true;
    }
    else{
        return false;
    }
}

static int drawPixel(int x, int y, int R, int G, int B, int A, int mode){
    if(x>=0 && x<overlay_w && y>=0 && y<overlay_h){
        assignPixel(x, y, R, G, B, A, mode);
        return true;
    }
    else{
        return false;
    }
}

static void drawSearch(int T_R, int T_G, int T_B, int T_A, int R, int G, int B, int A, int mode){
    intPosList temp;
    canvasSearch(T_R, T_G, T_B, T_A, &temp);
    int *pos = new int[2];
    while(temp.next(pos)){
        assignPixel(pos[0], pos[1], R, G, B, A, mode);
    }
}

static void canvasSearch(int T_R, int T_G, int T_B, int T_A, intPosList *input){
    for(int x = 0; x < overlay_w; x++){
        for(int y = 0; y < overlay_h; y++){
            if(
                overlay[4*(x+overlay_w*y)] == T_R &&
                overlay[4*(x+overlay_w*y)+1] == T_G &&
                overlay[4*(x+overlay_w*y)+2] == T_B &&
                overlay[4*(x+overlay_w*y)+3] == T_A
            ){
                input->push(x, y);
            }
        }
    }
    printf("Canvas search found %d matching pixels\n", input->counter);
}

static void drawSearchCircle(int T_R, int T_G, int T_B, int T_A, int r, int R, int G, int B, int A, int mode){
    intPosList temp;
    canvasSearch(T_R, T_G, T_B, T_A, &temp);
    int *pos = new int[2];
    while(temp.next(pos)){
        drawCircle(pos[0], pos[1], r, R, G, B, A, mode);
    }
}

static void drawSearchOval(int T_R, int T_G, int T_B, int T_A, int w, int h, int R, int G, int B, int A, int mode){
    intPosList temp;
    canvasSearch(T_R, T_G, T_B, T_A, &temp);
    int *pos = new int[2];
    while(temp.next(pos)){
        drawOval(pos[0], pos[1], w, h, R, G, B, A, mode);
    }
}

static int drawLine(int x0, int y0, int x1, int y1, int R, int G, int B, int A, int mode){
    float xd = x1-x0;
    float yd = y1-y0;

    float m = 0;
    float c;

    int inc = 1;
    int check = false;
    if(xd*xd>yd*yd){
        float m = yd/xd;
        c = y1-m*x1;
        if(x0>x1){inc = -1;}
        for(int x = x0; x!=x1; x+=inc){
            if(drawPixel(x, (int)round(m*x+c), R, G, B, A, mode)){
                check = true;
            }
        }
    }
    else{
        float m = xd/yd;
        c = x1-m*y1;
        if(y0>y1){inc = -1;}
        for(int y = y0; y!=y1; y+=inc){
            if(drawPixel((int)round(m*y+c), y, R, G, B, A, mode)){
                check = true;
            }
        }
    }
    return check;
}

static void drawCircle(int x, int y, int r, int R, int G, int B, int A, int mode){
    float l = 2*3.14159*r/4;
    int ld = (int)l+1;
    for(int k = 0; k<4*ld; k++){
        drawPixel((int)round(r*sin((float)k/2.0f/(float)ld*3.14159))+x, (int)round(r*cos((float)k/2.0f/(float)ld*3.14159))+y, R, G, B, A, mode);
    }
}

static void drawOval(int x, int y, int w, int h, int R, int G, int B, int A, int mode){
    float l = 3.14159*(w+h)/4;
    int ld = (int)l+1;
    for(int k = 0; k<4*ld; k++){
        drawPixel((int)round(w/2*sin((float)k/2.0f/(float)ld*3.14159))+x, (int)round(h/2*cos((float)k/2.0f/(float)ld*3.14159))+y, R, G, B, A, mode);
    }
}

static struct nk_image init_overlay(_gui *gui){
    overlay = (stbi_uc*)calloc(gui->output.imageh*gui->output.imagew*4, 1);
    overlay_h = gui->output.imageh;
    overlay_w = gui->output.imagew;

    GLuint tex;	
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gui->output.imagew, gui->output.imageh, 0, GL_RGBA, GL_UNSIGNED_BYTE, overlay);	
    glGenerateMipmap(GL_TEXTURE_2D);
    return nk_image_id((int)tex);

    gui->output.adjusted = true;
}

static struct nk_image update_overlay(_gui *gui){
    delete[] overlay;
    overlay = (stbi_uc*)calloc(gui->output.imageh*gui->output.imagew*4, 1);
    overlay_h = gui->output.imageh;
    overlay_w = gui->output.imagew;

    float x_step = (float)gui->output.imageh/4/gui->output.ypitch*gui->output.ystep;
    float ix = gui->output.xcenter_os+gui->output.xcenter;
    while(drawLine(ix-x_step, 0, ix+x_step, gui->output.imageh-1, 255, 50, 200, 255, DW_MODE_ADDITION)){
        ix -= gui->output.xspace;
    }
    ix = gui->output.xcenter_os+gui->output.xcenter+gui->output.xspace;
    while(drawLine(ix-x_step, 0, ix+x_step, gui->output.imageh-1, 255, 50, 200, 255, DW_MODE_ADDITION)){
        ix += gui->output.xspace;
    }

    float y_step = (float)gui->output.imagew/4/gui->output.xpitch*gui->output.xstep;
    float iy = gui->output.ycenter_os+gui->output.ycenter;
    while(drawLine(0, iy-y_step, gui->output.imagew-1, iy+y_step, 255, 50, 200, 255, DW_MODE_ADDITION)){
        iy -= gui->output.yspace;
    }
    iy = gui->output.ycenter_os+gui->output.ycenter+gui->output.yspace;
    while(drawLine(0, iy-y_step, gui->output.imagew-1, iy+y_step, 255, 50, 200, 255, DW_MODE_ADDITION)){
        iy += gui->output.yspace;
    }

    if(gui->output.full_preview){
        drawSearchOval(255, 100, 255, 255, gui->output.xpitch, gui->output.ypitch, 200, 50, 50, 200, DW_MODE_REPLACE);
    }
    drawOval(gui->output.xcenter_os+gui->output.xcenter, gui->output.ycenter_os+gui->output.ycenter,
            gui->output.xpitch, gui->output.ypitch,
            255,0,255,255, DW_MODE_REPLACE);

    GLuint tex;	
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gui->output.imagew, gui->output.imageh, 0, GL_RGBA, GL_UNSIGNED_BYTE, overlay);	
    glGenerateMipmap(GL_TEXTURE_2D);
    return nk_image_id((int)tex);
}

static void generateAngleMap(_gui *gui){
    intPosList absLensCenters;
    canvasSearch(255, 100, 255, 255, &absLensCenters);
    intPosList lensCenter;
    int *absPos = new int[2];
    int counter = 0; float min_x = 0; float max_x = 0; float min_y = 0; float max_y = 0;
    
    if(gui->output.xstep!=0 & gui->output.ystep!=0){
        float denom = (gui->output.xstep*gui->output.ystep)-(gui->output.xspace*gui->output.yspace);
        while(absLensCenters.next(absPos)){
            float abs_x = absPos[0]-(gui->output.xcenter_os+gui->output.xcenter);
            float abs_y = absPos[1]-(gui->output.ycenter_os+gui->output.ycenter);
            float x = (abs_y*gui->output.ystep-abs_x*gui->output.yspace)/denom;
            updateClamp(x, &max_x, &min_x);
            float y = (abs_x*gui->output.xstep-abs_y*gui->output.xspace)/denom;
            updateClamp(y, &max_y, &min_y);
            lensCenter.push(round(x), round(y));
        }
    }
    else{
        if(gui->output.ystep==0){
            while(absLensCenters.next(absPos)){
                float abs_x = absPos[0]-(gui->output.xcenter_os+gui->output.xcenter);
                float abs_y = absPos[1]-(gui->output.ycenter_os+gui->output.ycenter);
                float x = abs_x/gui->output.xspace;
                updateClamp(x, &max_x, &min_x);
                float y = (abs_y-x*gui->output.xstep)/gui->output.yspace;
                updateClamp(y, &max_y, &min_y);
                lensCenter.push(round(x), round(y));
            }
        }
        else{
            while(absLensCenters.next(absPos)){
                float abs_x = absPos[0]-(gui->output.xcenter_os+gui->output.xcenter);
                float abs_y = absPos[1]-(gui->output.ycenter_os+gui->output.ycenter);
                float y = abs_y/gui->output.yspace;
                updateClamp(y, &max_y, &min_y);
                float x = (abs_x-y*gui->output.ystep)/gui->output.xspace;
                updateClamp(x, &max_x, &min_x);
                lensCenter.push(round(x), round(y));
            }
        }
    }
    absLensCenters.return2Start();

    /*
    printf("X range from %f to %f, Y range from %f to %f\n", min_x, max_x, min_y, max_y);
    printf("X range from %d to %d, Y range from %d to %d\n", (int)round(min_x), (int)round(max_x), (int)round(min_y), (int)round(max_y));
    printf("Spreaded across %d points\n", lensCenter.counter);
    */

    int t_height = (int)round(max_y)-(int)round(min_y)+1;
    int t_width = (int)round(max_x)-(int)round(min_x)+1;
    stbi_uc *angleMap = (stbi_uc*)calloc(t_height*t_width*4, 1);
    
    char *tempPath = new char[256];
    sprintf(tempPath, "%s", gui->input.savePath);
    CreateDirectory(tempPath, NULL);

    sprintf(tempPath, "%s/config", gui->input.savePath);
    FILE *configFile = fopen(tempPath, "w");

    fprintf(configFile, "%d %d\n", t_width, t_height);
    fprintf(configFile, "%d\n", gui->output.sampling_fs-1);
    fclose(configFile);

    int *tPos = new int[2];
    while(absLensCenters.next(absPos) && lensCenter.next(tPos)){
        tPos[0] -= (int)round(min_x);
        tPos[1] -= (int)round(min_y);
        memcpy(angleMap+(4*(tPos[0]+t_width*tPos[1])), data+(4*(absPos[0]+overlay_w*absPos[1])), 4);
    }

    sprintf(tempPath, "%s/0_0.png", gui->input.savePath);
    if(!stbi_write_png(tempPath, t_width, t_height, 4, angleMap, t_width*4)){
        printf("Failed to write image!\n");
    }
    STBI_FREE(angleMap);

    absLensCenters.return2Start();
    lensCenter.return2Start();
    
    int x_offset = (int)round(gui->output.xpitch/2.0f/((float)gui->output.sampling_fs-1.0f));
    printf("%d\n",x_offset);
    for(int i = -(gui->output.sampling_fs-1); i < gui->output.sampling_fs; i++){
        if(i != 0){
            stbi_uc *angleMap = (stbi_uc*)calloc(t_height*t_width*4, 1);
            while(absLensCenters.next(absPos) && lensCenter.next(tPos)){
                tPos[0] -= (int)round(min_x);
                tPos[1] -= (int)round(min_y);
                char* ptr = _getPixel(absPos[0]+x_offset*i, absPos[1], (char*)data);
                if(ptr!=NULL){
                    memcpy(angleMap+(4*(tPos[0]+t_width*tPos[1])), ptr, 4);
                }
            }
            sprintf(tempPath, "%s/%d_%d.png", gui->input.savePath, i, 0);
            if(!stbi_write_png(tempPath, t_width, t_height, 4, angleMap, t_width*4)){
                printf("Failed to write image!\n");
            }
            STBI_FREE(angleMap);
            absLensCenters.return2Start();
            lensCenter.return2Start();
        }
    }
    
    int y_offset = round(gui->output.ypitch/2/(gui->output.sampling_fs-1));
    for(int j = -(gui->output.sampling_fs-1); j < gui->output.sampling_fs; j++){
        if(j != 0){
            stbi_uc *angleMap = (stbi_uc*)calloc(t_height*t_width*4, 1);
            while(absLensCenters.next(absPos) && lensCenter.next(tPos)){
                tPos[0] -= (int)round(min_x);
                tPos[1] -= (int)round(min_y);
                char* ptr = _getPixel(absPos[0], absPos[1]+y_offset*j, (char*)data);
                if(ptr!=NULL){
                    memcpy(angleMap+(4*(tPos[0]+t_width*tPos[1])), ptr, 4);
                }
            }
            sprintf(tempPath, "%s/%d_%d.png", gui->input.savePath, 0, j);
            if(!stbi_write_png(tempPath, t_width, t_height, 4, angleMap, t_width*4)){
                printf("Failed to write image!\n");
            }
            STBI_FREE(angleMap);
            absLensCenters.return2Start();
            lensCenter.return2Start();
        }
    }

    fclose(configFile);
}

static void updateClamp(float i, float *max, float *min){
    if(i > *max){
        *max = i;
    }
    if(i < *min){
        *min = i;
    }
}

static void loadPreset(_gui *gui){
    FILE *preset = fopen("config", "r");
    fscanf(preset, "%f %f %f %f %f %f %f %f %d\n",
        &(gui->output.xstep),
        &(gui->output.xpitch),
        &(gui->output.xspace),
        &(gui->output.xcenter),
        &(gui->output.ystep),
        &(gui->output.ypitch),
        &(gui->output.yspace),
        &(gui->output.ycenter),
        &(gui->output.sampling_fs)
    );
    fclose(preset);
}

static void savePreset(_gui *gui){
    FILE *preset = fopen("config", "w");
    fprintf(preset, "%f %f %f %f %f %f %f %f %d\n",
        gui->output.xstep,
        gui->output.xpitch,
        gui->output.xspace,
        gui->output.xcenter,
        gui->output.ystep,
        gui->output.ypitch,
        gui->output.yspace,
        gui->output.ycenter,
        gui->output.sampling_fs
    );
    fclose(preset);
}

static GLFWwindow *win;
struct nk_context ctx;
struct device device;
struct nk_font_atlas atlas;

int width = 0, height = 0;

struct nk_image displayImage;
struct nk_image overlayImage;
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

    glfwSetScrollCallback(win, scroll_callback);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
}

int mouse_l = false;
int mouse_l_last = false;
int mouse_r = false;
int mouse_r_last = false;
double mouse_s = 0.0;
int mouse_s_check = false;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        mouse_l = true;
    }
    else{mouse_l = false;}
    if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
        mouse_r = true;
    }
    else{mouse_r = false;}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    mouse_s = yoffset;
    mouse_s_check = true;
}

int start_xpos = 0;
int start_ypos = 0;
int pressed = false;

void gui_loop(_gui *gui){
    pump_input(&ctx, win);

    if (nk_begin(&ctx, "Photo", nk_rect(0, 0, 420, 120),
        NK_WINDOW_BORDER)) {
        nk_layout_row_begin(&ctx, NK_STATIC, 30, 3);
        {   
            nk_layout_row_push(&ctx, 100);
            nk_label(&ctx, "Image path:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 130);
            nk_edit_string_zero_terminated(&ctx, NK_EDIT_FIELD, gui->input.loadPath, 255, nk_filter_default);
            nk_layout_row_push(&ctx, 100);
            if (nk_button_label(&ctx, "Load")) {
                gui->output.loading = true;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 30, 3);
        {   
            nk_layout_row_push(&ctx, 100);
            nk_label(&ctx, "Folder path:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 130);
            nk_edit_string_zero_terminated(&ctx, NK_EDIT_FIELD, gui->input.savePath, 255, nk_filter_default);
            nk_layout_row_push(&ctx, 100);
            if (nk_button_label(&ctx, "Save")) {
                generateAngleMap(gui);
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 2);
        {
            nk_layout_row_push(&ctx, 180);
            if(nk_button_label(&ctx, "Reset pan")){
                gui->output.xoffset = 0;
                gui->output.yoffset = 0;
            }
            nk_layout_row_push(&ctx, 180);
            if(nk_button_label(&ctx, "Reset scale")){
                gui->output.scaling = gui->output.optm_scale;
            }
        }
        nk_layout_row_end(&ctx);
    }
    nk_end(&ctx);
    
    if (nk_begin(&ctx, "Processing", nk_rect(0, 120, 420, height-120),
        NK_WINDOW_BORDER)) {
        // Options

        nk_layout_row_begin(&ctx, NK_STATIC, 30, 3);
        {   
            nk_layout_row_push(&ctx, 100);
            nk_label(&ctx, "Option preset:", NK_TEXT_LEFT);
            if (nk_button_label(&ctx, "Load")) {
                loadPreset(gui);
            }
            if (nk_button_label(&ctx, "Save")) {
                savePreset(gui);
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 180);
            nk_label(&ctx, "Full preview:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, " ", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 110);
            if(nk_button_label(&ctx, gui->output.full_preview?"TOGGLED ON ":"TOGGLED OFF")){
                gui->output.full_preview=!gui->output.full_preview;
            }
        }

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 6);
        {
            nk_layout_row_push(&ctx, 120);
            nk_label(&ctx, "X-Step-Offset:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "-2", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 170);
            nk_slider_float(&ctx, -2, &gui->output.xstep, 2, 0.1);
            nk_layout_row_push(&ctx, 25);
            nk_label(&ctx, "2", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "+")) {
                gui->output.xstep+=0.01;
            }
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "-")) {
                gui->output.xstep-=0.01;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 135);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[10];
            sprintf(buf, "%.2f\0", gui->output.xstep);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 6);
        {
            nk_layout_row_push(&ctx, 120);
            nk_label(&ctx, "Y-Step-Offset:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "-2", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 170);
            nk_slider_float(&ctx, -2, &gui->output.ystep, 2, 0.1);
            nk_layout_row_push(&ctx, 25);
            nk_label(&ctx, "2", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "+")) {
                gui->output.ystep+=0.01;
            }
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "-")) {
                gui->output.ystep-=0.01;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 135);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[10];
            sprintf(buf, "%.2f\0", gui->output.ystep);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        /**/

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 6);
        {
            nk_layout_row_push(&ctx, 120);
            nk_label(&ctx, "X-Center:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "-40", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 170);
            nk_slider_float(&ctx, -40, &gui->output.xcenter, 40, 0.5);
            nk_layout_row_push(&ctx, 25);
            nk_label(&ctx, "40", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "+")) {
                gui->output.xcenter+=0.1;
            }
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "-")) {
                gui->output.xcenter-=0.1;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 135);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[10];
            sprintf(buf, "%.2f\0", gui->output.xcenter);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 6);
        {
            nk_layout_row_push(&ctx, 120);
            nk_label(&ctx, "Y-Center:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "-40", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 170);
            nk_slider_float(&ctx, -40, &gui->output.ycenter, 40, 0.5);
            nk_layout_row_push(&ctx, 25);
            nk_label(&ctx, "40", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "+")) {
                gui->output.ycenter+=0.1;
            }
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "-")) {
                gui->output.ycenter-=0.1;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 135);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[10];
            sprintf(buf, "%.2f\0", gui->output.ycenter);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        /**/

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 6);
        {
            nk_layout_row_push(&ctx, 120);
            nk_label(&ctx, "X-Pitch:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "0", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 170);
            nk_slider_float(&ctx, 0, &gui->output.xpitch, 100, 0.5);
            nk_layout_row_push(&ctx, 25);
            nk_label(&ctx, "100", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "+")) {
                gui->output.xpitch+=0.1;
            }
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "-")) {
                gui->output.xpitch-=0.1;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 135);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[10];
            sprintf(buf, "%.2f\0", gui->output.xpitch);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 6);
        {
            nk_layout_row_push(&ctx, 120);
            nk_label(&ctx, "Y-Pitch:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "0", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 170);
            nk_slider_float(&ctx, 0, &gui->output.ypitch, 100, 0.5);
            nk_layout_row_push(&ctx, 25);
            nk_label(&ctx, "100", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "+")) {
                gui->output.ypitch+=0.1;
            }
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "-")) {
                gui->output.ypitch-=0.1;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 135);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[10];
            sprintf(buf, "%.2f\0", gui->output.ypitch);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        /**/

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 6);
        {
            nk_layout_row_push(&ctx, 120);
            nk_label(&ctx, "X-Space:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "0", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 170);
            nk_slider_float(&ctx, 0, &gui->output.xspace, 100, 0.5);
            nk_layout_row_push(&ctx, 25);
            nk_label(&ctx, "100", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "+")) {
                gui->output.xspace+=0.1;
            }
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "-")) {
                gui->output.xspace-=0.1;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 135);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[10];
            sprintf(buf, "%.2f\0", gui->output.xspace);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 6);
        {
            nk_layout_row_push(&ctx, 120);
            nk_label(&ctx, "Y-Space:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "0", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 170);
            nk_slider_float(&ctx, 0, &gui->output.yspace, 100, 0.5);
            nk_layout_row_push(&ctx, 25);
            nk_label(&ctx, "100", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "+")) {
                gui->output.yspace+=0.1;
            }
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "-")) {
                gui->output.yspace-=0.1;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 135);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[10];
            sprintf(buf, "%.2f\0", gui->output.yspace);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);

        /**/

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 6);
        {
            nk_layout_row_push(&ctx, 120);
            nk_label(&ctx, "Sampling density:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 15);
            nk_label(&ctx, "0", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 170);
            nk_slider_int(&ctx, 1, &gui->output.sampling_fs, 40, 1);
            nk_layout_row_push(&ctx, 25);
            nk_label(&ctx, "100", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "+")) {
                gui->output.sampling_fs+=1;
            }
            nk_layout_row_push(&ctx, 20);
            if (nk_button_label(&ctx, "-")) {
                gui->output.sampling_fs-=1;
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 20, 3);
        {
            nk_layout_row_push(&ctx, 30);
            nk_label(&ctx, ">", NK_TEXT_CENTERED);
            nk_layout_row_push(&ctx, 135);
            nk_label(&ctx, " ", NK_TEXT_RIGHT);
            nk_layout_row_push(&ctx, 110);
            char buf[10];
            sprintf(buf, "%d\0", gui->output.sampling_fs);
            nk_label(&ctx, buf, NK_TEXT_CENTERED);
        }
        nk_layout_row_end(&ctx);
    }
    nk_end(&ctx);

    float new_sum = gui->output.sum();
    if(new_sum!=gui->output.old_sum){
        gui->output.old_sum = new_sum;
        gui->output.adjusted = true;
    }
    else{
        gui->output.adjusted = false;
    }

    double xpos, ypos;
    glfwGetCursorPos(win, &xpos, &ypos);
    if(ypos > 0 && ypos < height && xpos > 420 && xpos < width-500+420){
        if(mouse_l && !mouse_l_last && !pressed){
            start_xpos = xpos;
            start_ypos = ypos;
            pressed = true;
        }
        if(!mouse_l && mouse_l_last && pressed){
            gui->output.xoffset += gui->output.x_t_offset;
            gui->output.yoffset += gui->output.y_t_offset;
            gui->output.x_t_offset = 0;
            gui->output.y_t_offset = 0;
            pressed = false;
        }
        if(mouse_l && mouse_l_last && pressed){
            gui->output.x_t_offset = xpos-start_xpos;
            gui->output.y_t_offset = ypos-start_ypos;
        }

        mouse_l_last = mouse_l;
        mouse_r_last = mouse_r;

        if(mouse_s_check){
            if(gui->output.scaling+mouse_s>30){mouse_s = 30-gui->output.scaling;}
            if(gui->output.scaling+mouse_s<-30){mouse_s = -30-gui->output.scaling;}
            gui->output.scaling+=mouse_s;

            gui->output.xoffset += (xpos-420-gui->output.xoffset)*(1-pow(1.1, mouse_s));
            gui->output.yoffset += (ypos-gui->output.yoffset)*(1-pow(1.1, mouse_s));

            mouse_s_check = false;
            mouse_s = 0.0;
        }
    }
    if(mouse_s_check){mouse_s_check = false; mouse_s = 0.0;}
    
    if (nk_begin(&ctx, "Photos", nk_rect(420, 0, width-500, height),
        NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT)) {
        struct nk_command_buffer *canvas = nk_window_get_canvas(&ctx);
        const struct nk_color grid_color = nk_rgba(255, 255, 255, 255);
        gui->output.t_loading = false;
        if(gui->output.loading){
            displayImage = load_image(gui->input.loadPath, gui->output.imagew, gui->output.imageh);
            overlayImage = init_overlay(gui);
            gui->output.xcenter_os = gui->output.imagew/2;
            gui->output.ycenter_os = gui->output.imageh/2;
            if(displayImage.handle.id == 0){printf("Error loading image\n");}
            gui->output.loading = false;
            gui->output.t_loading = true;
        }
        if(gui->output.adjusted || gui->output.t_loading){
            overlayImage = update_overlay(gui);
        }
        if(displayImage.handle.id != 0){
            int h = gui->output.imageh;
            int w = gui->output.imagew;
            struct nk_rect window = nk_rect(420, 0, width-500, height);
            struct nk_rect displayWindow;
            struct nk_rect overlayWindow;
            float total_x_offset = gui->output.xoffset+gui->output.x_t_offset;
            float total_y_offset = gui->output.yoffset+gui->output.y_t_offset;
            if(gui->output.t_loading){
                {   
                    if(((float)h/(float)w)>((float)window.h/(float)window.w)){
                        displayWindow.w = (float)w/(float)h*(float)window.h;
                        gui->output.scaling = log2(displayWindow.w/gui->output.imagew)/log2(1.1);
                        gui->output.optm_scale = gui->output.scaling;
                        gui->output.xoffset = (float)(window.w-displayWindow.w)/2+window.x-420;
                        gui->output.yoffset = 0;
                    }
                    else{
                        displayWindow.h = (float)h/(float)w*(float)window.w;
                        gui->output.scaling = log2(displayWindow.h/gui->output.imageh)/log2(1.1);
                        gui->output.optm_scale = gui->output.scaling;
                        gui->output.xoffset = 0;
                        gui->output.yoffset = (float)(window.h-displayWindow.h)/2+window.y;
                    }
                }
            }
            {
                displayWindow.w = gui->output.imagew*pow(1.1,gui->output.scaling);
                displayWindow.h = gui->output.imageh*pow(1.1,gui->output.scaling);
                displayWindow.x = window.x+total_x_offset;
                displayWindow.y = window.y+total_y_offset;

                overlayWindow.w = gui->output.imagew*pow(1.1,gui->output.scaling);
                overlayWindow.h = gui->output.imageh*pow(1.1,gui->output.scaling);
                overlayWindow.x = window.x+total_x_offset;
                overlayWindow.y = window.y+total_y_offset;
            }
            nk_draw_image(canvas, displayWindow, &displayImage, grid_color);
            nk_draw_image(canvas, overlayWindow, &overlayImage, grid_color);
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