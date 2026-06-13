#include "display.h"

#define SV_MAIN_X 280
#define SV_RIGHT 1250

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
	const char *labels[4] = {"홈", "매칭", "설문", "프로필"};
	int ny[4] = {150, 150 + 65, 150 + 65 * 2, 150 + 65 * 3};
	dfsui_drawRect(ui, 0, 0, 250, WINDOW_HEIGHT, COLOR_WHITEPINK);
	dfsui_drawText(ui, "LIFO", ui->font_bbsig, COLOR_SUPERPINK, 110, 44,
				   MIDTOP, 0);
	for (int i = 0; i < 4; i++) {
		int hover = dfsui_inRect(ui->mx, ui->my, 40, ny[i], 170, 51);
		if (i == active || hover) {
			dfsui_drawRound(ui, 40, ny[i], 170, 51, 14, COLOR_PINK);
		}
		SDL_Color tc = (i == active || hover) ? COLOR_WHITE : COLOR_DURTYPINK;
		dfsui_drawText(ui, labels[i], ui->font_normal, tc, 72,
					   ny[i] + 51 / 2, CENTER, 0);
	}
	int logout_y = WINDOW_HEIGHT - 30 - 51;
	int logout_hover = dfsui_inRect(ui->mx, ui->my, 40, logout_y, 170, 51);
	if (logout_hover) {
		dfsui_drawRound(ui, 40, logout_y, 170, 51, 14, COLOR_PINK);
	}
	dfsui_drawText(ui, "로그아웃", ui->font_normal,
				   logout_hover ? COLOR_WHITE : COLOR_DURTYPINK, 72,
				   WINDOW_HEIGHT - 30 - 51 / 2, CENTER, 0);
}

static int dfsui_handleSidebarClick(SDL_Ui *ui, int active) {
	int ny[3] = {150, 150 + 65, 150 + 65 * 2};
	MainStateEnum states[3] = {HOME, BFS, DFS};
	for (int i = 0; i < 3; i++) {
		if (dfsui_inRect(ui->mx, ui->my, 40, ny[i], 170, 51)) {
			if (i == active) {
				return 0;
			}
			ui->next_state = states[i];
			return 1;
		}
	}
	if (dfsui_inRect(ui->mx, ui->my, 40, WINDOW_HEIGHT - 30 - 51, 170, 51)) {
		ui->next_state = LOGIN;
		return 1;
	}
	return 0;
}

static void dfsui_drawInput(SDL_Ui *ui, int x, int y, int w, int h, int focused,
							const char *label, const char *buf,
							const char *placeholder) {
	dfsui_drawText(ui, label, ui->font_small, COLOR_DURTYPINK, x, y - 28,
				   TOPLEFT, 0);
	dfsui_drawRound(ui, x, y, w, h, 12, focused ? COLOR_SUPERPINK : COLOR_PINK);
	dfsui_drawRound(ui, x + 2, y + 2, w - 4, h - 4, 10, COLOR_WHITEPINK);
	if (buf[0] == '\0' && !focused) {
		dfsui_drawText(ui, placeholder, ui->font_small, COLOR_WHITEGRAY, x + 16,
					   y + 16, TOPLEFT, w - 32);
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

	ui->is_mouse_down = false; // 이전 화면/트리의 클릭이 새어들지 않게 초기화

	while (!ui->quit) {
		DfsTreeNode *node = &tree->nodes[current];
		int n_opt = node->n_opt;
		int by = OY0 + n_opt * OGAP + 16;
		int prev_x = SV_MAIN_X, prev_w = 150;
		int next_x = SV_RIGHT - 180, next_w = 180;
		// 세분화(user_added) 질문은 답하지 않고 현재 유형으로 멈출 수 있다.
		int can_stop = node->user_added && node->code[0] != '\0';
		int stop_x = SV_MAIN_X + 270, stop_w = 320;

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
				ui->is_mouse_down = false;
				return 0;
			}
			for (int i = 0; i < n_opt; i++) {
				int oy = OY0 + i * OGAP;
				if (dfsui_inRect(ui->mx, ui->my, SV_MAIN_X, oy, 940, OH)) {
					sel = i;
				}
			}
			// 세분화 질문에서 "여기서 멈추기" → 현재(부모) 유형으로 확정
			if (can_stop &&
				dfsui_inRect(ui->mx, ui->my, stop_x, by, stop_w, 56)) {
				strncpy(out_code, node->code, MAX_TYPE_LEN - 1);
				out_code[MAX_TYPE_LEN - 1] = '\0';
				strncpy(out_name, node->name, DFS_NAME_LEN - 1);
				out_name[DFS_NAME_LEN - 1] = '\0';
				ui->is_mouse_down = false;
				return 1;
			}
			if (dfsui_inRect(ui->mx, ui->my, prev_x, by, prev_w, 56)) {
				if (sp == 0) {
					ui->is_mouse_down = false;
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
						ui->is_mouse_down = false;
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

		// 세분화 질문이면 안내 + "여기서 멈추기" 버튼
		if (can_stop) {
			char hint[160];
			snprintf(hint, sizeof(hint),
					 "추가된 세분화 질문이에요. 멈추면 '%s'(으)로 진단됩니다.",
					 node->name);
			dfsui_drawText(ui, hint, ui->font_small, COLOR_VIOLET,
						   SV_MAIN_X + 2, 116, TOPLEFT, 876);
			int stop_hover =
				dfsui_inRect(ui->mx, ui->my, stop_x, by, stop_w, 56);
			dfsui_drawRound(ui, stop_x, by, stop_w, 56, 14, COLOR_SOFTVIOLET);
			dfsui_drawRound(ui, stop_x + 2, by + 2, stop_w - 4, 52, 12,
							stop_hover ? COLOR_WHITEVIOLET : COLOR_WHITE);
			dfsui_drawText(ui, "여기서 멈추기", ui->font_small, COLOR_VIOLET,
						   stop_x + stop_w / 2, by + 28, CENTER, 0);
		}

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
	char o0[DFS_OPT_LEN] = "", o1[DFS_OPT_LEN] = "";
	char name0[DFS_NAME_LEN] = "", name1[DFS_NAME_LEN] = "";
	char desc0[DFS_DESC_LEN] = "", desc1[DFS_DESC_LEN] = "";
	// 포커스: 1 질문, 2 선택지1, 3 이름1, 4 설명1, 5 선택지2, 6 이름2, 7 설명2
	int focus = 0, mine = -1;
	char status[128] = " ";
	const char *pname = tree->nodes[leaf_idx].name;
	char *bufs[8] = {NULL, q_buf, o0, name0, desc0, o1, name1, desc1};
	size_t caps[8] = {0,			DFS_Q_LEN,	  DFS_OPT_LEN,
					  DFS_NAME_LEN, DFS_DESC_LEN, DFS_OPT_LEN,
					  DFS_NAME_LEN, DFS_DESC_LEN};

	const int QY = 128, BOXH = 50;
	const int G1_HDR = 192, R1Y = 218, D1Y = 276;
	const int G2_HDR = 344, R2Y = 370, D2Y = 428;
	const int OPT_X = 300, OPT_W = 320;
	const int NAME_X = 632, NAME_W = 300;
	const int RADIO_X = 944, RADIO_W = 150;
	const int SAVE_X = 300, SAVE_W = 200, CANCEL_X = 520, CANCEL_W = 160,
			  BTNY = 500;
	const int STATUS_Y = 566;
	const int rowY[2] = {R1Y, R2Y}, descY[2] = {D1Y, D2Y};

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
				if (focus >= 1 && focus <= 7) {
					char *dst = bufs[focus];
					size_t cap = caps[focus];
					if (strlen(dst) + strlen(event.text.text) < cap - 1) {
						strcat(dst, event.text.text);
					}
				}
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_BACKSPACE) {
					if (focus >= 1 && focus <= 7) {
						gui_utf8Backspace(bufs[focus]);
					}
				} else if (event.key.keysym.sym == SDLK_TAB) {
					focus = (focus >= 7) ? 1 : focus + 1;
				}
				break;
			}
		}

		if (ui->is_mouse_down) {
			if (dfsui_handleSidebarClick(ui, 2)) {
				ui->is_mouse_down = false;
				return 0;
			}
			if (dfsui_inRect(ui->mx, ui->my, OPT_X, QY, 940, BOXH)) {
				focus = 1;
			} else if (dfsui_inRect(ui->mx, ui->my, OPT_X, R1Y, OPT_W, BOXH)) {
				focus = 2;
			} else if (dfsui_inRect(ui->mx, ui->my, NAME_X, R1Y, NAME_W, BOXH)) {
				focus = 3;
			} else if (dfsui_inRect(ui->mx, ui->my, OPT_X, D1Y, 940, BOXH)) {
				focus = 4;
			} else if (dfsui_inRect(ui->mx, ui->my, OPT_X, R2Y, OPT_W, BOXH)) {
				focus = 5;
			} else if (dfsui_inRect(ui->mx, ui->my, NAME_X, R2Y, NAME_W, BOXH)) {
				focus = 6;
			} else if (dfsui_inRect(ui->mx, ui->my, OPT_X, D2Y, 940, BOXH)) {
				focus = 7;
			} else if (dfsui_inRect(ui->mx, ui->my, RADIO_X, R1Y, RADIO_W,
									BOXH)) {
				mine = 0;
				focus = 0;
			} else if (dfsui_inRect(ui->mx, ui->my, RADIO_X, R2Y, RADIO_W,
									BOXH)) {
				mine = 1;
				focus = 0;
			} else if (dfsui_inRect(ui->mx, ui->my, SAVE_X, BTNY, SAVE_W, 56)) {
				focus = 0;
				if (strlen(q_buf) == 0 || strlen(o0) == 0 || strlen(o1) == 0) {
					strcpy(status, "질문과 두 선택지를 모두 입력해주세요.");
				} else if (mine < 0) {
					strcpy(status, "둘 중 본인에 해당하는 쪽을 골라주세요.");
				} else if (!dfs_extend_leaf_named(tree, leaf_idx, q_buf, o0, o1,
												  name0, name1, desc0, desc1)) {
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
					ui->is_mouse_down = false;
					return 1;
				}
			} else if (dfsui_inRect(ui->mx, ui->my, CANCEL_X, BTNY, CANCEL_W,
									56)) {
				ui->is_mouse_down = false;
				return 0;
			} else {
				focus = 0;
			}
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);
		dfsui_drawSidebar(ui, 2);

		dfsui_drawText(ui, "유형 더 세분화하기", ui->font_big, COLOR_BLACK,
					   SV_MAIN_X, 30, TOPLEFT, 0);
		char sub[160];
		snprintf(sub, sizeof(sub), "'%s' 유형을 새 질문으로 두 갈래로 나눕니다",
				 pname);
		dfsui_drawText(ui, sub, ui->font_small, COLOR_GRAY, SV_MAIN_X + 2, 80,
					   TOPLEFT, 0);

		dfsui_drawInput(ui, OPT_X, QY, 940, BOXH, focus == 1, "추가할 질문",
						q_buf, "예: 주말엔 주로 뭐 해?");

		const char *opt_ph[2] = {"예: 집에서 쉰다", "예: 밖에서 논다"};
		const char *desc_ph = "이 유형 설명 (비우면 부모 설명 사용)";
		char *opt_buf[2] = {o0, o1};
		char *name_buf[2] = {name0, name1};
		char *desc_buf[2] = {desc0, desc1};
		int hdrY[2] = {G1_HDR, G2_HDR};
		int opt_focus[2] = {2, 5}, name_focus[2] = {3, 6}, desc_focus[2] = {4,
																			7};
		for (int k = 0; k < 2; k++) {
			char hdr[16];
			snprintf(hdr, sizeof(hdr), "갈래 %d", k + 1);
			dfsui_drawText(ui, hdr, ui->font_normal, COLOR_DURTYPINK, OPT_X,
						   hdrY[k], TOPLEFT, 0);
			dfsui_drawInput(ui, OPT_X, rowY[k], OPT_W, BOXH,
							focus == opt_focus[k], "", opt_buf[k], opt_ph[k]);
			dfsui_drawInput(ui, NAME_X, rowY[k], NAME_W, BOXH,
							focus == name_focus[k], "", name_buf[k],
							"유형 이름 (비우면 자동)");
			dfsui_drawInput(ui, OPT_X, descY[k], 940, BOXH,
							focus == desc_focus[k], "", desc_buf[k], desc_ph);

			int sel = (mine == k);
			int hov = dfsui_inRect(ui->mx, ui->my, RADIO_X, rowY[k], RADIO_W,
								   BOXH);
			dfsui_drawRound(ui, RADIO_X, rowY[k], RADIO_W, BOXH, 12,
							sel ? COLOR_DURTYPINK : COLOR_PINK);
			dfsui_drawRound(ui, RADIO_X + 2, rowY[k] + 2, RADIO_W - 4, BOXH - 4,
							10,
							sel ? COLOR_SUPERPINK
								: (hov ? COLOR_WHITEPINK : COLOR_WHITE));
			dfsui_drawText(ui, "내 유형", ui->font_small,
						   sel ? COLOR_WHITE : COLOR_GRAY,
						   RADIO_X + RADIO_W / 2, rowY[k] + BOXH / 2, CENTER, 0);
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
					   STATUS_Y, TOPLEFT, 0);

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
	return 0;
}

// ───────────────────────────────────────────────
// 트리 시각화 (그래프: 위→아래 노드-연결선)
// ───────────────────────────────────────────────
// 각 노드 중심 좌표를 px/py에 채운다. 잎은 col 순서대로 가로 배치,
// 내부 노드는 자식들의 가운데 x. 반환값은 해당 노드의 x.
static int treeview_layout(const DfsTree *t, int node, int depth, int *col,
						   int *px, int *py, int colW, int rowH, int baseX,
						   int baseY) {
	if (node < 0 || node >= t->n_nodes) {
		return baseX;
	}
	const DfsTreeNode *n = &t->nodes[node];
	py[node] = baseY + depth * rowH;
	if (n->is_leaf) {
		px[node] = baseX + (*col) * colW;
		(*col)++;
		return px[node];
	}
	long sum = 0;
	int cnt = 0;
	for (int i = 0; i < n->n_opt; i++) {
		sum += treeview_layout(t, n->child[i], depth + 1, col, px, py, colW,
							   rowH, baseX, baseY);
		cnt++;
	}
	px[node] = cnt ? (int)(sum / cnt) : baseX + (*col) * colW;
	return px[node];
}

// scale(%): 60~180. 노드/간격/박스를 확대·축소. 유형 코드(영문)는 표시하지 않음.
static void treeview_draw(SDL_Ui *ui, const DfsTree *t, const char *highlight,
						  int baseX, int baseY, int scale) {
	int colW = 215 * scale / 100;
	int rowH = 155 * scale / 100;
	int boxW = 180 * scale / 100;
	int lh = 64 * scale / 100;	// 잎/단순 분기 박스 높이
	int dh = 96 * scale / 100;	// 이름+질문 동시 표시 박스 높이
	int pillW = 168 * scale / 100, pillH = 34 * scale / 100;
	int wrap = boxW - 18;
	TTF_Font *nf = (scale >= 90) ? ui->font_small : ui->font_ssmall; // 이름
	TTF_Font *qf = ui->font_ssmall;									 // 질문/라벨
	int px[DFS_TREE_MAX_NODES], py[DFS_TREE_MAX_NODES];
	int col = 0;
	treeview_layout(t, t->root, 0, &col, px, py, colW, rowH, baseX, baseY);

	// 1) 간선
	SDL_SetRenderDrawColor(ui->renderer, 201, 116, 138, 255); // DURTYPINK
	for (int i = 0; i < t->n_nodes; i++) {
		const DfsTreeNode *n = &t->nodes[i];
		if (n->is_leaf) {
			continue;
		}
		for (int k = 0; k < n->n_opt; k++) {
			int c = n->child[k];
			if (c < 0 || c >= t->n_nodes) {
				continue;
			}
			SDL_RenderDrawLine(ui->renderer, px[i], py[i] + lh / 2, px[c],
							   py[c] - lh / 2);
		}
	}

	// 2) 노드 (이름·질문만, 코드는 숨김)
	for (int i = 0; i < t->n_nodes; i++) {
		const DfsTreeNode *n = &t->nodes[i];
		int bx = px[i] - boxW / 2;
		int hl = highlight && highlight[0] && n->code[0] &&
				 strcmp(n->code, highlight) == 0;
		if (n->is_leaf) {
			int by = py[i] - lh / 2;
			dfsui_drawRound(ui, bx, by, boxW, lh, 12,
							hl ? COLOR_SUPERPINK : COLOR_PINK);
			dfsui_drawRound(ui, bx + 2, by + 2, boxW - 4, lh - 4, 10,
							hl ? COLOR_WHITEPINK : COLOR_WHITE);
			dfsui_drawText(ui, n->name, nf,
						   hl ? COLOR_SUPERPINK : COLOR_DURTYPINK, px[i], py[i],
						   CENTER, wrap);
		} else if (n->code[0]) {
			// 세분화로 분기가 된 옛 잎: 유형명 + 질문 함께 표시.
			int by = py[i] - dh / 2;
			dfsui_drawRound(ui, bx, by, boxW, dh, 12,
							hl ? COLOR_SUPERPINK : COLOR_SOFTVIOLET);
			dfsui_drawRound(ui, bx + 2, by + 2, boxW - 4, dh - 4, 10,
							hl ? COLOR_WHITEPINK : COLOR_WHITEVIOLET);
			dfsui_drawText(ui, n->name, nf,
						   hl ? COLOR_SUPERPINK : COLOR_VIOLET, px[i],
						   by + dh / 6, MIDTOP, wrap);
			dfsui_drawText(ui, n->question, qf, COLOR_GRAY, px[i],
						   by + dh / 2, MIDTOP, wrap);
		} else {
			int by = py[i] - lh / 2;
			dfsui_drawRound(ui, bx, by, boxW, lh, 12, COLOR_SOFTVIOLET);
			dfsui_drawRound(ui, bx + 2, by + 2, boxW - 4, lh - 4, 10,
							COLOR_WHITEVIOLET);
			dfsui_drawText(ui, n->question, qf, COLOR_VIOLET, px[i], py[i],
						   CENTER, wrap);
		}
	}

	// 3) 간선 라벨: 질문에 대한 대답(선택지 텍스트)
	for (int i = 0; i < t->n_nodes; i++) {
		const DfsTreeNode *n = &t->nodes[i];
		if (n->is_leaf) {
			continue;
		}
		for (int k = 0; k < n->n_opt; k++) {
			int c = n->child[k];
			if (c < 0 || c >= t->n_nodes) {
				continue;
			}
			int mx = (px[i] + px[c]) / 2;
			int my = (py[i] + py[c]) / 2;
			dfsui_drawRound(ui, mx - pillW / 2, my - pillH / 2, pillW, pillH, 9,
							COLOR_WHITEPINK);
			dfsui_drawText(ui, n->opt_text[k], qf, COLOR_DURTYPINK, mx, my,
						   CENTER, pillW - 12);
		}
	}
}

static void dfsui_showTreeView(SDL_Ui *ui, DfsSurvey *self_s,
							   char self_codes[][MAX_TYPE_LEN],
							   DfsSurvey *ideal_s,
							   char ideal_codes[][MAX_TYPE_LEN]) {
	DfsTree *trees[6];
	const char *tabs[6];
	const char *hl[6];
	const char *self_labels[3] = {"내 성향", "내 애착", "내 언어"};
	const char *ideal_labels[3] = {"이상형 성향", "이상형 애착", "이상형 언어"};
	int ntab = 0;
	for (int i = 0; i < self_s->n_trees && ntab < 6; i++) {
		trees[ntab] = &self_s->trees[i];
		tabs[ntab] = self_labels[i < 3 ? i : 2];
		hl[ntab] = self_codes[i];
		ntab++;
	}
	for (int i = 0; i < ideal_s->n_trees && ntab < 6; i++) {
		trees[ntab] = &ideal_s->trees[i];
		tabs[ntab] = ideal_labels[i < 3 ? i : 2];
		hl[ntab] = ideal_codes[i];
		ntab++;
	}
	if (ntab == 0) {
		return;
	}

	int cur = 0, ox = 0, oy = 0, scale = 100;
	int dragging = 0, dsx = 0, dsy = 0, dox = 0, doy = 0;
	const int BACK_X = SV_MAIN_X, BACK_Y = 36, BACK_W = 110, BACK_H = 40;
	const int TAB_Y = 92, TAB_H = 42, TAB_W = 145, TAB_GAP = 152;
	const int ZOUT_X = WINDOW_WIDTH - 110, ZIN_X = WINDOW_WIDTH - 64,
			  ZBTN_Y = TAB_Y, ZBTN = 42; // 축소/확대 버튼
	const int TREE_TOP = TAB_Y + TAB_H + 24; // 트리/헤더 마스킹 경계

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
				if (ui->my > TREE_TOP && ui->mx > 260) { // 트리 영역 → 드래그
					dragging = 1;
					dsx = ui->mx;
					dsy = ui->my;
					dox = ox;
					doy = oy;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				dragging = 0;
				break;
			case SDL_MOUSEMOTION:
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				if (dragging) {
					ox = dox + (ui->mx - dsx);
					oy = doy + (ui->my - dsy);
				}
				break;
			case SDL_MOUSEWHEEL:
				oy += event.wheel.y * 40;
				ox += event.wheel.x * 40;
				break;
			case SDL_KEYDOWN: {
				SDL_Keycode k = event.key.keysym.sym;
				if (k == SDLK_ESCAPE) {
					return;
				} else if (k == SDLK_LEFT) {
					ox += 60;
				} else if (k == SDLK_RIGHT) {
					ox -= 60;
				} else if (k == SDLK_UP) {
					oy += 60;
				} else if (k == SDLK_DOWN) {
					oy -= 60;
				} else if (k == SDLK_PLUS || k == SDLK_EQUALS ||
						   k == SDLK_KP_PLUS) {
					if (scale < 180) scale += 20;
				} else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) {
					if (scale > 60) scale -= 20;
				}
				break;
			}
			}
		}

		if (ui->is_mouse_down && !dragging) {
			if (dfsui_inRect(ui->mx, ui->my, BACK_X, BACK_Y, BACK_W, BACK_H)) {
				ui->is_mouse_down = false;
				return;
			}
			if (dfsui_handleSidebarClick(ui, 2)) {
				ui->is_mouse_down = false;
				return;
			}
			for (int i = 0; i < ntab; i++) {
				int tx = SV_MAIN_X + i * TAB_GAP;
				if (dfsui_inRect(ui->mx, ui->my, tx, TAB_Y, TAB_W, TAB_H)) {
					if (cur != i) {
						cur = i;
						ox = 0;
						oy = 0;
					}
				}
			}
			if (dfsui_inRect(ui->mx, ui->my, ZOUT_X, ZBTN_Y, ZBTN, ZBTN)) {
				if (scale > 60) scale -= 20;
			} else if (dfsui_inRect(ui->mx, ui->my, ZIN_X, ZBTN_Y, ZBTN,
									ZBTN)) {
				if (scale < 180) scale += 20;
			}
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		// 트리부터 그린 뒤, 상단/사이드바를 덮어 영역 밖을 가린다.
		treeview_draw(ui, trees[cur], hl[cur], SV_MAIN_X + 120 + ox,
					  TREE_TOP + 30 + oy, scale);

		dfsui_drawRect(ui, 0, 0, WINDOW_WIDTH, TREE_TOP, COLOR_WHITE);
		dfsui_drawSidebar(ui, 2);

		dfsui_drawRound(ui, BACK_X, BACK_Y, BACK_W, BACK_H, 12, COLOR_SOFTPINK);
		dfsui_drawText(ui, "← 뒤로", ui->font_small, COLOR_DURTYPINK,
					   BACK_X + BACK_W / 2, BACK_Y + BACK_H / 2, CENTER, 0);
		dfsui_drawText(ui, "트리 시각화", ui->font_big, COLOR_BLACK,
					   BACK_X + BACK_W + 24, BACK_Y + 2, TOPLEFT, 0);

		for (int i = 0; i < ntab; i++) {
			int tx = SV_MAIN_X + i * TAB_GAP;
			int on = (i == cur);
			int hov = dfsui_inRect(ui->mx, ui->my, tx, TAB_Y, TAB_W, TAB_H);
			dfsui_drawRound(ui, tx, TAB_Y, TAB_W, TAB_H, 12,
							on ? COLOR_SUPERPINK : COLOR_PINK);
			dfsui_drawRound(ui, tx + 2, TAB_Y + 2, TAB_W - 4, TAB_H - 4, 10,
							on ? COLOR_SUPERPINK
							   : (hov ? COLOR_WHITEPINK : COLOR_WHITE));
			dfsui_drawText(ui, tabs[i], ui->font_small,
						   on ? COLOR_WHITE : COLOR_DURTYPINK, tx + TAB_W / 2,
						   TAB_Y + TAB_H / 2, CENTER, 0);
		}
		dfsui_drawText(ui, "드래그·휠로 이동, +/− 확대", ui->font_ssmall,
					   COLOR_GRAY, BACK_X + BACK_W + 220, BACK_Y + 14, TOPLEFT,
					   0);

		// 축소(−) / 확대(+) 버튼 + 배율 표시
		int zoh = dfsui_inRect(ui->mx, ui->my, ZOUT_X, ZBTN_Y, ZBTN, ZBTN);
		dfsui_drawRound(ui, ZOUT_X, ZBTN_Y, ZBTN, ZBTN, 10,
						zoh ? COLOR_DURTYPINK : COLOR_PINK);
		dfsui_drawText(ui, "−", ui->font_normal, COLOR_WHITE, ZOUT_X + ZBTN / 2,
					   ZBTN_Y + ZBTN / 2, CENTER, 0);
		int zih = dfsui_inRect(ui->mx, ui->my, ZIN_X, ZBTN_Y, ZBTN, ZBTN);
		dfsui_drawRound(ui, ZIN_X, ZBTN_Y, ZBTN, ZBTN, 10,
						zih ? COLOR_DURTYPINK : COLOR_PINK);
		dfsui_drawText(ui, "+", ui->font_normal, COLOR_WHITE, ZIN_X + ZBTN / 2,
					   ZBTN_Y + ZBTN / 2, CENTER, 0);
		char zlbl[16];
		snprintf(zlbl, sizeof(zlbl), "%d%%", scale);
		dfsui_drawText(ui, zlbl, ui->font_ssmall, COLOR_GRAY,
					   (ZOUT_X + ZIN_X + ZBTN) / 2, ZBTN_Y + ZBTN + 8, CENTER,
					   0);

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
}

// ───────────────────────────────────────────────
// 유형 도감: 트리별 모든 유형의 이름·설명을 스크롤 목록으로 보여준다.
// codes(있으면)와 일치하는 유형은 "내 유형"으로 강조.
// ───────────────────────────────────────────────
static void dfsui_showCodex(SDL_Ui *ui, DfsSurvey *s,
							char codes[][MAX_TYPE_LEN]) {
	int oy = 0, dragging = 0, dsy = 0, doy = 0;
	const int BACK_X = SV_MAIN_X, BACK_Y = 36, BACK_W = 110, BACK_H = 40;
	const int TOPMASK = 150;
	const int LX = SV_MAIN_X, LW = 920, CARDH = 88;

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
				if (ui->my > TOPMASK && ui->mx > 260) {
					dragging = 1;
					dsy = ui->my;
					doy = oy;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				dragging = 0;
				break;
			case SDL_MOUSEMOTION:
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				if (dragging) {
					oy = doy + (ui->my - dsy);
				}
				break;
			case SDL_MOUSEWHEEL:
				oy += event.wheel.y * 50;
				break;
			case SDL_KEYDOWN: {
				SDL_Keycode k = event.key.keysym.sym;
				if (k == SDLK_ESCAPE) {
					return;
				} else if (k == SDLK_UP) {
					oy += 60;
				} else if (k == SDLK_DOWN) {
					oy -= 60;
				}
				break;
			}
			}
		}

		if (ui->is_mouse_down && !dragging) {
			if (dfsui_inRect(ui->mx, ui->my, BACK_X, BACK_Y, BACK_W, BACK_H)) {
				ui->is_mouse_down = false;
				return;
			}
			if (dfsui_handleSidebarClick(ui, -1)) {
				ui->is_mouse_down = false;
				return;
			}
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		// 목록을 먼저 그리고 상단/사이드바로 영역 밖을 가린다.
		int y = TOPMASK + 16 + oy;
		for (int t = 0; t < s->n_trees; t++) {
			DfsTree *tr = &s->trees[t];
			dfsui_drawText(ui, tr->title, ui->font_normal, COLOR_SUPERPINK, LX,
						   y, TOPLEFT, 0);
			y += 44;
			for (int i = 0; i < tr->n_nodes; i++) {
				DfsTreeNode *n = &tr->nodes[i];
				if (!n->is_leaf) {
					continue;
				}
				int mine =
					codes && codes[t][0] && strcmp(n->code, codes[t]) == 0;
				dfsui_drawRound(ui, LX, y, LW, CARDH, 16,
								mine ? COLOR_SUPERPINK : COLOR_SOFTPINK);
				dfsui_drawRound(ui, LX + 2, y + 2, LW - 4, CARDH - 4, 14,
								mine ? COLOR_WHITEPINK : COLOR_WHITE);
				dfsui_drawText(ui, n->name, ui->font_normal,
							   mine ? COLOR_SUPERPINK : COLOR_DURTYPINK, LX + 24,
							   y + 12, TOPLEFT, LW - 220);
				if (mine) {
					dfsui_drawText(ui, "★ 내 유형", ui->font_small,
								   COLOR_SUPERPINK, LX + LW - 130, y + 16,
								   TOPLEFT, 0);
				}
				dfsui_drawText(ui, n->desc, ui->font_ssmall, COLOR_GRAY,
							   LX + 24, y + 48, TOPLEFT, LW - 48);
				y += CARDH + 12;
			}
			y += 14;
		}

		dfsui_drawRect(ui, 0, 0, WINDOW_WIDTH, TOPMASK, COLOR_WHITE);
		dfsui_drawSidebar(ui, -1);
		dfsui_drawRound(ui, BACK_X, BACK_Y, BACK_W, BACK_H, 12, COLOR_SOFTPINK);
		dfsui_drawText(ui, "← 뒤로", ui->font_small, COLOR_DURTYPINK,
					   BACK_X + BACK_W / 2, BACK_Y + BACK_H / 2, CENTER, 0);
		dfsui_drawText(ui, "유형 도감", ui->font_big, COLOR_BLACK,
					   BACK_X + BACK_W + 24, BACK_Y + 2, TOPLEFT, 0);
		dfsui_drawText(ui, "스크롤(휠·드래그)로 둘러보세요", ui->font_ssmall,
					   COLOR_GRAY, WINDOW_WIDTH - 260, BACK_Y + 14, TOPLEFT, 0);

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
}

// 세분화 버튼 레이아웃 (각 결과 줄 오른쪽).
#define DFSUI_SUB_X (SV_MAIN_X + 720)
#define DFSUI_SUB_W 180
#define DFSUI_SUB_H 30
#define DFSUI_SELF_Y(i) (212 + (i) * 36)
#define DFSUI_IDEAL_Y(i) (392 + (i) * 36)

// 결과 줄의 코드가 실제 leaf면 세분화 가능. 중간에 멈춘(분기 노드) 결과는 -1.
static int dfsui_canExtend(DfsSurvey *s, char codes[][MAX_TYPE_LEN], int i) {
	return dfs_find_leaf_by_code(&s->trees[i], codes[i]) >= 0;
}

// self 트리 i를 세분화하고, 같은 세분화를 이상형 트리에도 적용한다.
// 성공 시 1. (세분화는 '내 성향'에서만 시작하지만 구조는 양쪽 트리에 반영)
static int dfsui_resultExtend(SDL_Ui *ui, People *me, DfsSurvey *self_s,
							  char codes[][MAX_TYPE_LEN],
							  char names[][DFS_NAME_LEN], DfsSurvey *ideal_s,
							  int i) {
	int li = dfs_find_leaf_by_code(&self_s->trees[i], codes[i]);
	if (li < 0) {
		return 0;
	}
	char parent_code[MAX_TYPE_LEN];
	strncpy(parent_code, codes[i], MAX_TYPE_LEN - 1);
	parent_code[MAX_TYPE_LEN - 1] = '\0';

	char nc[MAX_TYPE_LEN], nn[DFS_NAME_LEN];
	if (!dfsui_showAddQuestion(ui, &self_s->trees[i], li, nc, nn)) {
		return 0;
	}

	// 내 성향 결과/People 갱신
	strncpy(codes[i], nc, MAX_TYPE_LEN - 1);
	codes[i][MAX_TYPE_LEN - 1] = '\0';
	strncpy(names[i], nn, DFS_NAME_LEN - 1);
	names[i][DFS_NAME_LEN - 1] = '\0';
	if (i == 0) {
		people_set_people_type(me, nc);
	} else if (i == 1) {
		people_set_people_attach(me, nc);
	} else if (i == 2) {
		people_set_people_lang(me, nc);
	}

	// 같은 세분화를 이상형 트리에도 적용 (코드/이름 동일하게 노드만 추가)
	if (i < ideal_s->n_trees) {
		DfsTree *it = &ideal_s->trees[i];
		int ili = dfs_find_leaf_by_code(it, parent_code);
		DfsTreeNode *sb = &self_s->trees[i].nodes[li]; // 방금 분기로 바뀜
		if (ili >= 0 && !sb->is_leaf && sb->n_opt >= 2) {
			DfsTreeNode *ch0 = &self_s->trees[i].nodes[sb->child[0]];
			DfsTreeNode *ch1 = &self_s->trees[i].nodes[sb->child[1]];
			if (dfs_extend_leaf_named(it, ili, sb->question, sb->opt_text[0],
									  sb->opt_text[1], ch0->name, ch1->name,
									  ch0->desc, ch1->desc)) {
				dfs_save_tree(it, it->save_path);
			}
		}
	}

	login_update_account(me);
	dfs_matching_reload(); // 세분화가 매칭 유사도에 반영되도록 캐시 무효화
	return 1;
}

// 결과 카드 한 줄 + (leaf면) "세분화" 버튼을 그린다.
static void dfsui_drawResultLine(SDL_Ui *ui, const char *title,
								 const char *name, const char *code, int y,
								 SDL_Color text_color, int can_ext) {
	char line[160];
	(void)code; // 유형 코드(영문)는 화면에 표시하지 않음
	snprintf(line, sizeof(line), "· %s : %s", title, name);
	dfsui_drawText(ui, line, ui->font_small, text_color, SV_MAIN_X + 40, y + 4,
				   TOPLEFT, 660);
	if (can_ext) {
		int hov = dfsui_inRect(ui->mx, ui->my, DFSUI_SUB_X, y, DFSUI_SUB_W,
							   DFSUI_SUB_H);
		dfsui_drawRound(ui, DFSUI_SUB_X, y, DFSUI_SUB_W, DFSUI_SUB_H, 12,
						COLOR_DURTYPINK);
		dfsui_drawRound(ui, DFSUI_SUB_X + 2, y + 2, DFSUI_SUB_W - 4,
						DFSUI_SUB_H - 4, 10,
						hov ? COLOR_WHITEPINK : COLOR_WHITE);
		dfsui_drawText(ui, "세분화", ui->font_small, COLOR_DURTYPINK,
					   DFSUI_SUB_X + DFSUI_SUB_W / 2, y + DFSUI_SUB_H / 2,
					   CENTER, 0);
	}
}

static void dfsui_showSurveyResult(SDL_Ui *ui, People *me, DfsSurvey *self_s,
								   char self_codes[][MAX_TYPE_LEN],
								   char self_names[][DFS_NAME_LEN],
								   DfsSurvey *ideal_s,
								   char ideal_codes[][MAX_TYPE_LEN],
								   char ideal_names[][DFS_NAME_LEN]) {
	int bx = SV_MAIN_X, bw = 200, byy = 540;
	int extended = 0; // 세분화는 설문당 1회 (재설문해야 다시 가능)
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
				ui->is_mouse_down = false; // 클릭이 홈 화면으로 새어들지 않게
				return;
			}
			if (dfsui_handleSidebarClick(ui, 2)) {
				ui->is_mouse_down = false;
				return;
			}
			// 트리 시각화 보기
			if (dfsui_inRect(ui->mx, ui->my, SV_MAIN_X + 220, byy, 200, 56)) {
				dfsui_showTreeView(ui, self_s, self_codes, ideal_s,
								   ideal_codes);
				ui->is_mouse_down = false;
				if (ui->next_state != DFS) {
					return; // 트리 화면에서 사이드바로 나갔으면 따른다
				}
				continue;
			}
			// 세분화는 '내 성향' 트리에서만, 설문당 1회.
			int handled = 0;
			for (int i = 0; !extended && i < self_s->n_trees && !handled; i++) {
				if (dfsui_canExtend(self_s, self_codes, i) &&
					dfsui_inRect(ui->mx, ui->my, DFSUI_SUB_X, DFSUI_SELF_Y(i),
								 DFSUI_SUB_W, DFSUI_SUB_H)) {
					if (dfsui_resultExtend(ui, me, self_s, self_codes,
										   self_names, ideal_s, i)) {
						extended = 1;
					}
					handled = 1;
				}
			}
			ui->is_mouse_down = false;
			// 세분화 화면에서 사이드바로 나갔으면 그 이동을 따른다.
			if (handled && ui->next_state != DFS) {
				return;
			}
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);
		dfsui_drawSidebar(ui, 2);

		dfsui_drawText(ui, "진단이 완료됐어요", ui->font_big, COLOR_BLACK,
					   SV_MAIN_X, 36, TOPLEFT, 0);
		dfsui_drawText(ui,
					   "leaf까지 진단된 트리는 '세분화'로 더 나눌 수 있어요. "
					   "매칭 유사도는 성향과 애착을 합쳐 계산됩니다.",
					   ui->font_small, COLOR_GRAY, SV_MAIN_X + 2, 92, TOPLEFT,
					   900);

		dfsui_drawRound(ui, SV_MAIN_X, 150, 940, 160, 20, COLOR_WHITEPINK);
		dfsui_drawText(ui, "내 연애 성향", ui->font_normal, COLOR_SUPERPINK,
					   SV_MAIN_X + 30, 172, TOPLEFT, 0);
		for (int i = 0; i < self_s->n_trees; i++) {
			dfsui_drawResultLine(ui, self_s->trees[i].title, self_names[i],
								 self_codes[i], DFSUI_SELF_Y(i), COLOR_DURTYPINK,
								 !extended &&
									 dfsui_canExtend(self_s, self_codes, i));
		}
		if (extended) {
			dfsui_drawText(
				ui, "세분화는 설문당 한 번이에요. 다시 하려면 재설문하세요.",
				ui->font_small, COLOR_VIOLET, SV_MAIN_X + 2, 500, TOPLEFT, 900);
		}

		dfsui_drawRound(ui, SV_MAIN_X, 330, 940, 160, 20, COLOR_WHITEVIOLET);
		dfsui_drawText(ui, "내 이상형", ui->font_normal, COLOR_VIOLET,
					   SV_MAIN_X + 30, 352, TOPLEFT, 0);
		for (int i = 0; i < ideal_s->n_trees; i++) {
			// 이상형은 세분화 버튼 없이 결과만 표시 (can_ext = 0).
			dfsui_drawResultLine(ui, ideal_s->trees[i].title, ideal_names[i],
								 ideal_codes[i], DFSUI_IDEAL_Y(i),
								 COLOR_SOFTVIOLET, 0);
		}

		int hov = dfsui_inRect(ui->mx, ui->my, bx, byy, bw, 56);
		dfsui_drawRound(ui, bx, byy, bw, 56, 14,
						hov ? COLOR_DURTYPINK : COLOR_SUPERPINK);
		dfsui_drawText(ui, "홈으로 →", ui->font_normal, COLOR_WHITE,
					   bx + bw / 2, byy + 28, CENTER, 0);

		int tv = dfsui_inRect(ui->mx, ui->my, SV_MAIN_X + 220, byy, 200, 56);
		dfsui_drawRound(ui, SV_MAIN_X + 220, byy, 200, 56, 14, COLOR_SOFTVIOLET);
		dfsui_drawRound(ui, SV_MAIN_X + 222, byy + 2, 196, 52, 12,
						tv ? COLOR_WHITEVIOLET : COLOR_WHITE);
		dfsui_drawText(ui, "트리 보기", ui->font_normal, COLOR_VIOLET,
					   SV_MAIN_X + 320, byy + 28, CENTER, 0);

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
	if (self_s->n_trees > 1) {
		people_set_people_attach(me, self_codes[1]); // 애착 차원 결과 저장
	}
	if (self_s->n_trees > 2) {
		people_set_people_lang(me, self_codes[2]); // 사랑의 언어 결과 저장
	}
	login_update_account(me);

	if (!dfsui_runSurvey(ui, ideal_s, ideal_codes, ideal_names)) {
		goto done;
	}
	strncpy(me->love_type, ideal_codes[0], MAX_TYPE_LEN - 1);
	me->love_type[MAX_TYPE_LEN - 1] = '\0';
	if (ideal_s->n_trees > 1) {
		people_set_people_love_attach(me, ideal_codes[1]); // 이상형 애착 저장
	}
	if (ideal_s->n_trees > 2) {
		people_set_people_love_lang(me, ideal_codes[2]); // 이상형 사랑의 언어
	}
	login_update_account(me);

	dfsui_showSurveyResult(ui, me, self_s, self_codes, self_names, ideal_s,
						   ideal_codes, ideal_names);

done:
	free(self_s);
	free(ideal_s);
	if (ui->next_state == DFS) {
		ui->next_state = HOME;
	}
}

// 홈 등에서 바로 호출하는 공개 래퍼: 설문 트리를 빌드하고 사용자 결과 코드를
// 채워 트리 시각화 화면을 띄운다.
void display_showTreeView(SDL_Ui *ui, People *me) {
	DfsSurvey *self_s = malloc(sizeof(DfsSurvey));
	DfsSurvey *ideal_s = malloc(sizeof(DfsSurvey));
	if (self_s == NULL || ideal_s == NULL) {
		free(self_s);
		free(ideal_s);
		return;
	}
	dfs_build_self_survey(self_s);
	dfs_build_ideal_survey(ideal_s);

	char sc[DFS_MAX_TREES][MAX_TYPE_LEN] = {0};
	char ic[DFS_MAX_TREES][MAX_TYPE_LEN] = {0};
	if (me != NULL) {
		strncpy(sc[0], me->type, MAX_TYPE_LEN - 1);
		strncpy(sc[1], me->attach, MAX_TYPE_LEN - 1);
		strncpy(sc[2], me->lang, MAX_TYPE_LEN - 1);
		strncpy(ic[0], me->love_type, MAX_TYPE_LEN - 1);
		strncpy(ic[1], me->love_attach, MAX_TYPE_LEN - 1);
		strncpy(ic[2], me->love_lang, MAX_TYPE_LEN - 1);
	}
	dfsui_showTreeView(ui, self_s, sc, ideal_s, ic);
	free(self_s);
	free(ideal_s);
}

// 유형 도감 화면(공개 래퍼). 내 성향 트리들 + 내 유형 강조.
void display_showCodex(SDL_Ui *ui, People *me) {
	DfsSurvey *s = malloc(sizeof(DfsSurvey));
	if (s == NULL) {
		return;
	}
	dfs_build_self_survey(s);
	char sc[DFS_MAX_TREES][MAX_TYPE_LEN] = {0};
	if (me != NULL) {
		strncpy(sc[0], me->type, MAX_TYPE_LEN - 1);
		strncpy(sc[1], me->attach, MAX_TYPE_LEN - 1);
		strncpy(sc[2], me->lang, MAX_TYPE_LEN - 1);
	}
	dfsui_showCodex(ui, s, sc);
	free(s);
}
