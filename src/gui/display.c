#include "display.h"

enum Focus { FOCUS_NONE, FOCUS_ID, FOCUS_PW };

static char g_mst_partner_id[MAX_ID_LEN] = "";

static void display_copyText(char *dst, size_t dst_size, const char *src) {
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

static int display_parsePeopleLine(const char *line, People **out) {
	if (line == NULL || out == NULL) {
		return 0;
	}
	if (strncmp(line, "<<<<<<<", 7) == 0 || strncmp(line, "=======", 7) == 0 ||
		strncmp(line, ">>>>>>>", 7) == 0) {
		return 0;
	}

	cJSON *root = cJSON_Parse(line);
	if (root == NULL) {
		return 0;
	}

	cJSON *name = cJSON_GetObjectItem(root, "name");
	cJSON *id = cJSON_GetObjectItem(root, "id");
	cJSON *type = cJSON_GetObjectItem(root, "type");
	cJSON *love_type = cJSON_GetObjectItem(root, "love_type");
	cJSON *gen = cJSON_GetObjectItem(root, "gen");
	cJSON *age = cJSON_GetObjectItem(root, "age");

	if (!cJSON_IsString(name) || !cJSON_IsString(id) || !cJSON_IsNumber(gen) ||
		!cJSON_IsNumber(age)) {
		cJSON_Delete(root);
		return 0;
	}

	char type_buf[MAX_TYPE_LEN] = "";
	char love_type_buf[MAX_TYPE_LEN] = "";
	if (cJSON_IsString(type)) {
		display_copyText(type_buf, sizeof(type_buf), type->valuestring);
	}
	if (cJSON_IsString(love_type)) {
		display_copyText(love_type_buf, sizeof(love_type_buf),
						 love_type->valuestring);
	}

	People *person = people_create_people(
		name->valuestring, id->valuestring, "", type_buf, love_type_buf,
		gen->valueint == GENDER_FEMALE ? GENDER_FEMALE : GENDER_MALE,
		age->valueint);
	if (person != NULL) {
		people_set_people_status(person, AVAILABLE);
		*out = person;
	}

	cJSON_Delete(root);
	return person != NULL;
}

static int display_loadPeople(People *people[], int max_people) {
	FILE *f = fopen(DATA_PATH_PEOPLE, "r");
	if (f == NULL) {
		return 0;
	}

	int count = 0;
	char line[1024];
	while (count < max_people && fgets(line, sizeof(line), f) != NULL) {
		People *person = NULL;
		if (display_parsePeopleLine(line, &person)) {
			people[count++] = person;
		}
	}

	fclose(f);
	return count;
}

static void display_freePeopleList(People *people[], int count) {
	for (int i = 0; i < count; i++) {
		people_delete_people(people[i]);
	}
}

static People *display_findPeopleById(People *people[], int count,
									   const char id[]) {
	for (int i = 0; i < count; i++) {
		if (people[i] != NULL && strcmp(people[i]->id, id) == 0) {
			return people[i];
		}
	}
	return NULL;
}

static int display_recommendScore(People *me, People *other) {
	if (me == NULL || other == NULL) {
		return 0;
	}
	if (strlen(me->type) == 0 || strlen(me->love_type) == 0 ||
		strlen(other->type) == 0 || strlen(other->love_type) == 0) {
		int diff = me->age > other->age ? me->age - other->age
										: other->age - me->age;
		int score = 80 - diff * 3;
		return score < 10 ? 10 : score;
	}
	return compat(me, other);
}

static int display_compareCandidate(const void *a, const void *b) {
	const Candidate *x = (const Candidate *)a;
	const Candidate *y = (const Candidate *)b;

	if (x->score != y->score) {
		return y->score - x->score;
	}
	return x->idx - y->idx;
}

static int display_collectRecommendations(People *me, People *people[],
										  int people_count,
										  People *recommendations[],
										  int scores[], int max_result) {
	Candidate ranked[128];
	int ranked_count = 0;

	for (int i = 0; i < people_count && ranked_count < 128; i++) {
		People *candidate = people[i];
		if (candidate == NULL || strcmp(candidate->id, me->id) == 0) {
			continue;
		}
		if (candidate->gen == me->gen) {
			continue;
		}
		ranked[ranked_count].idx = i;
		ranked[ranked_count].score = display_recommendScore(me, candidate);
		ranked_count++;
	}

	qsort(ranked, ranked_count, sizeof(Candidate), display_compareCandidate);

	int result_count = ranked_count < max_result ? ranked_count : max_result;
	for (int i = 0; i < result_count; i++) {
		recommendations[i] = people[ranked[i].idx];
		scores[i] = ranked[i].score;
	}
	return result_count;
}

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
	People *logged_in = NULL;
	char id_buf[MAX_ID_LEN] = "";
	char pw_buf[MAX_PW_LEN] = "";
	enum Focus focus = FOCUS_NONE;
	char status[128] = " ";
	SDL_Color status_color = COLOR_GRAY;
	int cx = 700 + 400 / 2;

	g_mst_partner_id[0] = '\0';

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
	const int RIGHT = 1250;	   // 본문 오른쪽 끝 x 1250 - 280 = 970

	// ── 사이드바 ──
	Object sidebar = gui_initObject(
		ui, BOX, 0, 0, TOPLEFT,
		(ObjectParam){.box = {SIDEBAR_W, WINDOW_HEIGHT, COLOR_WHITEPINK, 0}});
	Object t_logo = gui_initObject(
		ui, TEXT, 110, 44, MIDTOP,
		(ObjectParam){.text = {"LIFO", ui->font_bbsig, COLOR_SUPERPINK}});

	// 네비게이션 항목 (0:홈 이 현재 화면)
	enum { NAV_HOME, NAV_BFS, NAV_DFS, NAV_MST, NAV_PROFILE, NAV_CNT };
	char *nav_labels[NAV_CNT] = {"홈", "매칭", "설문", "커플", "프로필"};
	int nav_y[NAV_CNT] = {150, 150 + 65, 150 + 65 * 2, 150 + 65 * 3,
						  150 + 65 * 4};
	Object nav_box[NAV_CNT + 1];
	Object nav_txt[NAV_CNT + 1];
	for (int i = 0; i < NAV_CNT; i++) {
		nav_box[i] = gui_initObject(
			ui, BOX, 125, nav_y[i], MIDTOP,
			(ObjectParam){.box = {SIDEBAR_W - 80, 51, COLOR_PINK, 14}});
		nav_txt[i] = gui_initObject(
			ui, TEXT, 72, nav_y[i] + 51 / 2, CENTER,
			(ObjectParam){.text = {" ", ui->font_normal, COLOR_DURTYPINK}});
		gui_setText(&nav_txt[i], nav_labels[i]);
	}
	nav_box[NAV_CNT] = gui_initObject(
		ui, BOX, 125, WINDOW_HEIGHT - 30, MIDBOTTOM,
		(ObjectParam){.box = {SIDEBAR_W - 80, 51, COLOR_PINK, 14}});
	nav_txt[NAV_CNT] = gui_initObject(
		ui, TEXT, 72, WINDOW_HEIGHT - 30 - 51 / 2, CENTER,
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
		(ObjectParam){.text = {"커플 공간", ui->font_normal,
							   COLOR_DURTYPINK}});
	Object t_recordsub = gui_initObject(
		ui, TEXT, card2_x + 26, 522, TOPLEFT,
		(ObjectParam){.text = {"공유 캘린더와 버킷리스트", ui->font_small,
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
				ui->next_state = MST;
				break;
			} else if (gui_isInObject(&nav_box[NAV_MST], ui->mx, ui->my)) {
				ui->next_state = MST;
				break;
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

		SDL_RenderPresent(ui->renderer);

		ui->is_mouse_up = false;
		ui->is_mouse_down = false;
		ui->is_mouse_move = false;
	}
}

void display_showBFS(SDL_Ui *ui, People *me) {
	if (me == NULL) {
		ui->quit = true;
		return;
	}

	const int SIDEBAR_W = 250;
	const int LEFT = 280;
	const int RIGHT = 1250;
	People *loaded_people[128];
	People *recommendations[4];
	int recommend_scores[4] = {0};
	int loaded_count = display_loadPeople(loaded_people, 128);
	int recommend_count =
		display_collectRecommendations(me, loaded_people, loaded_count,
									   recommendations, recommend_scores, 4);
	MatchRequest incoming_requests[4];
	People *incoming_people[4] = {NULL};
	int incoming_count =
		match_store_load_incoming(me->id, incoming_requests, 4);
	for (int i = 0; i < incoming_count; i++) {
		incoming_people[i] =
			display_findPeopleById(loaded_people, loaded_count,
								   incoming_requests[i].from_id);
	}
	char match_status[160] = "추천 카드로 매칭 신청을 보내세요.";
	char accepted_partner_id[MAX_ID_LEN] = "";
	int has_accepted_partner = match_store_find_accepted_partner(
		me->id, accepted_partner_id, sizeof(accepted_partner_id));
	People *accepted_partner =
		has_accepted_partner
			? display_findPeopleById(loaded_people, loaded_count,
									 accepted_partner_id)
			: NULL;
	if (has_accepted_partner) {
		display_copyText(g_mst_partner_id, sizeof(g_mst_partner_id),
						 accepted_partner_id);
		snprintf(match_status, sizeof(match_status), "%s 님과 매칭된 상태예요.",
				 accepted_partner != NULL ? accepted_partner->name
										  : accepted_partner_id);
	}

	Object sidebar = gui_initObject(
		ui, BOX, 0, 0, TOPLEFT,
		(ObjectParam){.box = {SIDEBAR_W, WINDOW_HEIGHT, COLOR_WHITEPINK, 0}});
	Object t_logo = gui_initObject(
		ui, TEXT, 110, 44, MIDTOP,
		(ObjectParam){.text = {"LIFO", ui->font_bbsig, COLOR_SUPERPINK}});

	// 사이드바
	enum { NAV_HOME, NAV_BFS, NAV_DFS, NAV_MST, NAV_PROFILE, NAV_CNT };
	char *nav_labels[NAV_CNT] = {"홈", "매칭", "설문", "커플", "프로필"};
	int nav_y[NAV_CNT] = {150, 150 + 65, 150 + 65 * 2, 150 + 65 * 3,
						  150 + 65 * 4};
	Object nav_box[NAV_CNT + 1];
	Object nav_txt[NAV_CNT + 1];
	for (int i = 0; i < NAV_CNT; i++) {
		nav_box[i] = gui_initObject(
			ui, BOX, 125, nav_y[i], MIDTOP,
			(ObjectParam){.box = {SIDEBAR_W - 80, 51, COLOR_PINK, 14}});
		nav_txt[i] = gui_initObject(
			ui, TEXT, 72, nav_y[i] + 51 / 2, CENTER,
			(ObjectParam){.text = {" ", ui->font_normal, COLOR_DURTYPINK}});
		gui_setText(&nav_txt[i], nav_labels[i]);
	}
	nav_box[NAV_CNT] = gui_initObject(
		ui, BOX, 125, WINDOW_HEIGHT - 30, MIDBOTTOM,
		(ObjectParam){.box = {SIDEBAR_W - 80, 51, COLOR_PINK, 14}});
	nav_txt[NAV_CNT] = gui_initObject(
		ui, TEXT, 72, WINDOW_HEIGHT - 30 - 51 / 2, CENTER,
		(ObjectParam){.text = {"로그아웃", ui->font_normal, COLOR_DURTYPINK}});

	// 메인 기능 화면
	Object t_greet =
		gui_initObject(ui, TEXT, LEFT, 36, TOPLEFT,
					   (ObjectParam){.text = {"Best Fit Stable-Matching",
											  ui->font_big, COLOR_BLACK}});
	Object t_match_status =
		gui_initObject(ui, TEXT, LEFT, 82, TOPLEFT,
					   (ObjectParam){.text = {"추천 카드로 매칭 신청을 보내세요.",
											  ui->font_small, COLOR_GRAY}});
	int matchbox_w = (RIGHT - LEFT - 20) / 2;
	int match_x2 = matchbox_w + LEFT + 20;
	Object chuchon_border = gui_initObject(
		ui, BOX, LEFT, 150, TOPLEFT,
		(ObjectParam){.box = {matchbox_w, 520, COLOR_SUPERPINK, 22}});
	Object chuchon_fill = gui_initObject(
		ui, BOX, LEFT + 2, 150 + 2, TOPLEFT,
		(ObjectParam){.box = {matchbox_w - 4, 520 - 4, COLOR_WHITEPINK, 22}});
	Object jean_border = gui_initObject(
		ui, BOX, match_x2, 150, TOPLEFT,
		(ObjectParam){.box = {matchbox_w, 520, COLOR_SUPERPINK, 22}});
	Object jean_fill = gui_initObject(
		ui, BOX, match_x2 + 2, 150 + 2, TOPLEFT,
		(ObjectParam){.box = {matchbox_w - 4, 520 - 4, COLOR_WHITEPINK, 20}});
	Object chuchon_text =
		gui_initObject(ui, TEXT, LEFT + 15, 122, TOPLEFT,
					   (ObjectParam){.text = {"알고리즘 추천 상대",
											  ui->font_normal, COLOR_BLACK}});
	Object jean_text =
		gui_initObject(ui, TEXT, match_x2 + 15, 122, TOPLEFT,
					   (ObjectParam){.text = {"나에게 온 제안", ui->font_normal,
											  COLOR_BLACK}});

	int c_x = LEFT + 15, j_x = match_x2 + 15;
	int prpl_y[4] = {180, 300, 420, 540};
	Object c_prpl[4][5];
	Object j_prpl[4][5];
	Object c_name[4];
	Object c_info[4];
	Object c_score[4];
	Object c_action[4];
	Object j_name[4];
	Object j_info[4];
	Object j_action[4];
	Object j_cancel[4];
	Object t_empty = gui_initObject(
		ui, TEXT, c_x + 22, 190, TOPLEFT,
		(ObjectParam){.text = {"추천 가능한 상대가 아직 없어요.",
							   ui->font_small, COLOR_GRAY}});
	Object t_hint = gui_initObject(
		ui, TEXT, j_x + 20, 190, TOPLEFT,
		(ObjectParam){.text = {"받은 제안이 아직 없어요.",
							   ui->font_small, COLOR_GRAY}});
	for (int i = 0; i < 4; i++) {
		c_prpl[i][0] = gui_initObject(
			ui, BOX, c_x, prpl_y[i], TOPLEFT,
			(ObjectParam){.box = {matchbox_w - 30, 100, COLOR_SOFTPINK, 15}});
		j_prpl[i][0] = gui_initObject(
			ui, BOX, j_x, prpl_y[i], TOPLEFT,
			(ObjectParam){.box = {matchbox_w - 30, 100, COLOR_SOFTPINK, 15}});

		c_name[i] = gui_initObject(
			ui, TEXT, c_x + 20, prpl_y[i] + 14, TOPLEFT,
			(ObjectParam){.text = {" ", ui->font_normal, COLOR_DURTYPINK}});
		c_info[i] = gui_initObject(
			ui, TEXT, c_x + 20, prpl_y[i] + 48, TOPLEFT,
			(ObjectParam){.text = {" ", ui->font_small, COLOR_GRAY}});
		c_score[i] = gui_initObject(
			ui, TEXT, c_x + matchbox_w - 110, prpl_y[i] + 18, TOPLEFT,
			(ObjectParam){.text = {" ", ui->font_small, COLOR_SUPERPINK}});
		c_action[i] = gui_initObject(
			ui, TEXT, c_x + 20, prpl_y[i] + 72, TOPLEFT,
			(ObjectParam){.text = {"매칭 신청 보내기", ui->font_small,
								   COLOR_SUPERPINK}});
		j_name[i] = gui_initObject(
			ui, TEXT, j_x + 20, prpl_y[i] + 14, TOPLEFT,
			(ObjectParam){.text = {" ", ui->font_normal, COLOR_DURTYPINK}});
		j_info[i] = gui_initObject(
			ui, TEXT, j_x + 20, prpl_y[i] + 48, TOPLEFT,
			(ObjectParam){.text = {" ", ui->font_small, COLOR_GRAY}});
		j_action[i] = gui_initObject(
			ui, TEXT, j_x + 20, prpl_y[i] + 72, TOPLEFT,
			(ObjectParam){.text = {"수락하고 커플 공간 열기", ui->font_small,
								   COLOR_SUPERPINK}});
		j_cancel[i] = gui_initObject(
			ui, TEXT, j_x + matchbox_w - 130, prpl_y[i] + 72, TOPLEFT,
			(ObjectParam){.text = {" ", ui->font_small, COLOR_VIOLET}});

		if (i < recommend_count) {
			char info[128];
			char score[32];
			gui_setText(&c_name[i], recommendations[i]->name);
			snprintf(info, sizeof(info), "%s · %d세 · %s", recommendations[i]->id,
					 recommendations[i]->age,
					 recommendations[i]->gen == GENDER_MALE ? "남성" : "여성");
			gui_setText(&c_info[i], info);
			snprintf(score, sizeof(score), "%d점", recommend_scores[i]);
			gui_setText(&c_score[i], score);
		}
		if (i < incoming_count) {
			char info[128];
			People *sender = incoming_people[i];
			if (sender != NULL) {
				gui_setText(&j_name[i], sender->name);
				snprintf(info, sizeof(info), "%s · %d세 · %s", sender->id,
						 sender->age,
						 sender->gen == GENDER_MALE ? "남성" : "여성");
			} else {
				gui_setText(&j_name[i], incoming_requests[i].from_id);
				snprintf(info, sizeof(info), "%s", incoming_requests[i].from_id);
			}
			gui_setText(&j_info[i], info);
		} else if (incoming_count == 0 && has_accepted_partner && i == 0) {
			char info[128];
			if (accepted_partner != NULL) {
				gui_setText(&j_name[i], accepted_partner->name);
				snprintf(info, sizeof(info), "%s · %d세 · %s",
						 accepted_partner->id, accepted_partner->age,
						 accepted_partner->gen == GENDER_MALE ? "남성" : "여성");
			} else {
				gui_setText(&j_name[i], accepted_partner_id);
				snprintf(info, sizeof(info), "%s", accepted_partner_id);
			}
			gui_setText(&j_info[i], info);
			gui_setText(&j_action[i], "이미 매칭됨 · 커플 공간 열기");
			gui_setText(&j_cancel[i], "매칭 취소");
		}
	}

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
			// todo if()if()DFS;

			if (gui_isInObject(&nav_box[NAV_HOME], ui->mx, ui->my)) {
				ui->next_state = HOME;
				break;
			} else if (gui_isInObject(&nav_box[NAV_DFS], ui->mx, ui->my)) {
				ui->next_state = DFS;
				break;
			} else if (gui_isInObject(&nav_box[NAV_MST], ui->mx, ui->my)) {
				ui->next_state = MST;
				break;
			} else if (gui_isInObject(&nav_box[NAV_PROFILE], ui->mx, ui->my)) {
				// ui->next_state = PROFILE;
				//  break;
			} else if (gui_isInObject(&nav_box[NAV_CNT], ui->mx, ui->my)) {
				ui->next_state = LOGIN;
				break;
			}

			for (int i = 0; i < recommend_count; i++) {
				if (gui_isInObject(&c_prpl[i][0], ui->mx, ui->my)) {
					int result =
						match_store_send_request(me->id, recommendations[i]->id);
					if (result == 1) {
						snprintf(match_status, sizeof(match_status),
								 "%s 님에게 매칭 신청을 보냈어요.",
								 recommendations[i]->name);
					} else if (result == 2) {
						snprintf(match_status, sizeof(match_status),
								 "%s 님에게 이미 신청을 보냈어요.",
								 recommendations[i]->name);
					} else if (result == 3) {
						snprintf(match_status, sizeof(match_status),
								 "%s 님과 이미 매칭됐어요.",
								 recommendations[i]->name);
					} else {
						strcpy(match_status, "매칭 신청을 저장하지 못했어요.");
					}
					gui_setText(&t_match_status, match_status);
					break;
				}
			}
			for (int i = 0; i < incoming_count; i++) {
				if (gui_isInObject(&j_prpl[i][0], ui->mx, ui->my)) {
					if (match_store_accept_request(incoming_requests[i].from_id,
												   me->id)) {
						display_copyText(g_mst_partner_id,
										 sizeof(g_mst_partner_id),
										 incoming_requests[i].from_id);
						ui->next_state = MST;
					} else {
						strcpy(match_status, "제안을 수락하지 못했어요.");
						gui_setText(&t_match_status, match_status);
					}
					break;
				}
			}
			if (incoming_count == 0 && has_accepted_partner &&
				gui_isInObject(&j_cancel[0], ui->mx, ui->my)) {
				if (match_store_cancel_match(me->id, accepted_partner_id)) {
					has_accepted_partner = 0;
					accepted_partner = NULL;
					accepted_partner_id[0] = '\0';
					g_mst_partner_id[0] = '\0';
					strcpy(match_status, "매칭을 취소했어요.");
					gui_setText(&t_match_status, match_status);
					gui_setText(&j_name[0], " ");
					gui_setText(&j_info[0], " ");
					gui_setText(&j_action[0], " ");
					gui_setText(&j_cancel[0], " ");
				} else {
					strcpy(match_status, "매칭을 취소하지 못했어요.");
					gui_setText(&t_match_status, match_status);
				}
				break;
			}
			if (incoming_count == 0 && has_accepted_partner &&
				gui_isInObject(&j_prpl[0][0], ui->mx, ui->my)) {
				display_copyText(g_mst_partner_id, sizeof(g_mst_partner_id),
								 accepted_partner_id);
				ui->next_state = MST;
				break;
			}
			if (ui->next_state != BFS) {
				break;
			}
		}

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
		gui_presentObject(&t_match_status);
		gui_presentObject(&jean_border);
		gui_presentObject(&chuchon_border);

		gui_presentObject(&jean_fill);
		gui_presentObject(&chuchon_fill);

		gui_presentObject(&jean_text);
		gui_presentObject(&chuchon_text);
		for (int i = 0; i < 4; i++) {
			if (i < recommend_count) {
				int hover = gui_isInObject(&c_prpl[i][0], ui->mx, ui->my);
				c_prpl[i][0].textcolor = hover ? COLOR_PINK : COLOR_SOFTPINK;
			}
			gui_presentObject(&c_prpl[i][0]);
			if (i < incoming_count) {
				int hover = gui_isInObject(&j_prpl[i][0], ui->mx, ui->my);
				j_prpl[i][0].textcolor = hover ? COLOR_PINK : COLOR_SOFTPINK;
			} else if (incoming_count == 0 && has_accepted_partner && i == 0) {
				int hover = gui_isInObject(&j_prpl[i][0], ui->mx, ui->my);
				j_prpl[i][0].textcolor = hover ? COLOR_PINK : COLOR_SOFTPINK;
			}
			gui_presentObject(&j_prpl[i][0]);
			if (i < recommend_count) {
				gui_presentObject(&c_name[i]);
				gui_presentObject(&c_info[i]);
				gui_presentObject(&c_score[i]);
				gui_presentObject(&c_action[i]);
			}
			if (i < incoming_count) {
				gui_presentObject(&j_name[i]);
				gui_presentObject(&j_info[i]);
				gui_presentObject(&j_action[i]);
			} else if (incoming_count == 0 && has_accepted_partner && i == 0) {
				int cancel_hover = gui_isInObject(&j_cancel[i], ui->mx, ui->my);
				gui_setColorText(&j_cancel[i],
								 cancel_hover ? COLOR_SUPERPINK : COLOR_VIOLET);
				gui_presentObject(&j_name[i]);
				gui_presentObject(&j_info[i]);
				gui_presentObject(&j_action[i]);
				gui_presentObject(&j_cancel[i]);
			}
		}
		if (recommend_count == 0) {
			gui_presentObject(&t_empty);
		}
		if (incoming_count == 0 && !has_accepted_partner) {
			gui_presentObject(&t_hint);
		}

		SDL_RenderPresent(ui->renderer);

		ui->is_mouse_up = false;
		ui->is_mouse_down = false;
		ui->is_mouse_move = false;

		if (ui->next_state != BFS) {
			break;
		}
	}
	display_freePeopleList(loaded_people, loaded_count);
}

// ───────────────────────────────────────────────
// 설문(DFS) 화면 — image 2 스타일
// 즉시 모드(immediate-mode) 렌더링 헬퍼들. TEXT는 매 프레임 텍스처를 만들고
// 바로 파괴하므로 누수가 없고, 둥근 BOX는 텍스처를 만들지 않으므로 안전하다.
// ───────────────────────────────────────────────

// 근데 읽어보니까 코드 씹창나있어서 걍 다 다시짜는게 나을수도;; 그냥 밑에
// 참고하는 느낌으로 위에 display_showHome()양식 맞춰서 다시 짜세요

static int s_inRect(int px, int py, int x, int y, int w, int h) {
	return px >= x && px <= x + w && py >= y && py <= y + h;
}

// 채운 사각형(직각).
static void s_drawRect(SDL_Ui *ui, int x, int y, int w, int h, SDL_Color c) {
	SDL_Rect r = {x, y, w, h};
	SDL_SetRenderDrawColor(ui->renderer, c.r, c.g, c.b, c.a);
	SDL_RenderFillRect(ui->renderer, &r);
}

// 둥근 사각형 (텍스처 없음 → 매 프레임 호출해도 안전).
static void s_drawRound(SDL_Ui *ui, int x, int y, int w, int h, int radius,
						SDL_Color c) {
	Object o = gui_initObject(ui, BOX, x, y, TOPLEFT,
							  (ObjectParam){.box = {w, h, c, radius}});
	gui_presentObject(&o);
}

// 즉시 모드 텍스트. wrap>0이면 그 폭으로 줄바꿈. 매 프레임 텍스처 생성/파괴.
static void s_drawText(SDL_Ui *ui, const char *text, TTF_Font *font,
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

// 공통 사이드바를 그린다. active: 현재 활성 메뉴 index (설문=2).
static void s_drawSidebar(SDL_Ui *ui, int active) {
	const char *labels[5] = {"홈", "매칭", "설문", "커플", "프로필"};
	int ny[5] = {150, 212, 274, 336, 398};
	s_drawRect(ui, 0, 0, 260, WINDOW_HEIGHT, COLOR_WHITEPINK);
	s_drawText(ui, "LIFO", ui->font_big, COLOR_SUPERPINK, 40, 44, TOPLEFT, 0);
	for (int i = 0; i < 5; i++) {
		int hover = s_inRect(ui->mx, ui->my, 30, ny[i], 200, 52);
		if (i == active) {
			s_drawRound(ui, 30, ny[i], 200, 52, 14, COLOR_PINK);
		} else if (hover) {
			s_drawRound(ui, 30, ny[i], 200, 52, 14, COLOR_SOFTPINK);
		}
		SDL_Color tc = (i == active || hover) ? COLOR_WHITE : COLOR_DURTYPINK;
		s_drawText(ui, labels[i], ui->font_normal, tc, 70, ny[i] + 13, TOPLEFT,
				   0);
	}
	int logout_y = WINDOW_HEIGHT - 82;
	int logout_hover = s_inRect(ui->mx, ui->my, 30, logout_y, 200, 52);
	s_drawRound(ui, 30, logout_y, 200, 52, 14,
				logout_hover ? COLOR_SOFTPINK : COLOR_PINK);
	s_drawText(ui, "로그아웃", ui->font_normal,
			   logout_hover ? COLOR_WHITE : COLOR_DURTYPINK, 70,
			   logout_y + 13, TOPLEFT, 0);
}

#define SV_MAIN_X 300
#define SV_RIGHT 1240

// 트리 하나를 설문으로 진행한다.
// 완료(잎 도달)하면 1을 반환하고 out_code/out_name을 채운다.
// 사용자가 취소(첫 질문에서 뒤로/홈 클릭)하거나 창을 닫으면 0을 반환한다.
static int display_runTree(SDL_Ui *ui, DfsTree *tree, const char *big_title,
						   char out_code[], char out_name[]) {
	int current = tree->root;
	int sel = -1;
	int stack_node[DFS_TREE_MAX_NODES];
	int stack_sel[DFS_TREE_MAX_NODES];
	int sp = 0; // 답한 질문 수 = 현재 스텝 index

	char status[128] = " ";

	// 옵션/버튼 레이아웃
	const int OY0 = 330, OH = 64, OGAP = 76;

	while (!ui->quit) {
		DfsTreeNode *node = &tree->nodes[current];
		int n_opt = node->n_opt;
		int by = OY0 + n_opt * OGAP + 16; // 버튼 y
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
					ui->is_mouse_down = true; // 아래 '다음' 로직 재사용
					ui->mx = next_x + 1;
					ui->my = by + 1;
				} else if (k == SDLK_ESCAPE) {
					ui->is_mouse_down = true; // '이전'
					ui->mx = prev_x + 1;
					ui->my = by + 1;
				}
				break;
			}
			}
		}

		if (ui->is_mouse_down) {
			// 사이드바 '홈' 클릭 → 설문 취소
			if (s_inRect(ui->mx, ui->my, 30, 150, 200, 52)) {
				return 0;
			} else if (s_inRect(ui->mx, ui->my, 30, WINDOW_HEIGHT - 82, 200,
								52)) {
				ui->next_state = LOGIN;
				return 0;
			}
			// 옵션 선택
			for (int i = 0; i < n_opt; i++) {
				int oy = OY0 + i * OGAP;
				if (s_inRect(ui->mx, ui->my, SV_MAIN_X, oy, 940, OH)) {
					sel = i;
				}
			}
			// 이전
			if (s_inRect(ui->mx, ui->my, prev_x, by, prev_w, 56)) {
				if (sp == 0) {
					return 0; // 첫 질문에서 뒤로 → 취소
				}
				sp--;
				current = stack_node[sp];
				sel = stack_sel[sp];
				ui->is_mouse_down = false;
				continue;
			}
			// 다음
			if (s_inRect(ui->mx, ui->my, next_x, by, next_w, 56)) {
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

		// ── 렌더링 ──
		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);

		s_drawSidebar(ui, 2); // 설문 활성

		// 제목 + 진행 표시
		s_drawText(ui, big_title, ui->font_big, COLOR_BLACK, SV_MAIN_X, 28,
				   TOPLEFT, 0);
		char sub[96];
		snprintf(sub, sizeof(sub), "%s 트리", tree->title);
		s_drawText(ui, sub, ui->font_small, COLOR_GRAY, SV_MAIN_X + 2, 82,
				   TOPLEFT, 0);

		// 질문 카드
		s_drawRound(ui, SV_MAIN_X, 150, 940, 150, 20, COLOR_WHITEPINK);
		s_drawText(ui, "Q.", ui->font_small, COLOR_SUPERPINK, SV_MAIN_X + 32,
				   174, TOPLEFT, 0);
		s_drawText(ui, node->question, ui->font_normal, COLOR_BLACK,
				   SV_MAIN_X + 32, 208, TOPLEFT, 876);

		// 선택지
		for (int i = 0; i < n_opt; i++) {
			int oy = OY0 + i * OGAP;
			int selected = (sel == i);
			int hover = s_inRect(ui->mx, ui->my, SV_MAIN_X, oy, 940, OH);
			SDL_Color border = selected ? COLOR_SUPERPINK : COLOR_PINK;
			SDL_Color fill =
				(selected || hover) ? COLOR_WHITEPINK : COLOR_WHITE;
			s_drawRound(ui, SV_MAIN_X, oy, 940, OH, 14, border);
			s_drawRound(ui, SV_MAIN_X + 2, oy + 2, 936, OH - 4, 12, fill);
			// 라디오
			int rcx = SV_MAIN_X + 40, rcy = oy + OH / 2;
			s_drawRound(ui, rcx - 13, rcy - 13, 26, 26, 13, border);
			s_drawRound(ui, rcx - 10, rcy - 10, 20, 20, 10, COLOR_WHITE);
			if (selected) {
				s_drawRound(ui, rcx - 6, rcy - 6, 12, 12, 6, COLOR_SUPERPINK);
			}
			s_drawText(ui, node->opt_text[i], ui->font_small,
					   selected ? COLOR_DURTYPINK : COLOR_GRAY, SV_MAIN_X + 72,
					   oy + 18, TOPLEFT, 800);
		}

		// 이전 / 다음 버튼
		int prev_hover = s_inRect(ui->mx, ui->my, prev_x, by, prev_w, 56);
		s_drawRound(ui, prev_x, by, prev_w, 56, 14, COLOR_SOFTPINK);
		s_drawRound(ui, prev_x + 2, by + 2, prev_w - 4, 52, 12,
					prev_hover ? COLOR_WHITEPINK : COLOR_WHITE);
		s_drawText(ui, "← 이전", ui->font_small, COLOR_GRAY,
				   prev_x + prev_w / 2, by + 28, CENTER, 0);

		int next_hover = s_inRect(ui->mx, ui->my, next_x, by, next_w, 56);
		s_drawRound(ui, next_x, by, next_w, 56, 14,
					next_hover ? COLOR_DURTYPINK : COLOR_SUPERPINK);
		s_drawText(ui, "다음 →", ui->font_normal, COLOR_WHITE,
				   next_x + next_w / 2, by + 28, CENTER, 0);

		// 상태 문구
		s_drawText(ui, status, ui->font_small, COLOR_SUPERPINK, SV_MAIN_X,
				   by + 70, TOPLEFT, 0);

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
	return 0;
}

// 설문 하나(대주제 트리 묶음)를 순서대로 진행한다.
// 모든 트리를 끝내면 1, 취소/창닫기면 0. 각 트리 결과를 codes/names에 채운다.
static int display_runSurvey(SDL_Ui *ui, DfsSurvey *survey,
							 char codes[][MAX_TYPE_LEN],
							 char names[][DFS_NAME_LEN]) {
	int i = 0;
	while (i < survey->n_trees) {
		char big[80];
		snprintf(big, sizeof(big), "%s 진단", survey->name);
		int r = display_runTree(ui, &survey->trees[i], big, codes[i], names[i]);
		if (ui->quit) {
			return 0;
		}
		if (r == 0) {
			if (i == 0) {
				return 0; // 첫 대주제의 첫 질문에서 뒤로 → 설문 취소
			}
			i--; // 이전 대주제로 되돌아가기
			continue;
		}
		i++;
	}
	return 1;
}

// 라벨 + 입력 박스를 그린다 (즉시 모드).
static void s_drawInput(SDL_Ui *ui, int x, int y, int w, int h, int focused,
						const char *label, const char *buf,
						const char *placeholder) {
	s_drawText(ui, label, ui->font_small, COLOR_DURTYPINK, x, y - 28, TOPLEFT,
			   0);
	s_drawRound(ui, x, y, w, h, 12, focused ? COLOR_SUPERPINK : COLOR_PINK);
	s_drawRound(ui, x + 2, y + 2, w - 4, h - 4, 10, COLOR_WHITEPINK);
	if (buf[0] == '\0' && !focused) {
		s_drawText(ui, placeholder, ui->font_small, COLOR_WHITEGRAY, x + 16,
				   y + 16, TOPLEFT, w - 32);
	} else {
		char disp[DFS_Q_LEN + 4];
		snprintf(disp, sizeof(disp), "%s%s", buf, focused ? "_" : "");
		s_drawText(ui, disp, ui->font_small, COLOR_GRAY, x + 16, y + 16,
				   TOPLEFT, w - 32);
	}
}

static void s_appendText(char *buf, size_t buf_size, const char *text) {
	if (buf == NULL || text == NULL) {
		return;
	}
	if (strlen(buf) + strlen(text) < buf_size - 1) {
		strcat(buf, text);
	}
}

static void s_openMstSpace(MstCoupleSpace *space, People *me,
						   const char partner_buf[], int *opened,
						   char status[], size_t status_size) {
	if (partner_buf[0] == '\0') {
		snprintf(status, status_size, "수락된 매칭이 있어야 사용할 수 있어요.");
		return;
	}

	int existed = mst_load_space(space, me->id, partner_buf);
	if (!existed) {
		mst_init_space(space, me->id, partner_buf);
		mst_save_space(space);
	}
	display_copyText(g_mst_partner_id, sizeof(g_mst_partner_id), partner_buf);
	*opened = 1;
	snprintf(status, status_size, existed ? "커플 공간을 불러왔어요."
										  : "새 커플 공간을 만들었어요.");
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

	MstCoupleSpace space;
	char partner_buf[MAX_ID_LEN] = "";
	char event_date[MST_DATE_LEN] = "";
	char event_title[MST_TEXT_LEN] = "";
	char bucket_title[MST_TEXT_LEN] = "";
	char status[160] = "";
	int focus = MST_FOCUS_EVENT_DATE;
	int opened = 0;

	mst_init_space(&space, me->id, "");
	if (g_mst_partner_id[0] != '\0' &&
		match_store_has_accepted_match(me->id, g_mst_partner_id)) {
		display_copyText(partner_buf, sizeof(partner_buf), g_mst_partner_id);
	} else if (match_store_find_accepted_partner(me->id, partner_buf,
												 sizeof(partner_buf))) {
		display_copyText(g_mst_partner_id, sizeof(g_mst_partner_id),
						 partner_buf);
	} else {
		g_mst_partner_id[0] = '\0';
	}

	if (partner_buf[0] != '\0') {
		s_openMstSpace(&space, me, partner_buf, &opened, status,
					   sizeof(status));
	} else {
		strcpy(status, "아직 수락된 매칭이 없어요. 매칭 화면에서 제안을 수락해주세요.");
		focus = MST_FOCUS_NONE;
	}

	const int event_x = SV_MAIN_X;
	const int bucket_x = 790;
	const int panel_y = 248;
	const int panel_w = 450;
	const int panel_h = 390;
	const int input_y = 332;
	const int add_y = 408;
	const int list_y = 490;

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
				if (focus == MST_FOCUS_EVENT_DATE) {
					s_appendText(event_date, sizeof(event_date),
								 event.text.text);
				} else if (focus == MST_FOCUS_EVENT_TITLE) {
					s_appendText(event_title, sizeof(event_title),
								 event.text.text);
				} else if (focus == MST_FOCUS_BUCKET) {
					s_appendText(bucket_title, sizeof(bucket_title),
								 event.text.text);
				}
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_BACKSPACE) {
					if (focus == MST_FOCUS_EVENT_DATE) {
						gui_utf8Backspace(event_date);
					} else if (focus == MST_FOCUS_EVENT_TITLE) {
						gui_utf8Backspace(event_title);
					} else if (focus == MST_FOCUS_BUCKET) {
						gui_utf8Backspace(bucket_title);
					}
				} else if (event.key.keysym.sym == SDLK_TAB) {
					focus = (focus == MST_FOCUS_NONE ||
							 focus >= MST_FOCUS_BUCKET)
								? MST_FOCUS_EVENT_DATE
								: focus + 1;
				} else if (event.key.keysym.sym == SDLK_RETURN) {
					if (focus == MST_FOCUS_EVENT_DATE ||
						focus == MST_FOCUS_EVENT_TITLE) {
						if (!opened) {
							strcpy(status,
								   "수락된 매칭이 있어야 사용할 수 있어요.");
						} else if (mst_add_event(&space, event_date,
												 event_title)) {
							mst_save_space(&space);
							event_date[0] = '\0';
							event_title[0] = '\0';
							strcpy(status, "일정을 추가했어요.");
						} else {
							strcpy(status, "날짜와 일정 제목을 모두 입력해주세요.");
						}
					} else if (focus == MST_FOCUS_BUCKET) {
						if (!opened) {
							strcpy(status,
								   "수락된 매칭이 있어야 사용할 수 있어요.");
						} else if (mst_add_bucket_item(&space, bucket_title)) {
							mst_save_space(&space);
							bucket_title[0] = '\0';
							strcpy(status, "버킷리스트를 추가했어요.");
						} else {
							strcpy(status, "버킷리스트 내용을 입력해주세요.");
						}
					}
				} else if (event.key.keysym.sym == SDLK_ESCAPE) {
					ui->next_state = HOME;
				}
				break;
			}
		}

		if (ui->is_mouse_down) {
			if (s_inRect(ui->mx, ui->my, 30, 150, 200, 52)) {
				ui->next_state = HOME;
				break;
			} else if (s_inRect(ui->mx, ui->my, 30, 212, 200, 52)) {
				ui->next_state = BFS;
				break;
			} else if (s_inRect(ui->mx, ui->my, 30, 274, 200, 52)) {
				ui->next_state = DFS;
				break;
			} else if (s_inRect(ui->mx, ui->my, 30, WINDOW_HEIGHT - 82, 200,
								52)) {
				ui->next_state = LOGIN;
				break;
			}

			if (s_inRect(ui->mx, ui->my, event_x + 28, input_y, 130, 52)) {
				focus = MST_FOCUS_EVENT_DATE;
			} else if (s_inRect(ui->mx, ui->my, event_x + 172, input_y, 220,
								52)) {
				focus = MST_FOCUS_EVENT_TITLE;
			} else if (s_inRect(ui->mx, ui->my, event_x + 28, add_y, 170, 52)) {
				if (!opened) {
					strcpy(status, "수락된 매칭이 있어야 사용할 수 있어요.");
				} else if (mst_add_event(&space, event_date, event_title)) {
					mst_save_space(&space);
					event_date[0] = '\0';
					event_title[0] = '\0';
					strcpy(status, "일정을 추가했어요.");
				} else {
					strcpy(status, "날짜와 일정 제목을 모두 입력해주세요.");
				}
			} else if (s_inRect(ui->mx, ui->my, bucket_x + 28, input_y, 260,
								52)) {
				focus = MST_FOCUS_BUCKET;
			} else if (s_inRect(ui->mx, ui->my, bucket_x + 28, add_y, 170,
								52)) {
				if (!opened) {
					strcpy(status, "수락된 매칭이 있어야 사용할 수 있어요.");
				} else if (mst_add_bucket_item(&space, bucket_title)) {
					mst_save_space(&space);
					bucket_title[0] = '\0';
					strcpy(status, "버킷리스트를 추가했어요.");
				} else {
					strcpy(status, "버킷리스트 내용을 입력해주세요.");
				}
			} else {
				for (int i = 0; i < space.bucket_count && i < 5; i++) {
					if (s_inRect(ui->mx, ui->my, bucket_x + 28,
								 list_y + i * 34, 380, 28)) {
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
		s_drawSidebar(ui, 3);

		s_drawText(ui, "커플 공간", ui->font_big, COLOR_BLACK, SV_MAIN_X, 36,
				   TOPLEFT, 0);
		s_drawText(ui, "수락된 매칭 상대와 일정과 버킷리스트를 관리해요.",
				   ui->font_small, COLOR_GRAY, SV_MAIN_X + 2, 92, TOPLEFT, 0);

		s_drawRound(ui, SV_MAIN_X, 138, 660, 76, 18, COLOR_WHITEPINK);
		s_drawText(ui, "매칭 상대", ui->font_small, COLOR_DURTYPINK,
				   SV_MAIN_X + 24, 154, TOPLEFT, 0);
		s_drawText(ui, opened ? partner_buf : "수락된 매칭 없음",
				   ui->font_normal, opened ? COLOR_SUPERPINK : COLOR_GRAY,
				   SV_MAIN_X + 24, 182, TOPLEFT, 0);
		if (!opened) {
			s_drawText(ui, "매칭 화면에서 요청을 수락하면 자동으로 표시돼요.",
					   ui->font_small, COLOR_GRAY, SV_MAIN_X + 260, 184,
					   TOPLEFT, 360);
		}
		s_drawText(ui, status, ui->font_small, COLOR_SUPERPINK, SV_MAIN_X,
				   214, TOPLEFT, 0);

		s_drawRound(ui, event_x, panel_y, panel_w, panel_h, 20,
					COLOR_WHITEPINK);
		s_drawText(ui, "공유 캘린더", ui->font_normal, COLOR_SUPERPINK,
				   event_x + 28, panel_y + 24, TOPLEFT, 0);
		s_drawInput(ui, event_x + 28, input_y, 130, 52,
					focus == MST_FOCUS_EVENT_DATE, "날짜", event_date,
					"06/13");
		s_drawInput(ui, event_x + 172, input_y, 220, 52,
					focus == MST_FOCUS_EVENT_TITLE, "일정", event_title,
					"영화 보기");
		int event_hover = s_inRect(ui->mx, ui->my, event_x + 28, add_y, 170,
								   52);
		s_drawRound(ui, event_x + 28, add_y, 170, 52, 14,
					event_hover ? COLOR_DURTYPINK : COLOR_SUPERPINK);
		s_drawText(ui, "일정 추가", ui->font_small, COLOR_WHITE,
				   event_x + 113, add_y + 26, CENTER, 0);

		if (space.event_count == 0) {
			s_drawText(ui, "아직 등록된 일정이 없어요.", ui->font_small,
					   COLOR_GRAY, event_x + 28, list_y, TOPLEFT, panel_w - 56);
		}
		for (int i = 0; i < space.event_count && i < 5; i++) {
			char line[160];
			snprintf(line, sizeof(line), "%s  %s", space.events[i].date,
					 space.events[i].title);
			s_drawText(ui, line, ui->font_small, COLOR_DURTYPINK,
					   event_x + 28, list_y + i * 34, TOPLEFT, panel_w - 56);
		}

		s_drawRound(ui, bucket_x, panel_y, panel_w, panel_h, 20,
					COLOR_WHITEVIOLET);
		s_drawText(ui, "버킷리스트", ui->font_normal, COLOR_VIOLET,
				   bucket_x + 28, panel_y + 24, TOPLEFT, 0);
		s_drawInput(ui, bucket_x + 28, input_y, 260, 52,
					focus == MST_FOCUS_BUCKET, "하고 싶은 것", bucket_title,
					"한강 피크닉");
		int bucket_hover = s_inRect(ui->mx, ui->my, bucket_x + 28, add_y, 170,
									52);
		s_drawRound(ui, bucket_x + 28, add_y, 170, 52, 14,
					bucket_hover ? COLOR_DURTYPINK : COLOR_VIOLET);
		s_drawText(ui, "버킷 추가", ui->font_small, COLOR_WHITE,
				   bucket_x + 113, add_y + 26, CENTER, 0);

		if (space.bucket_count == 0) {
			s_drawText(ui, "아직 등록된 버킷리스트가 없어요.", ui->font_small,
					   COLOR_GRAY, bucket_x + 28, list_y, TOPLEFT,
					   panel_w - 56);
		}
		for (int i = 0; i < space.bucket_count && i < 5; i++) {
			char line[160];
			int hover = s_inRect(ui->mx, ui->my, bucket_x + 28,
								 list_y + i * 34, 380, 28);
			snprintf(line, sizeof(line), "%s %s",
					 space.buckets[i].done ? "[x]" : "[ ]",
					 space.buckets[i].title);
			s_drawText(ui, line, ui->font_small,
					   hover ? COLOR_SUPERPINK : COLOR_SOFTVIOLET,
					   bucket_x + 28, list_y + i * 34, TOPLEFT, panel_w - 56);
		}

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
}

// 유형(잎) 세분화 입력 화면.
// 질문 + 선택지 2개를 입력받아 트리를 확장·파일 저장하고, 본인이 고른 쪽의
// 새 하위 유형을 out_code/out_name으로 돌려준다. 추가 성공 시 1, 취소면 0.
static int display_showAddQuestion(SDL_Ui *ui, DfsTree *tree, int leaf_idx,
								   char out_code[], char out_name[]) {
	char q_buf[DFS_Q_LEN] = "";
	char o0[DFS_OPT_LEN] = "";
	char o1[DFS_OPT_LEN] = "";
	int focus = 0; // 0 none, 1 질문, 2 선택지1, 3 선택지2
	int mine = -1; // 본인에 해당하는 선택지 (0/1)
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
			if (s_inRect(ui->mx, ui->my, 30, 150, 200, 52)) {
				return 0; // 홈 nav → 취소
			} else if (s_inRect(ui->mx, ui->my, 30, WINDOW_HEIGHT - 82, 200,
								52)) {
				ui->next_state = LOGIN;
				return 0;
			}
			if (s_inRect(ui->mx, ui->my, 300, QY, 940, 56)) {
				focus = 1;
			} else if (s_inRect(ui->mx, ui->my, 300, O0Y, 720, 56)) {
				focus = 2;
			} else if (s_inRect(ui->mx, ui->my, 300, O1Y, 720, 56)) {
				focus = 3;
			} else if (s_inRect(ui->mx, ui->my, RADIO_X, O0Y, RADIO_W, 56)) {
				mine = 0;
				focus = 0;
			} else if (s_inRect(ui->mx, ui->my, RADIO_X, O1Y, RADIO_W, 56)) {
				mine = 1;
				focus = 0;
			} else if (s_inRect(ui->mx, ui->my, SAVE_X, BTNY, SAVE_W, 56)) {
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
			} else if (s_inRect(ui->mx, ui->my, CANCEL_X, BTNY, CANCEL_W, 56)) {
				return 0;
			} else {
				focus = 0;
			}
		}

		// ── 렌더링 ──
		SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
		SDL_RenderClear(ui->renderer);
		s_drawSidebar(ui, 2);

		s_drawText(ui, "유형 더 세분화하기", ui->font_big, COLOR_BLACK,
				   SV_MAIN_X, 36, TOPLEFT, 0);
		char sub[160];
		snprintf(sub, sizeof(sub), "'%s' 유형을 새 질문으로 두 갈래로 나눕니다",
				 pname);
		s_drawText(ui, sub, ui->font_small, COLOR_GRAY, SV_MAIN_X + 2, 92,
				   TOPLEFT, 0);

		s_drawInput(ui, 300, QY, 940, 56, focus == 1, "추가할 질문", q_buf,
					"예: 주말엔 주로 뭐 해?");
		s_drawInput(ui, 300, O0Y, 720, 56, focus == 2, "선택지 1", o0,
					"예: 집에서 쉰다");
		s_drawInput(ui, 300, O1Y, 720, 56, focus == 3, "선택지 2", o1,
					"예: 밖에서 논다");

		// '내 유형' 라디오 버튼 (각 선택지 옆)
		for (int k = 0; k < 2; k++) {
			int ry = (k == 0) ? O0Y : O1Y;
			int sel = (mine == k);
			int hov = s_inRect(ui->mx, ui->my, RADIO_X, ry, RADIO_W, 56);
			s_drawRound(ui, RADIO_X, ry, RADIO_W, 56, 12,
						sel ? COLOR_DURTYPINK : COLOR_PINK);
			s_drawRound(ui, RADIO_X + 2, ry + 2, RADIO_W - 4, 52, 10,
						sel ? COLOR_SUPERPINK
							: (hov ? COLOR_WHITEPINK : COLOR_WHITE));
			s_drawText(ui, "내 유형", ui->font_small,
					   sel ? COLOR_WHITE : COLOR_GRAY, RADIO_X + RADIO_W / 2,
					   ry + 28, CENTER, 0);
		}

		// 저장 / 취소
		int sh = s_inRect(ui->mx, ui->my, SAVE_X, BTNY, SAVE_W, 56);
		s_drawRound(ui, SAVE_X, BTNY, SAVE_W, 56, 14,
					sh ? COLOR_DURTYPINK : COLOR_SUPERPINK);
		s_drawText(ui, "저장", ui->font_normal, COLOR_WHITE,
				   SAVE_X + SAVE_W / 2, BTNY + 28, CENTER, 0);
		int ch = s_inRect(ui->mx, ui->my, CANCEL_X, BTNY, CANCEL_W, 56);
		s_drawRound(ui, CANCEL_X, BTNY, CANCEL_W, 56, 14, COLOR_SOFTPINK);
		s_drawRound(ui, CANCEL_X + 2, BTNY + 2, CANCEL_W - 4, 52, 12,
					ch ? COLOR_WHITEPINK : COLOR_WHITE);
		s_drawText(ui, "취소", ui->font_small, COLOR_GRAY,
				   CANCEL_X + CANCEL_W / 2, BTNY + 28, CENTER, 0);

		s_drawText(ui, status, ui->font_small, COLOR_SUPERPINK, SV_MAIN_X,
				   BTNY + 72, TOPLEFT, 0);

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
	return 0;
}

// 진단 결과(대주제별)를 모두 보여주는 마무리 화면. '홈으로' 클릭 시 반환.
static void display_showSurveyResult(SDL_Ui *ui, People *me, DfsSurvey *self_s,
									 char self_codes[][MAX_TYPE_LEN],
									 char self_names[][DFS_NAME_LEN],
									 DfsSurvey *ideal_s,
									 char ideal_codes[][MAX_TYPE_LEN],
									 char ideal_names[][DFS_NAME_LEN]) {
	int bx = SV_MAIN_X, bw = 200, byy = 520;
	int ex_x = 540, ex_w = 260; // '세분화' 버튼
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
			if (s_inRect(ui->mx, ui->my, bx, byy, bw, 56) ||
				s_inRect(ui->mx, ui->my, 30, 150, 200, 52)) {
				return; // 홈으로
			}
			if (s_inRect(ui->mx, ui->my, 30, WINDOW_HEIGHT - 82, 200, 52)) {
				ui->next_state = LOGIN;
				return;
			}
			// 내 성향 대표 유형 세분화
			if (s_inRect(ui->mx, ui->my, ex_x, byy, ex_w, 56)) {
				int li =
					dfs_find_leaf_by_code(&self_s->trees[0], self_codes[0]);
				if (li >= 0) {
					char nc[MAX_TYPE_LEN], nn[DFS_NAME_LEN];
					if (display_showAddQuestion(ui, &self_s->trees[0], li, nc,
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
		s_drawSidebar(ui, 2);

		s_drawText(ui, "진단이 완료됐어요", ui->font_big, COLOR_BLACK,
				   SV_MAIN_X, 36, TOPLEFT, 0);
		s_drawText(
			ui,
			"대주제별 결과예요. 매칭 유사도는 이 결과들을 합쳐 계산할 예정.",
			ui->font_small, COLOR_GRAY, SV_MAIN_X + 2, 92, TOPLEFT, 0);

		char line[160];
		// 내 성향 카드
		s_drawRound(ui, SV_MAIN_X, 150, 940, 160, 20, COLOR_WHITEPINK);
		s_drawText(ui, "내 연애 성향", ui->font_normal, COLOR_SUPERPINK,
				   SV_MAIN_X + 30, 172, TOPLEFT, 0);
		for (int i = 0; i < self_s->n_trees; i++) {
			snprintf(line, sizeof(line), "· %s : %s (%s)",
					 self_s->trees[i].title, self_names[i], self_codes[i]);
			s_drawText(ui, line, ui->font_small, COLOR_DURTYPINK,
					   SV_MAIN_X + 40, 216 + i * 36, TOPLEFT, 860);
		}

		// 이상형 카드
		s_drawRound(ui, SV_MAIN_X, 330, 940, 160, 20, COLOR_WHITEVIOLET);
		s_drawText(ui, "내 이상형", ui->font_normal, COLOR_VIOLET,
				   SV_MAIN_X + 30, 352, TOPLEFT, 0);
		for (int i = 0; i < ideal_s->n_trees; i++) {
			snprintf(line, sizeof(line), "· %s : %s (%s)",
					 ideal_s->trees[i].title, ideal_names[i], ideal_codes[i]);
			s_drawText(ui, line, ui->font_small, COLOR_SOFTVIOLET,
					   SV_MAIN_X + 40, 396 + i * 36, TOPLEFT, 860);
		}

		// 홈으로 버튼
		int hov = s_inRect(ui->mx, ui->my, bx, byy, bw, 56);
		s_drawRound(ui, bx, byy, bw, 56, 14,
					hov ? COLOR_DURTYPINK : COLOR_SUPERPINK);
		s_drawText(ui, "홈으로 →", ui->font_normal, COLOR_WHITE, bx + bw / 2,
				   byy + 28, CENTER, 0);

		// 내 성향 세분화 버튼
		int eh = s_inRect(ui->mx, ui->my, ex_x, byy, ex_w, 56);
		s_drawRound(ui, ex_x, byy, ex_w, 56, 14, COLOR_SOFTPINK);
		s_drawRound(ui, ex_x + 2, byy + 2, ex_w - 4, 52, 12,
					eh ? COLOR_WHITEPINK : COLOR_WHITE);
		s_drawText(ui, "내 성향 세분화하기", ui->font_small, COLOR_DURTYPINK,
				   ex_x + ex_w / 2, byy + 28, CENTER, 0);

		SDL_RenderPresent(ui->renderer);
		ui->is_mouse_down = false;
		ui->is_mouse_up = false;
		ui->is_mouse_move = false;
	}
}

void display_showSurvey(SDL_Ui *ui, People *me) {
	if (me == NULL) {
		ui->next_state = HOME;
		return;
	}

	// 설문 구조체가 커서 힙에 둔다 (스택 오버플로 방지).
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

	// 1) 내 성향 진단 (대주제 트리들)
	if (!display_runSurvey(ui, self_s, self_codes, self_names)) {
		goto done;
	}
	// 현재는 대표 트리(0번) 코드만 type에 저장 — 매칭 유사도 일반화는 추후.
	strncpy(me->type, self_codes[0], MAX_TYPE_LEN - 1);
	me->type[MAX_TYPE_LEN - 1] = '\0';

	// 2) 내 이상형 진단
	if (!display_runSurvey(ui, ideal_s, ideal_codes, ideal_names)) {
		goto done; // 성향은 이미 저장됨
	}
	strncpy(me->love_type, ideal_codes[0], MAX_TYPE_LEN - 1);
	me->love_type[MAX_TYPE_LEN - 1] = '\0';

	// 3) 결과 화면
	display_showSurveyResult(ui, me, self_s, self_codes, self_names, ideal_s,
							 ideal_codes, ideal_names);

done: //?????????
	free(self_s);
	free(ideal_s);
	if (ui->next_state != LOGIN) {
		ui->next_state = HOME;
	}
}
