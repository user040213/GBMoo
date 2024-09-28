#include "FrontendSystem.hpp"

const std::string vertexShaderSrc{
    "version 330 core\n"
    "layout(location = 0) in vec3 position;"
    "layout(location = 1) in vec2 inTexCoord;"
    "out vec2 outTexCoord;"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(position.x, position.y, position.z, 1.0);\n"
    "   outTexCoord = vec2(inTexCoord);\n"
    "}\n"
};

const std::string fragShaderSrc{
    "version 330 core\n"
    "out vec4 color;"
    "in vec2 texCoord;"
    "uniform sampler2D inTexture;"
    "void main()\n"
    "{\n"
    "   color = texture(inTexture, texCoord);\n"
    "}\n"
};

FrontendSystem::FrontendSystem(const char *winTitle, int windowWidth, int windowHeight)
{
    mCPU = new CPU();

    quit = false;

    SDL_InitSubSystem(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);


    windowObj = SDL_CreateWindow(winTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, 0);
    renderer = SDL_CreateRenderer(windowObj, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_SetWindowResizable(windowObj, SDL_TRUE);

    /*
    glContext = SDL_GL_CreateContext(windowObj);
    gladLoadGLLoader(SDL_GL_GetProcAddress);
    
    SDL_GL_SetSwapInterval(1);

    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.235f, 0.255f, 0.173f, 1.0f);

    
    //vertexSpec();
    //genGraphicsPipeline(); 

    // Testing Code 
    glGenTextures(1, &oglTexture);
    glBindTexture(GL_TEXTURE_2D, oglTexture);
    GLuint fbo{0};
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

    glGenTextures(1, &oglTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oglTexture);
    */

}

FrontendSystem::~FrontendSystem()
{
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(windowObj);
    SDL_Quit();
}

void FrontendSystem::run()
{
    uint64_t lastCycle{SDL_GetPerformanceCounter()};
    uint64_t curTime{SDL_GetPerformanceCounter()};
    double elapsedTime{0};
    while(!quit)
    {
        curTime = SDL_GetPerformanceCounter();
        pollInput();
        refreshScreen();
        elapsedTime = (static_cast<double>(curTime - lastCycle)) / (static_cast<double>(SDL_GetPerformanceFrequency()));

        if(elapsedTime >= 0.01675)
        {
            mCPU->runCPU(); 
            lastCycle = SDL_GetPerformanceCounter();
        }
        
    }
}

void FrontendSystem::pollInput()
{
    SDL_Event event{0};

    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT_RESIZED:
                // run the resize handling function
                resizeFrame();
                break;
            case SDL_KEYDOWN:
                parseKeyboard(event.key.keysym.scancode, true);
                break;
            case SDL_KEYUP:
                parseKeyboard(event.key.keysym.scancode, false);
                break;
            default:
                break;
        }
    }
}

void FrontendSystem::vertexSpec()
{
    std::vector<GLfloat> vertices
    {
        -1.0f, -1.0f, 0.0f,     0.0f, 1.0f,
        1.0f, -1.0f, 0.0f,      1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f,      0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,       1.0f, 0.0f
    };

    glGenVertexArrays(1, &vertexArrId);
    glBindVertexArray(vertexArrId);

    glGenBuffers(1, &vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(GLfloat), vertices.data(), GL_DYNAMIC_DRAW);

    std::vector<GLuint> indices
    {
        0, 1, 2,
        2, 3, 1
    };
    glGenBuffers(1, &elemBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 160, 144, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, (GLvoid*)mCPU->getPPUArray());

    
}

void FrontendSystem::genGraphicsPipeline()
{
    graphicsPipeline = createShaders(vertexShaderSrc, fragShaderSrc);
}

GLuint FrontendSystem::createShaders(const std::string &vertShader, const std::string &fragShader)
{
    GLuint shaderProgram = glCreateProgram();

    GLuint vShaderCompiled = compileShader(GL_VERTEX_SHADER, vertShader);
    GLuint fShaderCompiled = compileShader(GL_FRAGMENT_SHADER, fragShader);

    glAttachShader(shaderProgram, vShaderCompiled);
    glAttachShader(shaderProgram, fShaderCompiled);
    glLinkProgram(shaderProgram);

    glDeleteShader(vShaderCompiled);
    glDeleteShader(fShaderCompiled);
    glValidateProgram(shaderProgram);
    return shaderProgram;
}

GLuint FrontendSystem::compileShader(GLuint shaderType, const std::string &shaderSource)
{
    GLuint shader;
    if (shaderType == GL_VERTEX_SHADER)
    {
        shader = glCreateShader(GL_VERTEX_SHADER);
    }
    else
    {
        shader = glCreateShader(GL_FRAGMENT_SHADER);
    }

    const char* source{shaderSource.c_str()};
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    return shader;
}

void FrontendSystem::resizeFrame()
{
    int width{0};
    int height{0};
    SDL_GetWindowSize(windowObj, &width, &height);
    glViewport(0, 0, width, height);
}

void FrontendSystem::parseKeyboard(SDL_Scancode input, bool pressed)
{
    switch(input)
    {
        case SDL_SCANCODE_ESCAPE:
            quit = true;
            break;
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_G:
        case SDL_SCANCODE_F:
        case SDL_SCANCODE_SPACE:
        case SDL_SCANCODE_RETURN:
            mCPU->handleJoypadInput(input, pressed);
            break;
        default:
            break;
    }
}

void FrontendSystem::drawTex()
{

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 160, 144, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, (GLvoid*)mCPU->getPPUArray());
    
    /* TESTING CODE */
    //glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oglTexture, 0);
    //glBlitFramebuffer(0, 0, 160, 144, 0, 0, 600, 600, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    

    /* SDL PROTOTYPE CODE */
    
    SDL_UpdateTexture(texture, NULL, mCPU->getPPUArray(), 4*160);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    
    

    
}

void FrontendSystem::glDrawQuad()
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 160, 144, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, (GLvoid*)mCPU->getPPUArray());
    glBindTexture(GL_TEXTURE_2D, oglTexture);
    glBindVertexArray(vertexArrId);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void FrontendSystem::refreshScreen()
{
    
    //glClear(GL_COLOR_BUFFER_BIT);
    drawTex();
    //glDrawQuad();
    //SDL_GL_SwapWindow(windowObj);
}

void FrontendSystem::loadCPURom(const std::string fileName)
{
    mCPU->loadROM(fileName);
}
