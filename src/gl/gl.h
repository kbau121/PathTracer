#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>
using namespace glm;

#define GL_REPORT_ERRORS() gl::_reportErrors(__FILE__, __LINE__)

namespace gl {

void _reportErrors(const char* filePath, int lineNumber);

void clearColor(vec4 color);
void clearColor(vec3 color);
void clearColor(vec2 color);
void clearColor(float color);

void clearDepth(float depth);

class Program {
public:
    Program();
    ~Program();

    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    bool init(const char* vertexShaderSource, const char* fragmentShaderSource);
    bool cleanUp();

    GLuint getID() const;

    void use() const;

private:
    GLuint _id;
};

class Buffer {
public:
    Buffer();
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    bool init(void* data, size_t sizeBytes);
    bool cleanUp();

    GLuint getID() const;

private:
    GLuint _id;
};

enum class BindingStep
{
    PerVertex,
    PerInstance,
};

enum class BindingType
{
    Float,
    Int,
};

struct BindingDesc {
    Buffer* buffer;
    uint32_t numComponents;
    BindingType type;
    BindingStep step;
};

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    bool init();
    bool init(const std::vector<BindingDesc>& bindings);
    bool cleanUp();

    GLuint getID() const;

    void bind() const;

private:
    GLuint _id;
};

class Texture {
public:
    Texture();
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool init(ivec2 size, GLint internalFormat);
    bool init(ivec2 size, const uint8_t* data);
    bool cleanUp();

    GLuint getID() const;

    bool bind(uint32_t unit) const;

private:
    GLuint _id;
};

class Framebuffer {
public:
    Framebuffer();
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    bool init(std::vector<Texture*>& colorAttachments, Texture* depthAttachment);
    bool cleanUp();

    GLuint getID() const;

    bool bind() const;

    static bool bindDefault();

private:
    GLuint _id;
};

}
