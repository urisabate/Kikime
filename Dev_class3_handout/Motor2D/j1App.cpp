#include <iostream> 
#include <sstream> 

#include "p2Defs.h"
#include "p2Log.h"

#include "j1Window.h"
#include "j1Input.h"
#include "j1Render.h"
#include "j1Textures.h"
#include "j1Audio.h"
#include "j1Scene.h"
#include "j1App.h"

// Constructor
j1App::j1App(int argc, char* args[]) : argc(argc), args(args)
{
	frames = 0;

	input	= new j1Input();
	win		= new j1Window();
	render	= new j1Render();
	tex		= new j1Textures();
	audio	= new j1Audio();
	scene	= new j1Scene();

	// Ordered for awake / Start / Update
	// Reverse order of CleanUp
	AddModule(input);
	AddModule(win);
	AddModule(tex);
	AddModule(audio);
	AddModule(scene);

	// render last to swap buffer
	AddModule(render);
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

	modules.clear();

	config_doc.reset();
}

void j1App::AddModule(j1Module* module)
{
	module->Init();
	modules.add(module);
}

// Called before render is available
bool j1App::Awake()
{
	bool ret = LoadConfig();

	// self-config
	title.create(app_config.child("title").child_value());
	organization.create(app_config.child("organization").child_value());

	if(ret)
	{
		p2List_item<j1Module*>* item;
		item = modules.start;

		while(item != NULL && ret)
		{
			ret = item->data->Awake(config.child(item->data->name.GetString()));
			item = item->next;
		}
	}

	return ret;
}

// Called before the first frame
bool j1App::Start()
{
	bool ret = true;
	p2List_item<j1Module*>* item;
	item = modules.start;

	while(item != NULL && ret)
	{
		ret = item->data->Start();
		item = item->next;
	}

	return ret;
}

// Called each loop iteration
bool j1App::Update()
{
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
	return ret;
}


bool j1App::LoadConfig()
{
	pugi::xml_parse_result result = LoadXML(config_doc,"config.xml");

	if (result == NULL)
	{
		LOG("Could not load map xml file config.xml. pugi error: %s", result.description());
		return false;
	}
	else
	{
		config = config_doc.child("config");
		app_config = config.child("app");
	}

	return true;
}

pugi::xml_parse_result j1App::LoadXML(pugi::xml_document& doc, const char* path)
{
	pugi::xml_parse_result result = doc.load_file(path);

	if (result == NULL)
	{
		LOG("Could not load map xml file %s pugi error: %s",path, result.description());
	}

	return result;
}

// ---------------------------------------------
void j1App::PrepareUpdate()
{
	
}

// ---------------------------------------------
void j1App::FinishUpdate()
{
	// DONE TODO 1: This is a good place to call load / Save functions
	if (do_save) Save();
	if (do_load) Load();
}

bool j1App::Save() {

	pugi::xml_parse_result result = LoadXML(save_game_doc, "save_game.xml");

	bool ret = true;
	p2List_item<j1Module*>* item;
	j1Module* pModule = NULL;

	if (result) {

		save_node = save_game_doc.child("save");

		for (item = modules.start; item != NULL && ret; item = item->next)
		{
			pModule = item->data;

			if (pModule->active == false) {
				continue;
			}
			ret = pModule->DoSave();
		}
	}
	save_game_doc.save_file("save_game.xml");
	do_save = false;
	return ret;
}

bool j1App::Load()
{
	pugi::xml_parse_result result = LoadXML(save_game_doc, "save_game.xml");
	bool ret = true;
	p2List_item<j1Module*>* item;
	j1Module* pModule = NULL;

	if (result) {

		save_node = save_game_doc.child("save");

		for (item = modules.start; item != NULL && ret; item = item->next)
	{
			pModule = item->data;

			if (pModule->active == false) {
			continue;
			}
			ret = pModule->DoLoad();
		}
	}

	do_load = false;
	return ret;
}

// Call modules before each loop iteration
bool j1App::PreUpdate()
{
	bool ret = true;
	p2List_item<j1Module*>* item;
	//item = modules.start;
	j1Module* pModule = NULL;

	for(item = modules.start; item != NULL && ret; item = item->next)
	{
		pModule = item->data;

		if(pModule->active == false) {
			continue;
		}

		ret = pModule->PreUpdate();
	}

	return ret;
}

// Call modules on each loop iteration
bool j1App::DoUpdate()
{
	bool ret = true;
	p2List_item<j1Module*>* item;

	j1Module* pModule = NULL;

	for(item = modules.start; item != NULL && ret; item = item->next)
	{
		pModule = item->data;

		if(pModule->active == false) {
			continue;
		}

		ret = pModule->Update(dt);
	}

	return ret;
}

// Call modules after each loop iteration
bool j1App::PostUpdate()
{
	bool ret = true;
	p2List_item<j1Module*>* item;
	j1Module* pModule = NULL;

	for(item = modules.start; item != NULL && ret; item = item->next)
	{
		pModule = item->data;

		if(pModule->active == false) {
			continue;
		}

		ret = pModule->PostUpdate();
	}

	return ret;
}

// Called before quitting
bool j1App::CleanUp()
{
	bool ret = true;
	p2List_item<j1Module*>* item;
	item = modules.end;

	while(item != NULL && ret)
	{
		ret = item->data->CleanUp();
		item = item->prev;
	}

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


// DONE TODO 4: Create a simulation of the xml file to read 

// DONE TODO 5: Create a method to actually load an xml file
// then call all the modules to load themselves

// TODO 7: Create a method to save the current state

