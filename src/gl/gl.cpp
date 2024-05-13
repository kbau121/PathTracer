#include <cstdio>
#include <vector>

#include <GL/glew.h>

#include <error_handling.h>
#include <utils.h>

#include "gl.h"

namespace gl {

void _reportErrors(const char* filePath, int lineNumber)
{
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        return;
    }

    const char* errorString = "<unknown error>";
    if (error == GL_INVALID_ENUM) {
        errorString = "GL_INVALID_ENUM";
    } else if (error == GL_INVALID_VALUE) {
        errorString = "GL_INVALID_VALUE";
    } else if (error == GL_INVALID_OPERATION) {
        errorString = "GL_INVALID_OPERATION";
    } else if (error == GL_STACK_OVERFLOW) {
        errorString = "GL_STACK_OVERFLOW";
    } else if (error == GL_STACK_UNDERFLOW) {
        errorString = "GL_STACK_UNDERFLOW";
    } else if (error == GL_OUT_OF_MEMORY) {
        errorString = "GL_OUT_OF_MEMORY";
    } else if (error == GL_INVALID_FRAMEBUFFER_OPERATION) {
        errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
    } else if (error == GL_CONTEXT_LOST) {
        errorString = "GL_CONTEXT_LOST";
    }

    printf("GL Error [%d]: %s\n", error, errorString);
    printf("    at %s:%d\n", filePath, lineNumber);
}

void clearColor(vec4 color)
{
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void clearColor(vec3 color)
{
    clearColor(vec4(color, 0.0f));
}

void clearColor(vec2 color)
{
    clearColor(vec4(color, 0.0f, 0.0f));
}

void clearColor(float color)
{
    clearColor(vec4(color, 0.0f, 0.0f, 0.0f));
}

void clearDepth(float depth)
{
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
}

Program::Program()
{
}

Program::~Program()
{
}

static bool compileShader(GLenum shaderType, const char* shaderSource, GLuint* shader_out)
{
    GLuint shader = glCreateShader(shaderType);

    glShaderSource(shader, 1, &shaderSource, 0);
    glCompileShader(shader);

    GLint compileStatus = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, &logLength, log.data());

        printf("Shader compile failed. Log:\n%s\n", log.data());
        return false;
    }

    *shader_out = shader;
    return true;
}

static bool linkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint* program_out)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linkStatus = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<char> log(logLength);
        glGetProgramInfoLog(program, logLength, &logLength, log.data());

        printf("Program link failed. Log:\n%s\n", log.data());
        return false;
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    *program_out = program;
    return true;
}

bool Program::init(const char* vertexShaderSource, const char* fragmentShaderSource)
{
    GLuint vertexShader;
    LOG_AND_RETURN_IF_ERROR(compileShader(GL_VERTEX_SHADER, vertexShaderSource, &vertexShader));
    DEFER(glDeleteShader(vertexShader));

    GLuint fragmentShader;
    LOG_AND_RETURN_IF_ERROR(
        compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource, &fragmentShader));
    DEFER(glDeleteShader(fragmentShader));

    LOG_AND_RETURN_IF_ERROR(linkProgram(vertexShader, fragmentShader, &_id));

    return true;
}

bool Program::cleanUp()
{
    if (_id) {
        glDeleteProgram(_id);
        _id = 0;
    }

    return true;
}

void Program::use() const
{
    glUseProgram(_id);
}

GLuint Program::getID() const
{
    return _id;
}

Buffer::Buffer()
{
}

Buffer::~Buffer()
{
}

bool Buffer::init(void* data, size_t sizeBytes)
{
    GLuint buffer;
    glCreateBuffers(1, &buffer);

    glNamedBufferStorage(buffer, sizeBytes, data, 0);

    _id = buffer;
    return true;
}

bool Buffer::cleanUp()
{
    if (_id) {
        glDeleteBuffers(1, &_id);
        _id = 0;
    }

    return true;
}

GLuint Buffer::getID() const
{
    return _id;
}

VertexArray::VertexArray()
{
}

VertexArray::~VertexArray()
{
}

bool VertexArray::init()
{
    std::vector<BindingDesc> bindings;
    return init(bindings);
}

bool VertexArray::init(const std::vector<BindingDesc>& bindings)
{
    GLuint vertexArray = 0;
    glCreateVertexArrays(1, &vertexArray);

    for (size_t bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex) {
        const BindingDesc& binding = bindings[bindingIndex];

        uint32_t offsetBytes = 0;
        uint32_t strideBytes = binding.numComponents
            * ((binding.type == BindingType::Float) ? sizeof(float) : sizeof(uint32_t));
        glVertexArrayVertexBuffer(
            vertexArray,
            bindingIndex,
            binding.buffer->getID(),
            offsetBytes,
            strideBytes);
        glVertexArrayBindingDivisor(
            vertexArray,
            bindingIndex,
            binding.step == (BindingStep::PerVertex) ? 0 : 1);

        glEnableVertexArrayAttrib(vertexArray, bindingIndex);
        glVertexArrayAttribBinding(vertexArray, bindingIndex, bindingIndex);
        if (binding.type == BindingType::Float) {
            glVertexArrayAttribFormat(
                vertexArray,
                bindingIndex,
                binding.numComponents,
                GL_FLOAT,
                GL_FALSE,
                0);
        } else {
            glVertexArrayAttribIFormat(vertexArray, bindingIndex, binding.numComponents, GL_INT, 0);
        }
    }

    _id = vertexArray;
    return true;
}

bool VertexArray::cleanUp()
{
    if (_id) {
        glDeleteVertexArrays(1, &_id);
        _id = 0;
    }

    return true;
}

void VertexArray::bind() const
{
    glBindVertexArray(_id);
}

GLuint VertexArray::getID() const
{
    return _id;
}

Texture::Texture()
{
}

Texture::~Texture()
{
}

bool Texture::init(ivec2 size, GLint internalFormat)
{
    _id = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &_id);
    glTextureStorage2D(_id, 1, internalFormat, size.x, size.y);
    glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    GL_REPORT_ERRORS();

    return true;
}

bool Texture::init(ivec2 size, const uint8_t* data)
{
    RETURN_IF_ERROR(init(size, GL_RGBA8UI));

    glTextureSubImage2D(_id, 0, 0, 0, size.x, size.y, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, data);
    GL_REPORT_ERRORS();

    return true;
}

bool Texture::cleanUp()
{
    if (_id) {
        glDeleteTextures(1, &_id);
        _id = 0;
    }

    return true;
}

GLuint Texture::getID() const
{
    return _id;
}

bool Texture::bind(uint32_t unit) const
{
    glBindTextureUnit(unit, _id);

    return true;
}

Framebuffer::Framebuffer()
    : _id(0)
{
}

Framebuffer::~Framebuffer()
{
}

bool Framebuffer::init(std::vector<Texture*>& colorAttachments, Texture* depthAttachment)
{
    glCreateFramebuffers(1, &_id);

    std::vector<GLenum> attachmentNames;
    for (uint32_t i = 0; i < colorAttachments.size(); ++i) {
        Texture* colorAttachment = colorAttachments[i];
        glNamedFramebufferTexture(_id, GL_COLOR_ATTACHMENT0 + i, colorAttachment->getID(), 0);
        attachmentNames.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    if (depthAttachment) {
        glNamedFramebufferTexture(_id, GL_DEPTH_ATTACHMENT, depthAttachment->getID(), 0);
    }

    glNamedFramebufferDrawBuffers(_id, colorAttachments.size(), attachmentNames.data());

    return true;
}

bool Framebuffer::cleanUp()
{
    if (_id) {
        glDeleteFramebuffers(1, &_id);
        _id = 0;
    }

    return true;
}

GLuint Framebuffer::getID() const
{
    return _id;
}

bool Framebuffer::bind() const
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _id);

    return true;
}

bool Framebuffer::bindDefault()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    return true;
}

}
