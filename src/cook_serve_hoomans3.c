// get basename(const char*)
#define _GNU_SOURCE

#include "csd3_find_archive.h"
#include "game_maker.h"
#include "csh3_patch_def.h"

#include <strings.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#ifdef __linux__

// use sendfile() under Linux for zero-context switch file copy
#include <fcntl.h>
#include <sys/sendfile.h>

static int copyfile(const char *src, const char *dst) {
	int status =  0;
	int infd   = -1;
	int outfd  = -1;
	struct stat info;

	infd = open(src, O_RDONLY);
	if (infd < 0) {
		goto error;
	}

	if (fstat(infd, &info) < 0) {
		goto error;
	}

	outfd = open(dst, O_CREAT | O_WRONLY, info.st_mode);
	if (outfd < 0) {
		goto error;
	}

	if (sendfile(outfd, infd, NULL, (size_t)info.st_size) < (ssize_t)info.st_size) {
		goto error;
	}

	goto end;

error:
	status = -1;

end:
	if (infd >= 0) {
		close(infd);
		infd = -1;
	}

	if (outfd >= 0) {
		close(outfd);
		outfd = -1;
	}

	return status;
}

#elif defined(GM_WINDOWS)

#include <windows.h>
#include <errno.h>

static int copyfile(const char *src, const char *dst) {
	if (CopyFile(src, dst, FALSE)) {
		return 0;
	}

	switch (GetLastError()) {
		case ERROR_FILE_NOT_FOUND: errno = ENOENT; break;
		case ERROR_ACCESS_DENIED:  errno = EACCES; break;
		default:                   errno = EINVAL; break;
	}

	return -1;
}

const char *basename(const char *name) {
	if (name == NULL) {
		return NULL;
	}

	const size_t len = strlen(name);
	if (len == 0) {
		return name;
	}

	for (const char *ptr = name + len - 1; ptr > name; -- ptr) {
		if (*ptr == '\\' || *ptr == '/') {
			return ptr + 1;
		}
	}

	return name;
}

#else

static int copyfile(const char *src, const char *dst) {
	char buf[BUFSIZ];
	FILE *fsrc = NULL;
	FILE *fdst = NULL;
	int status = 0;

	fsrc = fopen(src, "rb");
	if (!fsrc) {
		goto error;
	}

	fdst = fopen(dst, "wb");
	if (!fdst) {
		goto error;
	}

	for (;;) {
		size_t count = fread(buf, 1, BUFSIZ, fsrc);

		if (count < BUFSIZ && ferror(fsrc)) {
			goto error;
		}

		if (count > 0 && fwrite(buf, 1, count, fdst) != count) {
			goto error;
		}

		if (count < BUFSIZ) {
			break;
		}
	}

	goto end;

error:
	status = -1;

end:

	if (fsrc) {
		fclose(fsrc);
		fsrc = NULL;
	}

	if (fdst) {
		fclose(fdst);
		fdst = NULL;
	}

	return status;
}
#endif

int main(int argc, char *argv[]) {
	char *game_name_buf = NULL;
	char *backup_name = NULL;
	int status = EXIT_SUCCESS;
	const char *game_name = NULL;
	const char *game_leaf = NULL;
	struct stat st;

	if (argc > 2) {
		fprintf(stderr, "*** ERROR: Please pass the %s file to this program.\n", CSH3_GAME_ARCHIVE);
		goto error;
	}
	else if (argc == 2) {
		game_name = argv[1];
	}
	else {
		game_name_buf = csd3_find_archive();
		if (game_name_buf == NULL) {
			fprintf(stderr, "*** ERROR: Couldn't find %s file.\n", CSH3_GAME_ARCHIVE);
			goto error;
		}
		game_name = game_name_buf;
	}
	game_leaf = basename(game_name);

	printf("Found game archive: %s\n", game_name);

	// create backup if there isn't one
	backup_name = GM_CONCAT(game_name, ".backup");
	if (backup_name == NULL) {
		perror("*** ERROR: creatig backup file name");
		goto error;
	}

	if (stat(backup_name, &st) == 0) {
		if (!S_ISREG(st.st_mode)) {
			fprintf(stderr, "*** ERROR: Backup file is not a regular file.\n");
			goto error;
		}
		printf("Keeping existing backup of game archive.\n");
	}
	else if (errno == ENOENT) {
		printf("Creating backup of game archive...\n");
		if (copyfile(game_name, backup_name) != 0) {
			perror("*** ERROR: Creatig backup");
			goto error;
		}
	}
	else {
		perror("*** ERROR: Error accessing backup file");
		goto error;
	}

	printf("If you want to remove the mod again delete %s and rename %s.backup to %s (both files are in the same folder).\n",
		game_leaf, game_leaf, game_leaf);

	printf("Patching the game...\n");

	// patch the archive
	if (gm_patch_archive(game_name, csh3_patches) != 0) {
		fprintf(stderr, "*** ERROR: Error patching archive: %s\n", strerror(errno));
		goto error;
	}

	printf("Successfully pached the game! :)\n");

	goto end;

error:
	status = EXIT_FAILURE;

end:
	if (game_name_buf != NULL) {
		free(game_name_buf);
		game_name_buf = NULL;
	}

	if (backup_name != NULL) {
		free(backup_name);
		backup_name = NULL;
	}

#ifdef GM_WINDOWS
	printf("Press ENTER to continue...");
	getchar();
#endif

	return status;
}
