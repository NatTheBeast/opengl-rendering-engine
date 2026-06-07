#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>

#include <inf2705/OpenGLApplication.hpp>

#include "model.hpp"
#include "shader_program.hpp"
#include "shaders.hpp"
#include "textures.hpp"

#define CHECK_GL_ERROR printGLError(__FILE__, __LINE__)

using namespace gl;
using namespace glm;

namespace
{
    constexpr int MAX_SPOT_LIGHTS = 12;
    constexpr std::size_t PARTICLE_COUNT = 80;

    float pseudoRandom(float x)
    {
        return fract(std::sin(x) * 43758.5453123f);
    }

    mat4 buildTransform(const vec3& position, const vec3& rotationDeg, const vec3& scaleValue)
    {
        mat4 model(1.0f);
        model = translate(model, position);
        model = rotate(model, radians(rotationDeg.x), vec3(1.0f, 0.0f, 0.0f));
        model = rotate(model, radians(rotationDeg.y), vec3(0.0f, 1.0f, 0.0f));
        model = rotate(model, radians(rotationDeg.z), vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scaleValue);
        return model;
    }

    GLuint createSolidTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
    {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        const unsigned char pixel[4] = { r, g, b, a };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    GLuint createParticleTexture()
    {
        constexpr int SIZE = 64;
        std::vector<unsigned char> pixels(SIZE * SIZE * 4, 0);

        for (int y = 0; y < SIZE; ++y)
        {
            for (int x = 0; x < SIZE; ++x)
            {
                const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(SIZE);
                const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(SIZE);
                const float dx = 2.0f * u - 1.0f;
                const float dy = 2.0f * v - 1.0f;
                const float dist = std::sqrt(dx * dx + dy * dy);
                const float alpha = std::max(0.0f, 1.0f - smoothstep(0.15f, 1.0f, dist));

                const std::size_t idx = static_cast<std::size_t>(4 * (y * SIZE + x));
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = static_cast<unsigned char>(255.0f * alpha);
            }
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE, SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    struct MaterialGpu
    {
        vec4 emission = vec4(0.0f);
        vec4 ambient = vec4(0.1f);
        vec4 diffuse = vec4(1.0f);
        vec4 specular = vec4(1.0f);
        vec4 params = vec4(32.0f, 0.0f, 0.0f, 0.0f);
    };

    struct DirectionalLightGpu
    {
        vec4 ambient = vec4(0.0f);
        vec4 diffuse = vec4(0.0f);
        vec4 specular = vec4(0.0f);
        vec4 direction = vec4(0.0f, -1.0f, 0.0f, 0.0f);
    };

    struct SpotLightGpu
    {
        vec4 ambient = vec4(0.0f);
        vec4 diffuse = vec4(0.0f);
        vec4 specular = vec4(0.0f);
        vec4 position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        vec4 direction = vec4(0.0f, -1.0f, 0.0f, 0.0f);
        vec4 params = vec4(12.0f, 24.0f, 0.0f, 0.0f);
    };

    struct LightingGpu
    {
        DirectionalLightGpu dirLight;
        std::array<SpotLightGpu, MAX_SPOT_LIGHTS> spotLights;
    };

    struct ParticleVertex
    {
        vec3 position;
        float zOrientation;
        vec4 color;
        vec2 size;
    };

    struct ParticleState
    {
        vec3 position = vec3(0.0f);
        vec3 velocity = vec3(0.0f);
        vec4 color = vec4(0.0f);
        vec2 size = vec2(0.0f);
        float zOrientation = 0.0f;
        float timeToLive = 0.0f;
        float maxTimeToLive = 1.0f;
        float seed = 0.0f;
    };
}

class PhongProgram : public ShaderProgram
{
public:
    GLuint mvpULoc = 0;
    GLuint viewULoc = 0;
    GLuint modelViewULoc = 0;
    GLuint normalMatrixULoc = 0;
    GLuint nSpotLightsULoc = 0;
    GLuint globalAmbientULoc = 0;
    GLuint useOrbGradientULoc = 0;
    GLuint orbInnerColorULoc = 0;
    GLuint orbOuterColorULoc = 0;

protected:
    void load() override
    {
        name_ = "PhongProgram";
        loadShaderSource(GL_VERTEX_SHADER, "./shaders/phong.vs.glsl");
        loadShaderSource(GL_FRAGMENT_SHADER, "./shaders/phong.fs.glsl");
        link();
    }

    void getAllUniformLocations() override
    {
        mvpULoc = glGetUniformLocation(id_, "mvp");
        viewULoc = glGetUniformLocation(id_, "view");
        modelViewULoc = glGetUniformLocation(id_, "modelView");
        normalMatrixULoc = glGetUniformLocation(id_, "normalMatrix");
        nSpotLightsULoc = glGetUniformLocation(id_, "nSpotLights");
        globalAmbientULoc = glGetUniformLocation(id_, "globalAmbient");
        useOrbGradientULoc = glGetUniformLocation(id_, "useOrbGradient");
        orbInnerColorULoc = glGetUniformLocation(id_, "orbInnerColor");
        orbOuterColorULoc = glGetUniformLocation(id_, "orbOuterColor");

        use();
        const GLint diffuseSampler = glGetUniformLocation(id_, "diffuseSampler");
        glUniform1i(diffuseSampler, 0);
        glUniform1i(useOrbGradientULoc, 0);
    }

    void assignAllUniformBlockIndexes() override
    {
        setUniformBlockBinding("MaterialBlock", 0);
        setUniformBlockBinding("LightingBlock", 1);
    }
};

struct App : public OpenGLApplication
{
    App()
        : cameraPosition_(0.0f, 3.0f, 12.0f)
        , cameraOrientation_(radians(-10.0f), 0.0f)
        , currentScene_(0)
        , isMouseMotionEnabled_(false)
    {
    }

    void init() override
    {
        setKeybindMessage(
            "ESC : quitter l'application." "\n"
            "T : changer de scène." "\n"
            "W : déplacer la caméra vers l'avant." "\n"
            "S : déplacer la caméra vers l'arrière." "\n"
            "A : déplacer la caméra vers la gauche." "\n"
            "D : déplacer la caméra vers la droite." "\n"
            "Q : déplacer la caméra vers le bas." "\n"
            "E : déplacer la caméra vers le haut." "\n"
            "Flèches : tourner la caméra." "\n"
            "Souris : tourner la caméra" "\n"
            "Espace : activer/désactiver la souris." "\n");

        glViewport(0, 0, static_cast<GLsizei>(window_.getSize().x), static_cast<GLsizei>(window_.getSize().y));
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glClearColor(0.03f, 0.03f, 0.06f, 1.0f);

        phongShader_.create();
        particlesDrawShader_.create();
        grassShader_.create();
        groundShader_.create();
        skyboxShader_.create();
        lensFlareShader_.create();
        
        glGenVertexArrays(1, &vaoDummy_);

        float sunDx, sunDy, sunDz, sunCr, sunCg, sunCb;
        skyTexture_ = loadEXRAsGLTexture("../textures/ciel.exr", &sunDx, &sunDy, &sunDz, &sunCr, &sunCg, &sunCb);
        if (skyTexture_ != 0) {
            sunDir_ = vec3(sunDx, sunDy, sunDz);
            sunColor_ = vec3(1.0f, 0.98f, 0.95f);
        }

        swordBladeModel_.load("../models/sword-blade.ply");
        swordGripModel_.load("../models/sword-grip.ply");
        swordGuardModel_.load("../models/sword-guard.ply");
        swordPommelModel_.load("../models/sword-pommel.ply");

        staffMainModel_.load("../models/staff-main.ply");
        staffSphereModel_.load("../models/staff-sphere.ply");

        swordTexture_.load("../textures/Longsword_10_low_Longsword_10_BaseColor.jpg");
        swordTexture_.setFiltering(GL_LINEAR);
        swordTexture_.setWrap(GL_REPEAT);
        swordTexture_.enableMipmap();

        staffTexture_.load("../textures/Staff.png");
        staffTexture_.setFiltering(GL_LINEAR);
        staffTexture_.setWrap(GL_CLAMP_TO_EDGE);
        staffTexture_.enableMipmap();

        whiteTexture_ = createSolidTexture(255, 255, 255, 255);
        particleTexture_ = createParticleTexture();

        initUniformBuffers();
        initParticles();
        initGround();
        initGrass();
    }

    void drawFrame() override
    {
        // macOS fix: re-request focus on first frame after window is fully visible
        if (frame_ == 0)
            window_.requestFocus();

        updateCameraInput();
        cameraOrientation_.x = clamp(cameraOrientation_.x, radians(-89.0f), radians(89.0f));
        elapsedTime_ += deltaTime_;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const mat4 proj = getPerspectiveProjectionMatrix();
        const mat4 view = getViewMatrix();

        updateLights();
        updateParticles();

        drawControlWindow();
        drawSkybox(view, proj);
        sceneMain(view, proj);
    }

    void onClose() override
    {
        if (materialUbo_ != 0)
            glDeleteBuffers(1, &materialUbo_);
        if (lightingUbo_ != 0)
            glDeleteBuffers(1, &lightingUbo_);
        if (particleVao_ != 0)
            glDeleteVertexArrays(1, &particleVao_);
        if (particleVbo_ != 0)
            glDeleteBuffers(1, &particleVbo_);
        if (whiteTexture_ != 0)
            glDeleteTextures(1, &whiteTexture_);
        if (particleTexture_ != 0)
            glDeleteTextures(1, &particleTexture_);

        if (vaoGrass_ != 0)
            glDeleteVertexArrays(1, &vaoGrass_);
        if (vboGrass_ != 0)
            glDeleteBuffers(1, &vboGrass_);
        if (eboGrass_ != 0)
            glDeleteBuffers(1, &eboGrass_);

        if (vaoGround_ != 0)
            glDeleteVertexArrays(1, &vaoGround_);
        if (vboGround_ != 0)
            glDeleteBuffers(1, &vboGround_);
        if (eboGround_ != 0)
            glDeleteBuffers(1, &eboGround_);

        if (skyTexture_ != 0)
            glDeleteTextures(1, &skyTexture_);
        if (skyVao_ != 0)
            glDeleteVertexArrays(1, &skyVao_);
    }

    void onKeyPress(const sf::Event::KeyPressed& key) override
    {
        using enum sf::Keyboard::Key;
        switch (key.code)
        {
        case Escape:
            window_.close();
            break;
        case Space:
            isMouseMotionEnabled_ = !isMouseMotionEnabled_;
            if (isMouseMotionEnabled_)
            {
                window_.setMouseCursorGrabbed(true);
                window_.setMouseCursorVisible(false);
            }
            else
            {
                window_.setMouseCursorGrabbed(false);
                window_.setMouseCursorVisible(true);
            }
            break;
        case T:
            currentScene_ = ++currentScene_ < N_SCENE_NAMES ? currentScene_ : 0;
            break;
        default:
            break;
        }
    }

    void onResize(const sf::Event::Resized& event) override
    {
        glViewport(0, 0, static_cast<GLsizei>(event.size.x), static_cast<GLsizei>(event.size.y));
    }

    void onMouseMove(const sf::Event::MouseMoved& mouseDelta) override
    {
        if (!isMouseMotionEnabled_)
            return;

        constexpr float MOUSE_SENSITIVITY = 0.1f;
        const float cameraMovementX = mouseDelta.position.y * MOUSE_SENSITIVITY;
        const float cameraMovementY = mouseDelta.position.x * MOUSE_SENSITIVITY;
        cameraOrientation_.y -= cameraMovementY * deltaTime_;
        cameraOrientation_.x -= cameraMovementX * deltaTime_;
    }

    void updateCameraInput()
    {
        if (!window_.hasFocus())
            return;

        if (isMouseMotionEnabled_)
        {
            const sf::Vector2u windowSize = window_.getSize();
            const sf::Vector2i windowHalfSize(static_cast<int>(windowSize.x / 2.0f), static_cast<int>(windowSize.y / 2.0f));
            sf::Mouse::setPosition(windowHalfSize, window_);
        }

        float cameraMovementX = 0.0f;
        float cameraMovementY = 0.0f;

        constexpr float KEYBOARD_MOUSE_SENSITIVITY = 1.5f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            cameraMovementX -= KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            cameraMovementX += KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            cameraMovementY -= KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            cameraMovementY += KEYBOARD_MOUSE_SENSITIVITY;

        cameraOrientation_.y -= cameraMovementY * deltaTime_;
        cameraOrientation_.x -= cameraMovementX * deltaTime_;

        vec3 positionOffset(0.0f);
        constexpr float SPEED = 8.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            positionOffset.z -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            positionOffset.z += SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            positionOffset.x -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            positionOffset.x += SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
            positionOffset.y -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
            positionOffset.y += SPEED;

        positionOffset = vec3(rotate(mat4(1.0f), cameraOrientation_.y, vec3(0.0f, 1.0f, 0.0f)) * vec4(positionOffset, 1.0f));
        cameraPosition_ += positionOffset * deltaTime_;
    }

    mat4 getViewMatrix()
    {
        // Detect camera movement for handheld intensity boost
        vec3 posDelta = cameraPosition_ - prevCameraPosition_;
        vec2 oriDelta = cameraOrientation_ - prevCameraOrientation_;
        float moveSpeed = length(posDelta) / std::max(deltaTime_, 0.001f);
        float rotSpeed = length(oriDelta) / std::max(deltaTime_, 0.001f);
        float motionAmount = clamp(moveSpeed * 0.04f + rotSpeed * 0.3f, 0.0f, 1.0f);
        prevCameraPosition_ = cameraPosition_;
        prevCameraOrientation_ = cameraOrientation_;

        // Smooth out the motion amount to prevent jitter from instant jumps in amplitude
        float targetBoost = 1.0f + motionAmount * 3.0f; // up to 4x when moving
        // Lerp towards target: quick to react, slower to fade out
        float lerpSpeed = (targetBoost > currentMoveBoost_) ? 10.0f : 2.0f;
        currentMoveBoost_ += (targetBoost - currentMoveBoost_) * clamp(deltaTime_ * lerpSpeed, 0.0f, 1.0f);

        // Handheld sway: layered sines at irrational frequencies for organic feel
        float t = elapsedTime_ * 0.4f; // Slower overall time multiplier for sluggish movement
        float baseIntensity = 0.012f;  // Wider panning at rest
        float intensity = baseIntensity * currentMoveBoost_;

        float swayPitch = intensity * (
            0.6f * sin(t * 1.37f) +
            0.3f * sin(t * 2.87f + 0.8f) +
            0.1f * sin(t * 7.13f + 2.1f)
        );
        float swayYaw = intensity * (
            0.5f * sin(t * 1.13f + 1.4f) +
            0.35f * sin(t * 3.31f + 0.3f) +
            0.15f * sin(t * 6.67f + 3.7f)
        );
        float swayRoll = intensity * 0.4f * (
            0.7f * sin(t * 0.97f + 2.5f) +
            0.3f * sin(t * 2.41f + 1.1f)
        );

        mat4 view(1.0f);
        view = rotate(view, swayRoll, vec3(0.0f, 0.0f, 1.0f));  // roll
        view = rotate(view, -(cameraOrientation_.x + swayPitch), vec3(1.0f, 0.0f, 0.0f));
        view = rotate(view, -(cameraOrientation_.y + swayYaw), vec3(0.0f, 1.0f, 0.0f));
        view = translate(view, -cameraPosition_);
        return view;
    }

    mat4 getPerspectiveProjectionMatrix() const
    {
        const float fov = radians(45.0f);
        const float aspect = static_cast<float>(window_.getSize().x) / static_cast<float>(window_.getSize().y);
        const float nearPlane = 0.1f;
        const float farPlane = 300.0f;
        return perspective(fov, aspect, nearPlane, farPlane);
    }

    // Shared helper: sets up lens flare uniforms, returns false if sun is behind camera
    bool setupLensFlareUniforms(const mat4& view, const mat4& proj)
    {
        vec4 sunClip = proj * view * vec4(cameraPosition_ + sunDir_ * 100.0f, 1.0f);
        if (sunClip.w <= 0.0f) return false;
        
        vec3 sunNdc = vec3(sunClip) / sunClip.w;
        vec2 sunScreen = vec2(sunNdc.x * 0.5f + 0.5f, sunNdc.y * 0.5f + 0.5f);
        
        lensFlareShader_.use();
        glUniform2f(lensFlareShader_.sunScreenPosULoc, sunScreen.x, sunScreen.y);
        glUniform1f(lensFlareShader_.aspectRatioULoc, getWindowAspect());
        glUniform3fv(lensFlareShader_.sunColorULoc, 1, value_ptr(sunColor_));
        glUniform1f(lensFlareShader_.timeULoc, elapsedTime_);
        return true;
    }

    // Pass 1: Sun glow + starburst — drawn BEFORE opaque geometry so it gets occluded
    void drawLensFlareSun(const mat4& view, const mat4& proj)
    {
        if (!setupLensFlareUniforms(view, proj)) return;
        
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glUniform1i(lensFlareShader_.renderModeULoc, 0);

        glBindVertexArray(vaoDummy_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Pass 2: Lens artifacts (ring, hexagons, caustic) — drawn AFTER geometry, always on top
    void drawLensFlareArtifacts(const mat4& view, const mat4& proj)
    {
        if (!setupLensFlareUniforms(view, proj)) return;
        
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glUniform1i(lensFlareShader_.renderModeULoc, 1);

        glBindVertexArray(vaoDummy_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void drawSkybox(const mat4& view, const mat4& proj)
    {
        if (skyTexture_ == 0)
            return;

        // Create a dummy VAO if needed (fullscreen triangle uses gl_VertexID)
        if (skyVao_ == 0)
            glGenVertexArrays(1, &skyVao_);

        // Extract rotation-only part of view matrix (no translation)
        const mat3 viewRot = mat3(view);
        const mat3 invViewRot = transpose(viewRot);
        const mat4 invProj = inverse(proj);

        skyboxShader_.use();
        glUniformMatrix4fv(skyboxShader_.invProjULoc, 1, GL_FALSE, value_ptr(invProj));
        glUniformMatrix3fv(skyboxShader_.invViewRotULoc, 1, GL_FALSE, value_ptr(invViewRot));
        glUniform1f(skyboxShader_.exposureULoc, getSkyExposure(view));
        glUniform1f(skyboxShader_.fovScaleULoc, skyFovScale_);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyTexture_);

        // Draw at depth = 1.0 (far plane), only where nothing has been drawn
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        glBindVertexArray(skyVao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

    void sceneMain(const mat4& view, const mat4& proj)
    {
        switch (currentScene_)
        {
        case 0:
        default:
            drawLensFlareSun(view, proj);     // Sun glow — will be occluded by geometry
            drawGround(view, proj);
            drawGrass(view, proj);
            drawSword(view, proj);
            drawStaff(view, proj);
            drawParticles(view, proj);
            drawLensFlareArtifacts(view, proj); // Lens artifacts — always on top
            break;
        }
    }

private:
    void initUniformBuffers()
    {
        glGenBuffers(1, &materialUbo_);
        glBindBuffer(GL_UNIFORM_BUFFER, materialUbo_);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialGpu), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, materialUbo_);

        glGenBuffers(1, &lightingUbo_);
        glBindBuffer(GL_UNIFORM_BUFFER, lightingUbo_);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(LightingGpu), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, lightingUbo_);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void initParticles()
    {
        particles_.resize(PARTICLE_COUNT);
        particleVertices_.resize(PARTICLE_COUNT);

        for (std::size_t i = 0; i < particles_.size(); ++i)
        {
            particles_[i].seed = static_cast<float>(i) * 9.73f;
            particles_[i].timeToLive = 0.0f;
        }

        glGenVertexArrays(1, &particleVao_);
        glGenBuffers(1, &particleVbo_);

        glBindVertexArray(particleVao_);
        glBindBuffer(GL_ARRAY_BUFFER, particleVbo_);
        glBufferData(GL_ARRAY_BUFFER, particleVertices_.size() * sizeof(ParticleVertex), particleVertices_.data(), GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), reinterpret_cast<void*>(offsetof(ParticleVertex, position)));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), reinterpret_cast<void*>(offsetof(ParticleVertex, zOrientation)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), reinterpret_cast<void*>(offsetof(ParticleVertex, color)));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), reinterpret_cast<void*>(offsetof(ParticleVertex, size)));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void initGround()
    {
        constexpr float GROUND_SIZE = 40.0f;
        constexpr int GRID_SIZE = 80;

        std::vector<vec3> groundVertices;
        std::vector<unsigned int> groundIndices;

        for (int z = 0; z < GRID_SIZE; ++z)
        {
            for (int x = 0; x < GRID_SIZE; ++x)
            {
                const float x0 = -GROUND_SIZE + x * (GROUND_SIZE * 2.0f / GRID_SIZE);
                const float x1 = x0 + (GROUND_SIZE * 2.0f / GRID_SIZE);
                const float z0 = -GROUND_SIZE + z * (GROUND_SIZE * 2.0f / GRID_SIZE);
                const float z1 = z0 + (GROUND_SIZE * 2.0f / GRID_SIZE);

                const unsigned int base = static_cast<unsigned int>(groundVertices.size());

                groundVertices.push_back(vec3(x0, 0.0f, z0));
                groundVertices.push_back(vec3(x1, 0.0f, z0));
                groundVertices.push_back(vec3(x1, 0.0f, z1));
                groundVertices.push_back(vec3(x0, 0.0f, z1));

                groundIndices.push_back(base + 0);
                groundIndices.push_back(base + 1);
                groundIndices.push_back(base + 2);
                groundIndices.push_back(base + 0);
                groundIndices.push_back(base + 2);
                groundIndices.push_back(base + 3);
            }
        }

        glGenVertexArrays(1, &vaoGround_);
        glGenBuffers(1, &vboGround_);
        glGenBuffers(1, &eboGround_);

        glBindVertexArray(vaoGround_);
        glBindBuffer(GL_ARRAY_BUFFER, vboGround_);
        glBufferData(GL_ARRAY_BUFFER, groundVertices.size() * sizeof(vec3), groundVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboGround_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, groundIndices.size() * sizeof(unsigned int), groundIndices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), reinterpret_cast<void*>(0));

        glBindVertexArray(0);
        
        groundIndexCount_ = static_cast<unsigned int>(groundIndices.size());
    }

    void initGrass()
    {
        constexpr float GRASS_SIZE = 40.0f;
        constexpr int GRID_SIZE = 30; // Reduced from 80 to fix extreme lag

        std::vector<vec3> grassVertices;
        std::vector<unsigned int> grassIndices;

        for (int z = 0; z < GRID_SIZE; ++z)
        {
            for (int x = 0; x < GRID_SIZE; ++x)
            {
                const float x0 = -GRASS_SIZE + x * (GRASS_SIZE * 2.0f / GRID_SIZE);
                const float x1 = x0 + (GRASS_SIZE * 2.0f / GRID_SIZE);
                const float z0 = -GRASS_SIZE + z * (GRASS_SIZE * 2.0f / GRID_SIZE);
                const float z1 = z0 + (GRASS_SIZE * 2.0f / GRID_SIZE);

                const unsigned int base = static_cast<unsigned int>(grassVertices.size());

                grassVertices.push_back(vec3(x0, 0.0f, z0));
                grassVertices.push_back(vec3(x1, 0.0f, z0));
                grassVertices.push_back(vec3(x1, 0.0f, z1));
                grassVertices.push_back(vec3(x0, 0.0f, z1));

                grassIndices.push_back(base + 0);
                grassIndices.push_back(base + 1);
                grassIndices.push_back(base + 2);
                grassIndices.push_back(base + 0);
                grassIndices.push_back(base + 2);
                grassIndices.push_back(base + 3);
            }
        }

        glGenVertexArrays(1, &vaoGrass_);
        glGenBuffers(1, &vboGrass_);
        glGenBuffers(1, &eboGrass_);

        glBindVertexArray(vaoGrass_);

        glBindBuffer(GL_ARRAY_BUFFER, vboGrass_);
        glBufferData(GL_ARRAY_BUFFER, grassVertices.size() * sizeof(vec3), grassVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboGrass_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, grassIndices.size() * sizeof(unsigned int), grassIndices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), reinterpret_cast<void*>(0));

        glBindVertexArray(0);

        grassIndexCount_ = static_cast<unsigned int>(grassIndices.size());
    }

    void updateLights()
    {
        lighting_.dirLight.ambient = vec4(sunColor_ * 0.4f, 0.0f);
        lighting_.dirLight.diffuse = vec4(sunColor_ * 1.5f, 0.0f);
        lighting_.dirLight.specular = vec4(sunColor_ * 1.8f, 0.0f);
        lighting_.dirLight.direction = vec4(normalize(-sunDir_), 0.0f);

        for (auto& spot : lighting_.spotLights)
            spot = SpotLightGpu{};

        const vec3 swordFocus = swordPosition_ + vec3(0.0f, 0.9f, 0.0f);
        lighting_.spotLights[0].ambient = vec4(0.18f, 0.14f, 0.10f, 0.0f);
        lighting_.spotLights[0].diffuse = vec4(1.00f, 0.82f, 0.62f, 0.0f);
        lighting_.spotLights[0].specular = vec4(0.95f, 0.88f, 0.76f, 0.0f);
        lighting_.spotLights[0].position = vec4(swordPosition_ + vec3(0.7f, 1.5f, 1.1f), 1.0f);
        lighting_.spotLights[0].direction = vec4(normalize(swordFocus - vec3(lighting_.spotLights[0].position)), 0.0f);
        lighting_.spotLights[0].params = vec4(16.0f, 28.0f, 0.0f, 0.0f);

        const vec3 staffOrbPosition = getStaffOrbPosition();
        const vec3 magicLightColor = magicColor_ * (0.35f * magicIntensity_);

        lighting_.spotLights[1].ambient = vec4(0.015f * magicColor_ * magicIntensity_, 0.0f);
        lighting_.spotLights[1].diffuse = vec4(magicLightColor, 0.0f);
        lighting_.spotLights[1].specular = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        lighting_.spotLights[1].position = vec4(staffOrbPosition + vec3(0.0f, 0.06f, 0.08f), 1.0f);
        lighting_.spotLights[1].direction = vec4(normalize(staffOrbPosition - vec3(lighting_.spotLights[1].position)), 0.0f);
        lighting_.spotLights[1].params = vec4(10.0f, 18.0f, 0.0f, 0.0f);

        glBindBuffer(GL_UNIFORM_BUFFER, lightingUbo_);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightingGpu), &lighting_);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void updateMaterialUbo(const MaterialGpu& material)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, materialUbo_);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(MaterialGpu), &material);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void drawModel(
        Model& model,
        const mat4& modelMatrix,
        const mat4& view,
        const mat4& proj,
        const MaterialGpu& material,
        GLuint rawTexture,
        bool useOrbGradient = false,
        const vec3& orbInnerColor = vec3(0.30f, 0.45f, 0.95f),
        const vec3& orbOuterColor = vec3(0.58f, 0.30f, 0.95f))
    {
        updateMaterialUbo(material);

        const mat4 modelView = view * modelMatrix;
        const mat4 mvp = proj * modelView;
        const mat3 normalMatrix = transpose(inverse(mat3(modelView)));

        phongShader_.use();
        glUniformMatrix4fv(phongShader_.mvpULoc, 1, GL_FALSE, value_ptr(mvp));
        glUniformMatrix4fv(phongShader_.viewULoc, 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(phongShader_.modelViewULoc, 1, GL_FALSE, value_ptr(modelView));
        glUniformMatrix3fv(phongShader_.normalMatrixULoc, 1, GL_FALSE, value_ptr(normalMatrix));
        glUniform1i(phongShader_.nSpotLightsULoc, 2);
        glUniform3fv(phongShader_.globalAmbientULoc, 1, value_ptr(globalAmbient_));

        glUniform1i(phongShader_.useOrbGradientULoc, useOrbGradient ? 1 : 0);
        glUniform3fv(phongShader_.orbInnerColorULoc, 1, value_ptr(orbInnerColor));
        glUniform3fv(phongShader_.orbOuterColorULoc, 1, value_ptr(orbOuterColor));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rawTexture);
        model.draw();

        glUniform1i(phongShader_.useOrbGradientULoc, 0);
    }

    void drawGround(const mat4& view, const mat4& proj)
    {
        const mat4 projView = proj * view;
        groundShader_.use();
        glUniformMatrix4fv(groundShader_.mvpULoc, 1, GL_FALSE, value_ptr(projView));
        glUniform1f(groundShader_.timeULoc, elapsedTime_);

        glDisable(GL_CULL_FACE);
        glBindVertexArray(vaoGround_);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(groundIndexCount_), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        glEnable(GL_CULL_FACE);
    }

    void drawGrass(const mat4& view, const mat4& proj)
    {
        const mat4 grassModel(1.0f);
        const mat4 grassMvp = proj * view * grassModel;
        const mat4 grassModelView = view * grassModel;

        grassShader_.use();
        glUniformMatrix4fv(grassShader_.mvpULoc, 1, GL_FALSE, value_ptr(grassMvp));
        glUniformMatrix4fv(grassShader_.modelViewULoc, 1, GL_FALSE, value_ptr(grassModelView));
        glUniform1f(grassShader_.timeULoc, elapsedTime_);
        glUniform3fv(grassShader_.sunLightDirULoc, 1, value_ptr(sunDir_));
        glUniform3fv(grassShader_.cameraPosULoc, 1, value_ptr(cameraPosition_));

        glPatchParameteri(GL_PATCH_VERTICES, 3);

        glDisable(GL_CULL_FACE);
        glBindVertexArray(vaoGrass_);
        glDrawElements(GL_PATCHES, static_cast<GLsizei>(grassIndexCount_), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    void drawSword(const mat4& view, const mat4& proj)
    {
        const mat4 swordModelMatrix = buildTransform(swordPosition_, swordRotationDeg_, swordScale_);

        MaterialGpu bladeMaterial;
        bladeMaterial.ambient = vec4(0.72f, 0.74f, 0.78f, 0.0f);
        bladeMaterial.diffuse = vec4(0.92f, 0.94f, 0.98f, 0.0f);
        bladeMaterial.specular = vec4(1.00f, 1.00f, 1.00f, 0.0f);
        bladeMaterial.params = vec4(128.0f, 0.0f, 0.0f, 0.0f);

        MaterialGpu guardMaterial;
        guardMaterial.ambient = vec4(0.42f, 0.40f, 0.38f, 0.0f);
        guardMaterial.diffuse = vec4(0.70f, 0.68f, 0.66f, 0.0f);
        guardMaterial.specular = vec4(0.80f, 0.80f, 0.80f, 0.0f);
        guardMaterial.params = vec4(72.0f, 0.0f, 0.0f, 0.0f);

        MaterialGpu gripMaterial;
        gripMaterial.ambient = vec4(0.34f, 0.20f, 0.10f, 0.0f);
        gripMaterial.diffuse = vec4(0.58f, 0.34f, 0.16f, 0.0f);
        gripMaterial.specular = vec4(0.08f, 0.08f, 0.08f, 0.0f);
        gripMaterial.params = vec4(10.0f, 0.0f, 0.0f, 0.0f);

        MaterialGpu pommelMaterial;
        pommelMaterial.ambient = vec4(0.30f, 0.18f, 0.10f, 0.0f);
        pommelMaterial.diffuse = vec4(0.48f, 0.28f, 0.14f, 0.0f);
        pommelMaterial.specular = vec4(0.14f, 0.14f, 0.14f, 0.0f);
        pommelMaterial.params = vec4(14.0f, 0.0f, 0.0f, 0.0f);

        drawModel(swordBladeModel_, swordModelMatrix, view, proj, bladeMaterial, whiteTexture_);
        drawModel(swordGuardModel_, swordModelMatrix, view, proj, guardMaterial, whiteTexture_);
        drawModel(swordGripModel_, swordModelMatrix, view, proj, gripMaterial, whiteTexture_);
        drawModel(swordPommelModel_, swordModelMatrix, view, proj, pommelMaterial, whiteTexture_);
    }

    void drawStaff(const mat4& view, const mat4& proj)
    {
        MaterialGpu staffMaterial;
        staffMaterial.ambient = vec4(0.45f, 0.34f, 0.22f, 0.0f);
        staffMaterial.diffuse = vec4(0.75f, 0.58f, 0.35f, 0.0f);
        staffMaterial.specular = vec4(0.10f, 0.10f, 0.10f, 0.0f);
        staffMaterial.params = vec4(18.0f, 0.0f, 0.0f, 0.0f);

        const vec3 animatedStaffPosition = getAnimatedStaffPosition();
        const vec3 animatedStaffRotation = vec3(
            staffRotationDeg_.x,
            staffRotationDeg_.y + elapsedTime_ * staffSpinSpeedDeg_,
            staffRotationDeg_.z);

        const mat4 staffMainMatrix = buildTransform(animatedStaffPosition, animatedStaffRotation, staffScale_);
        drawModel(staffMainModel_, staffMainMatrix, view, proj, staffMaterial, getTextureId(staffTexture_));

        MaterialGpu orbMaterial;
        orbMaterial.ambient = vec4(1.0f, 1.0f, 1.0f, 0.0f);
        orbMaterial.diffuse = vec4(1.0f, 1.0f, 1.0f, 0.0f);
        orbMaterial.specular = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        orbMaterial.params = vec4(1.0f, 0.0f, 0.0f, 0.0f);

        const mat4 staffOrbMatrix = buildTransform(animatedStaffPosition, animatedStaffRotation, staffScale_);

        const vec3 orbInner = vec3(0.35f, 0.65f, 1.00f);
        const vec3 orbOuter = vec3(0.48f, 0.28f, 0.90f);

        drawModel(staffSphereModel_, staffOrbMatrix, view, proj, orbMaterial, whiteTexture_, true, orbInner, orbOuter);
    }

    vec3 getAnimatedStaffPosition() const
    {
        vec3 position = staffPosition_;
        if (animateStaff_)
            position.y += staffFloatAmplitude_ * std::sin(elapsedTime_ * staffFloatSpeed_);
        return position;
    }

    vec3 getStaffOrbPosition() const
    {
        return getAnimatedStaffPosition() + vec3(0.0f, 1.05f * staffScale_.y, 0.0f);
    }

    void resetParticle(ParticleState& particle, std::size_t index)
    {
        const vec3 emitter = getStaffOrbPosition();

        const float angle = pseudoRandom(static_cast<float>(index) * 12.989f) * 6.28318f;
        const float radius = 0.015f + 0.02f * pseudoRandom(static_cast<float>(index) * 7.123f);

        particle.position = emitter + vec3(cos(angle) * radius, 0.0f, sin(angle) * radius);
        particle.velocity = vec3(cos(angle) * 0.05f, 0.15f + 0.05f * pseudoRandom(static_cast<float>(index)), sin(angle) * 0.05f);
        particle.timeToLive = 0.4f + 0.3f * pseudoRandom(static_cast<float>(index));
        particle.maxTimeToLive = particle.timeToLive;
        particle.size = vec2(0.03f);
        particle.color = vec4(magicColor_ * 0.6f, 0.0f);
        particle.zOrientation = angle;
    }

    void updateParticles()
    {
        if (!showParticles_)
        {
            for (auto& vertex : particleVertices_)
            {
                vertex.position = vec3(0.0f, -1000.0f, 0.0f);
                vertex.zOrientation = 0.0f;
                vertex.color = vec4(0.0f);
                vertex.size = vec2(0.0f);
            }
            uploadParticleBuffer();
            return;
        }

        for (std::size_t i = 0; i < particles_.size(); ++i)
        {
            ParticleState& particle = particles_[i];
            if (particle.timeToLive <= 0.0f)
            {
                resetParticle(particle, i);
            }
            else
            {
                particle.timeToLive -= deltaTime_;
                const float life = 1.0f - particle.timeToLive / particle.maxTimeToLive;
                particle.position += particle.velocity * deltaTime_ * particleSpeed_;
                particle.zOrientation += deltaTime_ * (0.8f + 0.8f * particleSpeed_);

                const float fadeIn = smoothstep(0.0f, 0.12f, life);
                const float fadeOut = 1.0f - smoothstep(0.65f, 1.0f, life);
                const float alpha = fadeIn * fadeOut * 0.95f;
                const vec3 particleColor = mix(magicColor_ * 0.45f, magicColor_ * (1.4f * magicIntensity_), life);

                particle.color = vec4(particleColor, alpha);
                particle.size = vec2(0.03f + 0.05f * life);
            }

            particleVertices_[i].position = particle.position;
            particleVertices_[i].zOrientation = particle.zOrientation;
            particleVertices_[i].color = particle.color;
            particleVertices_[i].size = particle.size;
        }

        uploadParticleBuffer();
    }

    void uploadParticleBuffer()
    {
        glBindBuffer(GL_ARRAY_BUFFER, particleVbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particleVertices_.size() * sizeof(ParticleVertex), particleVertices_.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void drawParticles(const mat4& view, const mat4& proj)
    {
        if (!showParticles_)
            return;

        particlesDrawShader_.use();
        glUniformMatrix4fv(particlesDrawShader_.projULoc, 1, GL_FALSE, value_ptr(proj));
        glUniformMatrix4fv(particlesDrawShader_.modelViewULoc, 1, GL_FALSE, value_ptr(view));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, particleTexture_);

        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glBindVertexArray(particleVao_);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particleVertices_.size()));
        glBindVertexArray(0);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
    }

    void drawControlWindow()
    {
        ImGui::Begin("Scene Parameters");
        ImGui::Combo("Scene", &currentScene_, SCENE_NAMES, N_SCENE_NAMES);
        ImGui::Separator();

        ImGui::Text("Staff animation");
        ImGui::Checkbox("Animate staff", &animateStaff_);
        ImGui::SliderFloat("Float amplitude", &staffFloatAmplitude_, 0.0f, 1.5f);
        ImGui::SliderFloat("Float speed", &staffFloatSpeed_, 0.0f, 6.0f);
        ImGui::SliderFloat("Spin speed", &staffSpinSpeedDeg_, 0.0f, 90.0f);

        ImGui::Separator();
        ImGui::Text("Skybox");
        ImGui::SliderFloat("Sky exposure", &skyExposure_, 0.1f, 5.0f);
        ImGui::SliderFloat("Sky FOV scale", &skyFovScale_, 0.1f, 5.0f);

        ImGui::Separator();
        ImGui::Text("Magic effect");
        ImGui::Checkbox("Show particles", &showParticles_);
        ImGui::ColorEdit3("Magic color", value_ptr(magicColor_));
        ImGui::SliderFloat("Magic intensity", &magicIntensity_, 0.0f, 4.0f);
        ImGui::SliderFloat("Particle speed", &particleSpeed_, 0.2f, 3.0f);

        if (ImGui::CollapsingHeader("Sword transform"))
        {
            ImGui::SliderFloat3("Sword position", value_ptr(swordPosition_), -5.0f, 5.0f);
            ImGui::SliderFloat3("Sword rotation", value_ptr(swordRotationDeg_), -180.0f, 180.0f);
            ImGui::SliderFloat3("Sword scale", value_ptr(swordScale_), 0.2f, 4.0f);
        }

        if (ImGui::CollapsingHeader("Staff transform"))
        {
            ImGui::SliderFloat3("Staff position", value_ptr(staffPosition_), -5.0f, 5.0f);
            ImGui::SliderFloat3("Staff base rotation", value_ptr(staffRotationDeg_), -180.0f, 180.0f);
            ImGui::SliderFloat3("Staff scale", value_ptr(staffScale_), 0.2f, 4.0f);
        }

        ImGui::Text("Camera position : %.2f %.2f %.2f", cameraPosition_.x, cameraPosition_.y, cameraPosition_.z);
        ImGui::End();
    }

    static GLuint getTextureId(Texture2D& texture)
    {
        texture.use();
        GLint current = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &current);
        glBindTexture(GL_TEXTURE_2D, 0);
        return static_cast<GLuint>(current);
    }

private:
    vec3 cameraPosition_;
    vec2 cameraOrientation_;
    vec3 prevCameraPosition_ = vec3(0.0f);
    vec2 prevCameraOrientation_ = vec2(0.0f);
    float currentMoveBoost_ = 1.0f;

    const char* const SCENE_NAMES[1] = {
        "Main Scene"
    };
    const int N_SCENE_NAMES = sizeof(SCENE_NAMES) / sizeof(SCENE_NAMES[0]);
    int currentScene_;

    bool isMouseMotionEnabled_;

    PhongProgram phongShader_;
    ParticlesDraw particlesDrawShader_;
    GrassShader grassShader_;
    BaseShader groundShader_;
    SkyboxShader skyboxShader_;
    LensFlareShader lensFlareShader_;

    Model swordBladeModel_;
    Model swordGripModel_;
    Model swordGuardModel_;
    Model swordPommelModel_;
    Model staffMainModel_;
    Model staffSphereModel_;
    Texture2D swordTexture_;
    Texture2D staffTexture_;

    GLuint whiteTexture_ = 0;
    GLuint particleTexture_ = 0;

    GLuint materialUbo_ = 0;
    GLuint lightingUbo_ = 0;

    LightingGpu lighting_;
    vec3 globalAmbient_ = vec3(0.18f, 0.18f, 0.20f);

    GLuint particleVao_ = 0;
    GLuint particleVbo_ = 0;
    std::vector<ParticleState> particles_;
    std::vector<ParticleVertex> particleVertices_;

    GLuint vaoGrass_ = 0;
    GLuint vboGrass_ = 0;
    GLuint eboGrass_ = 0;
    unsigned int grassIndexCount_ = 0;

    GLuint vaoGround_ = 0;
    GLuint vboGround_ = 0;
    GLuint eboGround_ = 0;
    unsigned int groundIndexCount_ = 0;
    
    GLuint vaoDummy_ = 0;

    GLuint skyTexture_ = 0;
    GLuint skyVao_ = 0;
    float skyExposure_ = 1.0f;
    float skyFovScale_ = 1.0f;

    vec3 sunDir_ = vec3(-0.35f, -1.0f, -0.2f);
    vec3 sunColor_ = vec3(1.0f);

    // Returns dynamic sky exposure based on camera looking direction vs sun
    float getSkyExposure(const mat4& view) const
    {
        // Extract camera forward from view matrix (3rd row, negated = world-space forward)
        vec3 camForward = -vec3(view[0][2], view[1][2], view[2][2]);
        camForward = normalize(camForward);

        // Dot with sun direction: +1 = looking at sun, -1 = away
        float d = dot(camForward, normalize(sunDir_));
        // Map [-1, 1] -> [0, 1]
        float t = clamp((d + 1.0f) * 0.5f, 0.0f, 1.0f);
        // Interpolate between 15% and 80% of base exposure
        float factor = mix(0.3f, 0.8f, t);
        return skyExposure_ * factor;
    }

    float elapsedTime_ = 0.0f;

    vec3 swordPosition_ = vec3(-1.35f, 0.0f, 0.0f);
    vec3 swordRotationDeg_ = vec3(0.0f, 0.0f, 18.0f);
    vec3 swordScale_ = vec3(2.1f);

    vec3 staffPosition_ = vec3(1.45f, 0.0f, 0.1f);
    vec3 staffRotationDeg_ = vec3(0.0f, 0.0f, 0.0f);
    vec3 staffScale_ = vec3(2.0f);

    bool animateStaff_ = true;
    bool showParticles_ = true;
    float staffFloatAmplitude_ = 0.25f;
    float staffFloatSpeed_ = 1.8f;
    float staffSpinSpeedDeg_ = 20.0f;

    vec3 magicColor_ = vec3(0.35f, 0.55f, 1.0f);
    float magicIntensity_ = 1.7f;
    float particleSpeed_ = 0.4f;
};

int main(int argc, char* argv[])
{
    WindowSettings settings = {};
    settings.videoMode = sf::VideoMode({1200, 600});
    settings.fps = 60;
    settings.context.depthBits = 24;
    settings.context.stencilBits = 8;
    settings.context.antiAliasingLevel = 4;
    settings.context.majorVersion = 4;
    settings.context.minorVersion = 1;
    settings.context.attributeFlags = sf::ContextSettings::Attribute::Core;

    App app;
    app.run(argc, argv, "Tp4", settings);
}
