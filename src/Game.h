#ifndef GAME
#define GAME

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <memory>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "ThreadPool.h"
#include "Parameters.h"
#include "Screen.h"
#include "MenuScreen.h"
// #include "GameScreen.h"
// #include "GameOverScreen.h"
#include "AssetManager.h"
#include "Battlefield.h"

namespace sfGame
{
class MenuScreen;
class GameScreen;
class GameOverScreen;
class Game
{
public:
	Game();
    ~Game();

	void run();

	void handleInput();
	void update();
	void render();
	sf::Vector2i windowPos(); 

	// static std::shared_ptr<Screen> screen;
    static Screen *screen;
    static MenuScreen *menuScreen;
    static GameScreen *gameScreen;
    static GameOverScreen *gameOverScreen;

private:
	sf::RenderWindow window;
	sf::Music bgMusic;
    sf::View view;


	static const sf::Time TimePerFrame;
};
}


#endif