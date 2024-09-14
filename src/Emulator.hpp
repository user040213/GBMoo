#ifndef EMULATOR_H
#define EMULATOR_H

#include <SDL.h>
#include <glew.h>
#include <SDL_opengl.h>
#include <chrono>
#include "CPU.hpp"


class Emulator
{
    public: 
        Emulator(const char* winTitle, int windowWidth, int windowHeight);
        ~Emulator();
        
    private:
        
        // for inputs
        void update();

        // for refreshing display
        void refresh(const void* pixels, int pitch);

        // main loop
        void loop();

        SDL_Window* windowObj;
        SDL_GLContext glContext;

        bool shutDown;
        CPU gameEmu;

        float clockTime;
        float delayTime;
        float drawTime;


        

};

#endif