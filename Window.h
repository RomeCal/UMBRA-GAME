#pragma once
#include "CommonValues.h"

class Window
{
public:
    Window(GLint width = 1280, GLint height = 720);
    ~Window();
    bool Init();   // tries OpenGL 4.6 first, falls back to 3.3 (logged)

    GLint GetBufferWidth()  const { return bufferWidth; }
    GLint GetBufferHeight() const { return bufferHeight; }
    bool  ShouldClose() const { return glfwWindowShouldClose(mainWindow) != 0; }
    void  SetShouldClose() { glfwSetWindowShouldClose(mainWindow, GLFW_TRUE); }
    void  SwapBuffers() { glfwSwapBuffers(mainWindow); }
    void  SetTitle(const std::string& t) { glfwSetWindowTitle(mainWindow, t.c_str()); }
    void  SetVsync(bool on) { glfwSwapInterval(on ? 1 : 0); }
    void  SetCursorCaptured(bool captured);

    const bool* GetKeys() const { return keys; }
    bool  Key(int k) const { return k >= 0 && k < 1024 && keys[k]; }
    bool  Pressed(int k);                  // true once per key press (edge)
    GLfloat GetXChange();
    GLfloat GetYChange();

    int glMajor, glMinor;

private:
    GLFWwindow* mainWindow;
    GLint width, height, bufferWidth, bufferHeight;
    bool keys[1024];
    bool pressedFlags[1024];
    GLfloat lastX, lastY, xChange, yChange;
    bool mouseFirstMoved;

    static void HandleKeys(GLFWwindow*, int key, int code, int action, int mode);
    static void HandleMouse(GLFWwindow*, double xPos, double yPos);
    static void HandleResize(GLFWwindow*, int w, int h);
};
