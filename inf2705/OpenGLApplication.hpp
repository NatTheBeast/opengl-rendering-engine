#pragma once


#include <cstddef>
#include <cstdint>

#include <array>
#include <ctime>
//#include <format>
#include <iostream>
#include <iomanip>
#include <memory>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <chrono>
#include <unordered_map>
#include <thread>

#ifdef _WIN32
	#include <Windows.h>
	#undef near
	#undef far
#endif

#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>

#include <inf2705/sfml_utils.hpp>
#include <inf2705/utils.hpp>


using namespace gl;


struct WindowSettings
{
	sf::VideoMode videoMode = sf::VideoMode({600, 600});
	int fps = 30;
	sf::ContextSettings context = {.depthBits = 24, .stencilBits = 8};
};

// Classe de base pour les application OpenGL. Fait pour nous la création de fenêtre et la gestion des événements.
// On doit en hériter et on peut surcharger init() et drawFrame() pour créer un programme de base.
// Les autres méthodes à surcharger sont pour la gestion d'événements.
class OpenGLApplication
{
public:
	virtual ~OpenGLApplication() = default;

	void run(int& argc, char* argv[], std::string_view title = "OpenGL Application", const WindowSettings& settings = {}) {
		// On pourrait avoir besoin des arguments de ligne de commande. Ça donne entre autre le nom de l'exécutable.
		argc_ = argc;
		argv_ = argv;

		settings_ = settings;

		// Créer la fenêtre et afficher les infos du contexte OpenGL.
		createWindowAndContext(title);
		printGLInfo();
		std::cout << std::endl;

		init(); // À surcharger

		// Commencer le chronomètre qui mesure le temps des trames. C'est des fois plus pratique d'avoir le temps depuis la dernière trame que le numéro de trame.
		startTime_ = std::chrono::system_clock::now();
		lastFrameTime_ = std::chrono::high_resolution_clock::now();
		deltaTime_ = 1.0f / settings_.fps;

		// État initial de la souris avant la première trame.
		currentMouseState_ = lastMouseState_ = getMouseState(window_);

		// Compteur de trames effectuées.
		frame_ = 0;

		printKeybinds();
		
		handleEvents();
		updateDeltaTime();
		ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

		// Tant que la fenêtre est ouverte (mis à jour dans la gestion d'événements) :
		while (window_.isOpen()) {			
			drawFrame(); // À surcharger
			
			ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// SFML fait le rafraîchissement de la fenêtre ainsi que le contrôle du framerate pour nous.
			// La fonction display fait le buffer swap (comme glutSwapBuffers) et attend à la prochaine trame selon le FPS qu'on a spécifié avec setFramerateLimit.
			window_.display();
            
            handleEvents();
			updateDeltaTime();
			ImGui_ImplOpenGL3_NewFrame();
            ImGui::NewFrame();

			frame_++;
		}
		
		ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
	}

	const sf::Window& getWindow() const { return window_; }

	// État de la souris (mis à jour une fois par trame avant la gestion d'événements).
	const MouseState& getMouse() const {
		return currentMouseState_;
	}

	// Numéro de la trame courante (première trame = 0).
	int getCurrentFrameNumber() const {
		return frame_;
	}

	// Temps de puis la dernière trame (mis à jour entre les trames).
	float getFrameDeltaTime() const {
		return deltaTime_;
	}

	// Ratio des dimensions de la fenêtre (x/y).
	float getWindowAspect() const {
		auto windowSize = window_.getSize();
		float aspect = (float)windowSize.x / windowSize.y;
		return aspect;
	}

	// Moment de début du chronomètre des trames (commence juste avant la première trame).
	auto getStartTime() const {
		return startTime_;
	}

	// Obtenir une string formatée du temps de départ.
	std::string formatStartTime(const std::string& format = "%Y-%m-%d %H:%M:%S") const {
		std::time_t timestamp = std::chrono::system_clock::to_time_t(startTime_);
		auto locTime = localtime(&timestamp);
		return (std::stringstream() << std::put_time(locTime, format.c_str())).str();
	}

	// Afficher les raccourcis clavier.
	void printKeybinds() const {
		if (not keybindMessage_.empty())
			std::cout << "\n" << "Raccourcis clavier" << "\n"
			          << keybindMessage_ << "\n" << std::endl;
	}

	void setKeybindMessage(std::string_view msg) {
		keybindMessage_ = trim(msg);
		if (not keybindMessage_.empty())
			keybindMessage_ = "    " + replaceAll(trim(msg), "\n", "\n    ");
	}

	// Afficher les informations de base de la carte graphique et de la version OpenGL des drivers.
	void printGLInfo() {
		auto openglVersion = glGetString(GL_VERSION);
		auto openglVendor = glGetString(GL_VENDOR);
		auto openglRenderer = glGetString(GL_RENDERER);
		auto glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
		auto& sfmlSettings = window_.getSettings();
		printf("OpenGL         %s\n", openglVersion);
		printf("GPU            %s, %s\n", openglRenderer, openglVendor);
		printf("GLSL           %s\n", glslVersion);
		printf("SFML Context   %i.%i\n", sfmlSettings.majorVersion, sfmlSettings.minorVersion);
		printf("Depth bits     %i\n", sfmlSettings.depthBits);
		printf("Stencil bits   %i\n", sfmlSettings.stencilBits);
	}

	sf::Image captureCurrentFrame(GLenum buffer = GL_FRONT) {
		// Les dimensions de la fenêtre.
		auto windowSize = window_.getSize();
		size_t numPixels = windowSize.x * windowSize.y;

		// Obtenir la source actuelle de glReadBuffer.
		GLint readBufferSrc;
		glGetIntegerv(GL_READ_BUFFER, &readBufferSrc);
		// Par défaut, lire du front buffer (le tampon d'affichage, donc ce qui est à l'écran). On remarque qu'on n'a pas besoin de faire glFinish(), vu que le tampon d'affichage est complet après le buffer swap.
		glReadBuffer(buffer);
		std::vector<uint8_t> pixels(numPixels * sizeof(sf::Color), 0);
		glReadPixels(0, 0, windowSize.x, windowSize.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		// Restaurer la source de glReadBuffer.
		glReadBuffer((GLenum)readBufferSrc);
		// Créer l'image avec les pixels lus.
		sf::Image img;
		img.resize({windowSize.x, windowSize.y}, pixels.data());
		// Renverser l'image verticalement à cause de l'origine (x,y=0,0) OpenGL qui est bas-gauche et celle des images SFML qui est haut-gauche.
		img.flipVertically();

		return img;
	}

	std::string saveScreenshot(const std::string& folder = "screenshots", const std::string& filename = "") {
		using namespace std::filesystem;

		path trimmedFilename = trim(filename);
		path trimmedFolder = trim(folder);

		// Capturer la trame actuelle.
		sf::Image frameImage = captureCurrentFrame();

		// Si le dossier cible n'existe pas, le créer.
		if (not trimmedFolder.empty())
			create_directory(trimmedFolder);

		// Construire le chemin et nom du fichier.
		std::string filePathStr;
		if (not trimmedFilename.empty()) {
			filePathStr = (trimmedFolder / trimmedFilename).make_preferred().string();
		} else {
			// Si aucun nom de fichier est fourni, construire un nom avec le nom de l'exécutable, l'heure de démarrage de l'application et le numéro de la trame actuelle.
			int frameNumber = getCurrentFrameNumber();
			std::string dateTimeStr = formatStartTime("%Y%m%d_%H%M%S");
			std::string execFilename = argv_[0];
			path execName = path(execFilename).stem();
			std::stringstream ss;
			std::string outputName = (trimmedFolder / execName).make_preferred().string();
			ss << outputName << "_" << dateTimeStr << "_" << frameNumber;
			filePathStr = ss.str();
		}

		// Faire l'écriture dans le fichier dans un fil parallèle pour moins ralentir le fil principal avec une écriture sur le disque. La capture (avec glReadPixels) doit être faite dans le fil principal, mais l'écriture sur le disque peut être faite en parallèle sans causer de problème de synchronisation. On remarque la capture par copie.
		std::thread savingThread([=]() {
			bool ok = frameImage.saveToFile(filePathStr);
			if (not ok)
			    std::cerr << "Could not write image to disk" << "\n";
		});
		// Détacher le fil pour qu'il se gère tout seul, donc pas besoin de join() ou de garder la variable vivante.
		savingThread.detach();

		return filePathStr;
	}

	// Les méthodes virtuelles suivantes sont à surcharger.

	// Appelée avant la première trame.
	virtual void init() { }

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	virtual void drawFrame() { }

	// Appelée lorsque la fenêtre se ferme.
	virtual void onClose() { }

	// Appelée lors d'une touche de clavier.
	virtual void onKeyPress(const sf::Event::KeyPressed& key) { }

	// Appelée lors d'une touche de clavier relâchée.
	virtual void onKeyRelease(const sf::Event::KeyReleased& key) { }

	// Appelée lors d'un bouton de souris appuyé.
	virtual void onMouseButtonPress(const sf::Event::MouseButtonPressed& mouseBtn) { }

	// Appelée lors d'un bouton de souris relâché.
	virtual void onMouseButtonRelease(const sf::Event::MouseButtonReleased& mouseBtn) { }

	// Appelée lors d'un mouvement de souris.
	virtual void onMouseMove(const sf::Event::MouseMoved& mouseDelta) { }

	// Appelée lors d'un défilement de souris.
	virtual void onMouseScroll(const sf::Event::MouseWheelScrolled& mouseScroll) { }

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	virtual void onResize(const sf::Event::Resized& event) { }

	// Appelée sur n'importe quel évènement (incluant ceux ci-dessus).
	virtual void onEvent(const sf::Event& event) { }

protected:
	void handleEvents() {
		lastMouseState_ = currentMouseState_;
		currentMouseState_ = getMouseState(window_);
		ImGuiIO& io = ImGui::GetIO();

		// Traiter les événements survenus depuis la dernière trame.
		while (auto event = window_.pollEvent()) {
			// N'importe quel événement.
			onEvent(*event); // À surcharger

			// L'utilisateur a voulu fermer la fenêtre (le X de la fenêtre, Alt+F4 sur Windows, etc.).
			if (event->is<sf::Event::Closed>()) {
				glFinish();
				onClose(); // À surcharger
				glFinish();
				window_.close();
			// Redimensionnement de la fenêtre.
			} else if (auto* e = event->getIf<sf::Event::Resized>()) {
				glViewport(0, 0, e->size.x, e->size.y);
                io.DisplaySize.x = e->size.x;
                io.DisplaySize.y = e->size.y;
				onResize(*e); // À surcharger
				lastResize_ = *e;
			// Touche appuyée.
			} else if (auto* e = event->getIf<sf::Event::KeyPressed>()) {
				onKeyPress(*e); // À surcharger
			// Touche relâchée.
			} else if (auto* e = event->getIf<sf::Event::KeyReleased>()) {
				onKeyRelease(*e); // À surcharger
			// Bouton appuyé.
			} else if (auto* e = event->getIf<sf::Event::MouseButtonPressed>()) {
			    io.AddMouseButtonEvent((int)e->button, true);
				onMouseButtonPress(*e); // À surcharger
			// Bouton relâché.
			} else if (auto* e = event->getIf<sf::Event::MouseButtonReleased>()) {
			    io.AddMouseButtonEvent((int)e->button, false);
				onMouseButtonRelease(*e); // À surcharger
			// Souris bougée.
			} else if (auto* e = event->getIf<sf::Event::MouseMoved>()) {
			    io.AddMousePosEvent(e->position.x, e->position.y);
				onMouseMove({{
					e->position.x - lastMouseState_.relative.x,
					e->position.y - lastMouseState_.relative.y
				}});
			// Souris défilée
			} else if (auto* e = event->getIf<sf::Event::MouseWheelScrolled>()) {
			    if (e->wheel == sf::Mouse::Wheel::Vertical)
    				io.AddMouseWheelEvent(0, e->delta);
				else
				    io.AddMouseWheelEvent(e->delta, 0);
				onMouseScroll(*e);
			}
		}
	}

	void createWindowAndContext(std::string_view title) {
		#ifdef _WIN32
			// Juste pour s'assurer d'avoir le codepage UTF-8 sur Windows avec Visual Studio.
			SetConsoleOutputCP(65001);
			SetConsoleCP(65001);
		#endif

		window_.create(
			settings_.videoMode, // Dimensions de fenêtre.
			sfStr(title), // Titre.
			sf::Style::Default, // Style de fenêtre (bordure, boutons X, etc.).
			sf::State::Windowed,
			settings_.context
		);
		window_.setFramerateLimit(settings_.fps);
		bool ok = window_.setActive(true);
		if (not ok)
			std::cerr << "Could not activate created window" << "\n";
		
		window_.requestFocus(); // Fix for macOS window not grabbing keyboard focus initially
		
		lastResize_ = {{window_.getSize().x, window_.getSize().y}};

		// On peut donner une « GetProcAddress » venant d'une autre librairie à glbinding.
		// Si on met nullptr, glbinding se débrouille avec sa propre implémentation.
		glbinding::Binding::initialize(nullptr);
		ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init();
		// Cette étape semble nécessaire sur Windows.
		ImGui::GetIO().DisplaySize.x = window_.getSize().x;
		ImGui::GetIO().DisplaySize.y = window_.getSize().y;
	}

	void updateDeltaTime() {
		using namespace std::chrono;
		auto t = high_resolution_clock::now();
		duration<float> dt = t - lastFrameTime_;
		deltaTime_ = dt.count();
		lastFrameTime_ = t;
		ImGui::GetIO().DeltaTime = deltaTime_;
	}

	sf::RenderWindow window_;
	sf::Event::Resized lastResize_ = {};
	int frame_ = 0;
	float deltaTime_ = 0.0f;
	std::chrono::system_clock::time_point startTime_;
	std::chrono::high_resolution_clock::time_point lastFrameTime_;
	MouseState lastMouseState_ = {};
	MouseState currentMouseState_ = {};

	int argc_ = 0;
	char** argv_ = nullptr;
	WindowSettings settings_;
	std::string keybindMessage_;
};


inline void printGLError(std::string_view sourceFile = "", int sourceLine = -1) {
	static const std::unordered_map<GLenum, std::string> codeToName = {
		{GL_NO_ERROR, "GL_NO_ERROR"},
		{GL_INVALID_ENUM, "GL_INVALID_ENUM"},
		{GL_INVALID_VALUE, "GL_INVALID_VALUE"},
		{GL_INVALID_OPERATION, "GL_INVALID_OPERATION"},
		{GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW"},
		{GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW"},
		{GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY"},
		{GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION"},
		{GL_INVALID_FRAMEBUFFER_OPERATION_EXT, "GL_INVALID_FRAMEBUFFER_OPERATION_EXT"},
		{GL_TABLE_TOO_LARGE, "GL_TABLE_TOO_LARGE"},
		{GL_TABLE_TOO_LARGE_EXT, "GL_TABLE_TOO_LARGE_EXT"},
		{GL_TEXTURE_TOO_LARGE_EXT, "GL_TEXTURE_TOO_LARGE_EXT"},
	};

	while (true) {
		GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
			break;

        if (not sourceFile.empty()) {
		    auto filename = std::filesystem::path(sourceFile).filename().string();
		    std::cerr << filename << "(" << sourceLine <<"): ";
	    }

		auto& errorName = codeToName.at(errorCode);
		std::cerr << "OpenGL Error 0x"<< std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)errorCode << std::dec << ": " << errorName.data() << "\n";
	}
}

