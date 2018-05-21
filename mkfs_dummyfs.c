#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dummy_fs.h"
//#include <uapi/linux/types.h>

#define CLEAN_SIZE_OFF	0x10000

/* Global variables */
char wipe_g[] = {0xcb, 0xcb, 0xcb, 0xcb, 0xcb, 0xcb, 0xcb, 0xcb};
char zero_g[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
char laf[] = {'.', 'l', 'o', 's', 't', '+', 'f', 'o', 'u', 'n', 'd', '\0'};
char dotdot[] = {'.', '.', '\0'};
char dot[] = {'.', '\0'};

// dm_ctime is used for inode time fields as create date.
time_t dm_ctime;

int wipe_out_device(int fd, int flag)
{
	int ret = 0;
	uint32_t zero_s = sizeof(zero_g);
	uint32_t wipe_s = sizeof(wipe_g);

	// write either zeros or debug cb to beggining of device CLEAN_SIZE lines
	for (int i = 0; i < CLEAN_SIZE_OFF; ++i) {
		if (flag == 0)
			if (wipe_s != write(fd, &wipe_g, wipe_s)) {
				ret = -1;
				break;
			}
		else
			if (zero_s != write(fd, &zero_g, zero_s)) {
				ret = -1;
				break;
			}
	}

	return ret;
}

int write_superblock(int fd)
{
	int ret = 0;

	// construct superblock:
	struct dm_superblock sb = {
		.s_version = 1,
		.s_magic = DM_MAGIC_NR,
		.s_blocksize = DM_DEFAULT_BSIZE,
		.s_block_olt = DM_OLT_OFFSET,
		.s_last_blk = DM_FS_SPACE_START,
		.s_inode_cnt = 3,
	};

	lseek(fd, DM_SUPER_OFFSET, SEEK_SET);

	// write super block
	if (sizeof(sb) != write(fd, &sb, sizeof(sb)))
		ret = -1;

	return ret;
}

int write_metadata(int fd)
{
	int ret = 0;

	// construct object location table:
	struct dm_olt olt = {
		.inode_table = DM_INODE_TABLE_OFFSET,
		.inode_cnt = 2,
		.inode_bitmap = DM_INODE_BITMAP_OFFSET,
	};

	lseek(fd, DM_OLT_OFFSET*DM_DEFAULT_BSIZE, SEEK_SET);
	if (sizeof(olt) != write(fd, &olt, sizeof(olt))) {
		ret = -1;
	}

	return ret;
}

int write_inode_table(int fd)
{
	int ret = 0;
	uint32_t it_s = (DM_INODE_NUMBER_TABLE * DM_INODE_SIZE * sizeof(uint32_t));

	// construct inode table (it is a inode)
	struct dm_inode inode_table = {
		.i_version = 1,
		.i_flags = 0,
		.i_mode = 0,
		.i_uid = 0,
		.i_ctime = dm_ctime,
		.i_mtime = dm_ctime,
		.i_size = 0,
		.i_addrb = {DM_INODE_TABLE_OFFSET+1, 0, 0},
		.i_addre = {DM_INODE_TABLE_OFFSET+1+DM_INODE_TABLE_SIZE, 0, 0},
	};

	lseek(fd, DM_INODE_TABLE_OFFSET * DM_DEFAULT_BSIZE, SEEK_SET);
	if (sizeof(inode_table) != write(fd, &inode_table, sizeof(inode_table))) {
		ret = -1;
	}

	lseek(fd, (DM_INODE_TABLE_OFFSET + 1) * DM_DEFAULT_BSIZE, SEEK_SET);
	// clear Extension first
	for (int i = 0; i < (it_s)/sizeof(zero_g); ++i)
		if (sizeof(zero_g) != write(fd, &zero_g, sizeof(zero_g))) {
			ret = -1;
		}

	// will Root and Lost+found inodes after they went written to the disk
	return ret;
}

int write_root_inode(int fd)
{
	int ret = 0;
	uint32_t dummy = 0;

	// construct root inode
	struct dm_inode root_inode = {
		.i_version = 1,
		.i_flags = 0,
		.i_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH,
		.i_uid = 0,
		.i_ctime = dm_ctime,
		.i_mtime = dm_ctime,
		.i_size = 0,
		.i_hrd_lnk = 1,
		.i_ino = DM_ROOT_INO,
		.i_addrb = {DM_ROOT_INODE_OFFSET+1, 0, 0},
		.i_addre = {DM_ROOT_INODE_OFFSET+DM_DEF_ALLOC+1, 0, 0},
	};

	lseek(fd, DM_ROOT_INODE_OFFSET*DM_DEFAULT_BSIZE, SEEK_SET);
	if (sizeof(root_inode) != write(fd, &root_inode, sizeof(root_inode))) {
		return -1;
	}

	//Fill Root Inode Table Extension with 0xdeeddeed
	lseek(fd, (DM_ROOT_INODE_OFFSET + 1) * DM_DEFAULT_BSIZE, SEEK_SET);
	dummy = 0xdeeddeed;
	for (int i = 0; i < (DM_DEF_ALLOC * DM_DEFAULT_BSIZE)/sizeof(uint32_t); ++i)
		if (sizeof(dummy) != write(fd, &dummy, sizeof(dummy))) {
			return -1;
		}

	// Write '..' reference to the root folder
	struct dm_dir_entry dot_dentry = {
		.inode_nr = DM_ROOT_INO,
		.name_len = sizeof(dotdot)/sizeof(char),
		.name = {0},
	};

	memcpy(dot_dentry.name, dotdot, sizeof(dotdot) / sizeof(char));
	lseek(fd, (DM_ROOT_INODE_OFFSET + 1) * DM_DEFAULT_BSIZE, SEEK_SET);

	if (sizeof(dot_dentry) != write(fd, &dot_dentry, sizeof(dot_dentry))) {
		perror("Error: root dotdot FAILURE!\n");
		ret = -1;
	}

	// Write '.' reference to the root folder
	struct dm_dir_entry sdot_dentry = {
		.inode_nr = DM_ROOT_INO,
		.name_len = sizeof(dot)/sizeof(char),
		.name = {0},
	};

	memcpy(sdot_dentry.name, dot, sizeof(dot) / sizeof(char));
	lseek(fd, (DM_ROOT_INODE_OFFSET + 1) * DM_DEFAULT_BSIZE + sizeof(sdot_dentry), SEEK_SET);

	if (sizeof(sdot_dentry) != write(fd, &sdot_dentry, sizeof(sdot_dentry))) {
		perror("Error: root dot FAILURE!\n");
		ret = -1;
	}
	return ret;
}

int write_lostfound_inode(int fd)
{
	int ret = 0;
	uint32_t dummy = 0xdeeddeed;

	// construct Lost and Found inode
	struct dm_inode laf_inode = {
		.i_version = 1,
		.i_flags = 0,
		.i_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,
		.i_ino = DM_LAF_INO,
		.i_uid = 0,
		.i_hrd_lnk = 1,
		.i_ctime = dm_ctime,
		.i_mtime = dm_ctime,
		.i_size = 0,
		.i_addrb = {DM_LF_INODE_OFFSET+1, 0, 0},
		.i_addre = {DM_LF_INODE_OFFSET+DM_DEF_ALLOC+1, 0, 0},
	};

	lseek(fd, DM_LF_INODE_OFFSET * DM_DEFAULT_BSIZE, SEEK_SET);
	if (sizeof(laf_inode) != write(fd, &laf_inode, sizeof(laf_inode))) {
		return -1;
	}

	// Fill Inode Table Extension with 0xdeeddeed
	lseek(fd, (DM_LF_INODE_OFFSET + 1) * DM_DEFAULT_BSIZE, SEEK_SET);
	for (int i = 0; i < (DM_DEF_ALLOC * DM_DEFAULT_BSIZE)/sizeof(uint32_t); ++i)
		if (sizeof(dummy) != write(fd, &dummy, sizeof(dummy))) {
			return -1;
		}

	// Write '..' reference to the lost+found folder as a root
	struct dm_dir_entry dot_dentry = {
		.inode_nr = DM_ROOT_INO,
		.name_len = sizeof(dotdot)/sizeof(char),
		.name = {0},
	};

	memcpy(dot_dentry.name, dotdot, sizeof(dotdot) / sizeof(char));
	lseek(fd, (DM_LF_INODE_OFFSET + 1) * DM_DEFAULT_BSIZE, SEEK_SET);

	if (sizeof(dot_dentry) != write(fd, &dot_dentry, sizeof(dot_dentry))) {
		perror("Error: lost+found dotdot FAILURE!\n");
		ret = -1;
	}

	// Write '.' reference to the root folder
	struct dm_dir_entry sdot_dentry = {
		.inode_nr = DM_ROOT_INO,
		.name_len = sizeof(dot)/sizeof(char),
		.name = {0},
	};

	memcpy(sdot_dentry.name, dot, sizeof(dot) / sizeof(char));
	lseek(fd, (DM_LF_INODE_OFFSET + 1) * DM_DEFAULT_BSIZE + sizeof(sdot_dentry), SEEK_SET);

	if (sizeof(sdot_dentry) != write(fd, &sdot_dentry, sizeof(sdot_dentry))) {
		perror("Error: root dot FAILURE!\n");
		ret = -1;
	}

	// Write lost and found folder name to root node
	struct dm_dir_entry laf_dentry = {
		.inode_nr = DM_LAF_INO,
		.name_len = sizeof(laf)/sizeof(char),
		.name = {0},
	};
	syncfs(fd);

	uint32_t off = (DM_ROOT_INODE_OFFSET + 1) * DM_DEFAULT_BSIZE + 2 * sizeof(laf_dentry);
	memcpy(laf_dentry.name, laf, sizeof(laf) / sizeof(char));
	lseek(fd, off, SEEK_SET);
	if (sizeof(laf_dentry) != write(fd, &laf_dentry, sizeof(laf_dentry))) {
		perror("Error: write_lostfound_inode FAILURE!\n");
		ret = -1;
	}

	return ret;
}

int write_root2itable(int fd)
{
	int ret = 0;
	uint32_t blk = DM_ROOT_INODE_OFFSET;

	// write root_inode to the inodetable as a first entry
	lseek(fd, (DM_INODE_TABLE_OFFSET + 1) * DM_DEFAULT_BSIZE, SEEK_SET);
	if (sizeof(blk) != write(fd, &blk, sizeof(uint32_t))) {
		perror("Error: write_root2itable FAILURE!\n");
		ret = -1;
	}

	return ret;
}

int write_laf2itable(int fd)
{
	int ret = 0;
	uint32_t off = sizeof(uint32_t), blk = DM_LF_INODE_OFFSET;

	// write lost+found to the inode table on index[1]
	lseek(fd, (DM_INODE_TABLE_OFFSET + 1) * DM_DEFAULT_BSIZE + off, SEEK_SET);
	if (off != write(fd, &blk, sizeof(blk))) {
		perror("Error: write_laf2itable FAILURE!\n");
		ret = -1;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	int fd;
	ssize_t ret;

	time(&dm_ctime);

	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		perror("Error: cannot open the device!\n");
		return -1;
	}

	// wipe out device before writing new on disk structure
	if (wipe_out_device(fd, 1)) {
		perror("Error: wipe_out_device failed!\n");
		ret = -1;
		goto werror;
	}

	if (write_superblock(fd)) {
		perror("Error: write_superblock failed!\n");
		ret = -1;
		goto werror;
	}

	if (write_metadata(fd)) {
		perror("Error: write_metadata failed!\n");
		ret = -1;
		goto werror;
	}

	if (write_inode_table(fd)) {
		perror("Error: write_inode_table failed!\n");
		ret = -1;
		goto werror;
	}

	if (write_root_inode(fd)) {
		perror("Error: write_root_inode failed!\n");
		ret = -1;
		goto werror;
	}

	if (write_lostfound_inode(fd)) {
		perror("Error: write_lostfound_inode failed!\n");
		ret = -1;
		goto werror;
	}

	if (write_root2itable(fd) || write_laf2itable(fd)) {
		perror("Error: write_root2itable && write_laf2itable failed!\n");
		ret = -1;
		goto werror;
	}

	close(fd);
	return 0;

werror:
	perror("Error occured! closing...\n");
	close(fd);
	return ret;
}
