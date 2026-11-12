#include "Window.h"
#include "Shader.h"

Window::Window(GLint w, GLint h)
    : glMajor(0), glMinor(0), mainWindow(nullptr), width(w), height(h), bufferWidth(0), bufferHeight(0),
      lastX(0), lastY(0), xChange(0), yChange(0), mouseFirstMoved(true)
{
    for (int i = 0; i < 1024; ++i) { keys[i] = false; pressedFlags[i] = false; }
}

bool Window::Init()
{
    if (!glfwInit()) { std::fprintf(stderr, "GLFW init failed\n"); return false; }

    // B1: ask for 4.6 first.  If a lab machine refuses, fall back to 3.3 and
    // tell the shader loader to rewrite the #version line (log it in the decision log).
    const int versions[2][2] = { {4, 6}, {3, 3} };
    for (int i = 0; i < 2 && !mainWindow; ++i)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, versions[i][0]);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, versions[i][1]);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        mainWindow = glfwCreateWindow(width, height, "UMBRA", nullptr, nullptr);
        if (mainWindow) { glMajor = versions[i][0]; glMinor = versions[i][1]; }
        else std::fprintf(stderr, "OpenGL %d.%d context refused, trying next.\n", versions[i][0], versions[i][1]);
    }
    if (!mainWindow) { std::fprintf(stderr, "Could not create a window.\n"); glfwTerminate(); return false; }

    if (glMajor == 3) Shader::SetVersionOverride("#version 330 core");

    glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);
    glfwMakeContextCurrent(mainWindow);
    glfwSetWindowUserPointer(mainWindow, this);
    glfwSetKeyCallback(mainWindow, HandleKeys);
    glfwSetCursorPosCallback(mainWindow, HandleMouse);
    glfwSetFramebufferSizeCallback(mainWindow, HandleResize);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::fprintf(stderr, "GLEW init failed\n"); return false; }
    glGetError();   // GLEW can leave a harmless GL_INVALID_ENUM behind

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glViewport(0, 0, bufferWidth, bufferHeight);
    glfwSwapInterval(1);

    std::printf("OpenGL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
    return true;
}

Window::~Window()
{
    if (mainWindow) glfwDestroyWindow(mainWindow);
    glfwTerminate();
}

void Window::SetCursorCaptured(bool captured)
{
    glfwSetInputMode(mainWindow, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    mouseFirstMoved = true;
    xChange = yChange = 0;
}

bool Window::Pressed(int k)
{
    if (k < 0 || k >= 1024) return false;
    bool r = pressedFlags[k];
    pressedFlags[k] = false;
    return r;
}

GLfloat Window::GetXChange() { GLfloat c = xChange; xChange = 0; return c; }
GLfloat Window::GetYChange() { GLfloat c = yChange; yChange = 0; return c; }

void Window::HandleKeys(GLFWwindow* w, int key, int, int action, int)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (key < 0 || key >= 1024) return;
    if (action == GLFW_PRESS)   { self->keys[key] = true; self->pressedFlags[key] = true; }
    if (action == GLFW_RELEASE) { self->keys[key] = false; }
}

void Window::HandleMouse(GLFWwindow* w, double x, double y)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self->mouseFirstMoved) { self->lastX = (GLfloat)x; self->lastY = (GLfloat)y; self->mouseFirstMoved = false; }
    self->xChange += (GLfloat)x - self->lastX;
    self->yChange += self->lastY - (GLfloat)y;
    self->lastX = (GLfloat)x;
    self->lastY = (GLfloat)y;
}

// B3: the framebuffer size changes -> main rebuilds the projection from the new aspect every frame.
void Window::HandleResize(GLFWwindow* w, int bw, int bh)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    self->bufferWidth = bw;
    self->bufferHeight = bh;
}
