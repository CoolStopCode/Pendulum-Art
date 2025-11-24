#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector> 

// Export FBO globals
GLuint exportFBO = 0;
GLuint exportTex = 0;
int exportW = 1440;
int exportH = 1440;
std::vector<unsigned char> exportPixels;

void createExportFBO() {
    if (exportFBO != 0) return; // already created

    glGenFramebuffers(1, &exportFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, exportFBO);

    glGenTextures(1, &exportTex);
    glBindTexture(GL_TEXTURE_2D, exportTex);
    // allocate texture storage for 4K RGBA8
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, exportW, exportH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // clamp to edge to avoid sampling wrap (not strictly necessary)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, exportTex, 0);

    GLenum drawBuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuf);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "EXPORT FBO NOT COMPLETE!\n";
    } else {
        std::cout << "Created export FBO " << exportW << "x" << exportH << "\n";
    }

    // allocate pixel buffer
    exportPixels.resize(exportW * exportH * 4);

    // unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void destroyExportFBO() {
    if (exportTex) { glDeleteTextures(1, &exportTex); exportTex = 0; }
    if (exportFBO) { glDeleteFramebuffers(1, &exportFBO); exportFBO = 0; }
    exportPixels.clear();
}

void flipPixelsVertically(std::vector<unsigned char>& buf, int w, int h) {
    int rowSize = w * 4;
    std::vector<unsigned char> tmp(rowSize);
    for (int y = 0; y < h / 2; ++y) {
        unsigned char* row1 = buf.data() + (size_t)y * rowSize;
        unsigned char* row2 = buf.data() + (size_t)(h - 1 - y) * rowSize;
        memcpy(tmp.data(), row1, rowSize);
        memcpy(row1, row2, rowSize);
        memcpy(row2, tmp.data(), rowSize);
    }
}

void exportFractalToPNG(GLuint shaderProgram, GLuint VAO, int windowW, int windowH, 
                       // uniforms your code uses:
                       float grav, float damping, float time_step, float time_total, 
                       float arm1_len, float arm2_len, float scaleX, float scaleY, float camX, float camY) 
{
    createExportFBO();

    // Save previous viewport
    GLint prevViewport[4]; 
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Bind FBO and set viewport to 4K
    glBindFramebuffer(GL_FRAMEBUFFER, exportFBO);
    glViewport(0, 0, exportW, exportH);

    // Clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Use program and set uniforms (names matching your shader)
    glUseProgram(shaderProgram);
    // Note: query uniform locations once in your program for speed; here we set them by location names for simplicity
    GLint loc;
    loc = glGetUniformLocation(shaderProgram, "uGrav"); if (loc>=0) glUniform1f(loc, grav);
    loc = glGetUniformLocation(shaderProgram, "uDamping"); if (loc>=0) glUniform1f(loc, damping);
    loc = glGetUniformLocation(shaderProgram, "uTime_step"); if (loc>=0) glUniform1f(loc, time_step);
    loc = glGetUniformLocation(shaderProgram, "uTime_total"); if (loc>=0) glUniform1f(loc, time_total);
    loc = glGetUniformLocation(shaderProgram, "uArm1Length"); if (loc>=0) glUniform1f(loc, arm1_len);
    loc = glGetUniformLocation(shaderProgram, "uArm2Length"); if (loc>=0) glUniform1f(loc, arm2_len);
    loc = glGetUniformLocation(shaderProgram, "uScaleX"); if (loc>=0) glUniform1f(loc, scaleX);
    loc = glGetUniformLocation(shaderProgram, "uScaleY"); if (loc>=0) glUniform1f(loc, scaleY);
    loc = glGetUniformLocation(shaderProgram, "uCamX"); if (loc>=0) glUniform1f(loc, camX);
    loc = glGetUniformLocation(shaderProgram, "uCamY"); if (loc>=0) glUniform1f(loc, camY);

    // If your shader needs time or resolution uniforms, set them here as well.
    // e.g. resolution:
    loc = glGetUniformLocation(shaderProgram, "uResolution");
    if (loc >= 0) glUniform2f(loc, (float)exportW, (float)exportH);

    // Draw fullscreen quad
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Read pixels
    // Ensure alignment is tight (no row padding)
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, exportW, exportH, GL_RGBA, GL_UNSIGNED_BYTE, exportPixels.data());

    // Flip vertically
    flipPixelsVertically(exportPixels, exportW, exportH);

    // Compose filename (timestamp could be added)
    const char* filename = "export.png";
    // stbi_write_png expects stride in bytes per row:
    int stride = exportW * 4;
    if (stbi_write_png(filename, exportW, exportH, 4, exportPixels.data(), stride)) {
        std::cout << "Saved " << filename << "\n";
    } else {
        std::cerr << "Failed to save " << filename << "\n";
    }

    // Unbind FBO and restore viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}


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
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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
    GLint arm1_lenLoc = glGetUniformLocation(shaderProgram, "uArm1Length");
    GLint arm2_lenLoc = glGetUniformLocation(shaderProgram, "uArm2Length");

    float slider_gravLoc = -500.0f;
    float slider_dampingLoc = 0.0f;
    float slider_arm1_lenLoc = 1.0f;
    float slider_arm2_lenLoc = 1.0f;

    float slider_time_stepLoc = 0.01f;
    float slider_time_totalLoc = 0.5f;

    float cam_zoom = 1.0f;
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
        glUniform1f(arm1_lenLoc, slider_arm1_lenLoc);
        glUniform1f(arm2_lenLoc, slider_arm2_lenLoc);

        glUniform1f(scaleX, cam_zoom);
        glUniform1f(scaleY, cam_zoom);
        glUniform1f(camX, cam_x);
        glUniform1f(camY, cam_y);
        
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                int new_w = e.window.data1;
                int new_h = e.window.data2;
                glViewport(0, 0, new_w, new_h);
            }

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
        ImGui::SliderFloat("Arm1 Length", &slider_arm1_lenLoc, 0.01f, 100.0f, "Value: %.01f");
        ImGui::SliderFloat("Arm2 Length", &slider_arm2_lenLoc, 0.01f, 100.0f, "Value: %.01f");
        ImGui::SliderFloat("Damping", &slider_dampingLoc, -10.0f, 10.0f, "Value: %0.2f");

        ImGui::Checkbox("Live Play", &live_play);
        if (live_play) {
            slider_time_totalLoc = slider_time_totalLoc + slider_time_stepLoc;
            if (slider_time_totalLoc > 10.0f) {
                slider_time_totalLoc = 0.0f;
            }
        }

        if (ImGui::Button("Export")) {
            // Ensure the GPU is finished with any prior frame
            glFinish();

            // get current window size for later restoration
            int winW, winH;
            SDL_GetWindowSize(window, &winW, &winH);

            exportFractalToPNG(shaderProgram, VAO, winW, winH,
                            slider_gravLoc, slider_dampingLoc, slider_time_stepLoc, slider_time_totalLoc,
                            slider_arm1_lenLoc, slider_arm2_lenLoc,
                            cam_zoom, cam_zoom, cam_x, cam_y);
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

    destroyExportFBO();

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}