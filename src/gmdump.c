#include "game_maker.h"
#include "csd3_find_archive.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	int status = 0;
	FILE *game = NULL;
	struct gm_index *index = NULL;
	const char *outdir = ".";
	const char *gamename = NULL;
	char *pathbuf = NULL;

	if (argc > 3) {
		fprintf(stderr, "*** usage: %s [archive] [outdir]\n", argv[0]);
		goto error;
	}

	for (int i = 1; i < argc; ++ i) {
		char *arg = argv[i];
		struct stat info;

		if (stat(arg, &info) < 0) {
			perror(arg);
			goto error;
		}
		else if (S_ISDIR(info.st_mode)) {
			outdir = arg;
		}
		else {
			gamename = arg;
		}
	}

	if (gamename == NULL) {
		pathbuf = csd3_find_archive();
		if (pathbuf == NULL) {
			fprintf(stderr, "*** ERROR: Couldn't find %s file.\n", CSH3_GAME_ARCHIVE);
			goto error;
		}
		gamename = pathbuf;
		printf("Found archive: %s\n", gamename);
	}

	printf("Reading archive...\n");
	game = fopen(gamename, "rb");
	if (!game) {
		perror(gamename);
		goto error;
	}

	index = gm_read_index(game);
	if (!index) {
		perror(gamename);
		goto error;
	}

	printf("Dumping files...\n");
	if (gm_dump_files(index, game, outdir) != 0) {
		perror(gamename);
		goto error;
	}

	printf("Successfully dumped all files.\n");

	goto end;

error:
	status = 1;

end:
	if (pathbuf) {
		free(pathbuf);
		pathbuf = NULL;
	}

	if (game) {
		fclose(game);
		game = NULL;
	}

	if (index) {
		gm_free_index(index);
		index = NULL;
	}

#ifdef GM_WINDOWS
	printf("Press ENTER to continue...");
	getchar();
#endif

	return status;
}
