#include "j1App.h"
#include "j1Render.h"
#include "Enemy.h"

Enemy::Enemy(int x, int y) : position(x, y)
{}

Enemy::~Enemy()
{
	if (collider != nullptr)
		collider->to_delete = true;
}

const Collider* Enemy::GetCollider() const
{
	return collider;
}

void Enemy::Draw(SDL_Texture* sprites)
{
	if(collider != nullptr)
		collider->SetPos(position.x, position.y);

	if (animation != nullptr)
		App->render->Blit(sprites, position.x, position.y, &(animation->GetCurrentFrame()));
}

void Enemy::PowerUp()
{
	// "Random" powerup spawn
	//if (position.x % 6 == 0)
	//App->particles->AddParticle(App->particles->power_up, position.x, position.y,COLLIDER_TYPE::COLLIDER_POWER_UP);
}