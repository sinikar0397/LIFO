#ifndef DISPLAY_H
#define DISPLAY_H

#include "../bfs/bfs.h"
#include "../bfs/match_store.h"
#include "../dfs/dfs.h"
#include "../mst/mst.h"
#include "../people/login.h"
#include "ui.h"

int display_countChars(const char *pw_buf);
People *display_showLogin(SDL_Ui *ui);
People *display_showSignup(SDL_Ui *ui);
void display_showHome(SDL_Ui *ui, People *me);
void display_showBFS(SDL_Ui *ui, People *me, MatchingInfo *infos, int n);
// void display_showMST(SDL_Ui *ui, People *me);

#endif // DISPLAY_H
