#include "csd3_find_archive.h"
#include "game_maker.h"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#if defined(GM_WINDOWS)
#	include <windows.h>

#define CSH3_DATA_WIN_PATH "\\steamapps\\common\\CookServeDelicious3\\data.win"

struct reg_path {
	HKEY    hKey;
	LPCTSTR lpSubKey;
	LPCTSTR lpValueName;
};

static char *get_path_from_registry(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueName) {
	HKEY hSubKey = 0;
	DWORD dwType = REG_SZ;
	DWORD dwSize = 0;
	char *path = NULL;

	if (RegOpenKeyEx(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &hSubKey) != ERROR_SUCCESS) {
		goto error;
	}

	if (RegQueryValueEx(hSubKey, lpValueName, NULL, &dwType, (LPBYTE)NULL, &dwSize) != ERROR_SUCCESS || dwType != REG_SZ) {
		goto error;
	}

	if (dwSize > ULONG_MAX - sizeof(CSH3_DATA_WIN_PATH)) {
		errno = ENAMETOOLONG;
		goto error;
	}

	// The data might or might not have a terminating '\0', but the size of
	// CSH3_DATA_WIN_PATH definitely contains space for a '\0' so this will
	// be always enough for the text of both sources plus '\0' and using
	// calloc() initialized the terminating '\0'.
	dwSize += sizeof(CSH3_DATA_WIN_PATH);

	path = calloc(dwSize, 1);
	if (path == NULL) {
		goto error;
	}

	DWORD dwSize2 = dwSize;
	if (RegQueryValueEx(hSubKey, lpValueName, NULL, &dwType, (LPBYTE)path, &dwSize2) != ERROR_SUCCESS || dwType != REG_SZ) {
		goto error;
	}

	if (dwSize2 > dwSize) {
		// registry key changed *right now*!
		errno = EAGAIN;
		goto error;
	}

	strcat(path, CSH3_DATA_WIN_PATH);

	goto end;

error:
	if (path != NULL) {
		free(path);
		path = NULL;
	}

end:
	if (hSubKey != 0) {
		RegCloseKey(hSubKey);
	}

	return path;
}

char *csd3_find_archive() {
	static const struct reg_path reg_paths[] = {
		// Have confirmed sightings of these keys:
		{ HKEY_LOCAL_MACHINE, TEXT("Software\\Valve\\Steam"),              TEXT("InstallPath") },
		{ HKEY_LOCAL_MACHINE, TEXT("Software\\Wow6432node\\Valve\\Steam"), TEXT("InstallPath") },
		{ HKEY_CURRENT_USER,  TEXT("Software\\Valve\\Steam"),              TEXT("SteamPath")   },

		// All the other possible combination, just to try everything:
		{ HKEY_CURRENT_USER,  TEXT("Software\\Wow6432node\\Valve\\Steam"), TEXT("SteamPath")   },
		{ HKEY_LOCAL_MACHINE, TEXT("Software\\Valve\\Steam"),              TEXT("SteamPath")   },
		{ HKEY_LOCAL_MACHINE, TEXT("Software\\Wow6432node\\Valve\\Steam"), TEXT("SteamPath")   },
		{ HKEY_CURRENT_USER,  TEXT("Software\\Valve\\Steam"),              TEXT("InstallPath") },
		{ HKEY_CURRENT_USER,  TEXT("Software\\Wow6432node\\Valve\\Steam"), TEXT("InstallPath") },
		{ 0,                  0,                                           0                   }
	};
	static const char* paths[] = {
		"C:\\Program Files\\Steam\\steamapps\\common\\CookServeDelicious3\\data.win",
		"C:\\Program Files (x86)\\Steam\\steamapps\\common\\CookServeDelicious3\\data.win",
		NULL
	};
	struct stat info;

	for (const struct reg_path* reg_path = reg_paths; reg_path->lpSubKey; ++ reg_path) {
		char *path = get_path_from_registry(reg_path->hKey, reg_path->lpSubKey, reg_path->lpValueName);
		if (path != NULL) {
			if (stat(path, &info) < 0) {
				if (errno != ENOENT) {
					perror(path);
				}
			}
			else if (S_ISREG(info.st_mode)) {
				return path;
			}
			free(path);
		}
	}

	// Fallback: Just guess
	for (const char** nameptr = paths; *nameptr; ++ nameptr) {
		if (stat(*nameptr, &info) < 0) {
			if (errno != ENOENT) {
				perror(*nameptr);
			}
		}
		else if (S_ISREG(info.st_mode)) {
			return strdup(*nameptr);
		}
	}

	errno = ENOENT;
	return NULL;
}
#elif defined(__APPLE__)

#define CSD_STEAM_ARCHIVE "Library/Application Support/Steam/SteamApps/common/CookServeDelicious3/Cook Serve Delicious 3.app/Contents/Resources/game.ios"
#define CSD_APP_ARCHIVE   "/Applications/Cook Serve Delicious 2.app/Contents/Resources/game.ios"

char *csd3_find_archive() {
	const char *home = getenv("HOME");
	struct stat info;

	if (home) {
		char *path = GM_JOIN_PATH(home, CSD_STEAM_ARCHIVE);
		if (path != NULL) {
			if (stat(path, &info) < 0) {
				if (errno != ENOENT) {
					perror(path);
				}
			}
			else if (S_ISREG(info.st_mode)) {
				return path;
			}
			free(path);
		}
	}

	if (stat(CSD_APP_ARCHIVE, &info) < 0) {
		if (errno != ENOENT) {
			perror(CSD_APP_ARCHIVE);
		}
		return NULL;
	}
	else if (S_ISREG(info.st_mode)) {
		return strdup(CSD_APP_ARCHIVE);
	}

	errno = ENOENT;
	return NULL;
}
#else // default: Linux
#include <dirent.h>

static char *find_path_ignore_case(const char *prefix, const char* const path[]) {
	if (prefix == NULL || path == NULL) {
		errno = EINVAL;
		return NULL;
	}

	char *filepath = strdup(prefix);

	if (filepath == NULL) {
		return NULL;
	}

	for (const char* const* nameptr = path; *nameptr; ++ nameptr) {
		const char* realname = NULL;
		DIR *dir = opendir(filepath);

		if (!dir) {
			if (errno != ENOENT) {
				perror(filepath);
			}
			free(filepath);
			return NULL;
		}

		for (;;) {
			errno = 0;
			struct dirent *entry = readdir(dir);
			if (entry) {
				if (strcasecmp(entry->d_name, *nameptr) == 0) {
					realname = entry->d_name;
					break;
				}
			}
			else if (errno == 0) {
				break; // end of dir
			}
			else {
				perror(filepath);
				free(filepath);
				closedir(dir);
				return NULL;
			}
		}

		if (!realname) {
			free(filepath);
			closedir(dir);
			errno = ENOENT;
			return NULL;
		}

		char *nextpath = GM_JOIN_PATH(filepath, realname);

		closedir(dir);
		free(filepath);

		if (nextpath == NULL) {
			return NULL;
		}

		filepath = nextpath;
	}

	return filepath;
}

char *csd3_find_archive() {
	// Steam was developed for Windows, which has case insenstive file names.
	// Therefore I can't assume a certain case and because I don't want to write
	// a parser for registry.vdf I scan the filesystem for certain names in a case
	// insensitive manner.
	const char* const* paths[] = {
		(const char * const[]){".local", "share", "Steam", "SteamApps", "common", "CookServeDelicious3" ,"assets", "game.unx", NULL},
		(const char * const[]){".steam", "Steam", "SteamApps", "common", "CookServeDelicious3", "assets", "game.unx", NULL},
		(const char * const[]){".wine", "drive_c", "Program Files (x86)", "Steam", "steamapps", "common", "CookServeDelicious3", "data.win", NULL},
		(const char * const[]){".wine", "drive_c", "Program Files", "Steam", "steamapps", "common", "CookServeDelicious3", "data.win", NULL},
		NULL
	};

	const char *home = getenv("HOME");

	if (!home) {
		errno = ENOENT;
		return NULL;
	}

	for (size_t index = 0; paths[index]; ++ index) {
		const char* const* path_spec = paths[index];
		char *path = find_path_ignore_case(home, path_spec);
		if (path != NULL) {
			struct stat info;

			if (stat(path, &info) < 0) {
				if (errno != ENOENT) {
					perror(path);
				}
			}
			else if (S_ISREG(info.st_mode)) {
				return path;
			}
			free(path);
		}
	}

	errno = ENOENT;
	return NULL;
}
#endif
