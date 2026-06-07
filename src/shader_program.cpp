#include "shader_program.hpp"

#include <iostream>

#include "inf2705/utils.hpp"


static bool checkShaderCompilingError(const char* name, GLuint id)
{
    GLint success;
    GLchar infoLog[1024];

    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(id, 1024, NULL, infoLog);
        std::cout << "Shader \"" << name << "\" compile error: " << infoLog << std::endl;
    }
    else
    {
        std::cout << "Shader \"" << name << "\" compiled successfully." << std::endl;
    }
    return success;
}


static bool checkProgramLinkingError(const char* name, GLuint id)
{
    GLint success;
    GLchar infoLog[1024];

    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(id, 1024, NULL, infoLog);
        std::cout << "Program \"" << name << "\" linking error: " << infoLog << std::endl;
    }
    else
    {
        std::cout << "Program \"" << name << "\" linked successfully.\n" << std::endl;
    }
    return success;
}

ShaderProgram::ShaderProgram()
: id_(0), name_("Uninitialized Name")
{

}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(id_);
}

void ShaderProgram::create()
{    
    id_ = glCreateProgram();
    load();
}

void ShaderProgram::reload()
{
    if (!id_)
        id_ = glCreateProgram();

    for (auto it = shaderSourcesCompiled_.cbegin(); it != shaderSourcesCompiled_.cend(); it++)
    {
        GLenum type;
        const std::string& path = it->first;
        if (path.find("vs") != std::string::npos)
            type = GL_VERTEX_SHADER;
        else if (path.find("tcs") != std::string::npos)
            type = GL_TESS_CONTROL_SHADER;
        else if (path.find("tes") != std::string::npos)
            type = GL_TESS_EVALUATION_SHADER;
        else if (path.find("gs") != std::string::npos)        
            type = GL_GEOMETRY_SHADER;
        else if (path.find("fs") != std::string::npos)
            type = GL_FRAGMENT_SHADER;
        else if (path.find("cs") != std::string::npos)
            type = GL_COMPUTE_SHADER;
        
        loadShaderSource(type, path.c_str());
    }
    link();
}

void ShaderProgram::use()
{
    glUseProgram(id_);
}



void ShaderProgram::loadShaderSource(GLenum type, const char* path)
{
    std::string code = readFile(path);
    GLuint shaderObject = glCreateShader(type);
    const char* codePtr = code.c_str();
    glShaderSource(shaderObject, 1, &codePtr, NULL);
    glCompileShader(shaderObject);
    if (checkShaderCompilingError(path, shaderObject))
    {
        glAttachShader(id_, shaderObject);
    }
    else
    {
        glDeleteShader(shaderObject);
        shaderObject = 0;
    }
    
    shaderSourcesCompiled_[path] = shaderObject;
}

void ShaderProgram::link()
{
    glLinkProgram(id_);
    if (!checkProgramLinkingError(name_, id_))
    {
        glDeleteProgram(id_);
        id_ = 0;
    }

    for (auto it = shaderSourcesCompiled_.cbegin(); it != shaderSourcesCompiled_.cend(); it++)
    {
        if (it->second)
        {
            if (id_)
                glDetachShader(id_, it->second);
            glDeleteShader(it->second);
        }
    }
    
    if (id_)
    {
        getAllUniformLocations();
        assignAllUniformBlockIndexes();
    }
}

void ShaderProgram::setUniformBlockBinding(const char* name, GLuint bindingIndex)
{
    GLuint blockIndex = glGetUniformBlockIndex(id_, name);
    glUniformBlockBinding(id_, blockIndex, bindingIndex);
}



