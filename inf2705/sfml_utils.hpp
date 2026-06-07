#pragma once

#ifdef _WIN32
	// Juste pour s'assurer de traiter le code source en UTF-8 sur Windows avec Visual Studio.
	#pragma execution_character_set("utf-8")
#endif


#include <cstddef>
#include <cstdint>

#include <string>
#include <unordered_map>

#include <SFML/Window.hpp>


struct MouseState
{
	bool buttons[sf::Mouse::ButtonCount] = {};
	// Les coordonnées en pixels dans le référentiel de l'écran (0,0 est coin haut-gauche).
	sf::Vector2i absolute;
	// Les coordonnées en pixels dans le référentiel de la fenêtre (0,0 est coin haut-gauche).
	sf::Vector2i relative;
	// Les coordonnées normalisées en [-1,1] où 0,0 est le centre de la fenêtre.
	sf::Vector2f normalized;
	bool isInsideWindow;
};


// Convertir une std::string UTF-8 en sf::String.
inline sf::String sfStr(std::string_view s) {
	return sf::String::fromUtf8(s.begin(), s.end());
}

// Obtenir la description d'une touche. Ce nom va dépendre de la langue de l'OS.
inline std::string getKeyDescription(const sf::Event::KeyPressed& key) {
	return std::string(sf::Keyboard::getDescription(key.scancode));
}

// Obtenir le nom (string) d'une touche de clavier par son code (enum).
inline std::string getKeyEnumName(sf::Keyboard::Key key) {
	using enum sf::Keyboard::Key;
	static const std::unordered_map<sf::Keyboard::Key, std::string> keyToName = {
		{Unknown, "Unknown"},
		{A, "A"},
		{B, "B"},
		{C, "C"},
		{D, "D"},
		{E, "E"},
		{F, "F"},
		{G, "G"},
		{H, "H"},
		{I, "I"},
		{J, "J"},
		{K, "K"},
		{L, "L"},
		{M, "M"},
		{N, "N"},
		{O, "O"},
		{P, "P"},
		{Q, "Q"},
		{R, "R"},
		{S, "S"},
		{T, "T"},
		{U, "U"},
		{V, "V"},
		{W, "W"},
		{X, "X"},
		{Y, "Y"},
		{Z, "Z"},
		{Num0, "Num0"},
		{Num1, "Num1"},
		{Num2, "Num2"},
		{Num3, "Num3"},
		{Num4, "Num4"},
		{Num5, "Num5"},
		{Num6, "Num6"},
		{Num7, "Num7"},
		{Num8, "Num8"},
		{Num9, "Num9"},
		{Escape, "Escape"},
		{LControl, "LControl"},
		{LShift, "LShift"},
		{LAlt, "LAlt"},
		{LSystem, "LSystem"},
		{RControl, "RControl"},
		{RShift, "RShift"},
		{RAlt, "RAlt"},
		{RSystem, "RSystem"},
		{Menu, "Menu"},
		{LBracket, "LBracket"},
		{RBracket, "RBracket"},
		{Semicolon, "Semicolon"},
		{Comma, "Comma"},
		{Period, "Period"},
		{Apostrophe, "Apostrophe"},
		{Slash, "Slash"},
		{Backslash, "Backslash"},
		{Grave, "Grave"},
		{Equal, "Equal"},
		{Hyphen, "Hyphen"},
		{Space, "Space"},
		{Enter, "Enter"},
		{Backspace, "Backspace"},
		{Tab, "Tab"},
		{PageUp, "PageUp"},
		{PageDown, "PageDown"},
		{End, "End"},
		{Home, "Home"},
		{Insert, "Insert"},
		{Delete, "Delete"},
		{Add, "Add"},
		{Subtract, "Subtract"},
		{Multiply, "Multiply"},
		{Divide, "Divide"},
		{Left, "Left"},
		{Right, "Right"},
		{Up, "Up"},
		{Down, "Down"},
		{Numpad0, "Numpad0"},
		{Numpad1, "Numpad1"},
		{Numpad2, "Numpad2"},
		{Numpad3, "Numpad3"},
		{Numpad4, "Numpad4"},
		{Numpad5, "Numpad5"},
		{Numpad6, "Numpad6"},
		{Numpad7, "Numpad7"},
		{Numpad8, "Numpad8"},
		{Numpad9, "Numpad9"},
		{F1, "F1"},
		{F2, "F2"},
		{F3, "F3"},
		{F4, "F4"},
		{F5, "F5"},
		{F6, "F6"},
		{F7, "F7"},
		{F8, "F8"},
		{F9, "F9"},
		{F10, "F10"},
		{F11, "F11"},
		{F12, "F12"},
		{F13, "F13"},
		{F14, "F14"},
		{F15, "F15"},
		{Pause, "Pause"},
	};

	auto it = keyToName.find(key);
	return it != keyToName.end() ? it->second : "";
}

// Obtenir le code (enum) d'une touche de clavier par son nom (string).
inline sf::Keyboard::Key getKeyEnum(const std::string& name) {
	using enum sf::Keyboard::Key;
	static const std::unordered_map<std::string, sf::Keyboard::Key> nameToKey = {
		{"Unknown", Unknown},
		{"A", A},
		{"B", B},
		{"C", C},
		{"D", D},
		{"E", E},
		{"F", F},
		{"G", G},
		{"H", H},
		{"I", I},
		{"J", J},
		{"K", K},
		{"L", L},
		{"M", M},
		{"N", N},
		{"O", O},
		{"P", P},
		{"Q", Q},
		{"R", R},
		{"S", S},
		{"T", T},
		{"U", U},
		{"V", V},
		{"W", W},
		{"X", X},
		{"Y", Y},
		{"Z", Z},
		{"Num0", Num0},
		{"Num1", Num1},
		{"Num2", Num2},
		{"Num3", Num3},
		{"Num4", Num4},
		{"Num5", Num5},
		{"Num6", Num6},
		{"Num7", Num7},
		{"Num8", Num8},
		{"Num9", Num9},
		{"0", Num0},
		{"1", Num1},
		{"2", Num2},
		{"3", Num3},
		{"4", Num4},
		{"5", Num5},
		{"6", Num6},
		{"7", Num7},
		{"8", Num8},
		{"9", Num9},
		{"Escape", Escape},
		{"LControl", LControl},
		{"LShift", LShift},
		{"LAlt", LAlt},
		{"LSystem", LSystem},
		{"RControl", RControl},
		{"RShift", RShift},
		{"RAlt", RAlt},
		{"RSystem", RSystem},
		{"Menu", Menu},
		{"LBracket", LBracket},
		{"RBracket", RBracket},
		{"Semicolon", Semicolon},
		{"Comma", Comma},
		{"Period", Period},
		{"Apostrophe", Apostrophe},
		{"Slash", Slash},
		{"Backslash", Backslash},
		{"Grave", Grave},
		{"Equal", Equal},
		{"Hyphen", Hyphen},
		{"Space", Space},
		{"Enter", Enter},
		{"Backspace", Backspace},
		{"Tab", Tab},
		{"PageUp", PageUp},
		{"PageDown", PageDown},
		{"End", End},
		{"Home", Home},
		{"Insert", Insert},
		{"Delete", Delete},
		{"Add", Add},
		{"Subtract", Subtract},
		{"Multiply", Multiply},
		{"Divide", Divide},
		{"Left", Left},
		{"Right", Right},
		{"Up", Up},
		{"Down", Down},
		{"Numpad0", Numpad0},
		{"Numpad1", Numpad1},
		{"Numpad2", Numpad2},
		{"Numpad3", Numpad3},
		{"Numpad4", Numpad4},
		{"Numpad5", Numpad5},
		{"Numpad6", Numpad6},
		{"Numpad7", Numpad7},
		{"Numpad8", Numpad8},
		{"Numpad9", Numpad9},
		{"F1", F1},
		{"F2", F2},
		{"F3", F3},
		{"F4", F4},
		{"F5", F5},
		{"F6", F6},
		{"F7", F7},
		{"F8", F8},
		{"F9", F9},
		{"F10", F10},
		{"F11", F11},
		{"F12", F12},
		{"F13", F13},
		{"F14", F14},
		{"F15", F15},
		{"Pause", Pause},
	};

	auto it = nameToKey.find(name);
	return it != nameToKey.end() ? it->second : Unknown;
}

inline MouseState getMouseState(const sf::WindowBase& window) {
	MouseState result;

	// Obtenir l'état des boutons.
	for (unsigned int i = 0; i < sf::Mouse::ButtonCount; i++) {
		auto btn = (sf::Mouse::Button)i;
		result.buttons[(int)btn] = sf::Mouse::isButtonPressed(btn);
	}

	sf::Vector2i windowSize(window.getSize().x, window.getSize().y);
	// Position absolue, donc par rapport à l'écran (dépasse les dimensions de l'écran si plusieurs moniteurs).
	result.absolute = sf::Mouse::getPosition();
	// Position relative à la fenêtre (son coin haut-gauche). Le coin haut-gauche est celui de la partie dessinable de la fenêtre et n'inclue pas le cadre de celle-ci.
	result.relative = sf::Mouse::getPosition(window);
	// Position relative au centre de la fenêtre, avec les y positifs vers le haut et normalisée et bornée dans [-1,1]. C'est l'équivalent des coords normalisées d'OpenGL.
	result.normalized.x =  (2.0f * result.relative.x - (windowSize.x - 1)) / (windowSize.x - 1);
	result.normalized.y = -(2.0f * result.relative.y - (windowSize.y - 1)) / (windowSize.y - 1);
	result.normalized.x = std::clamp(result.normalized.x, -1.0f, 1.0f);
	result.normalized.y = std::clamp(result.normalized.y, -1.0f, 1.0f);
	// Vérifier que le curseur est dans l'espace de la fenêtre.
	auto& rel = result.relative;
	result.isInsideWindow = 0 <= rel.x and rel.x < windowSize.x and
	                        0 <= rel.y and rel.y < windowSize.y;

	return result;
}

