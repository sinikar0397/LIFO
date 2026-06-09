#include "display.h"

// 입력 필드 포커스 상태
enum Focus { FOCUS_NONE, FOCUS_ID, FOCUS_PW };

// 자주 쓰는 색 (팔레트에 없는 값)
#define COLOR_DARKTEXT ((SDL_Color){60, 60, 70, 255})
#define COLOR_PLACEHOLDER ((SDL_Color){176, 156, 166, 255})
#define COLOR_PANEL ((SDL_Color){252, 232, 238, 255})
#define COLOR_FIELDBG ((SDL_Color){250, 244, 247, 255})
#define COLOR_OK ((SDL_Color){76, 175, 105, 255})
#define COLOR_ERR ((SDL_Color){211, 71, 110, 255})

// TEXT 오브젝트 생성 단축 헬퍼
static Object mkText(SDL_Ui *ui, const char *s, TTF_Font *font, SDL_Color color,
					 int x, int y, AnchorEnum anchor) {
	ObjectParam p;
	strncpy(p.text.text, s, sizeof(p.text.text) - 1);
	p.text.text[sizeof(p.text.text) - 1] = '\0';
	p.text.font = font;
	p.text.color = color;
	return gui_initObject(ui, TEXT, x, y, anchor, p);
}

// 둥근 카드/버튼: 테두리 색으로 한 번 채우고 안쪽을 fill 색으로 덮음
static void drawPanel(SDL_Renderer *r, SDL_Rect rect, int radius, SDL_Color fill,
					  SDL_Color border, int border_w) {
	if (border_w > 0) {
		gui_fillRoundedRect(r, rect, radius, border);
		SDL_Rect in = {rect.x + border_w, rect.y + border_w,
					   rect.w - 2 * border_w, rect.h - 2 * border_w};
		gui_fillRoundedRect(r, in, radius - border_w > 1 ? radius - border_w : 1,
							fill);
	} else {
		gui_fillRoundedRect(r, rect, radius, fill);
	}
}

static int pointInRect(SDL_Rect r, int x, int y) {
	return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

People *showLogin(SDL_Ui *ui) {
	People *logged_in = NULL;

	// 입력 버퍼
	char id_buf[MAX_ID_LEN] = "";
	char pw_buf[MAX_PW_LEN] = "";
	enum Focus focus = FOCUS_NONE;

	// 상태 메시지
	char status[128] = "";
	SDL_Color status_color = COLOR_GRAY;

	// 레이아웃 영역
	SDL_Rect panel = {0, 0, 480, WINDOW_HEIGHT};
	SDL_Rect card = {700, 110, 400, 510};
	int cx = card.x + card.w / 2;

	SDL_Rect id_field = {740, 250, 320, 52};
	SDL_Rect pw_field = {740, 318, 320, 52};
	SDL_Rect login_btn = {740, 398, 320, 54};
	SDL_Rect idcheck_btn = {740, 466, 155, 46};
	SDL_Rect signup_btn = {905, 466, 155, 46};
	SDL_Rect exit_btn = {740, 528, 320, 44};

	// ── 정적 텍스트 (한 번만 생성) ──
	Object t_logo =
		mkText(ui, "LIFO", ui->font_bbsig, COLOR_SUPERPINK, 56, 76, TOPLEFT);
	Object t_tagline = mkText(ui, "Love Is Found Optimally", ui->font_normal,
							  COLOR_DURTYPINK, 60, 206, TOPLEFT);
	Object t_sub1 = mkText(ui, "마음이 맞는 사람을", ui->font_big,
						   COLOR_DURTYPINK, 60, 330, TOPLEFT);
	Object t_sub2 = mkText(ui, "최적으로 찾아드려요.", ui->font_big,
						   COLOR_DURTYPINK, 60, 382, TOPLEFT);

	Object t_heading =
		mkText(ui, "로그인", ui->font_big, COLOR_SUPERPINK, cx, 150, MIDTOP);
	Object t_headsub = mkText(ui, "계정으로 계속하기", ui->font_small,
							  COLOR_GRAY, cx, 202, MIDTOP);

	Object t_login = mkText(ui, "로그인", ui->font_normal, COLOR_WHITE, cx,
							login_btn.y + login_btn.h / 2, CENTER);
	Object t_idcheck = mkText(ui, "아이디 확인", ui->font_small, COLOR_DURTYPINK,
							  idcheck_btn.x + idcheck_btn.w / 2,
							  idcheck_btn.y + idcheck_btn.h / 2, CENTER);
	Object t_signup = mkText(ui, "회원가입", ui->font_small, COLOR_DURTYPINK,
							 signup_btn.x + signup_btn.w / 2,
							 signup_btn.y + signup_btn.h / 2, CENTER);
	Object t_exit = mkText(ui, "종료", ui->font_small, COLOR_GRAY, cx,
						   exit_btn.y + exit_btn.h / 2, CENTER);

	// ── 동적 텍스트 (매 프레임 갱신) ──
	Object t_idval = mkText(ui, " ", ui->font_normal, COLOR_DARKTEXT,
							id_field.x + 18, id_field.y + 13, TOPLEFT);
	Object t_pwval = mkText(ui, " ", ui->font_normal, COLOR_DARKTEXT,
							pw_field.x + 18, pw_field.y + 13, TOPLEFT);
	Object t_status =
		mkText(ui, " ", ui->font_small, COLOR_GRAY, cx, 590, MIDTOP);

	SDL_StartTextInput();

	while (!ui->quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				ui->quit = true;
				break;
			case SDL_MOUSEMOTION:
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				break;
			case SDL_MOUSEBUTTONDOWN:
				ui->mx = event.button.x;
				ui->my = event.button.y;
				ui->is_mouse_down = true;
				break;
			case SDL_TEXTINPUT: {
				char *buf = (focus == FOCUS_ID)   ? id_buf
							: (focus == FOCUS_PW) ? pw_buf
												  : NULL;
				int cap = (focus == FOCUS_ID) ? MAX_ID_LEN : MAX_PW_LEN;
				if (buf) {
					if ((int)strlen(buf) + (int)strlen(event.text.text) <
						cap - 1) {
						strcat(buf, event.text.text);
					}
				}
				break;
			}
			case SDL_KEYDOWN: {
				char *buf = (focus == FOCUS_ID)   ? id_buf
							: (focus == FOCUS_PW) ? pw_buf
												  : NULL;
				if (event.key.keysym.sym == SDLK_BACKSPACE && buf) {
					gui_utf8Backspace(buf);
				} else if (event.key.keysym.sym == SDLK_TAB) {
					focus = (focus == FOCUS_ID) ? FOCUS_PW : FOCUS_ID;
				} else if (event.key.keysym.sym == SDLK_RETURN) {
					ui->is_mouse_down = true; // 엔터 = 로그인 클릭 취급
					ui->mx = login_btn.x + 1;
					ui->my = login_btn.y + 1;
				}
				break;
			}
			}
		}

		// ── 클릭 처리 ──
		if (ui->is_mouse_down) {
			int mx = ui->mx, my = ui->my;
			if (pointInRect(id_field, mx, my))
				focus = FOCUS_ID;
			else if (pointInRect(pw_field, mx, my))
				focus = FOCUS_PW;
			else if (pointInRect(login_btn, mx, my)) {
				focus = FOCUS_NONE;
				if (strlen(id_buf) == 0 || strlen(pw_buf) == 0) {
					strcpy(status, "아이디와 비밀번호를 입력해주세요.");
					status_color = COLOR_ERR;
				} else if (!login_does_ID_exist(id_buf)) {
					strcpy(status, "존재하지 않는 아이디입니다.");
					status_color = COLOR_ERR;
				} else {
					People *acc = login_get_account(id_buf);
					Password h = people_hash_password(pw_buf);
					if (acc && people_is_same_password(h, acc->pw)) {
						snprintf(status, sizeof(status),
								 "환영합니다, %s님!", acc->name);
						status_color = COLOR_OK;
						if (logged_in)
							people_delete_people(logged_in);
						logged_in = acc;
					} else {
						strcpy(status, "비밀번호가 일치하지 않습니다.");
						status_color = COLOR_ERR;
						if (acc)
							people_delete_people(acc);
					}
				}
			} else if (pointInRect(idcheck_btn, mx, my)) {
				focus = FOCUS_NONE;
				if (strlen(id_buf) == 0) {
					strcpy(status, "확인할 아이디를 입력해주세요.");
					status_color = COLOR_ERR;
				} else if (login_does_ID_exist(id_buf)) {
					strcpy(status, "이미 사용 중인 아이디입니다.");
					status_color = COLOR_ERR;
				} else {
					strcpy(status, "사용 가능한 아이디입니다.");
					status_color = COLOR_OK;
				}
			} else if (pointInRect(signup_btn, mx, my)) {
				focus = FOCUS_NONE;
				strcpy(status, "회원가입 화면은 준비 중입니다.");
				status_color = COLOR_GRAY;
			} else if (pointInRect(exit_btn, mx, my)) {
				ui->quit = true;
			} else {
				focus = FOCUS_NONE;
			}
			ui->is_mouse_down = false;
		}

		int mx = ui->mx, my = ui->my;

		// ── 렌더 ──
		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		// 좌측 브랜드 패널
		SDL_SetRenderDrawColor(ui->renderer, COLOR_PANEL.r, COLOR_PANEL.g,
							   COLOR_PANEL.b, COLOR_PANEL.a);
		SDL_RenderFillRect(ui->renderer, &panel);
		gui_presentObject(&t_logo);
		gui_presentObject(&t_tagline);
		gui_presentObject(&t_sub1);
		gui_presentObject(&t_sub2);

		// 로그인 카드 (흰 카드 + 연핑크 테두리)
		drawPanel(ui->renderer, card, 26, COLOR_WHITE, COLOR_SOFTPINK, 2);
		gui_presentObject(&t_heading);
		gui_presentObject(&t_headsub);

		// ID 입력 필드
		drawPanel(ui->renderer, id_field, 12, COLOR_FIELDBG,
				  focus == FOCUS_ID ? COLOR_SUPERPINK : COLOR_PINK, 2);
		// PW 입력 필드
		drawPanel(ui->renderer, pw_field, 12, COLOR_FIELDBG,
				  focus == FOCUS_PW ? COLOR_SUPERPINK : COLOR_PINK, 2);

		// 로그인 버튼 (primary, hover 시 진한 핑크)
		drawPanel(ui->renderer, login_btn, 14,
				  pointInRect(login_btn, mx, my) ? COLOR_DURTYPINK
												 : COLOR_SUPERPINK,
				  COLOR_WHITE, 0);
		gui_presentObject(&t_login);

		// 아이디 확인 버튼 (secondary)
		drawPanel(ui->renderer, idcheck_btn, 12,
				  pointInRect(idcheck_btn, mx, my) ? COLOR_SOFTPINK
												   : COLOR_WHITEPINK,
				  COLOR_PINK, 2);
		gui_presentObject(&t_idcheck);

		// 회원가입 버튼 (secondary)
		drawPanel(ui->renderer, signup_btn, 12,
				  pointInRect(signup_btn, mx, my) ? COLOR_SOFTPINK
												  : COLOR_WHITEPINK,
				  COLOR_PINK, 2);
		gui_presentObject(&t_signup);

		// 종료 버튼 (subtle)
		drawPanel(ui->renderer, exit_btn, 12,
				  pointInRect(exit_btn, mx, my) ? COLOR_SOFTPINK : COLOR_WHITE,
				  COLOR_SOFTPINK, 2);
		gui_presentObject(&t_exit);

		// ID 텍스트/플레이스홀더
		{
			char disp[64];
			SDL_Color col;
			if (strlen(id_buf) == 0 && focus != FOCUS_ID) {
				strcpy(disp, "아이디");
				col = COLOR_PLACEHOLDER;
			} else {
				snprintf(disp, sizeof(disp), "%s%s", id_buf,
						 focus == FOCUS_ID ? "|" : "");
				col = COLOR_DARKTEXT;
			}
			if (disp[0] == '\0')
				strcpy(disp, " ");
			gui_setColorText(&t_idval, col);
			gui_setText(&t_idval, disp);
			gui_presentObject(&t_idval);
		}

		// PW 텍스트 (마스킹)
		{
			char disp[128] = "";
			SDL_Color col;
			if (strlen(pw_buf) == 0 && focus != FOCUS_PW) {
				strcpy(disp, "비밀번호");
				col = COLOR_PLACEHOLDER;
			} else {
				for (int i = 0; i < (int)strlen(pw_buf); i++)
					strcat(disp, "●");
				if (focus == FOCUS_PW)
					strcat(disp, "|");
				col = COLOR_DARKTEXT;
			}
			if (disp[0] == '\0')
				strcpy(disp, " ");
			gui_setColorText(&t_pwval, col);
			gui_setText(&t_pwval, disp);
			gui_presentObject(&t_pwval);
		}

		// 상태 메시지
		{
			gui_setColorText(&t_status, status_color);
			gui_setText(&t_status, status[0] ? status : " ");
			gui_presentObject(&t_status);
		}

		SDL_RenderPresent(ui->renderer);

		ui->is_mouse_up = false;
		ui->is_mouse_move = false;

		// 셀프 스크린샷 모드: 한 프레임만 찍고 종료
		if (getenv("LIFO_SHOT")) {
			SDL_Surface *ss = SDL_CreateRGBSurface(
				0, WINDOW_WIDTH, WINDOW_HEIGHT, 32, 0x00FF0000, 0x0000FF00,
				0x000000FF, 0xFF000000);
			if (ss) {
				SDL_RenderReadPixels(ui->renderer, NULL,
									 SDL_PIXELFORMAT_ARGB8888, ss->pixels,
									 ss->pitch);
				IMG_SavePNG(ss, "image/shot.png");
				SDL_FreeSurface(ss);
			}
			ui->quit = true;
		}
	}

	SDL_StopTextInput();
	return logged_in;
}
