/* vi: set sw=4 ts=4: */
/*
 * Copyright (C) 2001 Christian Hohnstaedt.
 *
 *  All rights reserved.
 *
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  - Neither the name of the author nor the names of its contributors may be 
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * http://www.hohnstaedt.de/e2fsimage
 * email: christian@hohnstaedt.de
 *
 * $Id: dirent.c,v 1.3 2004/01/18 13:52:20 chris2511 Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Scan a directory and copy all the files to the e2dir
 * Arguments:
 *  fs - the filesystem
 *  e2dir - the directory inode in the e2file
 *  dirpath - the name of the directory containing all subdirs upto here
 */

int e2cpdir(ext2_filsys fs, ext2_ino_t e2dir, const char *dirpath)
{
	struct dirent **namelist;
	int i,ret, len, count;
	char path[256], *ppath;

	ret = scandir(dirpath, &namelist, 0, 0);
	if (ret < 0) {
		perror("scandir");
		return -1;
	}
	else {
		len = strlen(dirpath);
		strncpy(path, dirpath, 256);
		if (path[len-1] != '/') {
			path[len++] = '/';
		}
		ppath = path + len;
		count = ret;
		for (i = 0; i<count; i++) {
			if (!strncmp(".", namelist[i]->d_name, 2)) continue ;
			if (!strncmp("..", namelist[i]->d_name, 3)) continue ;
			strncpy(ppath, namelist[i]->d_name, 256 - len);
			free(namelist[i]);
			ret = e2filetype_select(fs, e2dir, path);
			if (ret) return ret;
        }
        free(namelist);
    }
	return 0;
}
	   
int e2filetype_select(ext2_filsys fs, ext2_ino_t e2dir, const char *path)
{
	struct stat s;  
	ext2_ino_t newe2dir;
	int ret;
	
	lstat(path, &s);
	if (S_ISDIR(s.st_mode)) {
		ret = e2mkdir(fs, e2dir, path, &newe2dir);
		if (ret) return ret;
		ret = e2cpdir(fs, newe2dir, path);
		if (ret) return ret;
	}
	if (S_ISREG(s.st_mode)) {
		ret = e2cp(fs, e2dir, path);
		if (ret) return ret;
	}
	if (S_ISLNK(s.st_mode)) {
		ret = e2symlink(fs, e2dir, path);
		if (ret) return ret;
	}
	return 0;
}	
