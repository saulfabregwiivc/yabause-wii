#ifndef _IOWII_H_
#define _IOWII_H_

#include <fat.h>
#include <sys/dir.h>

//////////////////////////////////////////////////////////////////////////////

#define MAX_FILENAME_LEN	128
#define MAX_FILEPATH_LEN	256

//////////////////////////////////////////////////////////////////////////////

struct file {
	char filename[MAX_FILENAME_LEN];
	struct stat filestat;
        int havesetting;
} ATTRIBUTE_PACKED;

//////////////////////////////////////////////////////////////////////////////

const char *get_basename(const char *filepath);
const char *str_sub (const char *s, unsigned int start, unsigned int end);
const char *str_tolower (const char *ct);
u32 sd_init(void);
s32 sd_readdir(const char *, struct file **, u32 *);

#endif
