#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/parser.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>
 
#include "hdummy.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gotoc");
MODULE_DESCRIPTION("Dummy FS");
 
struct file_system_type dummyfs_type = {
	.owner = THIS_MODULE,
	.name = "dummyfs",
	.mount = dummyfs_mount,
	.kill_sb = dummyfs_kill_superblock,
	.fs_flags = FS_REQUIRES_DEV
};

const struct inode_operations dummy_inode_ops = {
	.create = dm_create,
	.mkdir = dm_mkdir,
	.lookup = dm_lookup,
};

const struct file_operations dummy_file_ops = {
	.read_iter = dummy_read,
	.write_iter = dummy_write,
};

const struct super_operations dummy_sb_ops = {
	.destroy_inode = dm_destroy_inode,
	.put_super = dummyfs_put_super,
};

const struct file_operations dummy_dir_ops = {
	.owner = THIS_MODULE,
	.iterate_shared = dummy_readdir,
};

struct kmem_cache *dmy_inode_cache = NULL;

static int __init dummy_init(void)
{
	int ret = 0;

	dmy_inode_cache = kmem_cache_create("dmy_inode_cache",
					sizeof(struct dm_inode),
					0,
					(SLAB_RECLAIM_ACCOUNT| SLAB_MEM_SPREAD),
					NULL);

	if (!dmy_inode_cache)
		return -ENOMEM;

	ret = register_filesystem(&dummyfs_type);

	if (ret == 0)
		printk(KERN_INFO "Sucessfully registered dummyfs\n");
	else
		printk(KERN_ERR "Failed to register dummyfs. Error code: %d\n", ret);

	return ret;
}
module_init(dummy_init);
 
static void __exit dummy_exit(void)
{
	int ret;

	ret = unregister_filesystem(&dummyfs_type);
	kmem_cache_destroy(dmy_inode_cache);

	if (!ret)
		printk(KERN_INFO "Unregister dummy FS success\n");
	else
		printk(KERN_ERR "Failed to unregister dummy FS\n");

}
module_exit(dummy_exit);
