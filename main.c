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
			if (p != NULL) {
				char partner_id[MAX_ID_LEN] = "";
				int has_partner =
					match_store_find_accepted_partner(p->id, partner_id,
													  sizeof(partner_id));
				if (!has_partner && p->status == MATCHED &&
					p->lover[0] != '\0') {
					strncpy(partner_id, p->lover, sizeof(partner_id) - 1);
					partner_id[sizeof(partner_id) - 1] = '\0';
					has_partner = 1;
				}
				if (has_partner) {
					strncpy(g_mst_partner_id, partner_id,
							sizeof(g_mst_partner_id) - 1);
					g_mst_partner_id[sizeof(g_mst_partner_id) - 1] = '\0';
					ui->next_state = MST;
				}
			}
			break;
		case HOME:
			display_showHome(ui, p);
			break;
		case DFS:
			display_showDFS(ui, p);
			break;
		case BFS:
			display_showBFS(ui, p, infos, n, people);
			break;
		case MST:
			display_showMST(ui, p);
			break;
		case PROFILE:
			display_showProfile(ui, p);
			break;
		}
	}

	if (p != NULL) {
		people_delete_people(p);
	}
	gui_closeUi(ui);
	free(infos);
}
