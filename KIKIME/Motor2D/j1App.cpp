#include <iostream> 

#include "j1Window.h"
#include "j1Input.h"
#include "j1Render.h"
#include "j1Textures.h"
#include "j1Audio.h"
#include "j1Scene.h"
#include "j1Map.h"
#include "j1Debug.h"
#include "j1Collision.h"
#include "j1FadeToBlack.h"
#include "Player.h"
#include "j1App.h"
#include "j1FileSystem.h"
#include "j1EntityManager.h"
#include "j1PathFinding.h"
#include "j1Fonts.h"
#include "j1Gui.h"

#include "Brofiler/Brofiler.h"
#include "SDL_mixer\include\SDL_mixer.h"
#pragma comment( lib, "SDL_mixer/libx86/SDL2_mixer.lib" )

// Constructor
j1App::j1App(int argc, char* args[]) : argc(argc), args(args)
{
	BROFILER_CATEGORY("App->Constructor", Profiler::Color::AliceBlue)
	timer.Start();
	perf_timer.Start();

	want_to_save = want_to_load = false;
	game_over = false;
	to_exit = false;
	map_loaded = false;

	input			= new j1Input();
	win				= new j1Window();
	render			= new j1Render();
	tex				= new j1Textures();
	audio			= new j1Audio();
	scene			= new j1Scene();
	map				= new j1Map();
	debug			= new j1Debug();
	collision		= new j1Collision();
	fade			= new j1FadeToBlack();
	pathfinding		= new j1PathFinding();
	entitymanager	= new j1EntityManager();
	font			= new j1Fonts();
	gui				= new j1Gui();

	// Ordered for awake / Start / Update
	// Reverse order of CleanUp
	// RENDER last to swap buffer
	AddModule(input);
	AddModule(win);
	AddModule(tex);
	AddModule(audio);
	AddModule(map);
	AddModule(scene);
	AddModule(debug);
	AddModule(collision);
	AddModule(entitymanager);
	AddModule(pathfinding);
	AddModule(font);
	AddModule(gui);
	AddModule(fade);

	AddModule(render);

	LOG("App constructor takes %u ms", timer.Read());
	LOG("App constructor takes %u micro secs", perf_timer.ReadMs());

	player_in_main_menu		= true;
	framerate_cap			= 0;
	frame_count				= 0;
	frames_on_last_update	= 0;
	aux_frames_counter		= 0; 
	last_sec_fcount			= 0;
	avg_fps					= 0;

	delay_is_active			= true;
	want_to_load			= false;
	want_to_save			= false;
	
	pause					= false;
	to_exit					= false;

	dt						= 0;
	save_path				= nullptr;
	load_path				= nullptr;
	init_state_path			= nullptr;
}

// Destructor
j1App::~j1App()
{
	// release modules
	 p2List_item<j1Module*>* item = modules.end;

	while(item != NULL)
	{
		RELEASE(item->data);
		item = item->prev;
	}
	item->~p2List_item();
	modules.clear();
}

void j1App::AddModule(j1Module* module)
{
	module->Init();
	modules.add(module);
}

// Called before render is available
bool j1App::Awake()
{	
	//BROFILER_CATEGORY("App->Awake", Profiler::Color::AntiqueWhite)
	timer.Start();
	perf_timer.Start();

	bool ret = true;

		// BEGIN CONFIGURATION
	pugi::xml_document	config_doc;
	pugi::xml_node		config;
	pugi::xml_node		app_config;
	pugi::xml_node		file_system;

	pugi::xml_parse_result result = LoadXML(config_doc, "config.xml");

	if (result != NULL)
	{
		config		= config_doc.child("config");

		app_config	= config.child("app");
		file_system = config.child("file_system");

		title.create(app_config.child("title").child_value());
		organization.create(app_config.child("organization").child_value());

		load_path.create(file_system.child("load_path").child_value());
		save_path.create(file_system.child("save_path").child_value());
		init_state_path.create(file_system.child("init_state_path").child_value());

		LOG("LoadPath: %s", load_path.GetString());

		framerate_cap = app_config.attribute("framerate_cap").as_uint();
	
		// END CONFIGURATION

		// Awakening all modules
		p2List_item<j1Module*>* item = modules.start;

		while(item != NULL && ret)
		{
			ret = item->data->Awake(config.child(item->data->name.GetString()));
			item = item->next;
		}
	}

	LOG("App awake takes %u ms", timer.Read());
	LOG("App awake takes %u micro secs", perf_timer.ReadMs());

	return ret;
}

// Called before the first frame
bool j1App::Start()
{
	BROFILER_CATEGORY("App->Start", Profiler::Color::Aqua)
	App->win->SetTitle("RED");
	perf_timer.Start();
	timer.Start();
	aux_timer.Start();
	in_game_timer.sec = 0;
	in_game_timer.min = 0;
	pause = false;
	bool ret = true;
	p2List_item<j1Module*>* item = modules.start;

	while(item != NULL && ret)
	{
		ret = item->data->Start();
		item = item->next;
	}

	LOG("App start takes %u ms", timer.Read());
	LOG("App start takes %u micro secs", perf_timer.ReadMs());

	return ret;
}

// Called each loop iteration
bool j1App::Update()
{
	BROFILER_CATEGORY("App->Update", Profiler::Color::LightGreen)
	bool ret = true;
	PrepareUpdate();

	if(input->GetWindowEvent(WE_QUIT))
		ret = false;

		if(ret)
			ret = PreUpdate();

		if(ret)
			ret = DoUpdate();

		if(ret)
			ret = PostUpdate();

		FinishUpdate();

	return ret && !to_exit;
}

pugi::xml_parse_result j1App::LoadXML(pugi::xml_document& doc, const char* path)
{
	pugi::xml_parse_result result = doc.load_file(path);

	if (result == NULL)
	{
		LOG("Could not load xml file %s pugi error: %s",path, result.description());
	}
	else {
		LOG("%s loaded successfully", path);
	}

	return result;
}

// ---------------------------------------------
void j1App::PrepareUpdate()
{
	BROFILER_CATEGORY("App->PrepareUpdate", Profiler::Color::Aquamarine)
	frame_count++;
	aux_frames_counter++;

	dt = frame_time.ReadSec();

	frame_time.Start();
}

// ---------------------------------------------
void j1App::FinishUpdate()
{
	BROFILER_CATEGORY("App->FinishUpdate", Profiler::Color::Azure)
	if (want_to_save) SaveGameFile();
	if (want_to_load) LoadGameFile();

	if (aux_timer.ReadSec() >= 1.0f)
	{
		last_sec_fcount = aux_frames_counter;
		aux_frames_counter = 0;
		aux_timer.Start();
	}

	avg_fps = float(frame_count) / timer.ReadSec();
	float seconds_since_startup = timer.ReadSec();
	uint32 last_frame_ms = frame_time.Read();
	frames_on_last_update = last_sec_fcount;

	/*int x, y;
	App->input->GetMousePosition(x, y);
	iPoint map_coordinates = App->map->WorldToMap(x - App->render->camera.x, y - App->render->camera.y);
	static char title[256];
	sprintf_s(title, 256, "Av.FPS: %.2f Last Frame Ms: %02u Last sec frames: %i Last dt: %.3f Time since startup: %.3f Frame Count: %llu Map:%dx%d Tiles:%dx%d Tilesets:%d Tile:%d,%d ",
		avg_fps, last_frame_ms, frames_on_last_update, dt, seconds_since_startup, frame_count,
		App->map->data.width, App->map->data.height,
		App->map->data.tile_width, App->map->data.tile_height,
		App->map->data.tilesets.count(),
		map_coordinates.x, map_coordinates.y);
	App->win->SetTitle(title);*/

	if (delay_is_active)
	{
		SDL_Delay(abs((float)(1000 / framerate_cap) - last_frame_ms));
	}
	else
	{
		SDL_Delay(abs((float)(1000 / 98) - last_frame_ms));
	}
}

bool j1App::SaveGameFile() 
{
	BROFILER_CATEGORY("App->SaveGameFile", Profiler::Color::Beige)
	LOG("Saving new Game State to %s...", save_path.GetString());

	pugi::xml_document		save_game_doc; 
	pugi::xml_node			save_node;
	p2List_item<j1Module*>* item = modules.start;

	bool ret = true;

	save_node = save_game_doc.append_child("save");

	while (item != NULL && ret)
	{
		if (item->data->active) {
			ret = item->data->Save(save_node.append_child(item->data->name.GetString()));
		}
		item = item->next;
	}

	if (ret) {
		save_game_doc.save_file(save_path.GetString());
		LOG("...finished saving");
	}
	else
		LOG("...saving process interrupted with error on module %s", (item != NULL) ? item->data->name.GetString() : "unknown -> NULL pointer");
	audio->PlayFx(save);
	
	//gui->saving_point->initial_pos = { 0,0 };
	scene->saved_map = map->current_map->data.GetString();
	gui->saving_point->position.x = entitymanager->player_ref->position.x;
	gui->saving_point->position.y = entitymanager->player_ref->position.y + 18;
	gui->saving_point->SetVisible();
	save_game_doc.reset();
	want_to_save = false;
	return ret;
}

bool j1App::LoadGameFile()
{
	BROFILER_CATEGORY("App->LoadGameFile", Profiler::Color::Bisque)
		pugi::xml_document		save_game_doc;
		pugi::xml_node			save_node;
		pugi::xml_parse_result	result = LoadXML(save_game_doc, load_path.GetString());

	bool ret = result != NULL;

	p2List_item<j1Module*>* item = modules.start;

	if (ret) {
		LOG("Loading new Game State from %s...", load_path.GetString());

		save_node = save_game_doc.child("save");

		while (item != NULL && ret)
		{
			if (item->data->active) {
				ret = item->data->Load(save_node.child(item->data->name.GetString()));
			}
			item = item->next;
		}

		if (ret)
			LOG("...finished loading");
		else
			LOG("...loading process interrupted with error on module %s", (item != NULL) ? item->data->name.GetString() : "unknown -> NULL pointer");
	}
	entitymanager->player_load_position = entitymanager->player_ref->position;
	fade->FadeToBlack(scene, scene);
	want_to_load = false;
	return ret;
}


bool j1App::ExistsSaveGame()
{
	pugi::xml_document		save_game_doc;
	pugi::xml_parse_result	result = LoadXML(save_game_doc, load_path.GetString());

	return result != NULL;
}

float j1App::GetAvFPS()
{
	return avg_fps;
}

// Call modules before each loop iteration
bool j1App::PreUpdate()
{
	BROFILER_CATEGORY("App->PreUpdate", Profiler::Color::Blue)
	bool ret = true;
	p2List_item<j1Module*>* item = modules.start;
	
	while (item != NULL && ret)
	{
		if (item->data->active && !item->data->to_pause) {
			ret = item->data->PreUpdate();
		}
		item = item->next;
	}

	return ret;
}

// Call modules on each loop iteration
bool j1App::DoUpdate()
{
	BROFILER_CATEGORY("App->DoUpdate", Profiler::Color::Orange)
	bool ret = true;
	p2List_item<j1Module*>* item = modules.start;

	while (item != NULL && ret)
	{
		if (item->data->active && !item->data->to_pause) {
			ret = item->data->Update(dt);
		}
		item = item->next;
	}

	return ret;
}

// Call modules after each loop iteration
bool j1App::PostUpdate()
{
	BROFILER_CATEGORY("App->PostUpdate", Profiler::Color::BlueViolet)
	bool ret = true;
	p2List_item<j1Module*>* item = modules.start;
	
	while (item != NULL && ret)
	{
		if (item->data->active && !item->data->to_pause) {
			ret = item->data->PostUpdate();
		}
		item = item->next;
	}


	return ret;
}

// Called before quitting
bool j1App::CleanUp()
{
	BROFILER_CATEGORY("App->CleanUp", Profiler::Color::CadetBlue)
	timer.Start();
	perf_timer.Start();

	bool ret = true;
	p2List_item<j1Module*>* item = modules.end;
	
	while(item != NULL && ret)
	{
		LOG("Cleaning... %s", item->data->name.GetString());
		ret = item->data->CleanUp();
		//RELEASE(item->data);
		item = item->prev;

	}
	
	LOG("App cleanUp takes %u ms", timer.Read());
	LOG("App cleanUp takes %u micro secs", perf_timer.ReadMs());

	return ret;
}

// ---------------------------------------
int j1App::GetArgc() const
{
	return argc;
}

// ---------------------------------------
const char* j1App::GetArgv(int index) const
{
	if(index < argc)
		return args[index];
	else
		return NULL;
}

// ---------------------------------------
const char* j1App::GetTitle() const
{
	return title.GetString();
}

// ---------------------------------------
const char* j1App::GetOrganization() const
{
	return organization.GetString();
}

float j1App::GetTimerReadSec()
{
	return game_timer.ReadSec();
}

void j1App::LoadGame()
{
	if (!ExistsSaveGame()) return;

	want_to_load = true;
}

void j1App::SaveGame() const
{
	if (map->current_map == map->maps_path.start) return;

	want_to_save = true;
}


void j1App::GoToMainMenu() 
{
	map->current_map = map->maps_path.start;
	scene->current_track = audio->tracks_path.start;

	LOG("CURRENTMAP FROM TRANSITION: %s", App->map->current_map->data.GetString());

	fade->FadeToBlack(App->scene, App->scene);

	UnPauseGame();
}

void j1App::GameOver() 
{
	game_over = true;
	entitymanager->first_gameover = true;

	map->current_map = App->map->maps_path.start;
	scene->current_track = audio->tracks_path.start;

	fade->FadeToBlack(App->scene, App->scene);

	LOG("GAME OVER");
}

void j1App::Exit()
{
	to_exit = true;
}

bool j1App::RestartGame()
{
	if (pause)
	{
		UnPauseGame();
	}
	map->current_map = App->map->maps_path.start->next;
	scene->current_track = audio->tracks_path.start->next;

	fade->FadeToBlack(scene, scene);

	in_game_timer.sec = 0;
	in_game_timer.min = 0;

	game_timer.Start();

	return true;
}

void j1App::TogglePause()
{
	if (!pause) {
		PauseGame();
	}
	else UnPauseGame();
}

void j1App::PauseGame()
{
	if (!pause) {
		pause = true;
		entitymanager->to_pause = true;
		collision->to_pause		= true;
		map->to_pause			= true;
		pathfinding->to_pause	= true;
		audio->to_pause			= true;
	}
	else LOG("Game already paused");
}
void j1App::UnPauseGame()
{
	if (pause) {
		pause = false;
		audio->to_pause = false;
		entitymanager->to_pause = false;
		collision->to_pause		= false;
		map->to_pause			= false;
		pathfinding->to_pause	= false;
		game_timer.Start();
	}
	else LOG("Game is not paused");
}
bool j1App::RestartLevel(int player_lifes)
{
	BROFILER_CATEGORY("App->RestartLevel", Profiler::Color::Red);

	entitymanager->aux_score = 0;
	gui->last_death->SetVisible();
	render->ResetCamera();
	entitymanager->Restart(player_lifes);

	if (Mix_PausedMusic > 0)
		Mix_ResumeMusic();
	game_over = false;
	
	return true;
}

bool j1App::NextLevel() {

	if (App->map->current_map->next != NULL)
	{
		App->map->current_map = App->map->current_map->next;
		scene->current_track = scene->current_track->next;
	}
	else
	{
		App->map->current_map = App->map->maps_path.start;
		scene->current_track = audio->tracks_path.start;
	}

	LOG("Next level: %s", App->map->current_map->data.GetString());
	App->fade->FadeToBlack(App->scene, App->scene);
	return true;
}
//It can only be triggered from the main menu, if it's shown, hide. If it's hidden, show
void j1App::ShowCredits() {
	if (!gui->credits_ui->visible) {
		gui->credits_ui->SetVisible();
		gui->main_menu_ui->SetInvisible();
	}
	else {
		gui->credits_ui->SetInvisible();
		gui->main_menu_ui->SetVisible();
	}
}
//Can be reached from everywhere, we need to know where we are
void j1App::ShowSettings()
{
	
	if (player_in_main_menu) {
		if (!gui->settings_window->visible) {
			// player is in main menu and settings window is not visible
			gui->settings_window->SetVisible();
			gui->main_menu_ui->SetInvisible();
		}
		else {
			gui->settings_window->SetInvisible();
			gui->main_menu_ui->SetVisible();
		}
	}
	else {
		//Coming from ESC pause menu
		if (!gui->settings_window->visible) {
			// player is in main menu and settings window is not visible
			gui->settings_window->SetVisible();
			gui->in_game_pause_ui->SetInvisible();
		}
		else {
			gui->settings_window->SetInvisible();
			gui->in_game_pause_ui->SetVisible();
		}

	}
}

// It can only be triggered from in_game 
void j1App::ShowInGamePause() {
	if (!gui->in_game_pause_ui->visible ) {
		gui->in_game_pause_ui->SetVisible();
	}
	else {
		gui->in_game_pause_ui->SetInvisible();
	}
}
void j1App::ShowHelp()
{
	if (!gui->help_window->visible)
	{
		gui->help_window->SetVisible();
		gui->main_menu_ui->SetInvisible();
	}
	else
	{
		gui->help_window->SetInvisible();
		gui->main_menu_ui->SetVisible();
	}
}

void j1App::ChangeMusicVolume(int value)
{
	Mix_VolumeMusic(value);
}

void j1App::ChangeFXVolume(int value)
{
	Mix_Volume(-1,value);
}

void j1App::CalculateGuiPositions() {
	LOG("Recalculating gui elements position");

	// Recalculating window size
	SDL_GetWindowSize(win->window, &win->width, &win->height);
	SDL_UpdateWindowSurface(win->window);
	fade->screen = { 0, 0,  int(win->width * win->GetScale()), int(win->height * win->GetScale()) };

	//Translating the new window size to the windows
	p2List_item<UIElement*>* item = gui->windows.start;
	for (item; item != nullptr; item = item->next) {
		for (SDL_Rect &r : item->data->rect)
		{
			r.w = win->width;
			r.h = win->height;
		}
	}

	//in_game_gui is a special case
	for (SDL_Rect &r : gui->in_game_gui->rect)
	{
		r.w = win->width;
		r.h = win->height;
	}
	
	gui->CalculateElementsPosition();
}


