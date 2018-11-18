#pragma once
#include "SDL/include/SDL.h"
#include "j1Module.h"
#include "j1Collision.h"
#include "Animation.h"
#include "Entity.h"

class Player : public Entity
{

public:

	Player() {}
	Player(iPoint pos, Entity* e, SDL_Texture* sprites,entityType type);
	~Player();

	bool PreUpdate();
	bool Update(float dt);
	bool PostUpdate();

	void Draw();
	bool MovePlayer(float vel_x, float vel_y);
	bool SetPlayer(float x, float y);
	void Move();
	void MoveFree();
	bool Jump();
	bool DoubleJump();
	void PlayerAnimations();
	

public:

	SDL_Texture* graphics = nullptr;
};