#ifndef TEXTURES_H
#define TEXTURES_H

#include <glbinding/gl/gl.h>

using namespace gl;

// Load an EXR equirectangular map and return an OpenGL float texture ID.
// Returns 0 on failure.
GLuint loadEXRAsGLTexture(const char* path, float* outSunDirX = nullptr, float* outSunDirY = nullptr, float* outSunDirZ = nullptr, float* outSunColorR = nullptr, float* outSunColorG = nullptr, float* outSunColorB = nullptr);

class Texture2D 
{
public:
	Texture2D();
	~Texture2D();
	
	void load(const char* path);
	
	void setFiltering(GLenum filteringMode);
	void setWrap(GLenum wrapMode);

	void enableMipmap();

	void use();

private:
	GLuint m_id;
};


class TextureCubeMap
{
public:
	TextureCubeMap();
	~TextureCubeMap();
	
	void load(const char** path);

	void use();

private:
	GLuint m_id;
};


#endif // TEXTURES
