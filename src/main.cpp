#include <iostream>
#include <chrono>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glm/glm.hpp>
using namespace glm;
#include <glm/gtc/matrix_transform.hpp>

#include <error_handling.h>
#include <gl/gl.h>
#include <utils.h>

#include <shaderprogram.h>
#include <renderer.h>
#include <camera.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define LOG_GL false

#define CAMERA_SENSITIVITY 0.01f

// #################
// # Error Logging #
// #################

bool logIsCompiled(GLuint shader)
{
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        GLchar* errorLog = new GLchar[maxLength];
        glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);

        std::cout << errorLog << std::endl;

        glDeleteShader(shader);
        return false;
    }

    return true;
}

void logGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    UNUSED(source);
    UNUSED(id);
    UNUSED(length);
    UNUSED(userParam);

    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}

// ###################
// # Input Callbacks #
// ###################

struct CallbackAccessibleData {
    ivec2 mousePosition;
    Camera* camera;
};

static void mouseCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    CallbackAccessibleData& data = *(CallbackAccessibleData*)glfwGetWindowUserPointer(window);
    glm::ivec2 mousePosition = glm::vec2(xpos, ypos);
    glm::vec2 offset = glm::vec2(mousePosition - data.mousePosition);

    // Middle Mouse : Pan
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) != GLFW_RELEASE)
    {
        data.camera->pan(CAMERA_SENSITIVITY * glm::vec2(-offset.x, offset.y));
    }

    // Left Mouse : Orbit
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE)
    {
        data.camera->orbit(CAMERA_SENSITIVITY * glm::vec2(offset.x, -offset.y));
    }

    // Right Mouse : Zoom
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_RELEASE)
    {
        data.camera->zoom(glm::max(0.1f, 1.f + CAMERA_SENSITIVITY * offset.y));
    }

    data.mousePosition = mousePosition;
}

bool run()
{
    // ##############
    // # Scene Init #
    // ##############

    Scene* defaultScene = new Scene("assets/smoothCube.obj");
    DEFER(delete defaultScene);
    Camera* camera = new Camera(glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.f, 0.f, 0.f), glm::uvec2(1280, 720));
    DEFER(delete camera);

    // #############
    // # GLFW Init #
    // #############

    glfwSetErrorCallback([](int error, const char* description) {
        printf("GLFW Error [%d] via callback: '%s'\n", error, description);
    });

    LOG_AND_RETURN_IF_ERROR(glfwInit());
    DEFER(glfwTerminate());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "PathTracer", nullptr, nullptr);
    LOG_AND_RETURN_IF_ERROR(window);
    DEFER(glfwDestroyWindow(window));

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Callbacks
    CallbackAccessibleData callbackAccessibleData {ivec2(), camera};
    glfwSetWindowUserPointer(window, &callbackAccessibleData);

    glfwSetCursorPosCallback(window, mouseCursorPosCallback);

    LOG_AND_RETURN_IF_ERROR(glewInit() == GLEW_OK);

    // ###############
    // # OpenGL Init #
    // ###############

#if LOG_GL
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(logGLError, 0);
#endif
    GLuint vao;

    ShaderProgram program = ShaderProgram("src/shaders/pathtracer/pathtracer.vert.glsl", "src/shaders/pathtracer/pathtracer.frag.glsl");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    Renderer renderer(program, defaultScene, camera);

    // #############
    // # Main Loop #
    // #############

    glUseProgram(program.m_id);
    while (!glfwWindowShouldClose(window))
    {
        renderer.updateCamera();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(renderer.m_program.m_id);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glUseProgram(0);

    return true;
}

int main(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    std::cout << "PathTracer" << std::endl;

    if (!run())
    {
        return 1;
    }

    return 0;
}
