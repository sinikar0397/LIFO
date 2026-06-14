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
	People *p = NULL;
	People *people[MAX_PEOPLE] = {0};
	MatchingInfo *infos =
		(MatchingInfo *)calloc(MAX_PEOPLE, sizeof(MatchingInfo));
		
	login_init();
	gui_initUi(ui);
	int n = 0;

	if (infos == NULL) {
		free(infos);
		return 1;
	}

	while (!ui->quit) {
		switch (ui->next_state) {
		case LOGIN:
			if (p != NULL) {
				people_delete_people(p);
				p = NULL;
			}
			p = display_showLogin(ui);
			n = bfs_loadPeopleFromDatabase(people);
			initMatchingInfos(infos, people, n);
			break;
		case HOME:
			display_showHome(ui, p);
			break;
		case DFS:
			display_showDFS(ui, p);
			break;
		case BFS:
			display_showBFS(ui, p, infos, n);
			break;
		case MST:
			// display_showMST(ui, p);
			break;
		}
	}

	if (p != NULL) {
		people_delete_people(p);
	}
	gui_closeUi(ui);
	free(infos);
}
