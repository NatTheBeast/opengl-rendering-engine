#include "shader_program.hpp"
#include <glm/glm.hpp>

class BaseShader : public ShaderProgram
{
    public: 
        GLuint mvpULoc;
        GLuint timeULoc;
    protected:
        virtual void load() override;
        virtual void getAllUniformLocations() override;
};

class GrassShader : public ShaderProgram
{
public:
    GLuint mvpULoc;
    GLuint modelViewULoc;
    GLuint timeULoc; 
    GLuint sunLightDirULoc;
    GLuint cameraPosULoc;
protected:
    virtual void load() override;
    virtual void getAllUniformLocations() override;
};

class ParticlesUpdate : public ShaderProgram
{
public:
    GLuint timeULoc;
    GLuint deltaTimeULoc;
    GLuint emitterPositionULoc;
    GLuint emitterDirectionULoc;

protected:
    virtual void load() override;
    virtual void getAllUniformLocations() override;
};

class ParticlesDraw : public ShaderProgram
{
public:
    GLuint projULoc;
    GLuint modelViewULoc;

protected:
    virtual void load() override;
    virtual void getAllUniformLocations() override;
};

class SkyboxShader : public ShaderProgram
{
public:
    GLuint invProjULoc;
    GLuint invViewRotULoc;
    GLuint exposureULoc;
    GLuint fovScaleULoc;

protected:
    virtual void load() override;
    virtual void getAllUniformLocations() override;
};

class LensFlareShader : public ShaderProgram
{
public:
    GLuint sunScreenPosULoc;
    GLuint aspectRatioULoc;
    GLuint sunColorULoc;
    GLuint timeULoc;
    GLuint renderModeULoc;

protected:
    virtual void load() override;
    virtual void getAllUniformLocations() override;
};
