#ifndef _LINUX_DUMMYFS_H
#define _LINUX_DUMMYFS_H
/**
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/magic.h>
#endif
**/

/* Layout version */
#define DM_LAYOUT_VER		1


/* FS SIZE/OFFSET CONST */
#define DM_INODE_TSIZE		3
#define DM_SUPER_OFFSET		0
#define DM_DEFAULT_BSIZE	4096
#define DM_OLT_OFFSET		(DM_SUPER_OFFSET + 1)
#define DM_INODE_TABLE_OFFSET	(DM_OLT_OFFSET + 1)
#define DM_INODE_BITMAP_OFFSET	(DM_INODE_TABLE_OFFSET + DM_INODE_TABLE_SIZE + 1)
#define DM_ROOT_INODE_OFFSET	(DM_INODE_BITMAP_OFFSET + 1)
#define DM_ROOT_IN_EXT_OFF	(DM_ROOT_INODE_OFFSET + 1)
#define DM_LF_INODE_OFFSET	(DM_ROOT_IN_EXT_OFF + DM_DEF_ALLOC)
/* Default place where FS will start using after mkfs (all above are used for mkfs) */
#define DM_FS_SPACE_START	(DM_LF_INODE_OFFSET + DM_DEF_ALLOC)

/* FS constants */
#define DM_MAGIC_NR		0xf00dbeef
#define DM_INODE_SIZE		512
#define DM_INODE_NUMBER_TABLE	128
#define DM_INODE_TABLE_SIZE	(DM_INODE_NUMBER_TABLE * DM_INODE_SIZE)/DM_DEFAULT_BSIZE
#define DM_EMPTY_ENTRY		0xdeeddeed

#define DM_NAME_LEN		255
#define DM_DEF_ALLOC		4	/* By default alloc N blocks per extend */

/*
 * Special inode numbers 
 */
#define DM_BAD_INO		1 /* Bad blocks inode */
#define DM_ROOT_INO		2 /* Root inode nr */
#define DM_LAF_INO		3 /* Lost and Found inode nr */

/**
 * The on-disk superblock
 */
struct dm_superblock {
	uint32_t	s_magic;	/* magic number */
	uint32_t	s_version;	/* fs version */
	uint32_t	s_blocksize;	/* fs block size */
	uint32_t	s_block_olt;	/* Object location table block */
	uint32_t	s_inode_cnt;	/* number of inodes in inode table */
	uint32_t	s_last_blk;	/* just move forward with allocation */
};

/**
 * Object Location Table
 */
struct dm_olt {
	uint32_t	inode_table;		/* inode_table block location */
	uint32_t	inode_cnt;		/* number of inodes */
	uint32_t	inode_bitmap;		/* inode bitmap block */
};

/**
 * The on Disk inode
 */
struct dm_inode {
	uint8_t		i_version;	/* inode version */
	uint8_t		i_flags;	/* inode flags: TYPE */
	uint32_t	i_mode;		/* File mode */
	uint32_t	i_ino;		/* inode number */
	uint16_t	i_uid;		/* owner's user id */
	uint16_t	i_hrd_lnk;	/* number of hard links */
	uint32_t 	i_ctime;	/* Creation time */
	uint32_t	i_mtime;	/* Modification time*/
	uint32_t	i_size;		/* Number of bytes in file */
	/* address begin - end block, range exclusive: addres end (last block) does not belogs to extend! */
	uint32_t	i_addrb[DM_INODE_TSIZE];	/* Start block of extend ranges */
	uint32_t	i_addre[DM_INODE_TSIZE];	/* End block of extend ranges */
};

struct dm_dir_entry {
	uint32_t inode_nr;		/* inode number */
	uint32_t name_len;		/* Name length */
	char name[256];			/* File name, up to DM_NAME_LEN */
};

#endif /* _LINUX_DUMMYFS_H */
