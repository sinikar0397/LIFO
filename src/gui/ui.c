#include "ui.h"

int gui_initUi(SDL_Ui *ui) {
	if (SDL_Init(SDL_INIT_EVENTS) != 0) {
		fprintf(stderr, "%s\n", (SDL_GetError()));
		return (0);
	}

	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		// printf("SDL_image could not initialize! SDL_image Error: %s\n",
		//    IMG_GetError());
		return (0);
	}

	if (TTF_Init() != 0) {
		// printf("TTF Init Error: %s\n", TTF_GetError());
		return (0);
	}

	// Create an SDL window
	ui->window =
		SDL_CreateWindow("Love Is Found Optimally", SDL_WINDOWPOS_UNDEFINED,
						 SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT,
						 SDL_WINDOW_OPENGL);
	if (ui->window == 0) {
		fprintf(stderr, "%s\n", (SDL_GetError()));
		return (0);
	}

	// Create a renderer (accelerated and in sync with the display refresh rate)
	ui->renderer = SDL_CreateRenderer(
		ui->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ui->renderer == 0) {
		fprintf(stderr, "%s\n", (SDL_GetError()));
		return (0);
	}

	// Create a font
	ui->font_ssmall = TTF_OpenFont("gulsi.ttf", FONT_SIZE_SSMALL);
	ui->font_small = TTF_OpenFont("gulsi.ttf", FONT_SIZE_SMALL);
	ui->font_normal = TTF_OpenFont("gulsi.ttf", FONT_SIZE_NORMAL);
	ui->font_big = TTF_OpenFont("gulsi.ttf", FONT_SIZE_BIG);
	ui->font_bbig = TTF_OpenFont("gulsi.ttf", FONT_SIZE_BBIG);
	ui->font_bbsig = TTF_OpenFont("gulsi.ttf", FONT_SIZE_BBSIG);

	ui->quit = false;
	ui->next_state = LOGIN;
	ui->is_mouse_down = false;
	ui->is_mouse_up = false;
	ui->is_mouse_move = false;
	strcpy(ui->input_buf, " ");

	return (1);
}

void gui_closeUi(SDL_Ui *ui) {
	SDL_DestroyRenderer(ui->renderer);
	SDL_DestroyWindow(ui->window);

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

Object gui_initObject(SDL_Ui *ui, ObjectTypeEnum objtype, int x, int y,
					  AnchorEnum anchor, ObjectParam param) {
	Object obj;
	obj.ui = ui;
	obj.anchor = anchor;
	obj.objtype = objtype;

	if (objtype == IMAGE) {
		strcpy(obj.text, param.image.filename);
		obj.font = NULL;
		obj.textcolor = COLOR_WHITE;

		SDL_Surface *surface = IMG_Load(param.image.filename);
		if (surface == NULL) {
			printf("이미지 로드 실패");
			obj.texture = NULL;
			obj.dstrect = (SDL_Rect){x, y, 0, 0};
			return obj;
		}
		SDL_Texture *texture =
			SDL_CreateTextureFromSurface(ui->renderer, surface);
		obj.texture = texture;
		int w = (param.image.w != 0) ? param.image.w : surface->w;
		int h = (param.image.h != 0) ? param.image.h : surface->h;
		SDL_Rect rect = {x, y, w, h};
		switch (anchor) {
		case TOPLEFT:
			break;
		case MIDTOP:
			rect.x -= rect.w / 2;
			break;
		case CENTER:
			rect.x -= rect.w / 2;
			rect.y -= rect.h / 2;
			break;
		case MIDBOTTOM:
			rect.x -= rect.w / 2;
			rect.y -= rect.h;
			break;
		}
		obj.dstrect = rect;

		SDL_FreeSurface(surface);
		return obj;

	} else if (objtype == TEXT) {
		strcpy(obj.text, param.text.text);
		obj.font =
			(param.text.font != NULL) ? param.text.font : ui->font_normal;
		obj.textcolor = param.text.color;

		SDL_Surface *surface =
			TTF_RenderUTF8_Blended(obj.font, obj.text, obj.textcolor);
		if (surface == NULL) {
			obj.texture = NULL;
			obj.dstrect = (SDL_Rect){x, y, 0, 0};
			return obj;
		}

		SDL_Texture *texture =
			SDL_CreateTextureFromSurface(ui->renderer, surface);
		obj.texture = texture;
		SDL_Rect rect = {x, y, surface->w, surface->h};
		switch (anchor) {
		case TOPLEFT:
			break;
		case MIDTOP:
			rect.x -= rect.w / 2;
			break;
		case CENTER:
			rect.x -= rect.w / 2;
			rect.y -= rect.h / 2;
			break;
		case MIDBOTTOM:
			rect.x -= rect.w / 2;
			rect.y -= rect.h;
			break;
		}
		obj.dstrect = rect;

		SDL_FreeSurface(surface);
		return obj;
	} else if (objtype == BOX) {
		obj.textcolor = param.box.color;
		SDL_Surface *surface =
			SDL_CreateRGBSurface(0, param.box.w, param.box.h, 32, 0x00FF0000,
								 0x0000FF00, 0x000000FF, 0xFF000000);
		Uint32 color =
			SDL_MapRGBA(surface->format, obj.textcolor.r, obj.textcolor.g,
						obj.textcolor.b, obj.textcolor.a);
		SDL_FillRect(surface, NULL, color);

		obj.texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
		SDL_FreeSurface(surface);

		SDL_Rect rect = {x, y, param.box.w, param.box.h};
		switch (anchor) {
		case TOPLEFT:
			break;
		case MIDTOP:
			rect.x -= rect.w / 2;
			break;
		case CENTER:
			rect.x -= rect.w / 2;
			rect.y -= rect.h / 2;
			break;
		case MIDBOTTOM:
			rect.x -= rect.w / 2;
			rect.y -= rect.h;
			break;
		}
		obj.dstrect = rect;
		return obj;
	}

	return obj;
}

SDL_Rect gui_initRect(int x, int y, int w, int h, AnchorEnum anchor) {
	SDL_Rect rect;

	switch (anchor) {

	case TOPLEFT:
		rect.x = x;
		rect.y = y;
		break;

	case CENTER:
		rect.x = x - w / 2;
		rect.y = y - h / 2;
		break;

	case MIDTOP:
		rect.x = x - w / 2;
		rect.y = y;
		break;

	case MIDBOTTOM:
		rect.x = x - w / 2;
		rect.y = y - h;
		break;
	}

	rect.w = w;
	rect.h = h;
	return rect;
}

void gui_presentObject(Object *obj) {
	SDL_RenderCopy(obj->ui->renderer, obj->texture, NULL, &obj->dstrect);
}

void gui_moveObject(Object *obj, int x, int y) {
	switch (obj->anchor) {
	case TOPLEFT:
		obj->dstrect.x = x;
		obj->dstrect.y = y;
		break;

	case MIDTOP:
		obj->dstrect.x = x - obj->dstrect.w / 2;
		obj->dstrect.y = y;
		break;

	case CENTER:
		obj->dstrect.x = x - obj->dstrect.w / 2;
		obj->dstrect.y = y - obj->dstrect.h / 2;
		break;

	case MIDBOTTOM:
		obj->dstrect.x = x - obj->dstrect.w / 2;
		obj->dstrect.y = y - obj->dstrect.h;
		break;
	}
}

void gui_setText(Object *obj, char text[]) {
	int x, y;
	if (obj->objtype != TEXT) {
		return;
	}

	if (obj->texture) {
		SDL_DestroyTexture(obj->texture);
	}

	strcpy(obj->text, text);
	SDL_Surface *surface =
		TTF_RenderUTF8_Blended(obj->font, text, obj->textcolor);
	SDL_Texture *texture =
		SDL_CreateTextureFromSurface(obj->ui->renderer, surface);
	obj->texture = texture;

	switch (obj->anchor) {
	case TOPLEFT:
		obj->dstrect.w = surface->w;
		obj->dstrect.h = surface->h;
		break;

	case MIDTOP:
		x = obj->dstrect.x + obj->dstrect.w / 2;
		y = obj->dstrect.y;
		obj->dstrect.w = surface->w;
		obj->dstrect.h = surface->h;
		gui_moveObject(obj, x, y);
		break;

	case CENTER:
		x = obj->dstrect.x + obj->dstrect.w / 2;
		y = obj->dstrect.y + obj->dstrect.h / 2;
		obj->dstrect.w = surface->w;
		obj->dstrect.h = surface->h;
		gui_moveObject(obj, x, y);
		break;

	case MIDBOTTOM:
		x = obj->dstrect.x + obj->dstrect.w / 2;
		y = obj->dstrect.y + obj->dstrect.h;
		obj->dstrect.w = surface->w;
		obj->dstrect.h = surface->h;
		gui_moveObject(obj, x, y);
		break;
	}

	SDL_FreeSurface(surface);
}

void gui_setColorText(Object *obj, SDL_Color color) {
	if (obj->texture) {
		SDL_DestroyTexture(obj->texture);
	}
	obj->textcolor = color;
	SDL_Surface *surface =
		TTF_RenderUTF8_Blended(obj->font, obj->text, obj->textcolor);
	SDL_Texture *texture =
		SDL_CreateTextureFromSurface(obj->ui->renderer, surface);
	obj->texture = texture;
	SDL_FreeSurface(surface);
}

int gui_isInObject(Object *obj, int x, int y) {
	return (x >= obj->dstrect.x && x <= obj->dstrect.x + obj->dstrect.w &&
			y >= obj->dstrect.y && y <= obj->dstrect.y + obj->dstrect.h);
}

void gui_utf8Backspace(char *s) {
	int len = strlen(s);
	if (len == 0)
		return;

	int i = len - 1;

	while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80) {
		i--;
	}
	s[i] = '\0';
}
