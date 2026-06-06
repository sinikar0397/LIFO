#include "./src/bfs/bfs.h"
#include "./src/gui/ui.h"
#include "./src/headers.h"
#include "./src/people/login.h"
#include "./src/people/people.h"

int main(int argc, char **argv) {
	SDL_Ui ui_asdf;
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	ui_asdf.window = window;
	ui_asdf.renderer = renderer;
	SDL_Ui *ui = &ui_asdf;
	gui_initUi(ui);

	int mx, my;
	bool quit = false;
	bool is_mouse_up = false;
	bool is_mouse_down = false;
	bool is_mouse_move = false;
	char input_buf[256] = {0};
	strcpy(input_buf, " ");

	while (!quit) {
		SDL_Event event;
		SDL_PumpEvents();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				quit = true;
				break;

			case SDL_MOUSEBUTTONUP:
				is_mouse_up = true;
				mx = event.button.x;
				my = event.button.y;
				break;

				// case SDL_MOUSEBUTTONDOWN:
				// is_mouse_down = true;
				// mx = event.button.x;
				// my = event.button.y;
				// break;

				// case SDL_MOUSEMOTION:
				// is_mouse_move = true;
				// mx = event.button.x;
				// my = event.button.y;
				// break;

			case SDL_TEXTINPUT:
				break;

			case SDL_KEYDOWN:
				break;
			}
		}

		if (is_mouse_up) {
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		SDL_SetRenderDrawColor(ui->renderer, 0, 0, 0, 255);

		SDL_SetRenderDrawColor(ui->renderer, 170, 170, 170, 255);

		SDL_RenderPresent(ui->renderer);

		is_mouse_down = false;
		is_mouse_move = false;
		is_mouse_up = false;
	}

	gui_closeUi(ui);
}
