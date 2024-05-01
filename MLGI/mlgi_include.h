#ifndef mlgi_include
#define mlgi_include

#include <iostream>
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

#define DEFAULT_FILE_BUFLEN 8192

enum {connected, connecting, disconnected};
enum {doConnect, doDisconnect, doNothing};

typedef enum {
    FRAMESIZE_96X96=0,      //   96x96
    FRAMESIZE_QQVGA=1,      //  160x120
    FRAMESIZE_QCIF=2,       //  176x144
    FRAMESIZE_HQVGA=3,      //  240x176
    FRAMESIZE_240X240=4,    //  240x240
    FRAMESIZE_QVGA=5,       //  320x240
    FRAMESIZE_CIF=6,        //  400x296
    FRAMESIZE_HVGA=7,       //  480x320
    FRAMESIZE_VGA=8,        //  640x480
    FRAMESIZE_SVGA=9,       //  800x600
    FRAMESIZE_XGA=10,       // 1024x768
    FRAMESIZE_HD=11,        // 1280x720
    FRAMESIZE_SXGA=12,      // 1280x1024
    FRAMESIZE_UXGA=13,      // 1600x1200
    // 3MP Sensors
    FRAMESIZE_FHD=14,       // 1920x1080
    FRAMESIZE_P_HD=15,      //  720x1280
    FRAMESIZE_P_3MP=16,     //  864x1536
    FRAMESIZE_QXGA=17,      // 2048x1536
    // 5MP Sensors
    FRAMESIZE_QHD=18,       // 2560x1440
    FRAMESIZE_WQXGA=19,     // 2560x1600
    FRAMESIZE_P_FHD=20,     // 1080x1920
    FRAMESIZE_QSXGA=21,     // 2560x1920
    FRAMESIZE_INVALID=22
} framesize_t;

static const char *framesizes[] = {
    "       96X96",
    "QQVGA  160x120",
    "QCIF   176x144",
    "HQVGA  240x176",
    "       240X240",
    "QVGA   320x240",
    "CIF    400x296",
    "HVGA   480x320",
    "VGA    640x480",
    "SVGA   800x600",
    "XGA    1024x768",
    "HD     1280x720",
    "SXGA   1280x1024",
    "UXGA   1600x1200",
    // 3MP Sensors
    "FHD    1920x1080",
    "P HD   720x1280",
    "P 3MP  864x1536",
    "QXGA   2048x1536",
    // 5MP Sensors
    "QHD    2560x1440",
    "WQXGA  2560x1600",
    "P FHD  1080x1920",
    "QSXGA  2560x1920"
};

class resolutionSize_t{
    public:
    int h, w;
    resolutionSize_t(int w, int h){
        this->h = h;
        this->w = w;
    }
};

static const resolutionSize_t rs_table[] = {
    {96, 96},
    {160, 120},
    {176, 144},
    {240, 176},
    {240, 240},
    {320, 240},
    {400, 296},
    {480, 320},
    {640, 480},
    {800, 600},
    {1024, 768},
    {1280, 720},
    {1280, 1024},
    {1600, 1200},
    // 3MP Sensors
    {1920, 1080},
    {720, 1280},
    {864, 1536},
    {2048, 1536},
    // 5MP Sensors
    {2560, 1440},
    {2560, 1600},
    {1080, 1920},
    {2560, 1920}
};

typedef enum{
    NONE=0,
    NEGATIVE=1,
    GRAYSCALE=2,
    REDTINT=3,
    GREENTINT=4,
    BLUETINT=5,
    SEPIA=6
} effect_t;

static const char *timelapse_interval[] = {
    "1 second",
    "2 seconds",
    "3 seconds", 
    "5 seconds",
    "15 seconds",
    "30 seconds",
    "1 minute",
    "5 minutes",
    "15 minutes",
    "30 minutes",
    "1 hour",
    "2 hours",
    "12 hours",
    "1 day"
};

static const int timelapse_interval_t[] = {
    1*1000,
    2*1000,
    3*1000,
    5*1000,
    15*1000,
    30*1000,
    60*1000,
    300*1000,
    900*1000,
    1800*1000,
    1*3600*1000,
    2*3600*1000,
    12*3600*1000,
    24*3600*1000
};

static const char *effects[] = {
    "None",
    "Negative",
    "Grayscale",
    "Red Tint",
    "Green Tint",
    "Blue Tint",
    "Sepia"
};

static const char *awbMode[] = {
    "Auto",
    "Sunny",
    "Cloudy",
    "Office",
    "Home"
};

static const char *awbOff[] = {
    "Toggle AWB to config"
};

static const char *imgFormat[] = {
    ".jpg",
    ".png",
    ".bmp"
};

void copyFile(char *src, char *dst);

class _gui{
    class _input{
        public:
        char *photoTime = new char[48];
        char *loadPath = new char[256];
        char *savePath = new char[256];
    };
    class _output{
        public:
        int imgFmt = 0;
        int getStill = 0;
        int stillProgress = 0;
        int getStream = 0;
        int saveFrame = 0;
        int timeLapseOn = 0;
        int timeLapse_t = 1;
        int IMGcounter = 0;
    };

    public:
    _input input;
    _output output;
};

class _network{
    class _input{
        public:
        // Camera default config
        int frameSize = FRAMESIZE_QSXGA;
        int brightness = 0;
        int contrast = 0;
        int saturation = 0;
        int special_effect = NONE;
        int whitebal = 1;
        int awb_gain = 1;
        int wb_mode = 0;
        int exposure_ctrl = 0;
        int aec2 = 0;
        int ae_level = 0;
        int aec_value = 300;
        int gain_ctrl = 0;
        int agc_gain = 0;
        int gainceiling = 0;
        int bpc = 0;
        int wpc = 1;
        int raw_gma = 1;
        int lenc = 0;
        int hmirror = 0;
        int vflip = 0;
        int set_dcw = 1;
        int colorbar = 0;
        // Connection control
        char sendBuf[24];
        int serviceUp = true;
        int connectCmd = doDisconnect;
        char addrBuf[256] = "192.168.86.200";
    };
    class _output{
        public:
        char *cachePath = (char*)"cache/perm/verify.jpg";
        int lastIMGtick = 0;
        int IMGtick = 10000;
        int IMGcounter = 0;
        int avgComm_t = 1000;
        int connectStatus = disconnected;
        int connectError = 0;
    };

    public:
    _input input;
    _output output;
};

#endif