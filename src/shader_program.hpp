#pragma once

#include <glbinding/gl/gl.h>
using namespace gl;

#include <string>
#include <unordered_map>

class ShaderProgram
{
public:
    ShaderProgram();
    virtual ~ShaderProgram();

    void create();
    void reload();

    void use();

protected:
    void loadShaderSource(GLenum type, const char* path);
    void link();

    void setUniformBlockBinding(const char* name, GLuint bindingIndex);

    virtual void load() = 0;
    virtual void getAllUniformLocations() = 0;
    virtual void assignAllUniformBlockIndexes() {};

protected:
    GLuint id_;
    const char* name_;
    std::unordered_map<std::string, GLuint> shaderSourcesCompiled_;
};