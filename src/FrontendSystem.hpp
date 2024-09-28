#ifndef FRONTENDSYSTEM_H
#define FRONTENDSYSTEM_H

#include <SDL.h>
#include <glad.h>
#include "CPU.hpp"

/*
    A class to hold the SDL+OpenGL context as well as handle user input
    Shaders and other such graphics functions will also be defined here
*/
class FrontendSystem
{
    public:
        FrontendSystem(const char* winTitle, int windowWidth, int windowHeight);
        ~FrontendSystem();

        void run();

        void pollInput();
        void refreshScreen();
        void loadCPURom(const std::string fileName);
    
    private:
        bool quit;

        // Window and graphics
        SDL_Window* windowObj;
        SDL_GLContext glContext;

        SDL_Texture* texture;
        SDL_Renderer* renderer;

        GLuint oglTexture;
        GLuint vertexArrId;
        GLuint vertexBufferId;
        GLuint elemBufferId;

        GLuint graphicsPipeline;

        // Gameboy cpu
        CPU* mCPU;

        // To do: Consolidate the GL functions to their own class
        void vertexSpec();
        void genGraphicsPipeline();
        GLuint createShaders(const std::string& vertShader, const std::string& fragShader);
        GLuint compileShader(GLuint shaderType, const std::string &shaderSource);


        void resizeFrame();
        void parseKeyboard(SDL_Scancode input, bool pressed);
        void drawTex();
        void glDrawQuad();

};

#endif