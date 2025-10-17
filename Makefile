# Compiler and flags
CXX = g++
CXXFLAGS = -O2 -Wall -std=c++17 \
	-I "C:/Installed/SDL3/include" \
	-I "C:/Installed/glad/include" \
	-I "C:/Installed/imgui/include" \
	-I. \
	-I "C:/Installed"

LDFLAGS = \
    "C:/Installed/SDL3/lib/SDL3.lib" \
    "C:/Installed/SDL3/lib/SDL3_image.lib" \
    "C:/Installed/SDL3/lib/SDL3_ttf.lib" \
    -lopengl32 -lwinmm -ldsound -ldxguid -luser32 -lkernel32

# Source and output
SRC = main.cpp \
      C:/Installed/glad/src/glad.c \
      C:/Installed/imgui/src/imgui.cpp \
      C:/Installed/imgui/src/imgui_draw.cpp \
      C:/Installed/imgui/src/imgui_tables.cpp \
      C:/Installed/imgui/src/imgui_widgets.cpp \
      C:/Installed/imgui/src/imgui_impl_sdl3.cpp \
      C:/Installed/imgui/src/imgui_impl_opengl3.cpp

OUT = art.exe

# Build target
$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)


# Clean build files
clean:
	del /Q *.exe *.o *.res 2>nul || true
