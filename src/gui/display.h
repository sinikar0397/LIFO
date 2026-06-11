#ifndef DISPLAY_H
#define DISPLAY_H

#include "../people/login.h"
#include "ui.h"

int count_chars(const char *pw_buf);
People *showLogin(SDL_Ui *ui);
People *showSignup(SDL_Ui *ui);
void showHome(SDL_Ui *ui, People *me);

#endif // DISPLAY_H