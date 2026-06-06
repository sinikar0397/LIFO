#ifndef SDL_h
#define SDL_h

#include "../headers.h"
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>

#define STANDARD_FONT "gulsi.ttf"
#define WINDOW_WIDTH (768)	// 1536
#define WINDOW_HEIGHT (432) // 864
#define FONT_SIZE_SSMALL (8)
#define FONT_SIZE_SMALL (12)
#define FONT_SIZE_NORMAL (24)
#define FONT_SIZE_BIG (36)
#define FONT_SIZE_BBIG (108)
#define FONT_SIZE_BBSIG (90)
#define CENTER_X (768)
#define CENTER_Y (432)
#define TEXTLENMAX (256)
#define COLOR_WHITE ((SDL_Color){255, 255, 255, 255})
#define COLOR_BLACK ((SDL_Color){0, 0, 0, 255})
#define COLOR_GRAY ((SDL_Color){97, 97, 97, 255})
#define COLOR_YELLOW ((SDL_Color){255, 214, 75, 255})
#define COLOR_GREEN ((SDL_Color){122, 255, 23, 255})
#define COLOR_BLUE ((SDL_Color){184, 248, 251, 255})
#define COLOR_PURPLE ((SDL_Color){163, 116, 219, 255})

typedef enum _AnchorEnum { TOPLEFT, CENTER, MIDTOP, MIDBOTTOM } AnchorEnum;
typedef enum _ObjectTypeEnum { IMAGE, TEXT } ObjectTypeEnum;

typedef struct _SDL_Ui {
	SDL_Window *window;
	SDL_Renderer *renderer;
	TTF_Font *font_ssmall;
	TTF_Font *font_small;
	TTF_Font *font_normal;
	TTF_Font *font_big;
	TTF_Font *font_bbsig;
	TTF_Font *font_bbig;
} SDL_Ui;

typedef struct _Object {
	SDL_Ui *ui;
	SDL_Texture *texture;
	SDL_Rect dstrect;
	AnchorEnum anchor;
	ObjectTypeEnum objtype;
	TTF_Font *font;
	SDL_Color textcolor;
	char text[TEXTLENMAX];
	int status1;
	int status2;
	int status3;
} Object;

typedef struct _ObjectGroup {
	Object *objarr;
} ObjGroup; // 이거 쓸지는 몰?루

typedef struct _ImageBox {
	SDL_Ui *p_sdl_ui;
	SDL_Texture *texture;
	SDL_Rect dstrect;
	AnchorEnum anchor;
	char filename[30];
} ImageBox;

typedef struct _StaticTextBox {
	SDL_Ui *p_sdl_ui;
	TTF_Font *font;
	SDL_Color textcolor;
	char text[256];
	SDL_Texture *texture;
	SDL_Rect dstrect;
	AnchorEnum anchor;
} StaticTextBox;

typedef struct _TextBox {
	SDL_Ui *p_sdl_ui;
	TTF_Font *font;
	SDL_Color textcolor;
	char text[256];
	SDL_Texture *texture;
	SDL_Rect dstrect;
	AnchorEnum anchor;
} TextBox;

typedef struct _InputBox {
	SDL_Ui *p_sdl_ui;
	SDL_Color boxcolor;
	SDL_Texture *texture;
	SDL_Rect dstrect;
	bool focused;
} InputBox;

typedef struct _ImageParam {
	const char filename[64];
	int w, h;
} ImageParam;

typedef struct _TextParam {
	char text[256];
	TTF_Font *font;
	SDL_Color color;
} TextParam;

typedef union _ObjectParam {
	ImageParam image;
	TextParam text;
} ObjectParam;

int gui_initUi(SDL_Ui *ui);
void gui_closeUi(SDL_Ui *ui);

Object gui_initObject(SDL_Ui *ui, ObjectTypeEnum objtype, int x, int y,
					  AnchorEnum anchor, ObjectParam param);
SDL_Rect gui_initRect(int x, int y, int w, int h, AnchorEnum anchor);

void gui_presentObject(Object *obj);

void gui_moveObject(Object *obj, int x, int y);

void gui_setText(Object *obj, char text[]);

void gui_setColorText(Object *obj, SDL_Color color);

int gui_isInObject(Object *obj, int x, int y);

void gui_utf8Backspace(char *s);

#endif // SDL_H
