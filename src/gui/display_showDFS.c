#include "display.h"

#define SV_MAIN_X 300
#define SV_RIGHT 1240

static int dfsui_inRect(int px, int py, int x, int y, int w, int h) {
	return px >= x && px <= x + w && py >= y && py <= y + h;
}

static void dfsui_drawRect(SDL_Ui *ui, int x, int y, int w, int h,
						   SDL_Color c) {
	SDL_Rect r = {x, y, w, h};
	SDL_SetRenderDrawColor(ui->renderer, c.r, c.g, c.b, c.a);
	SDL_RenderFillRect(ui->renderer, &r);
}

static void dfsui_drawRound(SDL_Ui *ui, int x, int y, int w, int h, int radius,
							SDL_Color c) {
	Object o = gui_initObject(ui, BOX, x, y, TOPLEFT,
							  (ObjectParam){.box = {w, h, c, radius}});
	gui_presentObject(&o);
}

static void dfsui_drawText(SDL_Ui *ui, const char *text, TTF_Font *font,
						   SDL_Color color, int x, int y, AnchorEnum anchor,
						   int wrap) {
	if (text == NULL || text[0] == '\0') {
		return;
	}
	SDL_Surface *s =
		(wrap > 0) ? TTF_RenderUTF8_Blended_Wrapped(font, text, color, wrap)
				   : TTF_RenderUTF8_Blended(font, text, color);
	if (s == NULL) {
		return;
	}
	SDL_Texture *t = SDL_CreateTextureFromSurface(ui->renderer, s);
	SDL_Rect r = {x, y, s->w, s->h};
	switch (anchor) {
	case TOPLEFT:
		break;
	case MIDTOP:
		r.x -= r.w / 2;
		break;
	case CENTER:
		r.x -= r.w / 2;
		r.y -= r.h / 2;
		break;
	case MIDBOTTOM:
		r.x -= r.w / 2;
		r.y -= r.h;
		break;
	}
	SDL_RenderCopy(ui->renderer, t, NULL, &r);
	SDL_DestroyTexture(t);
	SDL_FreeSurface(s);
}

static void dfsui_drawSidebar(SDL_Ui *ui, int active) {
	const char *labels[5] = {"홈", "매칭", "설문", "커플", "프로필"};
	int ny[5] = {150, 212, 274, 336, 398};
	dfsui_drawRect(ui, 0, 0, 260, WINDOW_HEIGHT, COLOR_WHITEPINK);
	dfsui_drawText(ui, "LIFO", ui->font_bbsig, COLOR_SUPERPINK, 110, 44,
				   MIDTOP, 0);
	for (int i = 0; i < 5; i++) {
		int hover = dfsui_inRect(ui->mx, ui->my, 30, ny[i], 200, 52);
		if (i == active || hover) {
			dfsui_drawRound(ui, 30, ny[i], 200, 52, 14, COLOR_PINK);
		}
		SDL_Color tc = (i == active || hover) ? COLOR_WHITE : COLOR_DURTYPINK;
		dfsui_drawText(ui, labels[i], ui->font_normal, tc, 70, ny[i] + 13,
					   TOPLEFT, 0);
	}
	int logout_y = WINDOW_HEIGHT - 82;
	int logout_hover = dfsui_inRect(ui->mx, ui->my, 30, logout_y, 200, 52);
	if (logout_hover) {
		dfsui_drawRound(ui, 30, logout_y, 200, 52, 14, COLOR_PINK);
	}
	dfsui_drawText(ui, "로그아웃", ui->font_normal,
				   logout_hover ? COLOR_WHITE : COLOR_DURTYPINK, 70,
				   logout_y + 13, TOPLEFT, 0);
}

static int dfsui_handleSidebarClick(SDL_Ui *ui, int active) {
	int ny[4] = {150, 212, 274, 336};
	MainStateEnum states[4] = {HOME, BFS, DFS, MST};
	for (int i = 0; i < 4; i++) {
		if (dfsui_inRect(ui->mx, ui->my, 30, ny[i], 200, 52)) {
			if (i == active) {
				return 0;
			}
			ui->next_state = states[i];
			return 1;
		}
	}
	if (dfsui_inRect(ui->mx, ui->my, 30, WINDOW_HEIGHT - 82, 200, 52)) {
		ui->next_state = LOGIN;
		return 1;
	}
	return 0;
}

static void dfsui_drawInput(SDL_Ui *ui, int x, int y, int w, int h,
							int focused, const char *label, const char *buf,
							const char *placeholder) {
	dfsui_drawText(ui, label, ui->font_small, COLOR_DURTYPINK, x, y - 28,
				   TOPLEFT, 0);
	dfsui_drawRound(ui, x, y, w, h, 12,
					focused ? COLOR_SUPERPINK : COLOR_PINK);
	dfsui_drawRound(ui, x + 2, y + 2, w - 4, h - 4, 10, COLOR_WHITEPINK);
	if (buf[0] == '\0' && !focused) {
		dfsui_drawText(ui, placeholder, ui->font_small, COLOR_WHITEGRAY,
					   x + 16, y + 16, TOPLEFT, w - 32);
	} else {
		char disp[DFS_Q_LEN + 4];
		snprintf(disp, sizeof(disp), "%s%s", buf, focused ? "_" : "");
		dfsui_drawText(ui, disp, ui->font_small, COLOR_GRAY, x + 16, y + 16,
					   TOPLEFT, w - 32);
	}
}

static int dfsui_runTree(SDL_Ui *ui, DfsTree *tree, const char *big_title,
						 char out_code[], char out_name[]) {
	int current = tree->root;
	int sel = -1;
	int stack_node[DFS_TREE_MAX_NODES];
	int stack_sel[DFS_TREE_MAX_NODES];
	int sp = 0;
	char status[128] = " ";
	const int OY0 = 330, OH = 64, OGAP = 76;

	while (!ui->quit) {
		DfsTreeNode *node = &tree->nodes[current];
		int n_opt = node->n_opt;
		int by = OY0 + n_opt * OGAP + 16;
		int prev_x = SV_MAIN_X, prev_w = 150;
		int next_x = SV_RIGHT - 180, next_w = 180;

		SDL_Event event;
		SDL_PumpEvents();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				ui->quit = true;
				break;
			case SDL_MOUSEBUTTONDOWN:
				ui->is_mouse_down = true;
				ui->mx = event.button.x;
				ui->my = event.button.y;
				break;
			case SDL_MOUSEMOTION:
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				break;
			case SDL_KEYDOWN: {
				SDL_Keycode k = event.key.keysym.sym;
				if (k >= SDLK_1 && k <= SDLK_4) {
					int idx = k - SDLK_1;
					if (idx < n_opt) {
						sel = idx;
					}
				} else if (k == SDLK_RETURN) {
					ui->is_mouse_down = true;
					ui->mx = next_x + 1;
					ui->my = by + 1;
				} else if (k == SDLK_ESCAPE) {
					ui->is_mouse_down = true;
					ui->mx = prev_x + 1;
					ui->my = by + 1;
				}
				break;
			}
			}
		}

		if (ui->is_mouse_down) {
			if (dfsui_handleSidebarClick(ui, 2)) {
				return 0;
			}
			for (int i = 0; i < n_opt; i++) {
				int oy = OY0 + i * OGAP;
				if (dfsui_inRect(ui->mx, ui->my, SV_MAIN_X, oy, 940, OH)) {
					sel = i;
				}
			}
			if (dfsui_inRect(ui->mx, ui->my, prev_x, by, prev_w, 56)) {
				if (sp == 0) {
					return 0;
				}
				sp--;
				current = stack_node[sp];
				sel = stack_sel[sp];
				ui->is_mouse_down = false;
				continue;
			}
			if (dfsui_inRect(ui->mx, ui->my, next_x, by, next_w, 56)) {
				if (sel < 0) {
					strcpy(status, "선택지를 골라주세요.");
				} else {
					int next = node->child[sel];
					stack_node[sp] = current;
					stack_sel[sp] = sel;
					sp++;
					if (tree->nodes[next].is_leaf) {
						strncpy(out_code, tree->nodes[next].code,
								MAX_TYPE_LEN - 1);
						out_code[MAX_TYPE_LEN - 1] = '\0';
						strncpy(out_name, tree->nodes[next].name,
								DFS_NAME_LEN - 1);
						out_name[DFS_NAME_LEN - 1] = '\0';
						return 1;
					}
					current = next;
					sel = -1;
					status[0] = ' ';
					status[1] = '\0';
				}
			}
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);
		dfsui_drawSidebar(ui, 2);

		dfsui_drawText(ui, big_title, ui->font_big, COLOR_BLACK, SV_MAIN_X, 28,
					   TOPLEFT, 0);
		char sub[96];
		snprintf(sub, sizeof(sub), "%s 트리", tree->title);
		dfsui_drawText(ui, sub, ui->font_small, COLOR_GRAY, SV_MAIN_X + 2, 82,
					   TOPLEFT, 0);

		dfsui_drawRound(ui, SV_MAIN_X, 150, 940, 150, 20, COLOR_WHITEPINK);
		dfsui_drawText(ui, "Q.", ui->font_small, COLOR_SUPERPINK,
					   SV_MAIN_X + 32, 174, TOPLEFT, 0);
		dfsui_drawText(ui, node->question, ui->font_normal, COLOR_BLACK,
					   SV_MAIN_X + 32, 208, TOPLEFT, 876);

		for (int i = 0; i < n_opt; i++) {
			int oy = OY0 + i * OGAP;
			int selected = (sel == i);
			int hover = dfsui_inRect(ui->mx, ui->my, SV_MAIN_X, oy, 940, OH);
			SDL_Color border = selected ? COLOR_SUPERPINK : COLOR_PINK;
			SDL_Color fill =
				(selected || hover) ? COLOR_WHITEPINK : COLOR_WHITE;
			dfsui_drawRound(ui, SV_MAIN_X, oy, 940, OH, 14, border);
			dfsui_drawRound(ui, SV_MAIN_X + 2, oy + 2, 936, OH - 4, 12, fill);
			int rcx = SV_MAIN_X + 40, rcy = oy + OH / 2;
			dfsui_drawRound(ui, rcx - 13, rcy - 13, 26, 26, 13, border);
			dfsui_drawRound(ui, rcx - 10, rcy - 10, 20, 20, 10, COLOR_WHITE);
			if (selected) {
				dfsui_drawRound(ui, rcx - 6, rcy - 6, 12, 12, 6,
								COLOR_SUPERPINK);
			}
			dfsui_drawText(ui, node->opt_text[i], ui->font_small,
						   selected ? COLOR_DURTYPINK : COLOR_GRAY,
						   SV_MAIN_X + 72, oy + 18, TOPLEFT, 800);
		}

		int prev_hover = dfsui_inRect(ui->mx, ui->my, prev_x, by, prev_w, 56);
		dfsui_drawRound(ui, prev_x, by, prev_w, 56, 14, COLOR_SOFTPINK);
		dfsui_drawRound(ui, prev_x + 2, by + 2, prev_w - 4, 52, 12,
						prev_hover ? COLOR_WHITEPINK : COLOR_WHITE);
		dfsui_drawText(ui, "← 이전", ui->font_small, COLOR_GRAY,
					   prev_x + prev_w / 2, by + 28, CENTER, 0);

		int next_hover = dfsui_inRect(ui->mx, ui->my, next_x, by, next_w, 56);
		dfsui_drawRound(ui, next_x, by, next_w, 56, 14,
						next_hover ? COLOR_DURTYPINK : COLOR_SUPERPINK);
		dfsui_drawText(ui, "다음 →", ui->font_normal, COLOR_WHITE,
					   next_x + next_w / 2, by + 28, CENTER, 0);
		dfsui_drawText(ui, status, ui->font_small, COLOR_SUPERPINK, SV_MAIN_X,
					   by + 70, TOPLEFT, 0);

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
	return 0;
}

static int dfsui_runSurvey(SDL_Ui *ui, DfsSurvey *survey,
						   char codes[][MAX_TYPE_LEN],
						   char names[][DFS_NAME_LEN]) {
	int i = 0;
	while (i < survey->n_trees) {
		char big[80];
		snprintf(big, sizeof(big), "%s 진단", survey->name);
		int r = dfsui_runTree(ui, &survey->trees[i], big, codes[i], names[i]);
		if (ui->quit) {
			return 0;
		}
		if (r == 0) {
			if (i == 0) {
				return 0;
			}
			i--;
			continue;
		}
		i++;
	}
	return 1;
}

static int dfsui_showAddQuestion(SDL_Ui *ui, DfsTree *tree, int leaf_idx,
								 char out_code[], char out_name[]) {
	char q_buf[DFS_Q_LEN] = "";
	char o0[DFS_OPT_LEN] = "";
	char o1[DFS_OPT_LEN] = "";
	int focus = 0;
	int mine = -1;
	char status[128] = " ";
	const char *pname = tree->nodes[leaf_idx].name;
	const int QY = 166, O0Y = 300, O1Y = 400;
	const int SAVE_X = 300, SAVE_W = 200, CANCEL_X = 520, CANCEL_W = 160,
			  BTNY = 500;
	const int RADIO_X = 1040, RADIO_W = 200;

	while (!ui->quit) {
		SDL_Event event;
		SDL_PumpEvents();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				ui->quit = true;
				break;
			case SDL_MOUSEBUTTONDOWN:
				ui->is_mouse_down = true;
				ui->mx = event.button.x;
				ui->my = event.button.y;
				break;
			case SDL_MOUSEMOTION:
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				break;
			case SDL_TEXTINPUT:
				if (focus == 1) {
					if (strlen(q_buf) + strlen(event.text.text) < DFS_Q_LEN - 1)
						strcat(q_buf, event.text.text);
				} else if (focus == 2) {
					if (strlen(o0) + strlen(event.text.text) < DFS_OPT_LEN - 1)
						strcat(o0, event.text.text);
				} else if (focus == 3) {
					if (strlen(o1) + strlen(event.text.text) < DFS_OPT_LEN - 1)
						strcat(o1, event.text.text);
				}
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_BACKSPACE) {
					if (focus == 1)
						gui_utf8Backspace(q_buf);
					else if (focus == 2)
						gui_utf8Backspace(o0);
					else if (focus == 3)
						gui_utf8Backspace(o1);
				} else if (event.key.keysym.sym == SDLK_TAB) {
					focus = (focus >= 3) ? 1 : focus + 1;
				}
				break;
			}
		}

		if (ui->is_mouse_down) {
			if (dfsui_handleSidebarClick(ui, 2)) {
				return 0;
			}
			if (dfsui_inRect(ui->mx, ui->my, 300, QY, 940, 56)) {
				focus = 1;
			} else if (dfsui_inRect(ui->mx, ui->my, 300, O0Y, 720, 56)) {
				focus = 2;
			} else if (dfsui_inRect(ui->mx, ui->my, 300, O1Y, 720, 56)) {
				focus = 3;
			} else if (dfsui_inRect(ui->mx, ui->my, RADIO_X, O0Y, RADIO_W,
									56)) {
				mine = 0;
				focus = 0;
			} else if (dfsui_inRect(ui->mx, ui->my, RADIO_X, O1Y, RADIO_W,
									56)) {
				mine = 1;
				focus = 0;
			} else if (dfsui_inRect(ui->mx, ui->my, SAVE_X, BTNY, SAVE_W, 56)) {
				focus = 0;
				if (strlen(q_buf) == 0 || strlen(o0) == 0 || strlen(o1) == 0) {
					strcpy(status, "질문과 두 선택지를 모두 입력해주세요.");
				} else if (mine < 0) {
					strcpy(status, "둘 중 본인에 해당하는 쪽을 골라주세요.");
				} else if (!dfs_extend_leaf(tree, leaf_idx, q_buf, o0, o1)) {
					strcpy(status, "더 이상 세분화할 수 없어요.");
				} else {
					dfs_save_tree(tree, tree->save_path);
					int child = tree->nodes[leaf_idx].child[mine];
					strncpy(out_code, tree->nodes[child].code,
							MAX_TYPE_LEN - 1);
					out_code[MAX_TYPE_LEN - 1] = '\0';
					strncpy(out_name, tree->nodes[child].name,
							DFS_NAME_LEN - 1);
					out_name[DFS_NAME_LEN - 1] = '\0';
					return 1;
				}
			} else if (dfsui_inRect(ui->mx, ui->my, CANCEL_X, BTNY, CANCEL_W,
									56)) {
				return 0;
			} else {
				focus = 0;
			}
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);
		dfsui_drawSidebar(ui, 2);

		dfsui_drawText(ui, "유형 더 세분화하기", ui->font_big, COLOR_BLACK,
					   SV_MAIN_X, 36, TOPLEFT, 0);
		char sub[160];
		snprintf(sub, sizeof(sub), "'%s' 유형을 새 질문으로 두 갈래로 나눕니다",
				 pname);
		dfsui_drawText(ui, sub, ui->font_small, COLOR_GRAY, SV_MAIN_X + 2, 92,
					   TOPLEFT, 0);

		dfsui_drawInput(ui, 300, QY, 940, 56, focus == 1, "추가할 질문",
						q_buf, "예: 주말엔 주로 뭐 해?");
		dfsui_drawInput(ui, 300, O0Y, 720, 56, focus == 2, "선택지 1", o0,
						"예: 집에서 쉰다");
		dfsui_drawInput(ui, 300, O1Y, 720, 56, focus == 3, "선택지 2", o1,
						"예: 밖에서 논다");

		for (int k = 0; k < 2; k++) {
			int ry = (k == 0) ? O0Y : O1Y;
			int sel = (mine == k);
			int hov = dfsui_inRect(ui->mx, ui->my, RADIO_X, ry, RADIO_W, 56);
			dfsui_drawRound(ui, RADIO_X, ry, RADIO_W, 56, 12,
							sel ? COLOR_DURTYPINK : COLOR_PINK);
			dfsui_drawRound(ui, RADIO_X + 2, ry + 2, RADIO_W - 4, 52, 10,
							sel ? COLOR_SUPERPINK
								: (hov ? COLOR_WHITEPINK : COLOR_WHITE));
			dfsui_drawText(ui, "내 유형", ui->font_small,
						   sel ? COLOR_WHITE : COLOR_GRAY,
						   RADIO_X + RADIO_W / 2, ry + 28, CENTER, 0);
		}

		int sh = dfsui_inRect(ui->mx, ui->my, SAVE_X, BTNY, SAVE_W, 56);
		dfsui_drawRound(ui, SAVE_X, BTNY, SAVE_W, 56, 14,
						sh ? COLOR_DURTYPINK : COLOR_SUPERPINK);
		dfsui_drawText(ui, "저장", ui->font_normal, COLOR_WHITE,
					   SAVE_X + SAVE_W / 2, BTNY + 28, CENTER, 0);
		int ch = dfsui_inRect(ui->mx, ui->my, CANCEL_X, BTNY, CANCEL_W, 56);
		dfsui_drawRound(ui, CANCEL_X, BTNY, CANCEL_W, 56, 14, COLOR_SOFTPINK);
		dfsui_drawRound(ui, CANCEL_X + 2, BTNY + 2, CANCEL_W - 4, 52, 12,
						ch ? COLOR_WHITEPINK : COLOR_WHITE);
		dfsui_drawText(ui, "취소", ui->font_small, COLOR_GRAY,
					   CANCEL_X + CANCEL_W / 2, BTNY + 28, CENTER, 0);
		dfsui_drawText(ui, status, ui->font_small, COLOR_SUPERPINK, SV_MAIN_X,
					   BTNY + 72, TOPLEFT, 0);

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
	return 0;
}

static void dfsui_showSurveyResult(SDL_Ui *ui, People *me, DfsSurvey *self_s,
								   char self_codes[][MAX_TYPE_LEN],
								   char self_names[][DFS_NAME_LEN],
								   DfsSurvey *ideal_s,
								   char ideal_codes[][MAX_TYPE_LEN],
								   char ideal_names[][DFS_NAME_LEN]) {
	int bx = SV_MAIN_X, bw = 200, byy = 520;
	int ex_x = 540, ex_w = 260;
	while (!ui->quit) {
		SDL_Event event;
		SDL_PumpEvents();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				ui->quit = true;
				break;
			case SDL_MOUSEBUTTONDOWN:
				ui->is_mouse_down = true;
				ui->mx = event.button.x;
				ui->my = event.button.y;
				break;
			case SDL_MOUSEMOTION:
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_RETURN) {
					return;
				}
				break;
			}
		}
		if (ui->is_mouse_down) {
			if (dfsui_inRect(ui->mx, ui->my, bx, byy, bw, 56)) {
				ui->next_state = HOME;
				return;
			}
			if (dfsui_handleSidebarClick(ui, 2)) {
				return;
			}
			if (dfsui_inRect(ui->mx, ui->my, ex_x, byy, ex_w, 56)) {
				int li =
					dfs_find_leaf_by_code(&self_s->trees[0], self_codes[0]);
				if (li >= 0) {
					char nc[MAX_TYPE_LEN], nn[DFS_NAME_LEN];
					if (dfsui_showAddQuestion(ui, &self_s->trees[0], li, nc,
											  nn)) {
						strncpy(self_codes[0], nc, MAX_TYPE_LEN - 1);
						self_codes[0][MAX_TYPE_LEN - 1] = '\0';
						strncpy(self_names[0], nn, DFS_NAME_LEN - 1);
						self_names[0][DFS_NAME_LEN - 1] = '\0';
						strncpy(me->type, nc, MAX_TYPE_LEN - 1);
						me->type[MAX_TYPE_LEN - 1] = '\0';
					}
				}
			}
			ui->is_mouse_down = false;
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);
		dfsui_drawSidebar(ui, 2);

		dfsui_drawText(ui, "진단이 완료됐어요", ui->font_big, COLOR_BLACK,
					   SV_MAIN_X, 36, TOPLEFT, 0);
		dfsui_drawText(
			ui,
			"대주제별 결과예요. 매칭 유사도는 이 결과들을 합쳐 계산할 예정.",
			ui->font_small, COLOR_GRAY, SV_MAIN_X + 2, 92, TOPLEFT, 0);

		char line[160];
		dfsui_drawRound(ui, SV_MAIN_X, 150, 940, 160, 20, COLOR_WHITEPINK);
		dfsui_drawText(ui, "내 연애 성향", ui->font_normal, COLOR_SUPERPINK,
					   SV_MAIN_X + 30, 172, TOPLEFT, 0);
		for (int i = 0; i < self_s->n_trees; i++) {
			snprintf(line, sizeof(line), "· %s : %s (%s)",
					 self_s->trees[i].title, self_names[i], self_codes[i]);
			dfsui_drawText(ui, line, ui->font_small, COLOR_DURTYPINK,
						   SV_MAIN_X + 40, 216 + i * 36, TOPLEFT, 860);
		}

		dfsui_drawRound(ui, SV_MAIN_X, 330, 940, 160, 20, COLOR_WHITEVIOLET);
		dfsui_drawText(ui, "내 이상형", ui->font_normal, COLOR_VIOLET,
					   SV_MAIN_X + 30, 352, TOPLEFT, 0);
		for (int i = 0; i < ideal_s->n_trees; i++) {
			snprintf(line, sizeof(line), "· %s : %s (%s)",
					 ideal_s->trees[i].title, ideal_names[i], ideal_codes[i]);
			dfsui_drawText(ui, line, ui->font_small, COLOR_SOFTVIOLET,
						   SV_MAIN_X + 40, 396 + i * 36, TOPLEFT, 860);
		}

		int hov = dfsui_inRect(ui->mx, ui->my, bx, byy, bw, 56);
		dfsui_drawRound(ui, bx, byy, bw, 56, 14,
						hov ? COLOR_DURTYPINK : COLOR_SUPERPINK);
		dfsui_drawText(ui, "홈으로 →", ui->font_normal, COLOR_WHITE,
					   bx + bw / 2, byy + 28, CENTER, 0);

		int eh = dfsui_inRect(ui->mx, ui->my, ex_x, byy, ex_w, 56);
		dfsui_drawRound(ui, ex_x, byy, ex_w, 56, 14, COLOR_SOFTPINK);
		dfsui_drawRound(ui, ex_x + 2, byy + 2, ex_w - 4, 52, 12,
						eh ? COLOR_WHITEPINK : COLOR_WHITE);
		dfsui_drawText(ui, "내 성향 세분화하기", ui->font_small,
					   COLOR_DURTYPINK, ex_x + ex_w / 2, byy + 28, CENTER, 0);

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
}

void display_showDFS(SDL_Ui *ui, People *me) {
	if (me == NULL) {
		ui->next_state = HOME;
		return;
	}

	DfsSurvey *self_s = malloc(sizeof(DfsSurvey));
	DfsSurvey *ideal_s = malloc(sizeof(DfsSurvey));
	if (self_s == NULL || ideal_s == NULL) {
		free(self_s);
		free(ideal_s);
		ui->next_state = HOME;
		return;
	}
	dfs_build_self_survey(self_s);
	dfs_build_ideal_survey(ideal_s);

	char self_codes[DFS_MAX_TREES][MAX_TYPE_LEN];
	char self_names[DFS_MAX_TREES][DFS_NAME_LEN];
	char ideal_codes[DFS_MAX_TREES][MAX_TYPE_LEN];
	char ideal_names[DFS_MAX_TREES][DFS_NAME_LEN];

	if (!dfsui_runSurvey(ui, self_s, self_codes, self_names)) {
		goto done;
	}
	strncpy(me->type, self_codes[0], MAX_TYPE_LEN - 1);
	me->type[MAX_TYPE_LEN - 1] = '\0';

	if (!dfsui_runSurvey(ui, ideal_s, ideal_codes, ideal_names)) {
		goto done;
	}
	strncpy(me->love_type, ideal_codes[0], MAX_TYPE_LEN - 1);
	me->love_type[MAX_TYPE_LEN - 1] = '\0';

	dfsui_showSurveyResult(ui, me, self_s, self_codes, self_names, ideal_s,
						   ideal_codes, ideal_names);

done:
	free(self_s);
	free(ideal_s);
	if (ui->next_state == DFS) {
		ui->next_state = HOME;
	}
}
