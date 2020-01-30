#ifndef CSD3_FIND_ARCHIVE_H
#define CSD3_FIND_ARCHIVE_H
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
#	define CSH3_GAME_ARCHIVE "data.win"
#elif defined(__APPLE__)
#	define CSH3_GAME_ARCHIVE "game.ios"
#else
#	define CSH3_GAME_ARCHIVE "game.unx"
#endif

char *csd3_find_archive();

#ifdef __cplusplus
}
#endif

#endif