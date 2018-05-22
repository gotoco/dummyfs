#ifndef _KERN_DUMMYFS_H
#define _KERN_DUMMYFS_H

#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>

#include "dummy_fs.h"

extern const struct super_operations dummy_sb_ops;
extern const struct inode_operations dummy_inode_ops;
extern const struct file_operations dummy_dir_ops;
extern const struct file_operations dummy_file_ops;

extern struct kmem_cache *dmy_inode_cache;

struct dentry *dummyfs_mount(struct file_system_type *ft, int f, const char *dev, void *d);
int dm_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);
struct dentry *dm_lookup(struct inode *dir, struct dentry *child_dentry, unsigned int flags);

/* file.c */
ssize_t dummy_read(struct kiocb *iocb, struct iov_iter *to);
ssize_t dummy_write(struct kiocb *iocb, struct iov_iter *from);

/* dir.c */
int dummy_readdir(struct file *filp, struct dir_context *ctx);

/* inode.c */
int isave_intable(struct super_block *sb, struct dm_inode *dmi, u32 i_block);
void dm_destroy_inode(struct inode *inode);
void dm_fill_inode(struct super_block *sb, struct inode *des, struct dm_inode *src);
int dm_create_inode(struct inode *dir, struct dentry *dentry, umode_t mode);
int dm_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl);
void dm_store_inode(struct super_block *sb, struct dm_inode *dmi);

/* inode cache */
struct dm_inode *cache_get_inode(void);
void cache_put_inode(struct dm_inode **di);

/* super.c */
void dummyfs_put_super(struct super_block *sb);
void dummyfs_kill_superblock(struct super_block *sb);

#endif /* _KERN_DUMMYFS_H */ 
