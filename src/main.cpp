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

#define LOG_GL false

static const struct
{
    vec3 position;
    vec3 color;
} vertices[3] =
{
    {vec3(-0.6f, -0.4f, 0.f), vec3(1.f, 0.f, 0.f)},
    {vec3(0.6f, -0.4f, 0.f), vec3(0.f, 1.f, 0.f)},
    {vec3(0.f, 0.6f, 0.f), vec3(0.f, 0.f, 1.f)}
};

static const char* vertex_shader_text =
R"(
#version 460

layout(location = 0) uniform mat4 MVP;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

layout(location = 0) out vec3 vColor;

void main()
{
    vColor = aColor;
    gl_Position = MVP * vec4(aPos, 1.f);
}
)";

static const char* frag_shader_text =
R"(
#version 460

layout(location = 0) in vec3 vColor;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(vColor, 1.f);
}
)";

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

    // #############

#if LOG_GL
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(logGLError, 0);
#endif

    GLuint vao, vertex_buffer, vertex_shader, frag_shader, program;
    GLint loc_mvp, loc_pos, loc_col;

    // program
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, nullptr);
    glCompileShader(vertex_shader);

    if (!logIsCompiled(vertex_shader))
    {
        return 1;
    }

    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_shader_text, nullptr);
    glCompileShader(frag_shader);

    if (!logIsCompiled(frag_shader))
    {
        return 1;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);

    loc_mvp = glGetUniformLocation(program, "MVP");
    loc_pos = glGetAttribLocation(program, "aPos");
    loc_col = glGetAttribLocation(program, "aColor");

    // bindings
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(loc_pos);
    glVertexAttribPointer(loc_pos, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), 0);

    glEnableVertexAttribArray(loc_col);
    glVertexAttribPointer(loc_col, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*) sizeof(vec3));

    // #############

    while (!glfwWindowShouldClose(window))
    {
        float aspectRatio;
        int width, height;
        mat4x4 model, view, projection, mvp;

        glfwGetFramebufferSize(window, &width, &height);
        aspectRatio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        model = mat4(1.f);
        model = glm::rotate(model, (float) glfwGetTime(), vec3(0.f, 1.f, 0.f));

        view = mat4(1.f);
        view = glm::translate(view, glm::vec3(0.f, 0.f, -10.f));

        projection = glm::ortho(-aspectRatio, aspectRatio, -1.f, 1.f, 0.1f, 100.f);
        mvp = projection * view * model;

        glUseProgram(program);
        glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, &mvp[0][0]);
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
