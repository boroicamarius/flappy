#include "SDL.h"
#include "SDL_Image.h"
#include "SDL_ttf.h"
#include "UI_engine.h"

#include<thread>
#include<iostream>
#include<vector>
#include<cstdio>

using namespace UI_engine;

//std::string SPRITES_DIR = "C:/Users/boroi/Desktop/flappy-bird/sprites/";
std::string SPRITES_DIR = "sprites/";

const char* sprites(std::string location) {
	return (SPRITES_DIR + location + ".png").c_str();
}

std::deque<UI_image*> tubes;

#define cast(x,y) ((x)(y))
int anim[6] = { 1,2,1,0 };
int anim_frame = 0;
int pipe_timing = 0;
double mtime = 0.0;
double last_jump=0;
double speed = 2;

bool bird_killed = false;

SDL_Surface* bird_image[3];
SDL_Surface* pipes[2];

UI_collection* tube_canvas;

std::function<void(UI_window*)> move_base = [&](UI_window* w) {

#define DELTA_TIME ((double)w->deltaTime())

	UI_image* base = cast(UI_image*,w->object("base"));
	UI_image* bird = cast(UI_image*, w->object("bird"));
	if (not bird_killed)base->x(base->x() - DELTA_TIME/4.0);
	if (base->x() <= 288 - 336) base->x(0);

	mtime += DELTA_TIME;
	
#define NEXT_STEP 80.0
	if (mtime > NEXT_STEP) {
		mtime -= NEXT_STEP;
		anim_frame = (anim_frame + 1) % 4;
		bird->source(bird_image[anim[anim_frame]]);
		pipe_timing = (pipe_timing + 1) % 20;
		std::cout << pipe_timing << '\n';

		if (pipe_timing == 19) {
			UI_image* tube_up = new UI_image(w->width(),
                                             w->height() - w->object("base")->height() - rand() % (pipes[0]->h - 50) - 50,
                                             pipes[0]->w, pipes[1]->h, 1.0f);
          
			UI_image* tube_down = new UI_image(w->width(), 
                                               tube_up->y() - 125 - pipes[1]->h, 
                                               pipes[1]->w, pipes[1]->h, 1.0f);

			tube_up->source(pipes[0]);
			tube_up->setRenderSectionFull();

			tube_down->source(pipes[1]);
			tube_down->setRenderSectionFull();

			tube_canvas->addWithCustomName("tube_" + std::to_string(tubes.size()),tube_up);
			tube_canvas->addWithCustomName("tube_" + std::to_string(tubes.size()), tube_down);

			tubes.push_back(tube_up);
			tubes.push_back(tube_down);
			std::cout << "PIPE GENERATED\n";
		}

	}
	int destroy_num = 0;

	if(not bird_killed) for (auto& t : tubes) {

		std::cout << "TUBE CHECKED "<<t->x()<<'\n';
		t->x(t->x()-0.1*DELTA_TIME);

#define BIRD_HORIZONTAL_START bird->x()
#define BIRD_HORIZONTAL_END (bird->x() + bird->width())

#define BIRD_VERTICAL_START bird->y()
#define BIRD_VERTICAL_END (bird->y() + bird->height())

		if (
			(BIRD_HORIZONTAL_END > t->x()+4 && BIRD_HORIZONTAL_END < t->x() + t->width()-4) ||
			(BIRD_HORIZONTAL_START > t->x()+4 && BIRD_HORIZONTAL_START < t->x() + t->width()-4)
			)
			if(
				(BIRD_VERTICAL_END > t->y() && BIRD_VERTICAL_END < t->y() + t->height() ) || 
				(BIRD_VERTICAL_START > t->y() && BIRD_VERTICAL_START < t->y() + t->height())
				)
			w->object("game_over")->show(),bird_killed=true;

		if (t->x() < -t->width()) t->hide(),t->destroy(),++destroy_num;

	}

	while (destroy_num--) tubes.pop_front();

	bird->y(bird->y() + speed);

#define LOWEST_POINT 380
	if (bird->y() >= LOWEST_POINT) bird->y(LOWEST_POINT);
	if (not bird_killed) speed = (SDL_GetTicks() - last_jump > 100) ? 0.35 * DELTA_TIME : -0.4 * DELTA_TIME;
	else speed = 0;
	w->addEvent(move_base);
};

int main(int argc, char* argv[]) {

	srand((unsigned)time(NULL));

	UI_window window("Flappy Bird",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		288, 512, SDL_WINDOW_SHOWN, SDL_RENDERER_ACCELERATED,
		60.0);

	SDL_Surface* game_over_surface = IMG_Load(sprites("gameover"));
	UI_image game_over(window.width() / 2.0f, window.height() / 2.0f, game_over_surface->w, game_over_surface->h, 1.0f);
	game_over.source(game_over_surface);
	game_over.setRenderSectionFull();
	game_over.x(game_over.x() - game_over_surface->w / 2.0f);
	game_over.y(game_over.y() - game_over_surface->h / 2.0f);
	game_over.hide();

	bird_image[0] = IMG_Load(sprites("yellowbird-upflap"));
	bird_image[1] = IMG_Load(sprites("yellowbird-midflap"));
	bird_image[2] = IMG_Load(sprites("yellowbird-downflap"));

	pipes[0] = IMG_Load(sprites("pipe"));
	pipes[1] = IMG_Load(sprites("pipe-down"));

	tube_canvas = new UI_collection(0.0f, 0.0f, 1.0f);

	SDL_Surface* background_surface = IMG_Load(sprites("background-day"));
	std::cout << background_surface->w << ' ' << background_surface->h << ' ';
	UI_image background(0.0f, 0.0f, background_surface->w, background_surface->h, 1.0f); 
	background.source(background_surface);
	background.setRenderSectionFull();

	SDL_Surface* base_surface = IMG_Load(sprites("base"));
	UI_image base(0.0f, window.height() - base_surface->h, base_surface->w, base_surface->h, 1.0f);
	base.source(base_surface);
	base.setRenderSectionFull();

	SDL_Surface* bird_surface = IMG_Load(sprites("yellowbird-midflap"));
	UI_image bird(0.0f, 0.0f, bird_surface->w, bird_surface->h, 1.0f);
	bird.x(window.width() / 2.0f - bird_surface->w / 2.0f);
	bird.y(window.height() / 2.0f - bird_surface->h / 2.0f);
	bird.source(bird_surface);
	bird.setRenderSectionFull();
	bird.onMouseRelease = [&](UI::UI_eventData d) {
		last_jump = SDL_GetTicks();
	};
	bird.onKeyRelease = [&](UI::UI_eventData d) {
		if(d.e.key.keysym.sym == SDLK_SPACE)
			last_jump = SDL_GetTicks();
	};
	bird.toggleEvents();

	std::cout << base_surface->w << '\n';
	window.add(&background);
	window.add(&bird);
	window.add(tube_canvas);
	window.add(&base);
	window.add(&game_over);
	window.addEvent(move_base);
	processEvents(&window);
	return 0;
}