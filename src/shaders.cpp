#include "shaders.hpp"

#include <glm/gtc/type_ptr.hpp>

void BaseShader::load() 
{
    name_ = "BaseShader";
    loadShaderSource(GL_VERTEX_SHADER, "./shaders/base.vs.glsl");
    loadShaderSource(GL_FRAGMENT_SHADER, "./shaders/base.fs.glsl");
    link();
}

void BaseShader::getAllUniformLocations() 
{
    mvpULoc = glGetUniformLocation(id_, "mvp");
    timeULoc = glGetUniformLocation(id_, "time");
}


void GrassShader::load()
{
    name_ = "GrassShader";
    loadShaderSource(GL_VERTEX_SHADER, "./shaders/grass.vs.glsl");
    loadShaderSource(GL_TESS_CONTROL_SHADER, "./shaders/grass.tcs.glsl");
    loadShaderSource(GL_TESS_EVALUATION_SHADER, "./shaders/grass.tes.glsl");
    loadShaderSource(GL_GEOMETRY_SHADER, "./shaders/grass.gs.glsl");
    loadShaderSource(GL_FRAGMENT_SHADER, "./shaders/grass.fs.glsl");
    link();
}

void GrassShader::getAllUniformLocations()
{
    mvpULoc = glGetUniformLocation(id_, "mvp");
    modelViewULoc = glGetUniformLocation(id_, "modelView");
    timeULoc = glGetUniformLocation(id_, "time");
    sunLightDirULoc = glGetUniformLocation(id_, "sunLightDir");
    cameraPosULoc = glGetUniformLocation(id_, "cameraPos");
}

void ParticlesUpdate::load()
{
    name_ = "ParticlesUpdate";
    loadShaderSource(GL_COMPUTE_SHADER, "./shaders/particlesUpdate.cs.glsl");
    link();
}

void ParticlesUpdate::getAllUniformLocations()
{
    timeULoc = glGetUniformLocation(id_, "time");
    deltaTimeULoc = glGetUniformLocation(id_, "deltaTime");
    emitterPositionULoc = glGetUniformLocation(id_, "emitterPosition");
    emitterDirectionULoc = glGetUniformLocation(id_, "emitterDirection");
}

void ParticlesDraw::load()
{
    name_ = "ParticlesDraw";
    loadShaderSource(GL_VERTEX_SHADER, "./shaders/particlesDraw.vs.glsl");
    loadShaderSource(GL_GEOMETRY_SHADER, "./shaders/particlesDraw.gs.glsl");
    loadShaderSource(GL_FRAGMENT_SHADER, "./shaders/particlesDraw.fs.glsl");
    link();
}

void ParticlesDraw::getAllUniformLocations()
{
    projULoc = glGetUniformLocation(id_, "proj");
    modelViewULoc = glGetUniformLocation(id_, "modelView");

    use();
    GLint texloc = glGetUniformLocation(id_, "textureSampler");
    glUniform1i(texloc, 0);
}

void SkyboxShader::load()
{
    name_ = "SkyboxShader";
    loadShaderSource(GL_VERTEX_SHADER, "./shaders/skybox.vs.glsl");
    loadShaderSource(GL_FRAGMENT_SHADER, "./shaders/skybox.fs.glsl");
    link();
}

void SkyboxShader::getAllUniformLocations()
{
    invProjULoc = glGetUniformLocation(id_, "invProj");
    invViewRotULoc = glGetUniformLocation(id_, "invViewRot");
    exposureULoc = glGetUniformLocation(id_, "exposure");
    fovScaleULoc = glGetUniformLocation(id_, "fovScale");

    use();
    GLint texloc = glGetUniformLocation(id_, "skyTexture");
    glUniform1i(texloc, 0);
}

void LensFlareShader::load()
{
    name_ = "LensFlareShader";
    loadShaderSource(GL_VERTEX_SHADER, "./shaders/lensflare.vs.glsl");
    loadShaderSource(GL_FRAGMENT_SHADER, "./shaders/lensflare.fs.glsl");
    link();
}

void LensFlareShader::getAllUniformLocations()
{
    sunScreenPosULoc = glGetUniformLocation(id_, "sunScreenPos");
    aspectRatioULoc = glGetUniformLocation(id_, "aspectRatio");
    sunColorULoc = glGetUniformLocation(id_, "sunColor");
    timeULoc = glGetUniformLocation(id_, "time");
    renderModeULoc = glGetUniformLocation(id_, "renderMode");
}