#ifndef DISPLAY_H
#define DISPLAY_H

#include "../bfs/bfs.h"
#include "../bfs/match_store.h"
#include "../dfs/dfs.h"
#include "../mst/mst.h"
#include "../people/login.h"
#include "ui.h"

extern char g_mst_partner_id[MAX_ID_LEN];

int display_countChars(const char *pw_buf);
People *display_showLogin(SDL_Ui *ui);
People *display_showSignup(SDL_Ui *ui);
void display_showHome(SDL_Ui *ui, People *me);
void display_showBFS(SDL_Ui *ui, People *me, MatchingInfo *infos, int n);
void display_showDFS(SDL_Ui *ui, People *me);
void display_showMST(SDL_Ui *ui, People *me);
void display_showTreeView(SDL_Ui *ui, People *me); // 트리 시각화 (홈에서 호출)
void display_showCodex(SDL_Ui *ui, People *me);	   // 유형 도감 (홈에서 호출)

#endif // DISPLAY_H
