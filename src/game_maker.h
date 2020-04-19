#ifndef GAME_MAKER_H
#define GAME_MAKER_H
#pragma once

#include <stdio.h>
#include <inttypes.h>

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
#	define GM_WINDOWS
#	define GM_PATH_SEP '\\'
#else
#	define GM_PATH_SEP '/'
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum gm_filetype {
	GM_UNKNOWN = 0,
	GM_PNG,
	GM_WAVE,
	GM_OGG,
	GM_TXT,
};

enum gm_section {
	GM_END = 0,

	GM_GEN8,
	GM_OPTN,
	GM_EXTN,
	GM_SOND,
	GM_SPRT,
	GM_BGND,
	GM_PATH,
	GM_SCPT,
	GM_SHDR,
	GM_FONT,
	GM_TMLN,
	GM_OBJT,
	GM_ROOM,
	GM_DAFL,
	GM_TPAG,
	GM_CODE,
	GM_VARI,
	GM_FUNC,
	GM_STRG,
	GM_TXTR,
	GM_AUDO,
	GM_AGRP,
	GM_LANG,
	GM_GLOB,
	GM_EMBI,
	GM_TGIN,
};

enum gm_patch_src {
	GM_SRC_MEM,
	GM_SRC_FILE,
};

struct gm_patch_sprt_entry {
	size_t tpag_index;
	size_t x;
	size_t y;
	size_t width;
	size_t height;
	size_t txtr_index;
};

struct gm_patch {
	enum gm_section   section;
	size_t            index; // entry index
	enum gm_filetype  type;
	enum gm_patch_src patch_src;
	size_t            size;

	union {
		const uint8_t *data;
		const char *filename;
	} src;

	union {
		struct {
			size_t width;
			size_t height;
		} txtr;

		struct {
			const char *name;
			size_t entry_count;
			const struct gm_patch_sprt_entry *entries;
		} sprt;

		struct {
			const char *old;
			const char *new;
		} strg;
	} meta;
};

#define GM_PATCH_STRG(INDEX, OLD, NEW) \
	{ GM_STRG, (INDEX), GM_TXT, GM_SRC_MEM, 0, { .data = NULL }, { .strg = { (OLD), (NEW) } } }

#define GM_PATCH_SPRT(NAME, ENTRIES, ENTRY_COUNT) \
	{ GM_SPRT, 0, GM_PNG, GM_SRC_MEM, 0, { .data = NULL }, { .sprt = { (NAME), (ENTRY_COUNT), (ENTRIES) } } }

#define GM_PATCH_TXTR(INDEX, DATA, SIZE, WIDTH, HEIGHT) \
	{ GM_TXTR, (INDEX), GM_PNG, GM_SRC_MEM, (SIZE), { .data = (DATA) }, { .txtr = { (WIDTH), (HEIGHT) } } }

#define GM_PATCH_AUDO(INDEX, DATA, SIZE, TYPE) \
	{ GM_AUDO, (INDEX), (TYPE), GM_SRC_MEM, (SIZE), { .data = (DATA) }, { .txtr = { 0, 0 } } }

#define GM_PATCH_END \
	{ GM_END, 0, GM_UNKNOWN, GM_SRC_MEM, 0, { .data = NULL }, { .txtr = { 0, 0 } } }

struct gm_tpag {
	size_t x;
	size_t y;
	size_t width;
	size_t height;
	size_t txtr_index;
};

struct gm_entry {
	off_t            offset;
	size_t           size;
	enum gm_filetype type;

	union {
		struct {
			uint32_t unknown1;
			uint32_t unknown2;
			size_t width;
			size_t height;
		} txtr;

		struct {
			char *name;
			size_t tpag_count;
			struct gm_tpag *tpag;
		} sprt;

		char *strg;
	} meta;
};

struct gm_index {
	enum gm_section section;

	off_t  offset;
	size_t size;

	size_t entry_count;
	struct gm_entry *entries;
};

struct gm_patched_entry {
	off_t  offset;
	size_t size;

	const struct gm_patch *patch;
	const struct gm_entry *entry;
};

struct gm_patched_index {
	enum gm_section section;

	off_t  offset;
	size_t size;

	size_t entry_count;
	struct gm_patched_entry *entries;

	const struct gm_index *index;
};

struct gm_patched_index *gm_get_section(struct gm_patched_index *patched, enum gm_section section);
size_t                   gm_index_length(const struct gm_index *index);
int                      gm_patch_archive(const char *filename, const struct gm_patch *patches);
int                      gm_patch_archive_from_dir(const char *filename, const char *dirname);
int                      gm_patch_entry(struct gm_patched_index *index, const struct gm_patch *patch);
int                      gm_shift_tail(struct gm_patched_index *index, off_t offset);
void                     gm_free_patched_index(struct gm_patched_index *index);
const char              *gm_section_name(enum gm_section section);
const char              *gm_extension(enum gm_filetype type);
const char              *gm_typename(enum gm_filetype type);
enum gm_section          gm_parse_section(const uint8_t *magic);
int                      gm_read_index_sprt(FILE *game, struct gm_index *section);
int                      gm_read_index_strg(FILE *game, struct gm_index *section);
int                      gm_read_index_txtr(FILE *game, struct gm_index *section);
int                      gm_read_index_audo(FILE *game, struct gm_index *section);
struct gm_index         *gm_read_index(FILE *game);
void                     gm_free_index(struct gm_index *index);
size_t                   gm_form_size(const struct gm_patched_index *index);
int                      gm_write_hdr(FILE *fp, const uint8_t *magic, size_t size);
int                      gm_dump_files(const struct gm_index *index, FILE *game, const char *outdir);
char                    *gm_concat(const char *strs[], size_t nstrs);
char                    *gm_join_path(const char *comps[], size_t ncomps);

#define GM_CONCAT(...) \
	gm_concat( \
		(const char *[]){ __VA_ARGS__ }, \
		sizeof((const char *[]){ __VA_ARGS__ }) / sizeof(char*))

#define GM_JOIN_PATH(...) \
	gm_join_path( \
		(const char *[]){ __VA_ARGS__ }, \
		sizeof((const char *[]){ __VA_ARGS__ }) / sizeof(char*))

#ifdef __cplusplus
}
#endif
#endif
