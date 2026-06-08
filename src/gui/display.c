#include "display.h"

People *showLogin(SDL_Ui *ui) {
	People *people;
	Object background1 =
		gui_initObject(ui, BOX, 0, 0, TOPLEFT,
					   (ObjectParam){.box = {512 + 256, 720, COLOR_DURTYPINK}});

	while (!ui->quit) {
		SDL_Event event;
		SDL_PumpEvents();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				ui->quit = true;
				break;

			case SDL_MOUSEBUTTONUP:
				ui->is_mouse_up = true;
				ui->mx = event.button.x;
				ui->my = event.button.y;
				break;

			case SDL_MOUSEBUTTONDOWN:
				ui->is_mouse_down = true;
				ui->mx = event.button.x;
				ui->my = event.button.y;
				break;

			case SDL_MOUSEMOTION:
				ui->is_mouse_move = true;
				ui->mx = event.button.x;
				ui->my = event.button.y;
				break;

			case SDL_TEXTINPUT:
				// if (new_subject.focused) {
				// 	int curr_len = strlen(input_buf);
				// 	int add_len = strlen(event.text.text);
				// 	if (curr_len + add_len < (int)sizeof(input_buf)) {
				// 		strcat(input_buf, event.text.text);
				// 		set_text(&new_subject_text, input_buf);
				// 	}
				// }
				break;

			case SDL_KEYDOWN:
				// if (new_subject.focused) {
				// 	if (event.key.keysym.sym == SDLK_BACKSPACE) {
				// 		utf8_backspace(input_buf);
				// 		if (input_buf[0] == '\0') {
				// 			set_text(&new_subject_text, "  ");
				// 			strcpy(input_buf, " ");
				// 		} else {
				// 			set_text(&new_subject_text, input_buf);
				// 		}
				// 	}
				// 	if (event.key.keysym.sym == SDLK_RETURN) {
				// 		new_subject.focused = false;
				// 		add_subject = false;
				// 		add_total_study(p_user_info, input_buf);
				// 		strcpy(input_buf, " ");
				// 	}
				// }
				// if (event.key.keysym.sym == SDLK_F5) {
				// 	quit = true;
				// 	write_savefile(p_user_info);
				// 	create_newfile(p_user_info);
				// }
				if (event.key.keysym.sym == SDLK_RETURN) {
					ui->quit = true;
				}
				break;
			}
		}

		if (ui->is_mouse_up) {
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 240, 244, 255);
		SDL_RenderClear(ui->renderer);
		gui_presentObject(&background1);

		SDL_RenderPresent(ui->renderer);

		ui->is_mouse_up = false;
		ui->is_mouse_down = false;
		ui->is_mouse_move = false;

		if (ui->next_state != LOGIN) {
			break;
		}
	}

	return people;
}
