#include "display.h"

enum Focus { FOCUS_NONE, FOCUS_ID, FOCUS_PW };

int count_chars(const char *pw_buf) {
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

People *showLogin(SDL_Ui *ui) {
	People *logged_in = NULL;
	char id_buf[MAX_ID_LEN] = "";
	char pw_buf[MAX_PW_LEN] = "";
	enum Focus focus = FOCUS_NONE;
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
		(ObjectParam){.text = {"LIFO", ui->font_bbsig, COLOR_SUPERPINK}});
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
						snprintf(status, sizeof(status), "환영합니다, %s님!",
								 acc->name);
						status_color = COLOR_GREEN;
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
				People *new_acc = showSignup(ui);
				// if (new_acc) {
				// 	// 회원가입 성공 → 바로 로그인 처리
				// 	snprintf(status, sizeof(status), "환영합니다, %s님!",
				// 			 new_acc->name);
				// 	status_color = COLOR_GREEN;
				// 	if (logged_in) {
				// 		people_delete_people(logged_in);
				// 	}
				// 	logged_in = new_acc;
				// 	ui->next_state = DFS;
				// } else {
				// 	// 회원가입 취소 또는 실패
				// 	strcpy(status, " ");
				// 	status_color = COLOR_GRAY;
				// }
				// // 로그인 화면 복귀 후 렌더 상태 초기화
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
			for (int i = 0; i < count_chars(pw_buf); i++)
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

People *showSignup(SDL_Ui *ui) {
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
		(ObjectParam){.text = {"LIFO", ui->font_bbsig, COLOR_SUPERPINK}});
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
			}
			if (disp[0] == '\0')
				strcpy(disp, " ");
			gui_setText(&t_idval, disp);
		}
		// PW
		{
			char disp[128] = "";
			if (strlen(pw_buf) == 0 && focus != SF_PW) {
				strcpy(disp, "비밀번호");
				gui_setColorText(&t_pwval, COLOR_WHITEGRAY);
			} else {
				for (int i = 0; i < count_chars(pw_buf); i++)
					strcat(disp, "*");
				if (focus == SF_PW)
					strcat(disp, "_");
				gui_setColorText(&t_pwval, COLOR_GRAY);
			}
			if (disp[0] == '\0')
				strcpy(disp, " ");
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

// 로그인 후 보여주는 메인(홈) 화면.
// 왼쪽 사이드바 네비 + 내 정보 카드 + 통계 카드 + 매칭/설문/기록 액션으로 구성.
// 종료(창 닫기)할 때까지 자체 루프를 돈다.
void showHome(SDL_Ui *ui, People *me) {
	if (me == NULL) {
		ui->quit = true;
		return;
	}
	ui->next_state = HOME;

	const int SIDEBAR_W = 260; // 사이드바 너비
	const int MAIN_X = 300;	   // 본문 시작 x
	const int RIGHT = 1240;	   // 본문 오른쪽 끝 x

	char status[160] = " ";
	SDL_Color status_color = COLOR_GRAY;

	// ── 사이드바 ──
	Object sidebar = gui_initObject(
		ui, BOX, 0, 0, TOPLEFT,
		(ObjectParam){.box = {SIDEBAR_W, WINDOW_HEIGHT, COLOR_WHITEPINK, 0}});
	Object t_logo = gui_initObject(
		ui, TEXT, 40, 44, TOPLEFT,
		(ObjectParam){.text = {"LIFO", ui->font_big, COLOR_SUPERPINK}});

	// 네비게이션 항목 (0:홈 이 현재 화면)
	enum { NAV_HOME, NAV_MATCH, NAV_SURVEY, NAV_RECORD, NAV_PROFILE, NAV_CNT };
	char *nav_labels[NAV_CNT] = {"홈", "매칭", "설문", "기록", "프로필"};
	int nav_y[NAV_CNT] = {150, 212, 274, 336, 398};
	Object nav_box[NAV_CNT];
	Object nav_txt[NAV_CNT];
	for (int i = 0; i < NAV_CNT; i++) {
		nav_box[i] =
			gui_initObject(ui, BOX, 30, nav_y[i], TOPLEFT,
						   (ObjectParam){.box = {200, 52, COLOR_PINK, 14}});
		nav_txt[i] = gui_initObject(
			ui, TEXT, 70, nav_y[i] + 13, TOPLEFT,
			(ObjectParam){.text = {" ", ui->font_normal, COLOR_DURTYPINK}});
		gui_setText(&nav_txt[i], nav_labels[i]);
	}

	// ── 상단 인사말 ──
	char greet[64];
	snprintf(greet, sizeof(greet), "안녕, %s", me->name);
	Object t_greet =
		gui_initObject(ui, TEXT, MAIN_X, 36, TOPLEFT,
					   (ObjectParam){.text = {" ", ui->font_big, COLOR_BLACK}});
	gui_setText(&t_greet, greet);
	Object t_greetsub = gui_initObject(
		ui, TEXT, MAIN_X + 2, 92, TOPLEFT,
		(ObjectParam){.text = {"오늘도 좋은 인연 찾아볼까?", ui->font_small,
							   COLOR_GRAY}});

	// ── 내 정보 카드 ──
	Object profile_card =
		gui_initObject(ui, BOX, MAIN_X, 150, TOPLEFT,
					   (ObjectParam){.box = {690, 180, COLOR_WHITEPINK, 22}});
	Object avatar =
		gui_initObject(ui, BOX, MAIN_X + 30, 185, TOPLEFT,
					   (ObjectParam){.box = {90, 90, COLOR_PINK, 22}});

	Object t_name = gui_initObject(
		ui, TEXT, MAIN_X + 150, 188, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_normal, COLOR_DURTYPINK}});
	gui_setText(&t_name, me->name);

	char info1[160];
	if (strlen(me->type) > 0 && strlen(me->love_type) > 0)
		snprintf(info1, sizeof(info1), "내 유형 %s · 이상형 %s", me->type,
				 me->love_type);
	else if (strlen(me->type) > 0)
		snprintf(info1, sizeof(info1), "내 유형 %s", me->type);
	else
		strcpy(info1, "아직 유형 미설정 · 설문을 풀어보세요");
	Object t_info1 = gui_initObject(
		ui, TEXT, MAIN_X + 150, 235, TOPLEFT,
		(ObjectParam){.text = {" ", ui->font_small, COLOR_DURTYPINK}});
	gui_setText(&t_info1, info1);

	char info2[64];
	snprintf(info2, sizeof(info2), "%d세 · %s", me->age,
			 me->gen == GENDER_MALE ? "남성" : "여성");
	Object t_info2 =
		gui_initObject(ui, TEXT, MAIN_X + 150, 268, TOPLEFT,
					   (ObjectParam){.text = {" ", ui->font_small, COLOR_GRAY}});
	gui_setText(&t_info2, info2);

	// ── 우측 통계 카드 2개 ──
	Object stat1_card =
		gui_initObject(ui, BOX, 1005, 150, TOPLEFT,
					   (ObjectParam){.box = {235, 82, COLOR_WHITEPINK, 18}});
	Object t_stat1_label = gui_initObject(
		ui, TEXT, 1025, 166, TOPLEFT,
		(ObjectParam){.text = {"새 매칭", ui->font_small, COLOR_GRAY}});
	Object t_stat1_val = gui_initObject(
		ui, TEXT, 1025, 190, TOPLEFT,
		(ObjectParam){.text = {"-", ui->font_big, COLOR_DURTYPINK}});

	Object stat2_card =
		gui_initObject(ui, BOX, 1005, 248, TOPLEFT,
					   (ObjectParam){.box = {235, 82, COLOR_WHITEPINK, 18}});
	Object t_stat2_label = gui_initObject(
		ui, TEXT, 1025, 264, TOPLEFT,
		(ObjectParam){.text = {"기록 일수", ui->font_small, COLOR_GRAY}});
	Object t_stat2_val = gui_initObject(
		ui, TEXT, 1025, 288, TOPLEFT,
		(ObjectParam){.text = {"0", ui->font_big, COLOR_DURTYPINK}});

	// ── 메인 CTA: 매칭 찾으러 가기 ──
	Object cta = gui_initObject(
		ui, BOX, MAIN_X, 350, TOPLEFT,
		(ObjectParam){.box = {RIGHT - MAIN_X, 92, COLOR_SUPERPINK, 18}});
	Object t_cta = gui_initObject(
		ui, TEXT, MAIN_X + 30, 368, TOPLEFT,
		(ObjectParam){.text = {"매칭 찾으러 가기", ui->font_normal, COLOR_WHITE}});
	Object t_ctasub = gui_initObject(
		ui, TEXT, MAIN_X + 30, 408, TOPLEFT,
		(ObjectParam){.text = {"유사도 높은 후보를 찾아드려요", ui->font_small,
							   COLOR_WHITEPINK}});
	Object t_ctaarrow =
		gui_initObject(ui, TEXT, RIGHT - 50, 372, TOPLEFT,
					   (ObjectParam){.text = {"→", ui->font_big, COLOR_WHITE}});

	// ── 하단 액션 카드 2개: 설문 다시 풀기 / 오늘의 기록 ──
	int card_w = (RIGHT - MAIN_X - 20) / 2; // 두 카드 사이 간격 20
	int card2_x = MAIN_X + card_w + 20;

	Object survey_border =
		gui_initObject(ui, BOX, MAIN_X, 462, TOPLEFT,
					   (ObjectParam){.box = {card_w, 96, COLOR_SOFTPINK, 16}});
	Object survey_fill =
		gui_initObject(ui, BOX, MAIN_X + 2, 464, TOPLEFT,
					   (ObjectParam){.box = {card_w - 4, 92, COLOR_WHITE, 14}});
	Object t_survey = gui_initObject(
		ui, TEXT, MAIN_X + 28, 484, TOPLEFT,
		(ObjectParam){.text = {"설문 다시 풀기", ui->font_normal, COLOR_DURTYPINK}});
	Object t_surveysub = gui_initObject(
		ui, TEXT, MAIN_X + 28, 524, TOPLEFT,
		(ObjectParam){.text = {"성격 유형 재진단", ui->font_small, COLOR_GRAY}});

	Object record_border =
		gui_initObject(ui, BOX, card2_x, 462, TOPLEFT,
					   (ObjectParam){.box = {card_w, 96, COLOR_SOFTPINK, 16}});
	Object record_fill =
		gui_initObject(ui, BOX, card2_x + 2, 464, TOPLEFT,
					   (ObjectParam){.box = {card_w - 4, 92, COLOR_WHITE, 14}});
	Object t_record = gui_initObject(
		ui, TEXT, card2_x + 26, 484, TOPLEFT,
		(ObjectParam){.text = {"오늘의 기록", ui->font_normal, COLOR_DURTYPINK}});
	Object t_recordsub = gui_initObject(
		ui, TEXT, card2_x + 26, 524, TOPLEFT,
		(ObjectParam){.text = {"아직 작성 전", ui->font_small, COLOR_GRAY}});

	Object t_status =
		gui_initObject(ui, TEXT, MAIN_X, 588, TOPLEFT,
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
			}
		}

		if (ui->is_mouse_down) {
			// 매칭(네비 또는 CTA) → BFS, 설문 → DFS, 기록 → MST.
			// 해당 화면들은 아직 미구현이라 안내 문구만 표시한다.
			// (구현되면 아래 주석을 ui->next_state = ...; 로 바꾸면 된다.)
			if (gui_isInObject(&cta, ui->mx, ui->my) ||
				gui_isInObject(&nav_box[NAV_MATCH], ui->mx, ui->my)) {
				// ui->next_state = BFS;
				strcpy(status, "매칭 화면은 준비 중이에요. (곧 추가됩니다)");
				status_color = COLOR_SUPERPINK;
			} else if (gui_isInObject(&survey_border, ui->mx, ui->my) ||
					   gui_isInObject(&nav_box[NAV_SURVEY], ui->mx, ui->my)) {
				// ui->next_state = DFS;
				strcpy(status, "설문 화면은 준비 중이에요. (곧 추가됩니다)");
				status_color = COLOR_SUPERPINK;
			} else if (gui_isInObject(&record_border, ui->mx, ui->my) ||
					   gui_isInObject(&nav_box[NAV_RECORD], ui->mx, ui->my)) {
				// ui->next_state = MST;
				strcpy(status, "기록 화면은 준비 중이에요. (곧 추가됩니다)");
				status_color = COLOR_SUPERPINK;
			} else if (gui_isInObject(&nav_box[NAV_PROFILE], ui->mx, ui->my)) {
				strcpy(status, "프로필 화면은 준비 중이에요. (곧 추가됩니다)");
				status_color = COLOR_SUPERPINK;
			}
		}

		// ── 네비 hover/active 색상 ──
		for (int i = 0; i < NAV_CNT; i++) {
			int active = (i == NAV_HOME);
			int hover = gui_isInObject(&nav_box[i], ui->mx, ui->my);
			nav_box[i].textcolor = active ? COLOR_PINK : COLOR_SOFTPINK;
			SDL_Color want =
				(active || hover) ? COLOR_WHITE : COLOR_DURTYPINK;
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
		survey_fill.textcolor =
			gui_isInObject(&survey_border, ui->mx, ui->my) ? COLOR_WHITEPINK
														   : COLOR_WHITE;
		record_fill.textcolor =
			gui_isInObject(&record_border, ui->mx, ui->my) ? COLOR_WHITEPINK
														   : COLOR_WHITE;

		// 상태 텍스트
		gui_setColorText(&t_status, status_color);
		gui_setText(&t_status, status[0] ? status : " ");

		// ── 렌더링 ──
		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		gui_presentObject(&sidebar);
		gui_presentObject(&t_logo);
		for (int i = 0; i < NAV_CNT; i++) {
			int active = (i == NAV_HOME);
			int hover = gui_isInObject(&nav_box[i], ui->mx, ui->my);
			if (active || hover)
				gui_presentObject(&nav_box[i]);
			gui_presentObject(&nav_txt[i]);
		}

		gui_presentObject(&t_greet);
		gui_presentObject(&t_greetsub);

		gui_presentObject(&profile_card);
		gui_presentObject(&avatar);
		gui_presentObject(&t_name);
		gui_presentObject(&t_info1);
		gui_presentObject(&t_info2);

		gui_presentObject(&stat1_card);
		gui_presentObject(&t_stat1_label);
		gui_presentObject(&t_stat1_val);
		gui_presentObject(&stat2_card);
		gui_presentObject(&t_stat2_label);
		gui_presentObject(&t_stat2_val);

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

		gui_presentObject(&t_status);

		SDL_RenderPresent(ui->renderer);

		ui->is_mouse_up = false;
		ui->is_mouse_down = false;
		ui->is_mouse_move = false;

		if (ui->next_state != HOME) {
			break;
		}
	}
}
