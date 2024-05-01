#include "mlri_gui.h"
#include "mlri_include.h"

#include <iostream>

static int dFPS = 30;

_gui *gui = new _gui;

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

    sprintf(gui->input.loadPath, "input/test.png");
    sprintf(gui->input.savePath, "output/");
    mlri_gui::gui_init();
    int counter = 0;
    while(!mlri_gui::gui_shouldClose()){
        frame_start();
        mlri_gui::gui_loop(gui);
        counter++;
        frame_end();
    }
    mlri_gui::gui_terminate();
    
    std::cout << "Application correctly shutdown" << std::endl;

    //::ShowWindow(::GetConsoleWindow(), SW_SHOW);
    //system("pause");
}