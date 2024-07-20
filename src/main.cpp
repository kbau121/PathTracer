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
    Renderer* renderer;
};

static void mouseCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    CallbackAccessibleData& data = *(CallbackAccessibleData*)glfwGetWindowUserPointer(window);
    Renderer* renderer = data.renderer;
    Camera* camera = renderer->m_camera;

    glm::ivec2 mousePosition = glm::vec2(xpos, ypos);
    glm::vec2 offset = glm::vec2(mousePosition - data.mousePosition);

    // Middle Mouse : Pan
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) != GLFW_RELEASE)
    {
        camera->pan(CAMERA_SENSITIVITY * glm::vec2(-offset.x, offset.y));
        renderer->updateCamera();
    }

    // Left Mouse : Orbit
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE)
    {
        camera->orbit(CAMERA_SENSITIVITY * glm::vec2(offset.x, -offset.y));
        renderer->updateCamera();
    }

    // Right Mouse : Zoom
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_RELEASE)
    {
        camera->zoom(glm::max(0.1f, 1.f + CAMERA_SENSITIVITY * offset.y));
        renderer->updateCamera();
    }

    data.mousePosition = mousePosition;
}

static void windowSizeCallback(GLFWwindow* window, int width, int height)
{
    CallbackAccessibleData& data = *(CallbackAccessibleData*)glfwGetWindowUserPointer(window);
    data.renderer->resize(glm::uvec2(width, height));
}

bool run()
{
    // ##############
    // # Scene Init #
    // ##############

    // TODO add a way to define lights from referenced files
    Scene* defaultScene = new Scene("assets/transmissionBox.obj", "assets/");
    defaultScene->m_lights = std::vector<Scene::Light> { Scene::Light(glm::vec3(4.f), glm::vec3(0.f, 1.95f, 0.f), glm::vec3(3.14f / 2.f, 0.f, 0.f), glm::vec3(1.25f, 1.25f, 1.f)) };
    defaultScene->m_lightCount = glm::uvec4(1, 0, 0, 0);

    DEFER(delete defaultScene);
    Camera* camera = new Camera(glm::vec3(0.f, 1.5f, 15.f), glm::vec3(0.f, -0.25f, 0.f), glm::uvec2(1280, 720));
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
    glfwSetCursorPosCallback(window, mouseCursorPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);

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
    ShaderProgram postProgram = ShaderProgram("src/shaders/pathtracer/pathtracer.vert.glsl", "src/shaders/pathtracer/post.frag.glsl");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    Renderer renderer(program, postProgram, defaultScene, camera);

    // Callback data
    CallbackAccessibleData callbackAccessibleData {ivec2(), &renderer};
    glfwSetWindowUserPointer(window, &callbackAccessibleData);

    // #############
    // # Main Loop #
    // #############

    glUseProgram(program.m_id);
    while (!glfwWindowShouldClose(window))
    {
        renderer.draw();

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
