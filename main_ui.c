#include "./src/bfs/bfs.h"
#include "./src/gui/display.h"
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
	People *p;
	login_init();
	gui_initUi(ui);

	while (!ui->quit) {

		switch (ui->next_state) {
		case LOGIN:
			p = showLogin(ui);
			break;
		case DFS:
			break;
		case BFS:
			break;
		case MST:
			break;
		}
	}

	gui_closeUi(ui);
}
