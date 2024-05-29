#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <ctype.h>

#include "iowii.h"

extern char settingpath[512];
extern char saves_dir[512];
extern int eachsettingon;

//////////////////////////////////////////////////////////////////////////////

const char *get_basename(const char *filepath)
{
	const char *ptr = NULL;

	u32 len;
	s32 cnt;

	/* Get string length */
	len = strlen(filepath);

	/* Find basename */
	for (cnt = len-1; cnt >= 0; cnt--)
		if (filepath[cnt] == '/') {
			ptr = &filepath[cnt+1];
			break;
		}

	/* Return pointer */
	return ptr;
}

//////////////////////////////////////////////////////////////////////////////

const char *str_sub (const char *s, unsigned int start, unsigned int end)
{
	char *new_s = NULL;

	if (s != NULL && start < end)
	{
		new_s = malloc (sizeof (*new_s) * (end - start + 2));
		if (new_s != NULL)
		{
			int i;
			for (i = start; i <= end; i++)
			{
				new_s[i-start] = s[i];
			}
			new_s[i-start] = '\0';
		}
		else
		{
			fprintf (stderr, "Memoire insuffisante\n");
			exit(0);
		}
	}
	return new_s;
}

//////////////////////////////////////////////////////////////////////////////

const char *str_tolower (const char *ct)
{
   char *s = NULL;

   if (ct != NULL)
   {
      int i;
      s = malloc (sizeof (*s) * (strlen (ct) + 1));
      if (s != NULL)
      {
         for (i = 0; ct[i]; i++)
         {
            s[i] = (char)tolower ((int)ct[i]);
         }
         s[i] = '\0';
      }
   }
   return s;
}

//////////////////////////////////////////////////////////////////////////////

u32 sd_init(void)
{
	/* Initialize libfat */
	return fatInitDefault();
}

//////////////////////////////////////////////////////////////////////////////

s32 sd_readdir(const char *dirpath, struct file **out, u32 *files)
{
	static char filename[MAX_FILENAME_LEN];
        static char eachxmlfilename[MAX_FILENAME_LEN];
        char *p_c;

	struct file *filelist;
	struct stat filestat;

	DIR *dir;
	struct dirent *ent = NULL;
	struct stat st;
	u32 cnt = 0, nb_files = 0;

	/* Open directory */
	dir = opendir(dirpath);
	if (!dir)
		return -1;

	/* Get number of directory entries */
	while ((ent=readdir(dir)) != 0)
	{
		snprintf(filename, sizeof(filename), "%s/%s", dirpath, ent->d_name);
		if(stat(filename, &st) != 0) continue;
                if(S_ISREG(st.st_mode))
		{
		        snprintf(filename, sizeof(filename), ent->d_name);
			if (!strcmp(str_tolower(str_sub(filename, strlen(filename)-3, strlen(filename))), "iso") ||
				!strcmp(str_tolower(str_sub(filename, strlen(filename)-3, strlen(filename))), "cue") ||
				!strcmp(str_tolower(str_sub(filename, strlen(filename)-3, strlen(filename))), "xxx"))
			{
				nb_files++;
			}
		}
	}

	/* Reset directory */
	rewinddir(dir);

	/* Allocate memory */
	filelist = (struct file *)malloc(sizeof(struct file) * nb_files);
	if (!filelist) {
		closedir(dir);
		return -2;
	}

	/* Get directory entries */
	while ((ent=readdir(dir)) != 0)
	{
		snprintf(filename, sizeof(filename), "%s/%s", dirpath, ent->d_name);
		if(stat(filename, &st) != 0) continue;
                if(S_ISREG(st.st_mode))
		{
		        snprintf(filename, sizeof(filename), ent->d_name);
			if (!strcmp(str_tolower(str_sub(filename, strlen(filename)-3, strlen(filename))), "iso") ||
				!strcmp(str_tolower(str_sub(filename, strlen(filename)-3, strlen(filename))), "cue") ||
				!strcmp(str_tolower(str_sub(filename, strlen(filename)-3, strlen(filename))), "xxx"))
			{
				sprintf(filelist[cnt].filename, "%s", filename);
				//memcpy(&filelist[cnt].filestat, &filestat, sizeof(filestat));
                            //if(eachsettingon)
                            //{
/// set havesetting
                                strcpy(eachxmlfilename, filelist[cnt].filename);
                                p_c = strrchr(eachxmlfilename, '.');
                                *p_c = '\0';
                                strcat(eachxmlfilename, ".xml");
                                strcpy(settingpath, saves_dir);
                                strcat(settingpath, "/");
                                strcat(settingpath, eachxmlfilename);
                                filelist[cnt].havesetting =
                                 ((stat(settingpath, &filestat)) ? 0:1);
                             //}

				cnt++;
			}
		}
	}

	/* Close directory */
	closedir(dir);

	/* Copy data */
	*out   = filelist;
	*files = nb_files;

	return 0;
}

