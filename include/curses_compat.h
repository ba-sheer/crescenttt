#ifndef CURSES_COMPAT_H
#define CURSES_COMPAT_H

#ifdef _WIN32
#include <curses.h>
#else
#include <ncurses.h>
#endif

#endif