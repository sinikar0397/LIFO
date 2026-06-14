#include "display.h"

#define MST_MAIN_X 300

static void mstui_copyText(char *dst, size_t dst_size, const char *src) {
	if (dst_size == 0) {
		return;
	}
	if (src == NULL) {
		dst[0] = '\0';
		return;
	}
	strncpy(dst, src, dst_size - 1);
	dst[dst_size - 1] = '\0';
}

static int mstui_inRect(int px, int py, int x, int y, int w, int h) {
	return px >= x && px <= x + w && py >= y && py <= y + h;
}

static void mstui_drawRect(SDL_Ui *ui, int x, int y, int w, int h, SDL_Color c) {
	SDL_Rect r = {x, y, w, h};
	SDL_SetRenderDrawColor(ui->renderer, c.r, c.g, c.b, c.a);
	SDL_RenderFillRect(ui->renderer, &r);
}

static void mstui_drawRound(SDL_Ui *ui, int x, int y, int w, int h,
							int radius, SDL_Color c) {
	Object o = gui_initObject(ui, BOX, x, y, TOPLEFT, (ObjectParam){.box = {w, h, c, radius}});
	gui_presentObject(&o);
}

static void mstui_drawText(SDL_Ui *ui, const char *text, TTF_Font *font, SDL_Color color, int x, int y, AnchorEnum anchor, int wrap) {
	if (text == NULL || text[0] == '\0') {
		return;
	}
	SDL_Surface *s =
		(wrap > 0) ? TTF_RenderUTF8_Blended_Wrapped(font, text, color, wrap) : TTF_RenderUTF8_Blended(font, text, color);
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

static void mstui_drawSidebar(SDL_Ui *ui, int active) {
	const char *labels[3] = {"홈", "캘린더", "버킷리스트"};
	int ny[3] = {150, 212, 274};
	mstui_drawRect(ui, 0, 0, 260, WINDOW_HEIGHT, COLOR_WHITEPINK);
	mstui_drawText(ui, "LIFO", ui->font_bbsig, COLOR_SUPERPINK, 110, 44, MIDTOP, 0);
	for (int i = 0; i < 3; i++) {
		int hover = mstui_inRect(ui->mx, ui->my, 30, ny[i], 200, 52);
		if (i == active || hover) {
			mstui_drawRound(ui, 30, ny[i], 200, 52, 14, COLOR_PINK);
		}
		SDL_Color tc = (i == active || hover) ? COLOR_WHITE : COLOR_DURTYPINK;
		mstui_drawText(ui, labels[i], ui->font_normal, tc, 70, ny[i] + 13, TOPLEFT, 0);
	}

	int logout_y = WINDOW_HEIGHT - 82;
	int logout_hover = mstui_inRect(ui->mx, ui->my, 30, logout_y, 200, 52);
	if (logout_hover) {
		mstui_drawRound(ui, 30, logout_y, 200, 52, 14, COLOR_PINK);
	}
	mstui_drawText(ui, "로그아웃", ui->font_normal, logout_hover ? COLOR_WHITE : COLOR_DURTYPINK, 70, logout_y + 13, TOPLEFT, 0);
}

static void mstui_drawInput(SDL_Ui *ui, int x, int y, int w, int h, int focused, const char *label, const char *buf, const char *placeholder) {
	mstui_drawText(ui, label, ui->font_small, COLOR_DURTYPINK, x, y - 28, TOPLEFT, 0);
	mstui_drawRound(ui, x, y, w, h, 12, focused ? COLOR_SUPERPINK : COLOR_PINK);
	mstui_drawRound(ui, x + 2, y + 2, w - 4, h - 4, 10, COLOR_WHITEPINK);
	if (buf[0] == '\0' && !focused) {
		mstui_drawText(ui, placeholder, ui->font_small, COLOR_WHITEGRAY, x + 16, y + 16, TOPLEFT, w - 32);
	}
	else {
		char disp[DFS_Q_LEN + 4];
		snprintf(disp, sizeof(disp), "%s%s", buf, focused ? "_" : "");
		mstui_drawText(ui, disp, ui->font_small, COLOR_GRAY, x + 16, y + 16, TOPLEFT, w - 32);
	}
}

static void mstui_appendText(char *buf, size_t buf_size, const char *text) {
	if (buf == NULL || text == NULL) {
		return;
	}
	if (strlen(buf) + strlen(text) < buf_size - 1) {
		strcat(buf, text);
	}
}

static void mstui_openMstSpace(MstCoupleSpace *space, People *me, const char partner_buf[], int *opened, char status[], size_t status_size) {
	if (partner_buf[0] == '\0') {
		snprintf(status, status_size, "수락된 매칭이 있어야 사용할 수 있어요.");
		return;
	}

	int existed = mst_load_space(space, me->id, partner_buf);
	if (!existed) {
		mst_init_space(space, me->id, partner_buf);
		mst_save_space(space);
	}
	mstui_copyText(g_mst_partner_id, sizeof(g_mst_partner_id), partner_buf);
	*opened = 1;
	snprintf(status, status_size, existed ? "커플 공간을 불러왔어요." : "새 커플 공간을 만들었어요.");
}

void display_showMST(SDL_Ui *ui, People *me) {
	if (me == NULL) {
		ui->next_state = HOME;
		return;
	}

	enum {
		MST_FOCUS_NONE,
		MST_FOCUS_EVENT_DATE,
		MST_FOCUS_EVENT_TITLE,
		MST_FOCUS_BUCKET
	};
	enum { MST_VIEW_HOME, MST_VIEW_CALENDAR, MST_VIEW_BUCKET };

	MstCoupleSpace space;
	char partner_buf[MAX_ID_LEN] = "";
	char partner_name[MAX_NAME_LEN] = "";
	char event_date[MST_DATE_LEN] = "";
	char event_title[MST_TEXT_LEN] = "";
	char bucket_title[MST_TEXT_LEN] = "";
	char status[160] = "";
	int focus = MST_FOCUS_NONE;
	int view = MST_VIEW_HOME;
	int opened = 0;
	int breakup =0;

	mst_init_space(&space, me->id, "");
	if (g_mst_partner_id[0] != '\0' &&
		match_store_has_accepted_match(me->id, g_mst_partner_id)) {
		mstui_copyText(partner_buf, sizeof(partner_buf), g_mst_partner_id);
	}
	else if (match_store_find_accepted_partner(me->id, partner_buf, sizeof(partner_buf))) {
		mstui_copyText(g_mst_partner_id, sizeof(g_mst_partner_id), partner_buf);
	}
	else if (me->status == MATCHED && me->lover[0] != '\0') {
		mstui_copyText(partner_buf, sizeof(partner_buf), me->lover);
		mstui_copyText(g_mst_partner_id, sizeof(g_mst_partner_id), partner_buf);
	}
	else {
		g_mst_partner_id[0] = '\0';
	}

	if (partner_buf[0] != '\0') {
		mstui_openMstSpace(&space, me, partner_buf, &opened, status, sizeof(status));
		People *partner = login_get_account(partner_buf);
		if (partner != NULL) {
			mstui_copyText(partner_name, sizeof(partner_name), partner->name);
			people_delete_people(partner);
		}
		if (partner_name[0] == '\0') {
			mstui_copyText(partner_name, sizeof(partner_name), partner_buf);
		}
	}
	else {
		strcpy(status, "아직 수락된 매칭이 없어요. 매칭 화면에서 제안을 수락해주세요.");
		focus = MST_FOCUS_NONE;
	}

	const int event_x = MST_MAIN_X;
	const int bucket_x = MST_MAIN_X;
	const int panel_y = 248;
	const int panel_w = 900;
	const int panel_h = 390;
	const int input_y = 332;
	const int add_y = 408;
	const int list_y = 490;
	const int home_card_y = 360;
	const int home_card_w = 438;
	const int home_card_h = 150;
	const int home_gap = 24;

	while (!ui->quit) {
		if (ui->next_state != MST) {
			break;
		}

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
				if (view == MST_VIEW_CALENDAR &&
					focus == MST_FOCUS_EVENT_DATE) {
					mstui_appendText(event_date, sizeof(event_date), event.text.text);
				} 
				else if (view == MST_VIEW_CALENDAR && focus == MST_FOCUS_EVENT_TITLE) {
					mstui_appendText(event_title, sizeof(event_title), event.text.text);
				} 
				else if (view == MST_VIEW_BUCKET && focus == MST_FOCUS_BUCKET) {
					mstui_appendText(bucket_title, sizeof(bucket_title), event.text.text);
				}
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_BACKSPACE) {
					if (focus == MST_FOCUS_EVENT_DATE) {
						gui_utf8Backspace(event_date);
					} 
					else if (focus == MST_FOCUS_EVENT_TITLE) {
						gui_utf8Backspace(event_title);
					} 
					else if (focus == MST_FOCUS_BUCKET) {
						gui_utf8Backspace(bucket_title);
					}
				} 
				else if (event.key.keysym.sym == SDLK_TAB) {
					if (view == MST_VIEW_CALENDAR) {
						focus = (focus == MST_FOCUS_EVENT_DATE)
									? MST_FOCUS_EVENT_TITLE
									: MST_FOCUS_EVENT_DATE;
					} 
					else if (view == MST_VIEW_BUCKET) {
						focus = MST_FOCUS_BUCKET;
					} 
					else {
						focus = MST_FOCUS_NONE;
					}
				} 
				else if (event.key.keysym.sym == SDLK_RETURN) {
					if (view == MST_VIEW_CALENDAR &&
						(focus == MST_FOCUS_EVENT_DATE || focus == MST_FOCUS_EVENT_TITLE)) {
						if (!opened) {
							strcpy(status, "수락된 매칭이 있어야 사용할 수 있어요.");
						} 
						else if (mst_add_event(&space, event_date, event_title)) {
							mst_save_space(&space);
							event_date[0] = '\0';
							event_title[0] = '\0';
							strcpy(status, "일정을 추가했어요.");
						} 
						else {
							strcpy(status, "날짜와 일정 제목을 모두 입력해주세요.");
						}
					} 
					else if (view == MST_VIEW_BUCKET && focus == MST_FOCUS_BUCKET) {
						if (!opened) {
							strcpy(status, "수락된 매칭이 있어야 사용할 수 있어요.");
						} 
						else if (mst_add_bucket_item(&space, bucket_title)) {
							mst_save_space(&space);
							bucket_title[0] = '\0';
							strcpy(status, "버킷리스트를 추가했어요.");
						} 
						else {
							strcpy(status, "버킷리스트 내용을 입력해주세요.");
						}
					}
				} 
				else if (event.key.keysym.sym == SDLK_ESCAPE) {
					ui->next_state = HOME;
				}
				break;
			}
		}

		if (ui->is_mouse_down) {
			if (mstui_inRect(ui->mx, ui->my, 30, 150, 200, 52)) {
				view = MST_VIEW_HOME;
				focus = MST_FOCUS_NONE;
			} 
			else if (mstui_inRect(ui->mx, ui->my, 30, 212, 200, 52)) {
				view = MST_VIEW_CALENDAR;
				focus = MST_FOCUS_EVENT_DATE;
			} 
			else if (mstui_inRect(ui->mx, ui->my, 30, 274, 200, 52)) {
				view = MST_VIEW_BUCKET;
				focus = MST_FOCUS_BUCKET;
			} 
			else if (mstui_inRect(ui->mx, ui->my, 30, WINDOW_HEIGHT - 82, 200, 52)) {
				ui->next_state = LOGIN;
				break;
			} else if (view == MST_VIEW_HOME && breakup && mstui_inRect(ui->mx, ui->my, MST_MAIN_X + 600, 258, 132, 42)) {
				breakup = 0;
				strcpy(status, "잘 선택했어요");
			}else if (view == MST_VIEW_HOME && mstui_inRect(ui->mx, ui->my, MST_MAIN_X + 740, 258, 132, 42)) {
				if (!opened) {
					strcpy(status, "파기할 관계가 없어요.");
				} else if (!breakup) {
					breakup = 1;
					strcpy(status, "진짜로 헤어지기?");
				} else {
					match_store_cancel_match(me->id, partner_buf);
					people_set_people_status(me, AVAILABLE);
					me->lover[0] = '\0';
					login_add_people_to_hashtable(me);
					People *partner = login_get_account(partner_buf);
					if (partner != NULL) {
						people_set_people_status(partner, AVAILABLE);
						partner->lover[0] = '\0';
						login_add_people_to_hashtable(partner);
						people_delete_people(partner);
					}
					g_mst_partner_id[0] = '\0';
					partner_buf[0] = '\0';
					partner_name[0] = '\0';
					opened = 0;
					breakup = 0;
					ui->next_state = HOME;
					break;
				}
			}
			else if (view == MST_VIEW_HOME &&  mstui_inRect(ui->mx, ui->my, MST_MAIN_X, home_card_y,
									home_card_w, home_card_h)) {
				view = MST_VIEW_CALENDAR;
				focus = MST_FOCUS_EVENT_DATE;
			} 
			else if (view == MST_VIEW_HOME && mstui_inRect(ui->mx, ui->my,
									MST_MAIN_X + home_card_w + home_gap,
									home_card_y, home_card_w, home_card_h)) {
				view = MST_VIEW_BUCKET;
				focus = MST_FOCUS_BUCKET;
			} 
			else if (view == MST_VIEW_CALENDAR && mstui_inRect(ui->mx, ui->my, event_x + 28, input_y, 130, 52)) {
				focus = MST_FOCUS_EVENT_DATE;
			} 
			else if (view == MST_VIEW_CALENDAR && mstui_inRect(ui->mx, ui->my, event_x + 172, input_y,
									220, 52)) {
				focus = MST_FOCUS_EVENT_TITLE;
			} 
			else if (view == MST_VIEW_CALENDAR && mstui_inRect(ui->mx, ui->my, event_x + 28, add_y, 170,
									52)) {
				if (!opened) {
					strcpy(status, "수락된 매칭이 있어야 사용할 수 있어요.");
				} 
				else if (mst_add_event(&space, event_date, event_title)) {
					mst_save_space(&space);
					event_date[0] = '\0';
					event_title[0] = '\0';
					strcpy(status, "일정을 추가했어요.");
				} 
				else {
					strcpy(status, "날짜와 일정 제목을 모두 입력해주세요.");
				}
			}
			else if (view == MST_VIEW_CALENDAR) {
				for (int i = 0; i < space.event_count && i < 7; i++) {
					if (mstui_inRect(ui->mx, ui->my, event_x + panel_w - 104, list_y + i * 34 - 4, 72, 28)) {
						if (mst_delete_event(&space, i)) {
							mst_save_space(&space);
							strcpy(status, "일정을 삭제했어요.");
						}
						break;
					}
				}
			} 
			else if (view == MST_VIEW_BUCKET && mstui_inRect(ui->mx, ui->my, bucket_x + 28, input_y,260, 52)) {
				focus = MST_FOCUS_BUCKET;
			} 
			else if (view == MST_VIEW_BUCKET && mstui_inRect(ui->mx, ui->my, bucket_x + 28, add_y, 170,52)) {
				if (!opened) {
					strcpy(status, "수락된 매칭이 있어야 사용할 수 있어요.");
				} 
				else if (mst_add_bucket_item(&space, bucket_title)) {
					mst_save_space(&space);
					bucket_title[0] = '\0';
					strcpy(status, "버킷리스트를 추가했어요.");
				} 
				else {
					strcpy(status, "버킷리스트 내용을 입력해주세요.");
				}
			} 
			else if (view == MST_VIEW_BUCKET) {
				for (int i = 0; i < space.bucket_count && i < 7; i++) {
					if (mstui_inRect(ui->mx, ui->my, bucket_x + panel_w - 104, list_y + i * 34 - 4, 72, 28)) {
						if (mst_delete_bucket_item(&space, i)) {
							mst_save_space(&space);
							strcpy(status, "버킷리스트를 삭제했어요.");
						}
						break;
					}
					if (mstui_inRect(ui->mx, ui->my, bucket_x + 28, list_y + i * 34, 380, 28)) {
						mst_toggle_bucket_item(&space, i);
						mst_save_space(&space);
						strcpy(status, "버킷리스트 상태를 바꿨어요.");
						break;
					}
				}
			}
		}

		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);
		mstui_drawSidebar(ui, view);

		if (view == MST_VIEW_HOME) {
			char couple_line[160];
			char id_line[160];
			char count_line[96];
			int bucket_left = 0;
			for (int i = 0; i < space.bucket_count; i++) {
				if (!space.buckets[i].done) {
					bucket_left++;
				}
			}
			snprintf(couple_line, sizeof(couple_line), "%s  ♥  %s", me->name, opened ? partner_name : "상대 없음");
			snprintf(id_line, sizeof(id_line), "%s  ♥  %s", me->id, opened ? partner_buf : "-");

			mstui_drawText(ui, "커플 홈", ui->font_big, COLOR_BLACK,MST_MAIN_X, 36, TOPLEFT, 0);
			mstui_drawText(ui, "둘만의 일정과 버킷리스트를 한눈에 봐요.",  ui->font_small, COLOR_GRAY, MST_MAIN_X + 2, 92, TOPLEFT, 0);

			mstui_drawRound(ui, MST_MAIN_X, 132, 900, 190, 24,COLOR_WHITEPINK);
			mstui_drawText(ui, couple_line, ui->font_big, COLOR_DURTYPINK, MST_MAIN_X + 450, 154, CENTER, 0);
			mstui_drawText(ui, id_line, ui->font_small, COLOR_GRAY, MST_MAIN_X + 450, 180, CENTER, 0);
			mstui_drawText(ui, "D+1", ui->font_normal, COLOR_SUPERPINK, MST_MAIN_X+450, 230, CENTER, 0);
			if (breakup) {
				mstui_drawRound(ui, MST_MAIN_X + 600, 258, 132, 42, 12, COLOR_SOFTPINK);
				mstui_drawText(ui, "아니요", ui->font_small, COLOR_SUPERPINK, MST_MAIN_X + 666, 279, CENTER, 0);
				mstui_drawRound(ui, MST_MAIN_X + 740, 258, 132, 42, 12, COLOR_SUPERPINK);
				mstui_drawText(ui, "진짜", ui->font_small, COLOR_WHITE,
							MST_MAIN_X + 806, 279, CENTER, 0);
			} else {
				mstui_drawRound(ui, MST_MAIN_X + 740, 258, 132, 42, 12, COLOR_SOFTPINK);
				mstui_drawText(ui, "헤어지기", ui->font_small, COLOR_SUPERPINK, MST_MAIN_X + 806, 279, CENTER, 0);
			}
			mstui_drawText(ui, status, ui->font_small, COLOR_SUPERPINK, MST_MAIN_X + 28, 292, TOPLEFT, 0);

			int cal_hover =mstui_inRect(ui->mx, ui->my, MST_MAIN_X, home_card_y, home_card_w, home_card_h);
			mstui_drawRound(ui, MST_MAIN_X, home_card_y, home_card_w,home_card_h, 20, cal_hover ? COLOR_SOFTPINK : COLOR_WHITEPINK);
			mstui_drawText(ui, "캘린더", ui->font_normal, COLOR_SUPERPINK,MST_MAIN_X + 28, home_card_y + 26, TOPLEFT, 0);
			snprintf(count_line, sizeof(count_line), "등록된 일정 %d개", space.event_count);
			mstui_drawText(ui, count_line, ui->font_small, COLOR_GRAY,  MST_MAIN_X + 28, home_card_y + 72, TOPLEFT, 0);
			mstui_drawText(ui, "다가오는 약속을 정리해요", ui->font_small, COLOR_DURTYPINK, MST_MAIN_X + 28, home_card_y + 106, TOPLEFT, 0);

			int bucket_card_x = MST_MAIN_X + home_card_w + home_gap;
			int bucket_card_hover =mstui_inRect(ui->mx, ui->my, bucket_card_x, home_card_y, home_card_w, home_card_h);
			mstui_drawRound(ui, bucket_card_x, home_card_y, home_card_w,home_card_h, 20,bucket_card_hover ? COLOR_WHITEVIOLET  : COLOR_WHITEPINK);
			mstui_drawText(ui, "버킷리스트", ui->font_normal, COLOR_VIOLET, bucket_card_x + 28, home_card_y + 26, TOPLEFT, 0);
			snprintf(count_line, sizeof(count_line), "남은 일 %d개", bucket_left);
			mstui_drawText(ui, count_line, ui->font_small, COLOR_GRAY, bucket_card_x + 28, home_card_y + 72, TOPLEFT, 0);
			mstui_drawText(ui, "하고 싶은 일을 모아둬요", ui->font_small, COLOR_SOFTVIOLET, bucket_card_x + 28, home_card_y + 106, TOPLEFT, 0);
		} 
		else if (view == MST_VIEW_CALENDAR) {
			mstui_drawText(ui, "공유 캘린더", ui->font_big, COLOR_BLACK, MST_MAIN_X, 36, TOPLEFT, 0);
			mstui_drawText(ui, "함께 기억해야할 일정을 추가해요.", ui->font_small, COLOR_GRAY, MST_MAIN_X + 2, 92, TOPLEFT, 0);
			mstui_drawText(ui, status, ui->font_small, COLOR_SUPERPINK,MST_MAIN_X, 142, TOPLEFT, 0);

			mstui_drawRound(ui, event_x, panel_y, panel_w, panel_h, 20,COLOR_WHITEPINK);
			mstui_drawText(ui, "새 일정", ui->font_normal, COLOR_SUPERPINK, event_x + 28, panel_y + 24, TOPLEFT, 0);
			mstui_drawInput(ui, event_x + 28, input_y, 130, 52,focus == MST_FOCUS_EVENT_DATE, "날짜", event_date,"06/13");
			mstui_drawInput(ui, event_x + 172, input_y, 220, 52,focus == MST_FOCUS_EVENT_TITLE, "일정",event_title, "영화 보기");
			int event_hover =mstui_inRect(ui->mx, ui->my, event_x + 28, add_y, 170, 52);
			mstui_drawRound(ui, event_x + 28, add_y, 170, 52, 14,event_hover ? COLOR_DURTYPINK : COLOR_SUPERPINK);
			mstui_drawText(ui, "일정 추가", ui->font_small, COLOR_WHITE, event_x + 113, add_y + 26, CENTER, 0);

			if (space.event_count == 0) {
				mstui_drawText(ui, "아직 등록된 일정이 없어요.", ui->font_small, COLOR_GRAY, event_x + 28, list_y, TOPLEFT, panel_w - 56);
			}
			for (int i = 0; i < space.event_count && i < 7; i++) {
				char line[160];
				int del_x = event_x + panel_w - 104;
				int del_y = list_y + i * 34 - 4;
				int del_hover =mstui_inRect(ui->mx, ui->my, del_x, del_y, 72, 28);
				snprintf(line, sizeof(line), "%s  %s", space.events[i].date, space.events[i].title);
				mstui_drawText(ui, line, ui->font_small, COLOR_DURTYPINK, event_x + 28, list_y + i * 34, TOPLEFT, panel_w - 150);
				mstui_drawRound(ui, del_x, del_y, 72, 28, 10,del_hover ? COLOR_DURTYPINK : COLOR_SOFTPINK);
				mstui_drawText(ui, "삭제", ui->font_ssmall, del_hover ? COLOR_WHITE : COLOR_DURTYPINK, del_x + 36, del_y + 14, CENTER, 0);
			}
		} 
		else if (view == MST_VIEW_BUCKET) {
			mstui_drawText(ui, "버킷리스트", ui->font_big, COLOR_BLACK, MST_MAIN_X, 36, TOPLEFT, 0);
			mstui_drawText(ui, "해보고 싶은 일들을 하나씩 채워나가요.", ui->font_small, COLOR_GRAY, MST_MAIN_X + 2, 92, TOPLEFT, 0);
			mstui_drawText(ui, status, ui->font_small, COLOR_SUPERPINK, MST_MAIN_X, 142, TOPLEFT, 0);

			mstui_drawRound(ui, bucket_x, panel_y, panel_w, panel_h, 20,COLOR_WHITEVIOLET);
			mstui_drawText(ui, "새 버킷", ui->font_normal, COLOR_VIOLET,  bucket_x + 28, panel_y + 24, TOPLEFT, 0);
			mstui_drawInput(ui, bucket_x + 28, input_y, 260, 52,focus == MST_FOCUS_BUCKET, "하고 싶은 것",bucket_title, "한강 피크닉");
			int bucket_hover =mstui_inRect(ui->mx, ui->my, bucket_x + 28, add_y, 170, 52);
			mstui_drawRound(ui, bucket_x + 28, add_y, 170, 52, 14,bucket_hover ? COLOR_DURTYPINK : COLOR_VIOLET);
			mstui_drawText(ui, "버킷 추가", ui->font_small, COLOR_WHITE, bucket_x + 113, add_y + 26, CENTER, 0);

			if (space.bucket_count == 0) mstui_drawText(ui, "아직 등록된 버킷리스트가 없어요.",  ui->font_small, COLOR_GRAY, bucket_x + 28, list_y, TOPLEFT, panel_w - 56);
			for (int i = 0; i < space.bucket_count && i < 7; i++) {
				char line[160];
				int del_x = bucket_x + panel_w - 104;
				int del_y = list_y + i * 34 - 4;
				int hover = mstui_inRect(ui->mx, ui->my, bucket_x + 28, list_y + i * 34, 560, 28);
				int del_hover = mstui_inRect(ui->mx, ui->my, del_x, del_y, 72, 28);
				snprintf(line, sizeof(line), "%s %s", space.buckets[i].done ? "[x]" : "[ ]", space.buckets[i].title);
				mstui_drawText(ui, line, ui->font_small, hover ? COLOR_SUPERPINK : COLOR_SOFTVIOLET,  bucket_x + 28, list_y + i * 34, TOPLEFT, panel_w - 150);
				mstui_drawRound(ui, del_x, del_y, 72, 28, 10, del_hover ? COLOR_DURTYPINK : COLOR_SOFTPINK);
				mstui_drawText(ui, "삭제", ui->font_ssmall, del_hover ? COLOR_WHITE : COLOR_DURTYPINK, del_x + 36, del_y + 14, CENTER, 0);
			}
		}
		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
}
