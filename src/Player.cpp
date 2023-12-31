#include "Player.h"
#include <iostream>

using namespace sfGame;

Player::Player(Side side, sf::Sprite sprite, float HP, float FOV, float DEF):
    MilitaryUnit(side, sprite, HP, FOV, DEF, true) 
    {
        destination = sprite.getPosition();
        moveBehavior = new PlayerMove(3);
        rotateBehavior = new Rotatable(1);
    }

Player::~Player()
{
    delete moveBehavior;
    delete rotateBehavior;
}


void Player::handleInput(sf::RenderWindow &window)
{
	sf::Event event;
	while(window.pollEvent(event))
	{
		if (event.type == sf::Event::Closed) window.close();
		// if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
		// {
		// 	direction_ = Direction::Up;
		// 	accelerate_=true;	
		// }
		// else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
		// {	
		// 	direction_ = Direction::Down;
		// 	accelerate_=true;	
		// }
		// else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
		// {	
		// 	direction_ = Direction::Left;
		// 	accelerate_=true;
		// }	
		// else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
		// {
		// 	direction_ = Direction::Right;
		// 	accelerate_=true;
		// }
		// else if(event.type==sf::Event::KeyReleased||event.type==sf::Event::MouseButtonReleased)
		// {
		// 	accelerate_=false;
		// }
		else if(sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
            auto relativePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window)); 
            // auto offset = window.getView().getCenter() - sf::Vector2f(Game::windowWidth/2, Game::windowHeight/2);
            auto offset = window.getView().getCenter() - sf::Vector2f(640, 480);
			destination = relativePos + offset;
            std::cout<<destination.x<<","<<destination.y<<std::endl;
		}
	}

}

void Player::update(sf::Time delta)
{
    if(rotate())
        move();   
}



void Player::move()
{
    moveBehavior->move(*this, destination);
}

bool Player::rotate()
{
    return rotateBehavior->rotate(sprite, destination);
}