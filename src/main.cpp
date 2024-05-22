#include <iostream>

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

#define LOG_GL false

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

bool run()
{
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

    Scene* defaultScene = new Scene();
    DEFER(delete defaultScene);
    Renderer renderer(defaultScene, glm::ivec2(1280, 720), program);

    // #############
    // # Main Loop #
    // #############

    glUseProgram(program.m_id);
    while (!glfwWindowShouldClose(window))
    {
        float aspectRatio;
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        aspectRatio = width / (float) height;
        UNUSED(aspectRatio);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

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
