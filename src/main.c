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
 * $Id: main.c,v 1.25 2006/01/11 21:57:27 chris2511 Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <unistd.h>
#include <string.h>


static void usage(char *name)
{
	printf("%s [-f imgfile] [-d rootdir] [-u uid] [-g gid] [-s size] "
			"[-v] [-n] [-D devicefile]\n\n"
			"-t  filesystem type\n"
			"-b  block size\n"
			"-L  volume label\n"
			"-f  filesystem image file\n"
			"-d  root directory to be copied to the image\n"
			"-e  exclude dir/file\n"
			"-u  uid to use instead of the real one\n"
			"-g  gid to use instead of the real one\n"
			"-v  be verbose\n"
			"-S  skip unaccessible files and dirs\n"
			"-s  size of the filesystem\n"
			"-D  device filename\n"
			"-p  preserve uid and gid\n"
			"-n  do not create the filesystem, use an existing one\n"
			"-D  change name of the file .DEVICES\n"
			"-P  filename of the passwd file\n"
			"-G  filename of the group file\n"
			"-U  change name of the file .UIDGID file\n\n",
			name);
}

/* return the size in Kilobyte */
static long getsize(const char *t)
{
	int len, f;
	char c, *p;
	long size;
	
	f = 1;
	len = strlen(t);
	c = t[len-1];
	switch (c) {
		case 'g':
		case 'G': f *= 1024;
		case 'm':
		case 'M': f *= 1024;
		case 'k':
		case 'K': len--;
	}
	size = strtol(t, &p, 10) * f;
		
	if (p - t != len) {
		fprintf(stderr,
				"malformed size string: '%s', p-t:%zd, len:%d, Size:%ld\n",
				t, (p-t), len, size);
		size = -1;
	}
	return size;
}

int verbose;
char const *excluded[1024];
unsigned short excluded_num;

static void add_exclude(const char *s)
{
	if (excluded_num < sizeof(excluded) / sizeof(excluded[0]))
		excluded[excluded_num++] = s;
}

int main(int argc, char *argv[] )
{
	int ret = 0, c, create=1;
	long ksize=4096;
	int bsize = 0;
	char *e2fsfile = NULL;
	char *fstype = NULL;
	char *label = NULL;
	char passwd_file[256];
	char group_file[256];
	e2i_ctx_t e2c;
	struct cnt_t cnt;
	uiddb_t passwd;
	uiddb_t group;
	
	/* initialize ext2fs error table */
	init_ext2_err_tbl();

	/* prepare data structure */
	memset(&e2c, 0, sizeof(e2c));
	
	e2c.dev_file = ".DEVICES";
	e2c.uid_file = ".UIDGID";

	e2c.curr_path = NULL;
	e2c.cnt = &cnt;
	cnt.dir = cnt.regf = cnt.softln = cnt.hardln = cnt.specf = 0; 

	verbose = 0;
	e2c.unaccessible = -1;
	excluded_num = 0;

	printf("%s - Version: %s\n",  argv[0], VER);
	
	/* handle arguments and options */
	do {
		c = getopt(argc, argv, "vnhpf:d:u:g:s:D:U:P:G:St:234b:L:e:");
		switch (c) {
			case 'v': verbose = 1; break;
			case 'p': e2c.preserve_uidgid = 1; break;
			case 'u': e2c.default_uid = atoi(optarg); break;
			case 'g': e2c.default_gid = atoi(optarg); break;
			case 'f': e2fsfile = optarg; break;
			case 'd': e2c.root_path = e2c.curr_path = optarg; break;
			case 'h': usage(argv[0]); return 0;
			case 'n': create = 0; break;
			case 's': ksize = getsize(optarg); break;
			case 'b': bsize = atoi(optarg); break;
			case 't': fstype = optarg; break;
			case '2': fstype = "ext2"; break;
			case '3': fstype = "ext3"; break;
			case '4': fstype = "ext4"; break;
			case 'L': label = optarg; break;
			case 'D': e2c.dev_file = optarg; break;
			case 'U': e2c.uid_file = optarg; break;
			case 'P': e2c.pw_file = optarg; break;
			case 'G': e2c.grp_file = optarg; break;
			case 'S': e2c.unaccessible = 0; break;
			case 'e': add_exclude(optarg); break;
		}
			 
	} while (c >= 0);
	
	/* sanity check */
	if (e2fsfile == NULL || e2c.curr_path == NULL || ksize<0) {
		usage(argv[0]);
		return -1;
	}
	/* Setup passwd file */
	if (e2c.pw_file == NULL) {
		strncpy(passwd_file, e2c.curr_path, 256-11);
		strcat(passwd_file, "/etc/passwd");
		e2c.pw_file = passwd_file;
	}
    /* Setup group file */
    if (e2c.grp_file == NULL) {
        strncpy(group_file, e2c.curr_path, 256-10);
        strcat(group_file, "/etc/group");
        e2c.grp_file = group_file;
    }
	/* call mke2fs to create the initial filesystem */
	if (create) {
		ret = mke2fs(e2fsfile, fstype, label, bsize, ksize);
		if (ret !=0 ) return ret;
	}
	
	/* sanity check */
	if(! (e2c.curr_path && e2fsfile)) usage (argv[0]);
	
	/* prepare the inode database to lookup hardlinks */
	e2c.ino_db = inodb_init();
	if (e2c.ino_db == 0) return -1;

	e2c.passwd = &passwd;
	uiddb_init(e2c.passwd);
	read_passwd(&e2c); 
	
	e2c.group = &group;
	uiddb_init(e2c.group);
	read_group(&e2c);
	
	/* reserve memory for the file-copy */
	e2c.cp_buf = malloc(BUF_SIZE);
	if (!e2c.cp_buf) {
		fputs("Malloc failed\n", stderr);
		return -1;
	}
	
	/* open and read the filesystem image */
	ret = ext2fs_open (e2fsfile, EXT2_FLAG_RW, 0, 0, unix_io_manager, &e2c.fs);
	E2_ERR(ret, "Error opening filesystem: ", e2fsfile);

	ext2fs_read_inode_bitmap(e2c.fs);
	ext2fs_read_block_bitmap(e2c.fs);
	
	/* trigger the copy of the directory */
	ret = e2cpdir(&e2c, EXT2_ROOT_INO);

	/* release the memory occupied by the inode hash table */
	inodb_free(e2c.ino_db);
	
	/* Release the passwd list */
	uiddb_free(e2c.passwd);
	uiddb_free(e2c.group);
	
	free(e2c.cp_buf);
	
	/* write the filesystem to the image */
	ext2fs_flush(e2c.fs);
#ifdef MALLOC_DEBUG
	list_table();
#endif	
	if (ret) return ret;
	
	ret = ext2fs_close(e2c.fs);
	E2_ERR(ret, "Error closing the filesystem file:", e2fsfile);

	/* satisfy the user with some statistics */
	printf("Copied %d Directorys, %d regular files, %d symlinks\n"
			"%d hard links and %d special files - total %d\n",
			cnt.dir, cnt.regf, cnt.softln, cnt.hardln, cnt.specf, 
			cnt.dir+ cnt.regf+ cnt.softln+ cnt.hardln+ cnt.specf); 

	return ret;
}

