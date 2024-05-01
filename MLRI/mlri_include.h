#ifndef mlri_include
#define mlri_include

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <thread>
#include <chrono>
#include <windows.h>

#define DEFAULT_FILE_BUFLEN 8192

static const char *imgFormat[] = {
    ".jpg",
    ".png",
    ".bmp"
};

void copyFile(char *src, char *dst);

class intPosList{
    public:
    int *xpos, *ypos;
    int _pushCounter = 0;
    int counter = 0;
    int size = 16;

    intPosList(){
        this->xpos = new int[this->size];
        this->ypos = new int[this->size];
    }

    ~intPosList(){
        delete[] this->xpos;
        delete[] this->ypos;
    }

    void _expand(){
        int *t_xpos, *t_ypos;

        t_xpos = new int[this->size*2];
        t_ypos = new int[this->size*2];

        memcpy(t_xpos, this->xpos, this->size*sizeof(int));
        memcpy(t_ypos, this->ypos, this->size*sizeof(int));

        this->size *= 2;

        delete[] this->xpos;
        delete[] this->ypos;
        
        this->xpos = t_xpos;
        this->ypos = t_ypos;
    }

    void return2Start(){
        this->_pushCounter = 0;
    }

    void push(int x, int y){
        if(this->counter==this->size){
            this->_expand();
        }
        this->xpos[this->counter] = x;
        this->ypos[this->counter] = y;
        this->counter++;
    }

    int next(int *ptr){
        if(this->_pushCounter==this->counter){
            return false;
        }
        ptr[0] = this->xpos[_pushCounter];
        ptr[1] = this->ypos[_pushCounter];
        this->_pushCounter++;
        return true;
    }
};

class _gui{
    class _input{
        public:
        char *loadPath = new char[256];
        char *savePath = new char[256];
    };
    class _output{
        public:
        int imgFmt = 0;
        int loading = false;
        int t_loading = false;
        int adjusted = false;
        int full_preview = false;

        float sum(){
            return xstep+xpitch+xspace+xcenter+ystep+ypitch+yspace+ycenter+sampling_fs+full_preview;
        }

        float old_sum = -1;

        float xstep = 0;
        float xpitch = 20;
        float xspace = 40;
        float xcenter = 0;

        float ystep = 0;
        float ypitch = 20;
        float yspace = 40;
        float ycenter = 0;

        float xoffset = 0;
        float yoffset = 0;
        float scaling = 0;
        float optm_scale = 0;

        int sampling_fs = 1;

        float xcenter_os = 0;
        float ycenter_os = 0;

        float x_t_offset = 0;
        float y_t_offset = 0;

        int imageh = 0;
        int imagew = 0;
    };

    public:
    _input input;
    _output output;
};

#endif