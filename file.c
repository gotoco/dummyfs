#include <linux/uio.h>

#include "hdummy.h"

ssize_t dm_get_loffset(struct dm_inode *di, loff_t off)
{
	ssize_t ret = DM_EMPTY_ENTRY;
	loff_t add = 0;
	u32 i = 0;

	if (off > DM_DEFAULT_BSIZE)
		add += DM_DEFAULT_BSIZE % off;

	for (i = 0; i < DM_INODE_TSIZE; ++i) {
		if (di->i_addrb[i] + off > di->i_addre[i]) {
			off -= (di->i_addre[i] - di->i_addrb[i]);
		} else {
			ret = di->i_addrb[i] + off;
			break;
		}
	}

	BUG_ON(ret == 0xdeadbeef);

	return ret;
}

ssize_t dummy_read(struct kiocb *iocb, struct iov_iter *to)
{
	struct super_block *sb;
	struct inode *inode;
	struct dm_inode *dinode;
	struct buffer_head *bh;
	char *buffer;
	void *buf = to->iov->iov_base;
	int nbytes;
	size_t count = iov_iter_count(to);
	loff_t off = iocb->ki_pos;
	loff_t end = off + count;
	size_t blk = 0;


	inode = iocb->ki_filp->f_path.dentry->d_inode;
	sb = inode->i_sb;
	dinode = inode->i_private;
	if (off) {
		return 0;
	}

	/* calculate datablock number here */
	blk = dm_get_loffset(dinode, off);
	bh = sb_bread(sb, blk);
	if (!bh) {
        	printk(KERN_ERR "Failed to read data block %lu\n", blk);
		return 0;
	}

	buffer = (char *)bh->b_data + (off % DM_DEFAULT_BSIZE);
	nbytes = min((size_t)(dinode->i_size - off), count);

	if (copy_to_user(buf, buffer, nbytes)) {
		brelse(bh);
		printk(KERN_ERR
			"Error copying file content to userspace buffer\n");
		return -EFAULT;
	}

	brelse(bh);
	iocb->ki_pos += nbytes;

	return nbytes;
}

ssize_t dm_alloc_if_necessary(struct dm_superblock *sb, struct dm_inode *di,
				loff_t off, size_t cnt)
{
	// Mock it until using bitmap
	return 0;
}

ssize_t dummy_write(struct kiocb *iocb, struct iov_iter *from)
{
	struct super_block *sb;
	struct inode *inode;
	struct dm_inode *dinode;
	struct buffer_head *bh;
	struct dm_superblock *dsb;
	void *buf = from->iov->iov_base; 
	loff_t off = iocb->ki_pos;
	size_t count = iov_iter_count(from);
	size_t blk = 0;	
	size_t boff = 0;
	char *buffer;
	int ret;

	inode = iocb->ki_filp->f_path.dentry->d_inode;
	sb = inode->i_sb;
	dinode = inode->i_private;
	dsb = sb->s_fs_info;
	
	ret = generic_write_checks(iocb, from);
	if (ret <= 0) {
	    printk(KERN_INFO "DummyFS: generic_write_checks Failed: %d", ret); 
	    return ret;
	}
	
	/* calculate datablock to write alloc if necessary */
	blk = dm_alloc_if_necessary(dsb, dinode, off, count);
	/* dummy files are contigous so offset can be easly calculated */
	boff = dm_get_loffset(dinode, off);
	bh = sb_bread(sb, boff);
	if (!bh) {
	    printk(KERN_ERR "Failed to read data block %lu\n", blk);
	    return 0;
	}
	
	buffer = (char *)bh->b_data + (off % DM_DEFAULT_BSIZE);
	if (copy_from_user(buffer, buf, count)) {
	    brelse(bh);
	    printk(KERN_ERR
	           "Error copying file content from userspace buffer "
	           "to kernel space\n");
	    return -EFAULT;
	}
	
	iocb->ki_pos += count; 
	
	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);
	
	dinode->i_size = max((size_t)(dinode->i_size), count);

	dm_store_inode(sb, dinode);
	
	return count;
}

