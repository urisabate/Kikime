#ifndef __j1DEBUG_H__
#define __j1DEBUG_H__

#include "j1Module.h"


class j1Debug :	public j1Module
{
public:
	j1Debug();

	~j1Debug();

	bool Awake(pugi::xml_node& config);

	bool Start();

	bool PostUpdate();

	bool Update(float dt);


public:

	p2SString		not;
	p2SString		debug_path;

	SDL_Texture*	not_tex;
	SDL_Texture*	debug_tex;

	iPoint			camera_motion;
	iPoint			last_motion;
	iPoint			zero;

	bool			show_colliders;
	bool			start_motion;
	bool			free_camera;
	bool			debug_bool = false;

	Collider*		debug_col;

};

#endif