#include "mlgi_gui.h"
#include "mlgi_network.h"
#include "mlgi_include.h"

#include <iostream>

static int dFPS = 60;

_gui *gui = new _gui;
_network *network = new _network;

void update_config(){
    network->input.sendBuf[0]  = (char)16 + (char)network->input.frameSize;
    network->input.sendBuf[1]  = (char)16 + (char)network->input.brightness;
    network->input.sendBuf[2]  = (char)16 + (char)network->input.contrast;
    network->input.sendBuf[3]  = (char)16 + (char)network->input.saturation;
    network->input.sendBuf[4]  = (char)16 + (char)network->input.special_effect;
    network->input.sendBuf[5]  = (char)16 + (char)network->input.whitebal;
    network->input.sendBuf[6]  = (char)16 + (char)network->input.awb_gain;
    network->input.sendBuf[7]  = (char)16 + (char)network->input.wb_mode;
    network->input.sendBuf[8]  = (char)16 + (char)network->input.exposure_ctrl;
    network->input.sendBuf[9]  = (char)16 + (char)network->input.aec2;
    network->input.sendBuf[10] = (char)16 + (char)network->input.ae_level;
    network->input.sendBuf[11] = (char)16 + (char)(network->input.aec_value/5);
    network->input.sendBuf[12] = (char)16 + (char)network->input.gain_ctrl;
    network->input.sendBuf[13] = (char)16 + (char)network->input.agc_gain;
    network->input.sendBuf[14] = (char)16 + (char)network->input.gainceiling;
    network->input.sendBuf[15] = (char)16 + (char)network->input.bpc;
    network->input.sendBuf[16] = (char)16 + (char)network->input.wpc;
    network->input.sendBuf[17] = (char)16 + (char)network->input.raw_gma;
    network->input.sendBuf[18] = (char)16 + (char)network->input.lenc;
    network->input.sendBuf[19] = (char)16 + (char)network->input.hmirror;
    network->input.sendBuf[20] = (char)16 + (char)network->input.vflip;
    network->input.sendBuf[21] = (char)16 + (char)network->input.set_dcw;
    network->input.sendBuf[22] = (char)16 + (char)network->input.colorbar;
    network->input.sendBuf[23] = '\0';
};

std::chrono::steady_clock frame_clock;
std::chrono::time_point<std::chrono::steady_clock> frameStart = frame_clock.now();



void frame_end(){
    int runTime = std::chrono::duration_cast<std::chrono::milliseconds>(frame_clock.now()-frameStart).count();
    float delayTime = 1000.0f/(float)dFPS-(float)runTime;
    if(delayTime < 0){
        printf("GUI can't keep up with designated FPS!!!  ");
        printf("%d/%d/%f\n", runTime, (int)delayTime, 1000.0f/(float)dFPS);
    }
    else{
        Sleep((int)delayTime);
        //printf("%d/%d/%f\n", runTime, (int)delayTime, 1000.0f/(float)dFPS);
    }
}

void frame_start(){
    frameStart = frame_clock.now();
}

int main(int argc, char *argv[]){
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);

    mlgi_gui::gui_init();
    gui->output.IMGcounter = network->output.IMGcounter-1;
    std::thread network_thread(network_loop, network, gui);
    update_config();
    FILE *config = fopen("config", "wb");
    fwrite(network->input.sendBuf, sizeof(char), sizeof(network->input.sendBuf), config);
    fclose(config);
    int counter = 0;
    while(!mlgi_gui::gui_shouldClose()){
        frame_start();
        mlgi_gui::gui_loop(network, gui);
        update_config();
        counter++;
        frame_end();
    }
    mlgi_gui::gui_terminate();
    network->input.serviceUp = false;
    network_thread.join();
    
    std::cout << "Application correctly shutdown" << std::endl;

    //::ShowWindow(::GetConsoleWindow(), SW_SHOW);
    //system("pause");
}