#pragma once


#include <cstddef>
#include <cstdint>

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>
#include <random>
#include <algorithm>
#include <chrono>

#include <glbinding/gl/enum.h>


inline std::string readFile(std::string_view filename) {
	// Ouvrir le fichier
	std::ifstream file(filename.data());
	// Lire et retourner le contenu du fichier
	std::stringstream buffer;
    buffer << file.rdbuf();
    std::string contentStr = buffer.str();
    if (contentStr.empty())
        std::cout << "Warning: File \"" << filename << "\" does not exist or is empty." << std::endl;
    return contentStr;
}

inline std::string ltrim(std::string_view str) {
	auto it = std::find_if(str.begin(), str.end(), [](char c) { return not iswspace(c); });
	return (it == str.end()) ? "" : std::string(it, str.end());
}

inline std::string rtrim(std::string_view str) {
	auto it = std::find_if(str.rbegin(), str.rend(), [](char c) { return not iswspace(c); });
	return (it == str.rend()) ? "" : std::string(str.begin(), it.base());
}

inline std::string trim(std::string_view str) {
	return ltrim(rtrim(str));
}

inline std::string replaceAll(std::string str, const std::string& oldSubStr, const std::string& newSubStr) {
	size_t pos = 0;
	while ((pos = str.find(oldSubStr, pos)) != std::string::npos) {
		str.replace(pos, oldSubStr.length(), newSubStr);
		pos += newSubStr.length();
	}
	return str;
}

template <typename T>
inline constexpr gl::GLenum getTypeGLenum() {
	using namespace gl;

	if constexpr (std::is_same_v<T, GLbyte>)
		return GL_BYTE;
	else if constexpr (std::is_same_v<T, GLubyte>)
		return GL_UNSIGNED_BYTE;
	else if constexpr (std::is_same_v<T, GLshort>)
		return GL_SHORT;
	else if constexpr (std::is_same_v<T, GLushort>)
		return GL_UNSIGNED_SHORT;
	else if constexpr (std::is_same_v<T, GLint>)
		return GL_INT;
	else if constexpr (std::is_same_v<T, GLuint>)
		return GL_UNSIGNED_INT;
	else if constexpr (std::is_same_v<T, GLfloat>)
		return GL_FLOAT;
	else if constexpr (std::is_same_v<T, GLdouble>)
		return GL_DOUBLE;
	else
		return GL_INVALID_ENUM;
}

template <typename T>
constexpr gl::GLenum getTypeGLenum_v = getTypeGLenum<T>();

template <typename T1, typename T2, typename... Ts>
inline constexpr bool isTypeOneOf() {
	if constexpr (sizeof...(Ts) > 0)
		return std::is_same_v<T1, T2> or isTypeOneOf<T1, Ts...>();
	else
		return std::is_same_v<T1, T2>;
}

template <typename T1, typename T2, typename... Ts>
constexpr bool isTypeOneOf_v = isTypeOneOf<T1, T2, Ts...>();

inline double rand01()
{
	static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
	static std::uniform_real_distribution<double> distribution(0, 1);
	return distribution(generator);
}

