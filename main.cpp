#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

std::string LoadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    } else {
        std::cout << "File opened successfully: " << path << std::endl;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        std::cerr << "Shader compilation error: " << info << std::endl;
    } else {
        std::cout << "Shader compiled successfully" << std::endl;
    }
    return shader;
}

GLuint CreateProgram(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSource = LoadFile(vertPath);
    std::string fragSource = LoadFile(fragPath);

    if (vertSource.empty() || fragSource.empty()) {
        std::cerr << "Failed to load shader sources" << std::endl;
        return 0;
    } else {
        std::cout << "Shader sources loaded successfully" << std::endl;
    }


    GLuint vertShader = CompileShader(GL_VERTEX_SHADER, vertSource.c_str());
    GLuint fragShader = CompileShader(GL_FRAGMENT_SHADER, fragSource.c_str());

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, nullptr, info);
        std::cerr << "Program link error: " << info << std::endl;
    } else {
        std::cout << "Program linked successfully" << std::endl;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return program;
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 1) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    } else {
        std::cout << "SDL_Init succeeded" << std::endl;
    }

    SDL_Window* window = SDL_CreateWindow("Pendulum Art",
                                          1000, 1000,
                                          SDL_WINDOW_OPENGL);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return 1;
    }

    GLuint shaderProgram = CreateProgram("shader.vert", "shader.frag");

    if (!shaderProgram) return 1;

    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    unsigned int indices[] = {0, 1, 2, 2, 3, 0};

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);

    GLint gravLoc = glGetUniformLocation(shaderProgram, "uGrav");
    GLint arm1_changeLoc = glGetUniformLocation(shaderProgram, "uArm1_change");
    GLint arm2_changeLoc = glGetUniformLocation(shaderProgram, "uArm2_change");
    GLint arm1_startLoc = glGetUniformLocation(shaderProgram, "uArm1_start");
    GLint arm2_startLoc = glGetUniformLocation(shaderProgram, "uArm2_start");
    GLint time_stepLoc = glGetUniformLocation(shaderProgram, "uTime_step");
    GLint time_totalLoc = glGetUniformLocation(shaderProgram, "uTime_total");

    float test = 0.0001f;
    glUniform1f(gravLoc, -500.0f);
    glUniform1f(arm1_changeLoc, 0.01f);
    glUniform1f(arm2_changeLoc, 0.01f);
    glUniform1f(arm1_startLoc, 0.0f);
    glUniform1f(arm2_startLoc, 0.0f);
    glUniform1f(time_stepLoc, 0.1f);
    glUniform1f(time_totalLoc, 0.5f);


    bool running = true;
    SDL_Event e;

    
    while (running) {
        test *= 1.01f;
        glUniform1f(arm1_changeLoc, test);
        glUniform1f(arm2_changeLoc, test);
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
        }
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);        
        SDL_GL_SwapWindow(window);

    }

    glDeleteProgram(shaderProgram);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}