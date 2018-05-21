# Simple implementation of the Linux filesystem
DummyFS is simple implementation of FileSystem with basic on-disk layout.
The main goal of this FS is educational puropse.


## How to start?
Clone repository then specify Linux kernel dir `KDIR` inside Makefile.

```bash
# to create kernel modules
make

# to make mkfs
gcc -o mkfs_dmfs mkfs_dummyfs.c -g

# to create initial filesystem on device i.e. for device /dev/sdb use mkfs
./mkfs_dmfs /dev/sdb

# you can see disk layout in the hex if you will, to do so:
dd if=/dev/sdb bs=1k count=20 | hexdump

# insert kernel module
# (Please mind the fact that it is learning module,
# so good idea is to run it on VM or save all data before!)
insmod ./dmyfs.ko

# Then mount filesystem (again given device /dev/sdb, and mount point /mnt):
mount -t dummyfs /dev/sdb /mnt

# verfy if the fs is mounted properly
mount | grep dummyfs

ls -al /mnt
...

# create file in dummyfs
touch /mnt/test

# do simple 'hello word'
echo hello_from_dummyfs > /mnt/test
cat /mnt/test
...
```

## More resources:
See slide-deck from my presentation "Writing Linux FS4Fun"
```bash
./Documentation/WrittingFS4Fun.pdf
```
