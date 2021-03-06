#pragma once
#include "Entity.h"

enum coinType {
	GREEN_DIAMOND,
	BLUE_DIAMOND,
	HEART,

	NOTYPE
};

class Coin : public Entity
{
public:
	Coin() 
	{
		coin_type = coinType::NOTYPE;
		points = 0;
	}

	Coin(iPoint pos, Coin* e) : Entity(pos, e) {

		coin_type	= e->coin_type;
		points		= e->points;
		picked		= false;

		switch (coin_type)
		{
		case GREEN_DIAMOND:
			LOG("GREEN_DIAMOND CREATED");
			break;
		case BLUE_DIAMOND:
			LOG("BLUE_DIAMOND CREATED");
			break;
		case HEART:
			LOG("HEART CREATED");
			break;
		default:
			break;
		}
	}
	~Coin() {}

	void Draw(SDL_Texture *sprites)
	{
		if (App->pause) {
			current_animation->speed = 0;
		}
		else current_animation->speed = def_anim_speed * App->dt;

		if (!picked)
		{
			App->render->Blit(sprites, position.x, position.y, &current_animation->GetCurrentFrame(), 1, 0);
			if (collider != nullptr) collider->SetPos(position.x, position.y);
		}
	}

public:

	coinType coin_type;
	int points;
	bool picked;

};

