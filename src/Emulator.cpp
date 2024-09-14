#include "Emulator.hpp"

Emulator::Emulator(const char *winTitle, int windowWidth, int windowHeight)
{
    windowObj = SDL_CreateWindow(winTitle, 50, 50, windowWidth, windowHeight, SDL_WINDOW_OPENGL);

    glContext = SDL_GL_CreateContext(windowObj);

    if(glewInit() != GLEW_OK)
    {
        std::cout << "Could not initialize glew" << std::endl;
    }

    
}

Emulator::~Emulator()
{
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(windowObj);
    SDL_Quit();
}

void Emulator::loop()
{
    
}
