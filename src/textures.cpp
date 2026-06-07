#include "textures.hpp"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>

// Use system zlib instead of bundled miniz (project already has zlib via vcpkg)
#define TINYEXR_USE_MINIZ 0
#include <zlib.h>
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"
#include <cstdlib>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Texture2D::Texture2D()
: m_id(0)
{ 

}


static GLenum channelsFormat(int nChannels) {
    
    switch (nChannels) {
    case 1: return GL_RED;
    case 2: return GL_RG;
    case 3: return GL_RGB;
    case 4: return GL_RGBA;
    default: return GL_RGB;
    }
}

void Texture2D::load(const char* path)
{
    int width, height, nChannels;
    stbi_set_flip_vertically_on_load(true);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    unsigned char* data = stbi_load(path, &width, &height, &nChannels, 0);
    if (data == NULL) {
        std::cout << "Error loading texture \"" << path << "\": " << stbi_failure_reason() << std::endl;
        return;
    }
    
    if (m_id == 0) {
        glGenTextures(1, &m_id);
    }

    glBindTexture(GL_TEXTURE_2D, m_id);
    const GLenum format = channelsFormat(nChannels);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
}

Texture2D::~Texture2D()
{
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
    }
}

void Texture2D::setFiltering(GLenum filteringMode)
{
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filteringMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filteringMode);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::setWrap(GLenum wrapMode)
{
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::enableMipmap()
{
    glBindTexture(GL_TEXTURE_2D, m_id);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::use()
{
    glBindTexture(GL_TEXTURE_2D, m_id);
}

//
// Cubemap
//

TextureCubeMap::TextureCubeMap()
: m_id(0)
{

}

void TextureCubeMap::load(const char** pathes)
{
    const size_t N_TEXTURES = 6;
    unsigned char* datas[N_TEXTURES];
    int widths[N_TEXTURES];
    int heights[N_TEXTURES];
    int nChannels[N_TEXTURES];
    stbi_set_flip_vertically_on_load(false);
    for (unsigned int i = 0; i < 6; i++)
    {
        datas[i] = stbi_load(pathes[i], &widths[i], &heights[i], &nChannels[i], 0);
        if (datas[i] == NULL)
            std::cout << "Error loading texture \"" << pathes[i] << "\": " << stbi_failure_reason() << std::endl;
    }

        
    if (m_id == 0) {
        glGenTextures(1, &m_id);
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


    for (unsigned int i = 0; i < 6; i++)
    {
        if (!datas[i]) continue;

        const GLenum format = channelsFormat(nChannels[i]);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, widths[i],heights[i], 0, format, GL_UNSIGNED_BYTE, datas[i]);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);


    for (unsigned int i = 0; i < 6; i++)
    {
        stbi_image_free(datas[i]);
    }
}

TextureCubeMap::~TextureCubeMap()
{
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
    }
}

void TextureCubeMap::use()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
}


GLuint loadEXRAsGLTexture(const char* path, float* outSunDirX, float* outSunDirY, float* outSunDirZ, float* outSunColorR, float* outSunColorG, float* outSunColorB)
{
    float* rgba = nullptr;
    int width = 0;
    int height = 0;
    const char* err = nullptr;

    int ret = LoadEXR(&rgba, &width, &height, path, &err);
    if (ret != TINYEXR_SUCCESS)
    {
        std::cerr << "Error loading EXR \"" << path << "\": "
            << (err ? err : "unknown error") << std::endl;
        FreeEXRErrorMessage(err);
        return 0;
    }

    std::cout << "Loaded EXR: " << path << " (" << width << "x" << height << ")" << std::endl;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, rgba);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Compute brightest spot for the sun
    if (outSunDirX || outSunDirY || outSunDirZ || outSunColorR || outSunColorG || outSunColorB)
    {
        float maxBrightness = -1.0f;
        int maxPixelIndex = 0;

        for (int i = 0; i < width * height; ++i)
        {
            float r = rgba[4 * i + 0];
            float g = rgba[4 * i + 1];
            float b = rgba[4 * i + 2];
            float brightness = r * 0.2126f + g * 0.7152f + b * 0.0722f;
            if (brightness > maxBrightness)
            {
                maxBrightness = brightness;
                maxPixelIndex = i;
            }
        }

        int maxY = maxPixelIndex / width;
        int maxX = maxPixelIndex % width;

        float u = (maxX + 0.5f) / static_cast<float>(width);
        float v = (maxY + 0.5f) / static_cast<float>(height);

        float phi = (u - 0.5f) * 2.0f * M_PI;
        float theta = (0.5f - v) * M_PI;

        if (outSunDirX) *outSunDirX = std::cos(phi) * std::cos(theta);
        if (outSunDirY) *outSunDirY = std::sin(theta);
        if (outSunDirZ) *outSunDirZ = std::sin(phi) * std::cos(theta);

        if (outSunColorR || outSunColorG || outSunColorB)
        {
            float r = rgba[4 * maxPixelIndex + 0];
            float g = rgba[4 * maxPixelIndex + 1];
            float b = rgba[4 * maxPixelIndex + 2];

            float maxVal = std::fmax(r, std::fmax(g, b));
            if (maxVal > 0.0f) {
                r /= maxVal;
                g /= maxVal;
                b /= maxVal;
            }

            if (outSunColorR) *outSunColorR = r;
            if (outSunColorG) *outSunColorG = g;
            if (outSunColorB) *outSunColorB = b;
        }
    }

    free(rgba);
    return tex;
}

