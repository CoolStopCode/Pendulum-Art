#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

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

    GLuint shaderProgram = CreateProgram("shader.vert", "pendulum.frag");

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Initialize SDL3 + OpenGL backend
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330");


    GLint gravLoc = glGetUniformLocation(shaderProgram, "uGrav");
    GLint damping = glGetUniformLocation(shaderProgram, "uDamping");
    GLint scaleX = glGetUniformLocation(shaderProgram, "uScaleX");
    GLint scaleY = glGetUniformLocation(shaderProgram, "uScaleY");
    GLint camX = glGetUniformLocation(shaderProgram, "uCamX");
    GLint camY = glGetUniformLocation(shaderProgram, "uCamY");
    GLint time_stepLoc = glGetUniformLocation(shaderProgram, "uTime_step");
    GLint time_totalLoc = glGetUniformLocation(shaderProgram, "uTime_total");

    float slider_gravLoc = -500.0f;
    float slider_dampingLoc = 0.0f;
    float slider_time_stepLoc = 0.1f;
    float slider_time_totalLoc = 0.5f;

    float cam_zoom = 0.00001f;
    float cam_x = 0.0f;
    float cam_y = 0.0f;

    bool live_play = false;

    bool running = true;
    SDL_Event e;

    
    while (running) {
        if (SDL_GetKeyboardState(nullptr)[SDL_GetScancodeFromName("W")]) {
            cam_y += 0.005f * cam_zoom;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_GetScancodeFromName("S")]) {
            cam_y -= 0.005f * cam_zoom;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_GetScancodeFromName("A")]) {
            cam_x -= 0.005f * cam_zoom;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_GetScancodeFromName("D")]) {
            cam_x += 0.005f * cam_zoom;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_GetScancodeFromName("Up")]) {
            float old_cam_zoom = cam_zoom;
            cam_zoom *= 1.01f;
            cam_x += 0.5f * (old_cam_zoom - cam_zoom);
            cam_y += 0.5f * (old_cam_zoom - cam_zoom);

        }
        if (SDL_GetKeyboardState(nullptr)[SDL_GetScancodeFromName("Down")]) {
            float old_cam_zoom = cam_zoom;
            cam_zoom *= 0.99f;
            cam_x += 0.5f * (old_cam_zoom - cam_zoom);
            cam_y += 0.5f * (old_cam_zoom - cam_zoom);
        }

        glUniform1f(damping, slider_dampingLoc);
        glUniform1f(gravLoc, slider_gravLoc);
        glUniform1f(time_stepLoc, slider_time_stepLoc);
        glUniform1f(time_totalLoc, slider_time_totalLoc);

        glUniform1f(scaleX, cam_zoom);
        glUniform1f(scaleY, cam_zoom);
        glUniform1f(camX, cam_x);
        glUniform1f(camY, cam_y);
        
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT)
                running = false;
        }



        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Your GUI code here
        ImGui::Begin("Parameters");
        ImGui::Text("ImGui with SDL3 and OpenGL3");

        ImGui::SliderFloat("Gravity", &slider_gravLoc, 5000.0f, -5000.0f, "Value: %1.0f");
        ImGui::SliderFloat("Time Step (Accuracy)", &slider_time_stepLoc, 0.001f, 0.5f, "Value: %.3f");
        ImGui::SliderFloat("Time Total", &slider_time_totalLoc, 0.01f, 10.0f, "Value: %0.2f");
        ImGui::SliderFloat("Damping", &slider_dampingLoc, -10.0f, 10.0f, "Value: %0.2f");

        ImGui::Checkbox("Live Play", &live_play);
        if (live_play) {
            slider_time_totalLoc = slider_time_totalLoc + slider_time_stepLoc;
            if (slider_time_totalLoc > 10.0f) {
                slider_time_totalLoc = 0.0f;
            }
        }

        ImGui::End();

        // Rendering
        ImGui::Render();


        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); 
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);

    }

    glDeleteProgram(shaderProgram);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}