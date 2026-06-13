#include "display.h"

char g_mst_partner_id[MAX_ID_LEN] = "";

int display_countChars(const char *pw_buf) {
	int count = 0;
	int i = 0;

	while (pw_buf[i] != '\0') {
		unsigned char c = (unsigned char)pw_buf[i];

		if (c <= 0x7F) {
			// ASCII (영어, 숫자, 특수문자) - 1바이트
			i += 1;
		} else if ((c & 0xF0) == 0xE0) {
			// UTF-8 3바이트 문자 (한글 등)
			i += 3;
		} else if ((c & 0xE0) == 0xC0) {
			// UTF-8 2바이트 문자 (라틴 확장 등)
			i += 2;
		} else if ((c & 0xF8) == 0xF0) {
			// UTF-8 4바이트 문자 (이모지 등)
			i += 4;
		} else {
			// 알 수 없는 바이트 - 1칸 건너뜀
			i += 1;
		}

		count++;
	}

	return count;
}

People *display_showLogin(SDL_Ui *ui) {
	enum Focus { FOCUS_NONE, FOCUS_ID, FOCUS_PW } focus;
	People *logged_in = NULL;
	char id_buf[MAX_ID_LEN] = "";
	char pw_buf[MAX_PW_LEN] = "";
	focus = FOCUS_NONE;
	char status[128] = " ";
	SDL_Color status_color = COLOR_GRAY;
	int cx = 700 + 400 / 2;

	Object panel = gui_initObject(
		ui, BOX, 0, 0, TOPLEFT,
		(ObjectParam){.box = {480, WINDOW_HEIGHT, COLOR_WHITEPINK, 0}});
	Object card_border =
		gui_initObject(ui, BOX, 700, 110, TOPLEFT,
					   (ObjectParam){.box = {400, 530, COLOR_SOFTPINK, 26}});
	Object card_fill =
		gui_initObject(ui, BOX, 702, 112, TOPLEFT,
					   (ObjectParam){.box = {396, 526, COLOR_WHITE, 24}});

	Object id_field_border =
		gui_initObject(ui, BOX, 740, 250, TOPLEFT,
					   (ObjectParam){.box = {320, 52, COLOR_PINK, 12}});
	Object id_field_fill =
		gui_initObject(ui, BOX, 742, 252, TOPLEFT,
					   (ObjectParam){.box = {316, 48, COLOR_WHITEPINK, 10}});

	Object pw_field_border =
		gui_initObject(ui, BOX, 740, 318, TOPLEFT,
					   (ObjectParam){.box = {320, 52, COLOR_PINK, 12}});
	Object pw_field_fill =
		gui_initObject(ui, BOX, 742, 320, TOPLEFT,
					   (ObjectParam){.box = {316, 48, COLOR_WHITEPINK, 10}});

	Object login_btn =
		gui_initObject(ui, BOX, 740, 398, TOPLEFT,
					   (ObjectParam){.box = {320, 54, COLOR_SUPERPINK, 14}});

	Object idcheck_btn_border =
		gui_initObject(ui, BOX, 740, 466, TOPLEFT,
					   (ObjectParam){.box = {155, 46, COLOR_PINK, 12}});
	Object idcheck_btn_fill =
		gui_initObject(ui, BOX, 742, 468, TOPLEFT,
					   (ObjectParam){.box = {151, 42, COLOR_WHITEPINK, 10}});

	Object signup_btn_border =
		gui_initObject(ui, BOX, 905, 466, TOPLEFT,
					   (ObjectParam){.box = {155, 46, COLOR_PINK, 12}});
	Object signup_btn_fill =
		gui_initObject(ui, BOX, 907, 468, TOPLEFT,
					   (ObjectParam){.box = {151, 42, COLOR_WHITEPINK, 10}});

	Object exit_btn_border =
		gui_initObject(ui, BOX, 740, 528, TOPLEFT,
					   (ObjectParam){.box = {320, 44, COLOR_SOFTPINK, 12}});
	Object exit_btn_fill =
		gui_initObject(ui, BOX, 742, 530, TOPLEFT,
					   (ObjectParam){.box = {316, 40, COLOR_WHITE, 10}});

	Object t_logo = gui_initObject(
		ui, TEXT, 56, 76, TOPLEFT,
		(ObjectParam){.text = {"LIFO", ui->font_bbig, COLOR_SUPERPINK}});
	Object t_tagline = gui_initObject(
		ui, TEXT, 60, 206, TOPLEFT,
		(ObjectParam){.text = {"Love Is Found Optimally", ui->font_normal,
							   COLOR_DURTYPINK}});
	Object t_sub1 =
		gui_initObject(ui, TEXT, 60, 330, TOPLEFT,
					   (ObjectParam){.text = {"마음이 맞는 사람을",
											  ui->font_big, COLOR_DURTYPINK}});
	Object t_sub2 =
		gui_initObject(ui, TEXT, 60, 382, TOPLEFT,
					   (ObjectParam){.text = {"최적으로 찾아드려요.",
											  ui->font_big, COLOR_DURTYPINK}});

	Object t_heading = gui_initObject(
		ui, TEXT, cx, 150, MIDTOP,
		(ObjectParam){.text = {"로그인", ui->font_big, COLOR_SUPERPINK}});
	Object t_headsub =
		gui_initObject(ui, TEXT, cx, 202, MIDTOP,
					   (ObjectParam){.text = {"계정으로 계속하기",
											  ui->font_small, COLOR_GRAY}});

	Object t_login = gui_initObject(
		ui, TEXT, cx, 398 + 54 / 2, CENTER,
		(ObjectParam){.text = {"로그인", ui->font_normal, COLOR_WHITE}});
	Object t_idcheck =
		gui_initObject(ui, TEXT, 740 + 155 / 2, 466 + 46 / 2, CENTER,
					   (ObjectParam){.text = {"아이디 확인", ui->font_small,
											  COLOR_DURTYPINK}});
	Object t_signup = gui_initObject(
		ui, TEXT, 905 + 155 / 2, 466 + 46 / 2, CENTER,
		(ObjectParam){.text = {"회원가입", ui->font_small, COLOR_DURTYPINK}});
	Object t_exit = gui_initObject(
		ui, TEXT, cx, 528 + 44 / 2, CENTER,
		(ObjectParam){.text = {"종료", ui->font_small, COLOR_GRAY}});

	Object t_idval = gui_initObject(
		ui, TEXT, 740 + 18, 250 + 13, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_normal, COLOR_WHITEPINK}});
	Object t_pwval = gui_initObject(
		ui, TEXT, 740 + 18, 318 + 13, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_normal, COLOR_WHITEPINK}});
	Object t_status = gui_initObject(
		ui, TEXT, cx, 590, MIDTOP,
		(ObjectParam){.text = {" ", ui->font_small, COLOR_GRAY}});

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
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				break;

			case SDL_TEXTINPUT:
				if (focus == FOCUS_ID) {
					if (strlen(id_buf) + strlen(event.text.text) <
						MAX_ID_LEN - 1) {
						strcat(id_buf, event.text.text);
					}
				} else if (focus == FOCUS_PW) {
					if (strlen(pw_buf) + strlen(event.text.text) <
						MAX_PW_LEN - 1) {
						strcat(pw_buf, event.text.text);
					}
				}
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_BACKSPACE) {
					if (focus == FOCUS_ID) {
						gui_utf8Backspace(id_buf);
					} else if (focus == FOCUS_PW) {
						gui_utf8Backspace(pw_buf);
					}
				}
				if (event.key.keysym.sym == SDLK_TAB) {
					focus = (focus == FOCUS_ID) ? FOCUS_PW : FOCUS_ID;
				}
				if (event.key.keysym.sym == SDLK_RETURN) {
					ui->is_mouse_down = true;
					ui->mx = login_btn.dstrect.x + 1;
					ui->my = login_btn.dstrect.y + 1;
				}
				break;
			}
		}

		if (ui->is_mouse_down) {
			if (gui_isInObject(&id_field_border, ui->mx, ui->my)) {
				focus = FOCUS_ID;
			} else if (gui_isInObject(&pw_field_border, ui->mx, ui->my)) {
				focus = FOCUS_PW;
			} else if (gui_isInObject(&login_btn, ui->mx, ui->my)) {
				focus = FOCUS_NONE;
				if (strlen(id_buf) == 0 || strlen(pw_buf) == 0) {
					strcpy(status, "아이디와 비밀번호를 입력해주세요.");
					status_color = COLOR_SUPERPINK;
				} else if (!login_does_ID_exist(id_buf)) {
					strcpy(status, "존재하지 않는 아이디입니다.");
					status_color = COLOR_SUPERPINK;
				} else {
					People *acc = login_get_account(id_buf);
					Password h = people_hash_password(pw_buf);
					if (acc && people_is_same_password(h, acc->pw)) {
						if (logged_in) {
							people_delete_people(logged_in);
						}
						logged_in = acc;
						ui->next_state = HOME;
					} else {
						strcpy(status, "비밀번호가 일치하지 않습니다.");
						status_color = COLOR_SUPERPINK;
						if (acc) {
							people_delete_people(acc);
						}
					}
				}
			} else if (gui_isInObject(&idcheck_btn_border, ui->mx, ui->my)) {
				focus = FOCUS_NONE;
				if (strlen(id_buf) == 0) {
					strcpy(status, "확인할 아이디를 입력해주세요.");
					status_color = COLOR_SUPERPINK;
				} else if (login_does_ID_exist(id_buf)) {
					strcpy(status, "이미 사용 중인 아이디입니다.");
					status_color = COLOR_SUPERPINK;
				} else {
					strcpy(status, "사용 가능한 아이디입니다.");
					status_color = COLOR_GREEN;
				}
			} else if (gui_isInObject(&signup_btn_border, ui->mx, ui->my)) {
				focus = FOCUS_NONE;
				People *new_acc = display_showSignup(ui);
				ui->is_mouse_down = false;
				ui->is_mouse_up = false;
				ui->is_mouse_move = false;
			} else if (gui_isInObject(&exit_btn_border, ui->mx, ui->my)) {
				ui->quit = true;
			} else {
				focus = FOCUS_NONE;
			}
		}

		id_field_border.textcolor =
			(focus == FOCUS_ID) ? COLOR_SUPERPINK : COLOR_PINK;
		pw_field_border.textcolor =
			(focus == FOCUS_PW) ? COLOR_SUPERPINK : COLOR_PINK;

		login_btn.textcolor = gui_isInObject(&login_btn, ui->mx, ui->my)
								  ? COLOR_DURTYPINK
								  : COLOR_SUPERPINK;
		idcheck_btn_fill.textcolor =
			gui_isInObject(&idcheck_btn_border, ui->mx, ui->my)
				? COLOR_SOFTPINK
				: COLOR_WHITEPINK;
		signup_btn_fill.textcolor =
			gui_isInObject(&signup_btn_border, ui->mx, ui->my)
				? COLOR_SOFTPINK
				: COLOR_WHITEPINK;
		exit_btn_fill.textcolor =
			gui_isInObject(&exit_btn_border, ui->mx, ui->my) ? COLOR_SOFTPINK
															 : COLOR_WHITE;

		char disp_id[64];
		if (strlen(id_buf) == 0 && focus != FOCUS_ID) {
			strcpy(disp_id, "아이디");
			gui_setColorText(&t_idval, COLOR_WHITEGRAY);
		} else {
			snprintf(disp_id, sizeof(disp_id), "%s%s", id_buf,
					 focus == FOCUS_ID ? "_" : "");
			gui_setColorText(&t_idval, COLOR_GRAY);
		}
		if (disp_id[0] == '\0')
			strcpy(disp_id, " ");
		gui_setText(&t_idval, disp_id);

		char disp_pw[128] = "";
		if (strlen(pw_buf) == 0 && focus != FOCUS_PW) {
			strcpy(disp_pw, "비밀번호");
			gui_setColorText(&t_pwval, COLOR_WHITEGRAY);
		} else {
			for (int i = 0; i < display_countChars(pw_buf); i++)
				strcat(disp_pw, "*");
			if (focus == FOCUS_PW) {
				strcat(disp_pw, "_");
			}
			gui_setColorText(&t_pwval, COLOR_GRAY);
		}
		if (disp_pw[0] == '\0')
			strcpy(disp_pw, " ");
		gui_setText(&t_pwval, disp_pw);

		// 상태 텍스트 갱신
		gui_setColorText(&t_status, status_color);
		gui_setText(&t_status, status[0] ? status : " ");

		// ── 렌더링 ──
		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		gui_presentObject(&panel);
		gui_presentObject(&t_logo);
		gui_presentObject(&t_tagline);
		gui_presentObject(&t_sub1);
		gui_presentObject(&t_sub2);

		gui_presentObject(&card_border);
		gui_presentObject(&card_fill);
		gui_presentObject(&t_heading);
		gui_presentObject(&t_headsub);

		gui_presentObject(&id_field_border);
		gui_presentObject(&id_field_fill);
		gui_presentObject(&pw_field_border);
		gui_presentObject(&pw_field_fill);

		gui_presentObject(&login_btn);
		gui_presentObject(&t_login);

		gui_presentObject(&idcheck_btn_border);
		gui_presentObject(&idcheck_btn_fill);
		gui_presentObject(&t_idcheck);

		gui_presentObject(&signup_btn_border);
		gui_presentObject(&signup_btn_fill);
		gui_presentObject(&t_signup);

		gui_presentObject(&exit_btn_border);
		gui_presentObject(&exit_btn_fill);
		gui_presentObject(&t_exit);

		gui_presentObject(&t_idval);
		gui_presentObject(&t_pwval);
		gui_presentObject(&t_status);

		SDL_RenderPresent(ui->renderer);

		// 프레임 끝에서 마우스 상태 초기화
		ui->is_mouse_up = false;
		ui->is_mouse_down = false;
		ui->is_mouse_move = false;

		if (ui->next_state != LOGIN) {
			break;
		}
	}

	return logged_in;
}

People *display_showSignup(SDL_Ui *ui) {
	enum SignupFocus { SF_NONE, SF_ID, SF_PW, SF_NAME, SF_AGE } focus = SF_NONE;

	char id_buf[MAX_ID_LEN] = "";
	char pw_buf[MAX_PW_LEN] = "";
	char name_buf[MAX_NAME_LEN] = "";
	char age_buf[8] = "";
	int gender = -1;

	char status[128] = " ";
	SDL_Color status_color = COLOR_GRAY;
	int id_checked = 0;

	int cx = 700 + 400 / 2;

	// ── 배경 / 카드 ──
	Object panel = gui_initObject(
		ui, BOX, 0, 0, TOPLEFT,
		(ObjectParam){.box = {480, WINDOW_HEIGHT, COLOR_WHITEPINK, 0}});
	Object card_border =
		gui_initObject(ui, BOX, 700, 80, TOPLEFT,
					   (ObjectParam){.box = {400, 590, COLOR_SOFTPINK, 26}});
	Object card_fill =
		gui_initObject(ui, BOX, 702, 82, TOPLEFT,
					   (ObjectParam){.box = {396, 586, COLOR_WHITE, 24}});

	// ── 입력 필드 (border + fill) ──
	// ID
	Object id_border =
		gui_initObject(ui, BOX, 740, 190, TOPLEFT,
					   (ObjectParam){.box = {320, 50, COLOR_PINK, 12}});
	Object id_fill =
		gui_initObject(ui, BOX, 742, 192, TOPLEFT,
					   (ObjectParam){.box = {316, 46, COLOR_WHITEPINK, 10}});
	// PW
	Object pw_border =
		gui_initObject(ui, BOX, 740, 252, TOPLEFT,
					   (ObjectParam){.box = {320, 50, COLOR_PINK, 12}});
	Object pw_fill =
		gui_initObject(ui, BOX, 742, 254, TOPLEFT,
					   (ObjectParam){.box = {316, 46, COLOR_WHITEPINK, 10}});
	// 이름
	Object name_border =
		gui_initObject(ui, BOX, 740, 314, TOPLEFT,
					   (ObjectParam){.box = {320, 50, COLOR_PINK, 12}});
	Object name_fill =
		gui_initObject(ui, BOX, 742, 316, TOPLEFT,
					   (ObjectParam){.box = {316, 46, COLOR_WHITEPINK, 10}});
	// 나이
	Object age_border =
		gui_initObject(ui, BOX, 740, 376, TOPLEFT,
					   (ObjectParam){.box = {150, 50, COLOR_PINK, 12}});
	Object age_fill =
		gui_initObject(ui, BOX, 742, 378, TOPLEFT,
					   (ObjectParam){.box = {146, 46, COLOR_WHITEPINK, 10}});

	// ── 성별 버튼 ──
	Object male_border =
		gui_initObject(ui, BOX, 905, 376, TOPLEFT,
					   (ObjectParam){.box = {70, 50, COLOR_PINK, 12}});
	Object male_fill =
		gui_initObject(ui, BOX, 907, 378, TOPLEFT,
					   (ObjectParam){.box = {66, 46, COLOR_WHITEPINK, 10}});
	Object female_border =
		gui_initObject(ui, BOX, 990, 376, TOPLEFT,
					   (ObjectParam){.box = {70, 50, COLOR_PINK, 12}});
	Object female_fill =
		gui_initObject(ui, BOX, 992, 378, TOPLEFT,
					   (ObjectParam){.box = {66, 46, COLOR_WHITEPINK, 10}});

	// ── 중복확인 버튼 ──
	Object idcheck_border =
		gui_initObject(ui, BOX, 740, 438, TOPLEFT,
					   (ObjectParam){.box = {320, 44, COLOR_PINK, 12}});
	Object idcheck_fill =
		gui_initObject(ui, BOX, 742, 440, TOPLEFT,
					   (ObjectParam){.box = {316, 40, COLOR_WHITEPINK, 10}});

	// ── 완료 버튼 ──
	Object done_btn =
		gui_initObject(ui, BOX, 740, 498, TOPLEFT,
					   (ObjectParam){.box = {320, 52, COLOR_SUPERPINK, 14}});

	// ── 취소 버튼 ──
	Object cancel_border =
		gui_initObject(ui, BOX, 740, 562, TOPLEFT,
					   (ObjectParam){.box = {320, 42, COLOR_SOFTPINK, 12}});
	Object cancel_fill =
		gui_initObject(ui, BOX, 742, 564, TOPLEFT,
					   (ObjectParam){.box = {316, 38, COLOR_WHITE, 10}});

	// ── 왼쪽 패널 텍스트 ──
	Object t_logo = gui_initObject(
		ui, TEXT, 56, 76, TOPLEFT,
		(ObjectParam){.text = {"LIFO", ui->font_bbig, COLOR_SUPERPINK}});
	Object t_tagline = gui_initObject(
		ui, TEXT, 60, 206, TOPLEFT,
		(ObjectParam){.text = {"Love Is Found Optimally", ui->font_normal,
							   COLOR_DURTYPINK}});
	Object t_sub1 =
		gui_initObject(ui, TEXT, 60, 330, TOPLEFT,
					   (ObjectParam){.text = {"마음이 맞는 사람을",
											  ui->font_big, COLOR_DURTYPINK}});
	Object t_sub2 =
		gui_initObject(ui, TEXT, 60, 382, TOPLEFT,
					   (ObjectParam){.text = {"최적으로 찾아드려요.",
											  ui->font_big, COLOR_DURTYPINK}});

	// ── 카드 고정 텍스트 ──
	Object t_heading = gui_initObject(
		ui, TEXT, cx, 110, MIDTOP,
		(ObjectParam){.text = {"회원가입", ui->font_big, COLOR_SUPERPINK}});
	Object t_headsub = gui_initObject(
		ui, TEXT, cx, 160, MIDTOP,
		(ObjectParam){.text = {"새 계정 만들기", ui->font_small, COLOR_GRAY}});

	Object t_done = gui_initObject(
		ui, TEXT, cx, 498 + 52 / 2, CENTER,
		(ObjectParam){.text = {"가입 완료", ui->font_normal, COLOR_WHITE}});
	Object t_cancel = gui_initObject(
		ui, TEXT, cx, 562 + 42 / 2, CENTER,
		(ObjectParam){.text = {"취소", ui->font_small, COLOR_GRAY}});
	Object t_idcheck = gui_initObject(
		ui, TEXT, 740 + 320 / 2, 438 + 44 / 2, CENTER,
		(ObjectParam){.text = {"중복 확인", ui->font_small, COLOR_DURTYPINK}});
	Object t_male = gui_initObject(
		ui, TEXT, 905 + 70 / 2, 376 + 50 / 2, CENTER,
		(ObjectParam){.text = {"남", ui->font_normal, COLOR_DURTYPINK}});
	Object t_female = gui_initObject(
		ui, TEXT, 990 + 70 / 2, 376 + 50 / 2, CENTER,
		(ObjectParam){.text = {"여", ui->font_normal, COLOR_DURTYPINK}});

	// ── 입력값 표시 텍스트 ──
	Object t_idval = gui_initObject(
		ui, TEXT, 740 + 16, 190 + 12, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_normal, COLOR_WHITEGRAY}});
	Object t_pwval = gui_initObject(
		ui, TEXT, 740 + 16, 252 + 12, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_normal, COLOR_WHITEGRAY}});
	Object t_nameval = gui_initObject(
		ui, TEXT, 740 + 16, 314 + 12, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_normal, COLOR_WHITEGRAY}});
	Object t_ageval = gui_initObject(
		ui, TEXT, 740 + 16, 376 + 12, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_normal, COLOR_WHITEGRAY}});
	Object t_status = gui_initObject(
		ui, TEXT, cx, 618, MIDTOP,
		(ObjectParam){.text = {" ", ui->font_small, COLOR_GRAY}});

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
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				break;
			case SDL_TEXTINPUT:
				switch (focus) {
				case SF_ID:
					if (strlen(id_buf) + strlen(event.text.text) <
						MAX_ID_LEN - 1) {
						strcat(id_buf, event.text.text);
						id_checked = 0; // ID 변경 시 중복확인 초기화
					}
					break;
				case SF_PW:
					if (strlen(pw_buf) + strlen(event.text.text) <
						MAX_PW_LEN - 1)
						strcat(pw_buf, event.text.text);
					break;
				case SF_NAME:
					if (strlen(name_buf) + strlen(event.text.text) <
						MAX_NAME_LEN - 1)
						strcat(name_buf, event.text.text);
					break;
				case SF_AGE:
					// 숫자만 허용
					if (strlen(age_buf) < 3 && event.text.text[0] >= '0' &&
						event.text.text[0] <= '9')
						strcat(age_buf, event.text.text);
					break;
				default:
					break;
				}
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_BACKSPACE) {
					switch (focus) {
					case SF_ID:
						gui_utf8Backspace(id_buf);
						id_checked = 0;
						break;
					case SF_PW:
						gui_utf8Backspace(pw_buf);
						break;
					case SF_NAME:
						gui_utf8Backspace(name_buf);
						break;
					case SF_AGE:
						if (strlen(age_buf) > 0)
							age_buf[strlen(age_buf) - 1] = '\0';
						break;
					default:
						break;
					}
				}
				if (event.key.keysym.sym == SDLK_TAB) {
					focus = (focus + 1) % 5; // SF_NONE~SF_AGE 순환
					if (focus == SF_NONE) {
						focus += 1;
					}
				}
				break;
			}
		}

		if (ui->is_mouse_down) {
			if (gui_isInObject(&id_border, ui->mx, ui->my))
				focus = SF_ID;
			else if (gui_isInObject(&pw_border, ui->mx, ui->my))
				focus = SF_PW;
			else if (gui_isInObject(&name_border, ui->mx, ui->my))
				focus = SF_NAME;
			else if (gui_isInObject(&age_border, ui->mx, ui->my))
				focus = SF_AGE;

			// 성별 선택
			else if (gui_isInObject(&male_border, ui->mx, ui->my)) {
				focus = SF_NONE;
				gender = 0;
			} else if (gui_isInObject(&female_border, ui->mx, ui->my)) {
				focus = SF_NONE;
				gender = 1;
			}

			// 중복 확인
			else if (gui_isInObject(&idcheck_border, ui->mx, ui->my)) {
				focus = SF_NONE;
				if (strlen(id_buf) == 0) {
					strcpy(status, "아이디를 입력해주세요.");
					status_color = COLOR_SUPERPINK;
					id_checked = 0;
				} else if (login_does_ID_exist(id_buf)) {
					strcpy(status, "이미 사용 중인 아이디입니다.");
					status_color = COLOR_SUPERPINK;
					id_checked = 0;
				} else {
					strcpy(status, "사용 가능한 아이디입니다.");
					status_color = COLOR_GREEN;
					id_checked = 1;
				}
			}

			// 가입 완료
			else if (gui_isInObject(&done_btn, ui->mx, ui->my)) {
				focus = SF_NONE;
				int age_val = atoi(age_buf);

				if (!id_checked) {
					strcpy(status, "아이디 중복 확인을 해주세요.");
					status_color = COLOR_SUPERPINK;
				} else if (strlen(pw_buf) == 0) {
					strcpy(status, "비밀번호를 입력해주세요.");
					status_color = COLOR_SUPERPINK;
				} else if (strlen(name_buf) == 0) {
					strcpy(status, "이름을 입력해주세요.");
					status_color = COLOR_SUPERPINK;
				} else if (strlen(age_buf) == 0 || age_val <= 0 ||
						   age_val > 150) {
					strcpy(status, "올바른 나이를 입력해주세요.");
					status_color = COLOR_SUPERPINK;
				} else if (gender == -1) {
					strcpy(status, "성별을 선택해주세요.");
					status_color = COLOR_SUPERPINK;
				} else {
					// 모든 항목 검증 통과 → 계정 생성
					People *new_acc = people_create_people(
						name_buf, id_buf, pw_buf, "",
						"", // type, love_type: 추후 DFS 화면에서 입력
						(enum Gender)gender, age_val);
					login_add_people_to_hashtable(new_acc);
					return new_acc;
				}
			}

			// 취소
			else if (gui_isInObject(&cancel_border, ui->mx, ui->my)) {
				return NULL;
			}

			else {
				focus = SF_NONE;
			}
		}

		// ── 포커스 색상 갱신 ──
		id_border.textcolor = (focus == SF_ID) ? COLOR_SUPERPINK : COLOR_PINK;
		pw_border.textcolor = (focus == SF_PW) ? COLOR_SUPERPINK : COLOR_PINK;
		name_border.textcolor =
			(focus == SF_NAME) ? COLOR_SUPERPINK : COLOR_PINK;
		age_border.textcolor = (focus == SF_AGE) ? COLOR_SUPERPINK : COLOR_PINK;

		// 성별 선택 강조
		male_fill.textcolor = (gender == 0) ? COLOR_PINK : COLOR_WHITEPINK;
		female_fill.textcolor = (gender == 1) ? COLOR_PINK : COLOR_WHITEPINK;
		male_border.textcolor = (gender == 0) ? COLOR_SUPERPINK : COLOR_PINK;
		female_border.textcolor = (gender == 1) ? COLOR_SUPERPINK : COLOR_PINK;

		// 완료 버튼 hover
		done_btn.textcolor = gui_isInObject(&done_btn, ui->mx, ui->my)
								 ? COLOR_DURTYPINK
								 : COLOR_SUPERPINK;
		idcheck_fill.textcolor = gui_isInObject(&idcheck_border, ui->mx, ui->my)
									 ? COLOR_SOFTPINK
									 : COLOR_WHITEPINK;
		cancel_fill.textcolor = gui_isInObject(&cancel_border, ui->mx, ui->my)
									? COLOR_SOFTPINK
									: COLOR_WHITE;

		// ── 입력값 텍스트 갱신 ──
		// ID
		{
			char disp[MAX_ID_LEN + 4];
			if (strlen(id_buf) == 0 && focus != SF_ID) {
				strcpy(disp, "아이디");
				gui_setColorText(&t_idval, COLOR_WHITEGRAY);
			} else {
				snprintf(disp, sizeof(disp), "%s%s", id_buf,
						 focus == SF_ID ? "_" : "");
				gui_setColorText(&t_idval, COLOR_GRAY);
			}
			if (disp[0] == '\0') {
				strcpy(disp, " ");
			}
			gui_setText(&t_idval, disp);
		}
		// PW
		{
			char disp[128] = "";
			if (strlen(pw_buf) == 0 && focus != SF_PW) {
				strcpy(disp, "비밀번호");
				gui_setColorText(&t_pwval, COLOR_WHITEGRAY);
			} else {
				for (int i = 0; i < display_countChars(pw_buf); i++) {
					strcat(disp, "*");
				}
				if (focus == SF_PW) {
					strcat(disp, "_");
				}
				gui_setColorText(&t_pwval, COLOR_GRAY);
			}
			if (disp[0] == '\0') {
				strcpy(disp, " ");
			}
			gui_setText(&t_pwval, disp);
		}
		// 이름
		{
			char disp[MAX_NAME_LEN + 4];
			if (strlen(name_buf) == 0 && focus != SF_NAME) {
				strcpy(disp, "이름");
				gui_setColorText(&t_nameval, COLOR_WHITEGRAY);
			} else {
				snprintf(disp, sizeof(disp), "%s%s", name_buf,
						 focus == SF_NAME ? "_" : "");
				gui_setColorText(&t_nameval, COLOR_GRAY);
			}
			if (disp[0] == '\0')
				strcpy(disp, " ");
			gui_setText(&t_nameval, disp);
		}
		// 나이
		{
			char disp[16];
			if (strlen(age_buf) == 0 && focus != SF_AGE) {
				strcpy(disp, "나이");
				gui_setColorText(&t_ageval, COLOR_WHITEGRAY);
			} else {
				snprintf(disp, sizeof(disp), "%s%s", age_buf,
						 focus == SF_AGE ? "_" : "");
				gui_setColorText(&t_ageval, COLOR_GRAY);
			}
			if (disp[0] == '\0')
				strcpy(disp, " ");
			gui_setText(&t_ageval, disp);
		}

		// 상태 텍스트
		gui_setColorText(&t_status, status_color);
		gui_setText(&t_status, status[0] ? status : " ");

		// ── 렌더링 ──
		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		gui_presentObject(&panel);
		gui_presentObject(&t_logo);
		gui_presentObject(&t_tagline);
		gui_presentObject(&t_sub1);
		gui_presentObject(&t_sub2);

		gui_presentObject(&card_border);
		gui_presentObject(&card_fill);
		gui_presentObject(&t_heading);
		gui_presentObject(&t_headsub);

		gui_presentObject(&id_border);
		gui_presentObject(&id_fill);
		gui_presentObject(&pw_border);
		gui_presentObject(&pw_fill);
		gui_presentObject(&name_border);
		gui_presentObject(&name_fill);
		gui_presentObject(&age_border);
		gui_presentObject(&age_fill);

		gui_presentObject(&male_border);
		gui_presentObject(&male_fill);
		gui_presentObject(&t_male);
		gui_presentObject(&female_border);
		gui_presentObject(&female_fill);
		gui_presentObject(&t_female);

		gui_presentObject(&idcheck_border);
		gui_presentObject(&idcheck_fill);
		gui_presentObject(&t_idcheck);

		gui_presentObject(&done_btn);
		gui_presentObject(&t_done);

		gui_presentObject(&cancel_border);
		gui_presentObject(&cancel_fill);
		gui_presentObject(&t_cancel);

		gui_presentObject(&t_idval);
		gui_presentObject(&t_pwval);
		gui_presentObject(&t_nameval);
		gui_presentObject(&t_ageval);
		gui_presentObject(&t_status);

		SDL_RenderPresent(ui->renderer);

		ui->is_mouse_up = false;
		ui->is_mouse_down = false;
		ui->is_mouse_move = false;
	}
	return NULL;
}

void display_showHome(SDL_Ui *ui, People *me) {
	if (me == NULL) {
		ui->quit = true;
		return;
	}

	const int SIDEBAR_W = 250; // 사이드바 너비
	const int LEFT = 280;	   // 본문 시작 x
	const int RIGHT = 1250;	   // 본문 오른쪽 끝 x
	const int NAV_X = 125;
	const int NAV_W = SIDEBAR_W - 80;
	const int NAV_H = 51;
	const int NAV_TEXT_X = 72;

	// ── 사이드바 ──
	Object sidebar = gui_initObject(
		ui, BOX, 0, 0, TOPLEFT,
		(ObjectParam){.box = {SIDEBAR_W, WINDOW_HEIGHT, COLOR_WHITEPINK, 0}});
	Object t_logo = gui_initObject(
		ui, TEXT, 110, 44, MIDTOP,
		(ObjectParam){.text = {"LIFO", ui->font_bbsig, COLOR_SUPERPINK}});

	// 네비게이션 항목 (0:홈 이 현재 화면)
	enum { NAV_HOME, NAV_BFS, NAV_DFS, NAV_PROFILE, NAV_CNT };
	char *nav_labels[NAV_CNT] = {"홈", "매칭", "설문", "프로필"};
	int nav_y[NAV_CNT] = {150, 150 + 65, 150 + 65 * 2, 150 + 65 * 3};
	Object nav_box[NAV_CNT + 1];
	Object nav_txt[NAV_CNT + 1];
	for (int i = 0; i < NAV_CNT; i++) {
		nav_box[i] = gui_initObject(
			ui, BOX, NAV_X, nav_y[i], MIDTOP,
			(ObjectParam){.box = {NAV_W, NAV_H, COLOR_PINK, 14}});
		nav_txt[i] = gui_initObject(
			ui, TEXT, NAV_TEXT_X, nav_y[i] + NAV_H / 2, CENTER,
			(ObjectParam){.text = {" ", ui->font_normal, COLOR_DURTYPINK}});
		gui_setText(&nav_txt[i], nav_labels[i]);
	}
	nav_box[NAV_CNT] = gui_initObject(
		ui, BOX, NAV_X, WINDOW_HEIGHT - 30, MIDBOTTOM,
		(ObjectParam){.box = {NAV_W, NAV_H, COLOR_PINK, 14}});
	nav_txt[NAV_CNT] = gui_initObject(
		ui, TEXT, NAV_TEXT_X, WINDOW_HEIGHT - 30 - NAV_H / 2, CENTER,
		(ObjectParam){.text = {"로그아웃", ui->font_normal, COLOR_DURTYPINK}});

	// ── 상단 인사말 ──
	char greet[64];
	snprintf(greet, sizeof(greet), "안녕하세요, %s 님!", me->name);
	Object t_greet =
		gui_initObject(ui, TEXT, LEFT, 36, TOPLEFT,
					   (ObjectParam){.text = {" ", ui->font_big, COLOR_BLACK}});
	gui_setText(&t_greet, greet);

	// ── 내 정보 카드 ──
	Object profile_card =
		gui_initObject(ui, BOX, LEFT, 150, TOPLEFT,
					   (ObjectParam){.box = {970, 180, COLOR_WHITEPINK, 22}});
	Object avatar =
		gui_initObject(ui, BOX, LEFT + 75, 150 + profile_card.dstrect.h / 2,
					   CENTER, (ObjectParam){.box = {90, 90, COLOR_PINK, 22}});

	Object t_name = gui_initObject(
		ui, TEXT, LEFT + 150, 190, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_normal, COLOR_DURTYPINK}});
	gui_setText(&t_name, me->name);

	char info1[160];
	if (strlen(me->type) > 0 && strlen(me->love_type) > 0) {
		snprintf(info1, sizeof(info1), "내 유형 %s · 이상형 %s",
				 dfs_type_name(me->type), dfs_type_name(me->love_type));
	} else if (strlen(me->type) > 0) {
		snprintf(info1, sizeof(info1), "내 유형 %s", dfs_type_name(me->type));
	} else {
		strcpy(info1, "아직 유형 미설정 · 연애 유형 검사를 시작하세요");
	}
	Object t_info1 = gui_initObject(
		ui, TEXT, LEFT + 150, 240, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_small, COLOR_DURTYPINK}});
	gui_setText(&t_info1, info1);

	snprintf(info1, sizeof(info1), "%d세 · %s", me->age,
			 me->gen == GENDER_MALE ? "남성" : "여성");
	Object t_info2 = gui_initObject(
		ui, TEXT, LEFT + 150, 275, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_small, COLOR_GRAY}});
	gui_setText(&t_info2, info1);

	Object cta = gui_initObject(
		ui, BOX, LEFT, 350, TOPLEFT,
		(ObjectParam){.box = {RIGHT - LEFT, 92, COLOR_SUPERPINK, 18}});
	Object t_cta = gui_initObject(
		ui, TEXT, LEFT + 30, 368, TOPLEFT,
		(ObjectParam){.text = {"매칭 시작하기", ui->font_normal, COLOR_WHITE}});
	Object t_ctasub = gui_initObject(
		ui, TEXT, LEFT + 30, 408, TOPLEFT,
		(ObjectParam){.text = {"최적의 이상형을 찾아보세요", ui->font_small,
							   COLOR_WHITEPINK}});
	Object t_ctaarrow = gui_initObject(
		ui, TEXT, RIGHT - 80, cta.dstrect.y + cta.dstrect.h / 2, CENTER,
		(ObjectParam){.text = {"→", ui->font_big, COLOR_WHITE}});

	// ── 하단 액션 카드 2개: 설문 다시 풀기 / 오늘의 기록 ──
	int card_w = (RIGHT - LEFT - 20) / 2; // 두 카드 사이 간격 20
	int card2_x = LEFT + card_w + 20;

	Object survey_border =
		gui_initObject(ui, BOX, LEFT, 462, TOPLEFT,
					   (ObjectParam){.box = {card_w, 96, COLOR_SOFTPINK, 16}});
	Object survey_fill =
		gui_initObject(ui, BOX, LEFT + 2, 464, TOPLEFT,
					   (ObjectParam){.box = {card_w - 4, 92, COLOR_WHITE, 14}});
	Object t_survey = gui_initObject(
		ui, TEXT, LEFT + 28, 484, TOPLEFT,
		(ObjectParam){.text = {"연애 성격 유형 검사 시작하기", ui->font_normal,
							   COLOR_DURTYPINK}});
	Object t_surveysub =
		gui_initObject(ui, TEXT, LEFT + 28, 522, TOPLEFT,
					   (ObjectParam){.text = {"성격 유형 재진단",
											  ui->font_small, COLOR_GRAY}});

	Object record_border =
		gui_initObject(ui, BOX, card2_x, 462, TOPLEFT,
					   (ObjectParam){.box = {card_w, 96, COLOR_SOFTPINK, 16}});
	Object record_fill =
		gui_initObject(ui, BOX, card2_x + 2, 464, TOPLEFT,
					   (ObjectParam){.box = {card_w - 4, 92, COLOR_WHITE, 14}});
	Object t_record = gui_initObject(
		ui, TEXT, card2_x + 28, 484, TOPLEFT,
		(ObjectParam){.text = {"개발자에게 매점 사주기", ui->font_normal,
							   COLOR_DURTYPINK}});
	Object t_recordsub = gui_initObject(
		ui, TEXT, card2_x + 26, 522, TOPLEFT,
		(ObjectParam){.text = {"nyamnyam", ui->font_small, COLOR_GRAY}});

	// ── 둘째 줄 카드: 트리 시각화 / 유형 도감 ──
	Object tree_border =
		gui_initObject(ui, BOX, LEFT, 570, TOPLEFT,
					   (ObjectParam){.box = {card_w, 96, COLOR_SOFTVIOLET, 16}});
	Object tree_fill =
		gui_initObject(ui, BOX, LEFT + 2, 572, TOPLEFT,
					   (ObjectParam){.box = {card_w - 4, 92, COLOR_WHITE, 14}});
	Object t_tree = gui_initObject(
		ui, TEXT, LEFT + 28, 592, TOPLEFT,
		(ObjectParam){.text = {"트리 시각화", ui->font_normal, COLOR_VIOLET}});
	Object t_treesub = gui_initObject(
		ui, TEXT, LEFT + 28, 630, TOPLEFT,
		(ObjectParam){.text = {"유형 트리를 그래프로 보기", ui->font_small,
							   COLOR_GRAY}});

	Object codex_border =
		gui_initObject(ui, BOX, card2_x, 570, TOPLEFT,
					   (ObjectParam){.box = {card_w, 96, COLOR_SOFTVIOLET, 16}});
	Object codex_fill =
		gui_initObject(ui, BOX, card2_x + 2, 572, TOPLEFT,
					   (ObjectParam){.box = {card_w - 4, 92, COLOR_WHITE, 14}});
	Object t_codex = gui_initObject(
		ui, TEXT, card2_x + 28, 592, TOPLEFT,
		(ObjectParam){.text = {"유형 도감", ui->font_normal, COLOR_VIOLET}});
	Object t_codexsub = gui_initObject(
		ui, TEXT, card2_x + 28, 630, TOPLEFT,
		(ObjectParam){.text = {"모든 유형 설명 보기", ui->font_small,
							   COLOR_GRAY}});

	while (!ui->quit) {
		if (ui->next_state != HOME) {
			break;
		}

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
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				break;
			}
		}

		if (ui->is_mouse_down) {
			if (gui_isInObject(&cta, ui->mx, ui->my) ||
				gui_isInObject(&nav_box[NAV_BFS], ui->mx, ui->my)) {
				ui->next_state = BFS;
				break;
			} else if (gui_isInObject(&survey_border, ui->mx, ui->my) ||
					   gui_isInObject(&nav_box[NAV_DFS], ui->mx, ui->my)) {
				ui->next_state = DFS;
				break;
			} else if (gui_isInObject(&record_border, ui->mx, ui->my)) {
				// ui->next_state = MST;
				//  break;
			} else if (gui_isInObject(&tree_border, ui->mx, ui->my)) {
				display_showTreeView(ui, me); // 블로킹 서브화면, 끝나면 홈 복귀
				ui->is_mouse_down = false;
			} else if (gui_isInObject(&codex_border, ui->mx, ui->my)) {
				display_showCodex(ui, me);
				ui->is_mouse_down = false;
			} else if (gui_isInObject(&nav_box[NAV_PROFILE], ui->mx, ui->my)) {
				// ui->next_state = PROFILE;
				//  break;
			} else if (gui_isInObject(&nav_box[NAV_CNT], ui->mx, ui->my)) {
				ui->next_state = LOGIN;
				break;
			}
		}

		for (int i = 0; i <= NAV_CNT; i++) {
			int active = (i == NAV_HOME);
			int hover = gui_isInObject(&nav_box[i], ui->mx, ui->my);
			nav_box[i].textcolor = active ? COLOR_PINK : COLOR_PINK; //??
			SDL_Color want = (active || hover) ? COLOR_WHITE : COLOR_DURTYPINK;
			if (want.r != nav_txt[i].textcolor.r ||
				want.g != nav_txt[i].textcolor.g ||
				want.b != nav_txt[i].textcolor.b) {
				gui_setColorText(&nav_txt[i], want);
			}
		}

		// CTA hover
		cta.textcolor = gui_isInObject(&cta, ui->mx, ui->my) ? COLOR_DURTYPINK
															 : COLOR_SUPERPINK;
		// 액션 카드 hover (채움색)
		survey_fill.textcolor = gui_isInObject(&survey_border, ui->mx, ui->my)
									? COLOR_WHITEPINK
									: COLOR_WHITE;
		record_fill.textcolor = gui_isInObject(&record_border, ui->mx, ui->my)
									? COLOR_WHITEPINK
									: COLOR_WHITE;
		tree_fill.textcolor = gui_isInObject(&tree_border, ui->mx, ui->my)
								  ? COLOR_WHITEVIOLET
								  : COLOR_WHITE;
		codex_fill.textcolor = gui_isInObject(&codex_border, ui->mx, ui->my)
								   ? COLOR_WHITEVIOLET
								   : COLOR_WHITE;

		// ── 렌더링 ──
		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		gui_presentObject(&sidebar);
		gui_presentObject(&t_logo);
		for (int i = 0; i <= NAV_CNT; i++) {
			int active = (i == NAV_HOME);
			int hover = gui_isInObject(&nav_box[i], ui->mx, ui->my);
			if (active || hover) {
				gui_presentObject(&nav_box[i]);
			}
			gui_presentObject(&nav_txt[i]);
		}

		gui_presentObject(&t_greet);

		gui_presentObject(&profile_card);
		gui_presentObject(&avatar);
		gui_presentObject(&t_name);
		gui_presentObject(&t_info1);
		gui_presentObject(&t_info2);

		gui_presentObject(&cta);
		gui_presentObject(&t_cta);
		gui_presentObject(&t_ctasub);
		gui_presentObject(&t_ctaarrow);

		gui_presentObject(&survey_border);
		gui_presentObject(&survey_fill);
		gui_presentObject(&t_survey);
		gui_presentObject(&t_surveysub);

		gui_presentObject(&record_border);
		gui_presentObject(&record_fill);
		gui_presentObject(&t_record);
		gui_presentObject(&t_recordsub);

		gui_presentObject(&tree_border);
		gui_presentObject(&tree_fill);
		gui_presentObject(&t_tree);
		gui_presentObject(&t_treesub);

		gui_presentObject(&codex_border);
		gui_presentObject(&codex_fill);
		gui_presentObject(&t_codex);
		gui_presentObject(&t_codexsub);

		SDL_RenderPresent(ui->renderer);

		ui->is_mouse_up = false;
		ui->is_mouse_down = false;
		ui->is_mouse_move = false;
	}
}

void display_showBFS(SDL_Ui *ui, People *me, MatchingInfo *infos, int n) {
	if (me == NULL) {
		ui->quit = true;
		return;
	}

	const int SIDEBAR_W = 250;
	const int LEFT = 280;
	const int RIGHT = 1250;
	const int NAV_X = 125;
	const int NAV_W = SIDEBAR_W - 80;
	const int NAV_H = 51;
	const int NAV_TEXT_X = 72;

	Object sidebar = gui_initObject(
		ui, BOX, 0, 0, TOPLEFT,
		(ObjectParam){.box = {SIDEBAR_W, WINDOW_HEIGHT, COLOR_WHITEPINK, 0}});
	Object t_logo = gui_initObject(
		ui, TEXT, 110, 44, MIDTOP,
		(ObjectParam){.text = {"LIFO", ui->font_bbsig, COLOR_SUPERPINK}});

	// 사이드바
	enum { NAV_HOME, NAV_BFS, NAV_DFS, NAV_PROFILE, NAV_CNT };
	char *nav_labels[NAV_CNT] = {"홈", "매칭", "설문", "프로필"};
	int nav_y[NAV_CNT] = {150, 150 + 65, 150 + 65 * 2, 150 + 65 * 3};
	Object nav_box[NAV_CNT + 1];
	Object nav_txt[NAV_CNT + 1];
	for (int i = 0; i < NAV_CNT; i++) {
		nav_box[i] = gui_initObject(
			ui, BOX, NAV_X, nav_y[i], MIDTOP,
			(ObjectParam){.box = {NAV_W, NAV_H, COLOR_PINK, 14}});
		nav_txt[i] = gui_initObject(
			ui, TEXT, NAV_TEXT_X, nav_y[i] + NAV_H / 2, CENTER,
			(ObjectParam){.text = {" ", ui->font_normal, COLOR_DURTYPINK}});
		gui_setText(&nav_txt[i], nav_labels[i]);
	}
	nav_box[NAV_CNT] = gui_initObject(
		ui, BOX, NAV_X, WINDOW_HEIGHT - 30, MIDBOTTOM,
		(ObjectParam){.box = {NAV_W, NAV_H, COLOR_PINK, 14}});
	nav_txt[NAV_CNT] = gui_initObject(
		ui, TEXT, NAV_TEXT_X, WINDOW_HEIGHT - 30 - NAV_H / 2, CENTER,
		(ObjectParam){.text = {"로그아웃", ui->font_normal, COLOR_DURTYPINK}});

	// 메인 기능 화면
	Object t_greet =
		gui_initObject(ui, TEXT, LEFT, 36, TOPLEFT,
					   (ObjectParam){.text = {"Best Fit Stable-Matching",
											  ui->font_big, COLOR_BLACK}});

	// 카드 화면
	int CARD_X = LEFT + 250;
	int CARD_Y = 100;
	int CARD_W = 400;				  // 카드 너비
	int CARD_H = 580;				  // 카드 높이
	int BTN_X = CARD_X + CARD_W + 40; // 버튼 x 시작
	int BTN_W = 200;
	int BTN_H = 60;
	int BTN_GAP = 80; // 버튼 간격

	// 카드 (border + fill)
	Object card_border = gui_initObject(
		ui, BOX, CARD_X, CARD_Y, TOPLEFT,
		(ObjectParam){.box = {CARD_W, CARD_H, COLOR_SOFTPINK, 26}});
	Object card_fill = gui_initObject(
		ui, BOX, CARD_X + 2, CARD_Y + 2, TOPLEFT,
		(ObjectParam){.box = {CARD_W - 4, CARD_H - 4, COLOR_WHITE, 24}});

	// 아바타 (카드 상단 중앙)
	Object avatar =
		gui_initObject(ui, BOX, CARD_X + CARD_W / 2, CARD_Y + 170, CENTER,
					   (ObjectParam){.box = {300, 300, COLOR_PINK, 26}});

	// 카드 내 텍스트 오브젝트들 (내용은 루프마다 setText로 갱신)
	Object t_card_name =
		gui_initObject(ui, TEXT, CARD_X + 40, CARD_Y + 340, TOPLEFT,
					   (ObjectParam){.text = {"아야츠노 유니", ui->font_big,
											  COLOR_DURTYPINK}});
	Object t_card_age = gui_initObject(
		ui, TEXT, CARD_X + 40, CARD_Y + 370, TOPLEFT,
		(ObjectParam){.text = {"4살", ui->font_normal, COLOR_GRAY}});
	Object t_card_type = gui_initObject(
		ui, TEXT, CARD_X + 40, CARD_Y + 400, TOPLEFT,
		(ObjectParam){
			.text = {"유니콘나라 유니콘", ui->font_normal, COLOR_DURTYPINK}});
	Object t_card_lovetype = gui_initObject(
		ui, TEXT, CARD_X + 40, CARD_Y + 430, TOPLEFT,
		(ObjectParam){.text = {"아르냥", ui->font_normal, COLOR_GRAY}});

	Object card_set[10] = {card_border,	   card_fill,  avatar,
						   t_card_name,	   t_card_age, t_card_type,
						   t_card_lovetype};

	// ── 버튼 3개: 수락 / 채팅 / 거절 ──
	int btn_y_accept = CARD_Y + 60;
	int btn_y_chat = CARD_Y + 60 + BTN_GAP + BTN_H;
	int btn_y_reject = CARD_Y + 60 + (BTN_GAP + BTN_H) * 2;

	Object btn_accept_border = gui_initObject(
		ui, BOX, BTN_X, btn_y_accept, TOPLEFT,
		(ObjectParam){.box = {BTN_W, BTN_H, COLOR_SUPERPINK, 16}});
	Object btn_accept_fill = gui_initObject(
		ui, BOX, BTN_X + 2, btn_y_accept + 2, TOPLEFT,
		(ObjectParam){.box = {BTN_W - 4, BTN_H - 4, COLOR_SUPERPINK, 14}});
	Object t_accept = gui_initObject(
		ui, TEXT, BTN_X + BTN_W / 2, btn_y_accept + BTN_H / 2, CENTER,
		(ObjectParam){.text = {"수락", ui->font_normal, COLOR_WHITE}});

	Object btn_chat_border =
		gui_initObject(ui, BOX, BTN_X, btn_y_chat, TOPLEFT,
					   (ObjectParam){.box = {BTN_W, BTN_H, COLOR_PINK, 16}});
	Object btn_chat_fill = gui_initObject(
		ui, BOX, BTN_X + 2, btn_y_chat + 2, TOPLEFT,
		(ObjectParam){.box = {BTN_W - 4, BTN_H - 4, COLOR_WHITEPINK, 14}});
	Object t_chat = gui_initObject(
		ui, TEXT, BTN_X + BTN_W / 2, btn_y_chat + BTN_H / 2, CENTER,
		(ObjectParam){.text = {"채팅", ui->font_normal, COLOR_DURTYPINK}});

	Object btn_reject_border = gui_initObject(
		ui, BOX, BTN_X, btn_y_reject, TOPLEFT,
		(ObjectParam){.box = {BTN_W, BTN_H, COLOR_SOFTPINK, 16}});
	Object btn_reject_fill = gui_initObject(
		ui, BOX, BTN_X + 2, btn_y_reject + 2, TOPLEFT,
		(ObjectParam){.box = {BTN_W - 4, BTN_H - 4, COLOR_WHITE, 14}});
	Object t_reject = gui_initObject(
		ui, TEXT, BTN_X + BTN_W / 2, btn_y_reject + BTN_H / 2, CENTER,
		(ObjectParam){.text = {"거절", ui->font_normal, COLOR_GRAY}});

	// ── 매칭 실행 버튼 (카드 아래) ──
	const int RUN_Y = CARD_Y + CARD_H - 52 - 24;
	Object btn_run = gui_initObject(
		ui, BOX, CARD_X + 50, RUN_Y, TOPLEFT,
		(ObjectParam){.box = {CARD_W - 100, 52, COLOR_SUPERPINK, 14}});
	Object t_run = gui_initObject(
		ui, TEXT, CARD_X + CARD_W / 2, RUN_Y + 26, CENTER,
		(ObjectParam){.text = {"매칭 실행", ui->font_normal, COLOR_WHITE}});

	Object t_status = gui_initObject(
		ui, TEXT, LEFT, CARD_Y + CARD_H - 90, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_small, COLOR_GRAY}});

	// ── 매칭 상태 ──
	// me의 index를 infos에서 찾기
	int me_idx = -1;
	for (int i = 0; i < n; i++) {
		if (infos[i].person != NULL && strcmp(infos[i].person->id, me->id)) {
			me_idx = i;
			break;
		}
	}
	// printf("%d\n", me_idx);

	// stableMatching 결과 저장용
	Pair result[MAX_PEOPLE];
	int result_cnt = 0;
	int matched_idx = -1; // 현재 제안된 상대 index
	char status_msg[128] = " ";
	SDL_Color status_color = COLOR_GRAY;

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
				ui->mx = event.motion.x;
				ui->my = event.motion.y;
				break;
			}
		}

		if (ui->is_mouse_up) {
			// todo if()if()DFS;

			if (gui_isInObject(&nav_box[NAV_HOME], ui->mx, ui->my)) {
				ui->next_state = HOME;
				break;
			} else if (gui_isInObject(&nav_box[NAV_DFS], ui->mx, ui->my)) {
				ui->next_state = DFS;
				break;
			} // 네비 더 짜기

			// ── 매칭 실행 ──
			else if (gui_isInObject(&btn_run, ui->mx, ui->my) &&
					 !(matched_idx >= 0 && infos[matched_idx].person != NULL)) {
				if (me_idx == -1) {
					strcpy(status_msg, "내 계정을 찾을 수 없습니다.");
					status_color = COLOR_SUPERPINK;
				} else if (me->status == MATCHED) {
					strcpy(status_msg, "이미 매칭된 상태입니다.");
					status_color = COLOR_SUPERPINK;
				} else {
					me->status = AVAILABLE; //?

					int proposers[MAX_PEOPLE];
					int proposer_cnt;
					if (me->gen == GENDER_MALE) {
						proposers[0] = me_idx;
						proposer_cnt = 1;
					} else {
						// 여성인 경우 AVAILABLE 남성 전체를 proposer로
						proposer_cnt =
							collectUser(infos, n, GENDER_MALE, proposers);
					}

					int(*rank_table)[MAX_PEOPLE] =
						calloc((size_t)n, sizeof(*rank_table));
					if (rank_table == NULL) {
						strcpy(status_msg, "메모리 할당 실패.");
						status_color = COLOR_SUPERPINK;
					} else {
						result_cnt =
							stableMatching(infos, n, proposers, proposer_cnt,
										   rank_table, result);
						free(rank_table);

						// me가 포함된 쌍 찾기
						matched_idx = -1;
						for (int i = 0; i < result_cnt; i++) {
							if (result[i].p1 == me_idx ||
								result[i].p2 == me_idx) {
								matched_idx = (result[i].p1 == me_idx)
												  ? result[i].p2
												  : result[i].p1;
								break;
							}
						}

						if (matched_idx == -1) {
							strcpy(status_msg,
								   "현재 매칭 가능한 상대가 없습니다.");
							status_color = COLOR_GRAY;
						} else {
							strcpy(status_msg, "매칭 상대를 찾았습니다! 수락 "
											   "또는 거절을 선택하세요.");
							status_color = COLOR_GREEN;
						}
					}
				}
			}

			// ── 수락 ──
			else if (gui_isInObject(&btn_accept_border, ui->mx, ui->my)) {
				if (matched_idx == -1) {
					strcpy(status_msg, "먼저 매칭을 실행해주세요.");
					status_color = COLOR_SUPERPINK;
				} else if (me->status != PROPOSED) {
					strcpy(status_msg, "수락할 제안이 없습니다.");
					status_color = COLOR_SUPERPINK;
				} else {
					confirmMatch(infos, me_idx, matched_idx);
					strcpy(status_msg, "매칭이 성사되었습니다!");
					status_color = COLOR_GREEN;
				}
			}

			// ── 채팅 (미구현 안내) ──
			else if (gui_isInObject(&btn_chat_border, ui->mx, ui->my)) {
				strcpy(status_msg, "채팅 기능은 준비 중입니다.");
				status_color = COLOR_GRAY;
			}

			// ── 거절 ──
			else if (gui_isInObject(&btn_reject_border, ui->mx, ui->my)) {
				if (matched_idx == -1) {
					strcpy(status_msg, "먼저 매칭을 실행해주세요.");
					status_color = COLOR_SUPERPINK;
				} else if (me->status != PROPOSED) {
					strcpy(status_msg, "거절할 제안이 없습니다.");
					status_color = COLOR_SUPERPINK;
				} else {
					// 상대를 차단 목록에 추가 → bfsCompat에서 -INF 적용
					blockUser(me, infos[matched_idx].person);
					rejectMatch(infos, me_idx, matched_idx);
					matched_idx = -1;
					strcpy(status_msg, "거절했습니다. 매칭 실행을 다시 눌러 "
									   "다음 상대를 찾아보세요.");
					status_color = COLOR_DURTYPINK;
				}
			}
		}

		// ── 버튼 hover 색상 갱신 ──
		btn_accept_fill.textcolor =
			gui_isInObject(&btn_accept_border, ui->mx, ui->my)
				? COLOR_DURTYPINK
				: COLOR_SUPERPINK;
		btn_chat_fill.textcolor =
			gui_isInObject(&btn_chat_border, ui->mx, ui->my) ? COLOR_SOFTPINK
															 : COLOR_WHITEPINK;
		btn_reject_fill.textcolor =
			gui_isInObject(&btn_reject_border, ui->mx, ui->my) ? COLOR_SOFTPINK
															   : COLOR_WHITE;
		btn_run.textcolor = gui_isInObject(&btn_run, ui->mx, ui->my)
								? COLOR_DURTYPINK
								: COLOR_SUPERPINK;

		for (int i = 0; i <= NAV_CNT; i++) {
			int active = (i == NAV_BFS);
			int hover = gui_isInObject(&nav_box[i], ui->mx, ui->my);
			nav_box[i].textcolor = active ? COLOR_PINK : COLOR_PINK; //??
			SDL_Color want = (active || hover) ? COLOR_WHITE : COLOR_DURTYPINK;
			if (want.r != nav_txt[i].textcolor.r ||
				want.g != nav_txt[i].textcolor.g ||
				want.b != nav_txt[i].textcolor.b) {
				gui_setColorText(&nav_txt[i], want);
			}
		}

		// ── 카드 텍스트 갱신 ──
		if (matched_idx >= 0 && infos[matched_idx].person != NULL) {
			People *partner = infos[matched_idx].person;

			// 이름
			gui_setText(&t_card_name, partner->name);

			// 나이 · 성별
			char age_gen[32];
			snprintf(age_gen, sizeof(age_gen), "%d세 · %s", partner->age,
					 partner->gen == GENDER_MALE ? "남성" : "여성");
			gui_setText(&t_card_age, age_gen);

			// 성격 유형
			char type_str[64];
			if (strlen(partner->type) > 0) {
				snprintf(type_str, sizeof(type_str), "유형: %s",
						 dfs_type_name(partner->type));
			} else {
				strcpy(type_str, "유형: 미설정");
			}
			gui_setText(&t_card_type, type_str);

			// 이상형 유형
			char love_str[64];
			if (strlen(partner->love_type) > 0) {
				snprintf(love_str, sizeof(love_str), "이상형: %s",
						 dfs_type_name(partner->love_type));
			} else {
				strcpy(love_str, "이상형: 미설정");
			}
			gui_setText(&t_card_lovetype, love_str);
		} else {
			gui_setText(&t_card_name, "상대를 찾는 중...");
			gui_setText(&t_card_age, " ");
			gui_setText(&t_card_type, " ");
			gui_setText(&t_card_lovetype, " ");
		}

		gui_setColorText(&t_status, status_color);
		gui_setText(&t_status, status_msg[0] ? status_msg : " ");

		// ── 렌더링 ──
		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		gui_presentObject(&sidebar);
		gui_presentObject(&t_logo);
		for (int i = 0; i <= NAV_CNT; i++) {
			int active = (i == NAV_BFS);
			int hover = gui_isInObject(&nav_box[i], ui->mx, ui->my);
			if (active || hover) {
				gui_presentObject(&nav_box[i]);
			}
			gui_presentObject(&nav_txt[i]);
		}

		gui_presentObject(&t_greet);

		// 카드
		gui_presentObject(&card_border);
		gui_presentObject(&card_fill);
		gui_presentObject(&avatar);
		gui_presentObject(&t_card_name);
		gui_presentObject(&t_card_age);
		gui_presentObject(&t_card_type);
		gui_presentObject(&t_card_lovetype);

		// 버튼 3개
		gui_presentObject(&btn_accept_border);
		gui_presentObject(&btn_accept_fill);
		gui_presentObject(&t_accept);

		gui_presentObject(&btn_chat_border);
		gui_presentObject(&btn_chat_fill);
		gui_presentObject(&t_chat);

		gui_presentObject(&btn_reject_border);
		gui_presentObject(&btn_reject_fill);
		gui_presentObject(&t_reject);

		// 매칭 실행 버튼
		if (!(matched_idx >= 0 && infos[matched_idx].person != NULL)) {
			gui_presentObject(&btn_run);
			gui_presentObject(&t_run);
		}

		gui_presentObject(&t_status);

		SDL_RenderPresent(ui->renderer);

		ui->is_mouse_up = false;
		ui->is_mouse_down = false;
		ui->is_mouse_move = false;

		if (ui->next_state != BFS) {
			break;
		}
	}
}
