/*
  Big Brother File System
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.
  A copy of that code is included in the file fuse.h
  
  The point of this FUSE filesystem is to provide an introduction to
  FUSE.  It was my first FUSE filesystem as I got to know the
  software; hopefully, the comments in this code will help people who
  follow later to get a gentler introduction.

  This might be called a no-op filesystem:  it doesn't impose
  filesystem semantics on top of any other existing structure.  It
  simply reports the requests that come in, and passes them to an
  underlying filesystem.  The information is saved in a logfile named
  bbfs.log, in the directory from which you run bbfs.
*/
#include "config.h"
#include "params.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <openssl/sha.h>
#include <stdbool.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"

#define METADATA "/.metadata"
#define BLOCKS "/.blocks/"
#define LOAD "/.load"
#define BLOCKSIZE 4096

// File descriptors for the our folders
int blocks_fd;
int metadata_fd;
struct node* root;
int size_of_root;

/************************** OUR SAVE FUCTION ****************************************/

struct node{
    char *hash_value;         //The hash value of the file by SHA1
    int exist;                //The number of times the block exists
};

void myfinsert(char* hash_value, int exist) {
    // struct node* new_node = mycreate(hash_value);
    struct node new_node;
    new_node.hash_value = (char*) malloc(strlen(hash_value) + 1);
    strcpy(new_node.hash_value, hash_value);
    new_node.exist = exist;
    if(root == NULL){
        root = (struct node*) malloc(sizeof(struct node));
        root[size_of_root] = new_node;
        size_of_root++;
        return;
    }
    root = (struct node*) realloc(root, (size_of_root + 1) * sizeof(struct node));
    root[size_of_root] = new_node;
    size_of_root++;
}

void swap(struct node* a, struct node* b) {
    struct node temp = *a;
    *a = *b;
    *b = temp;
}

int partition(struct node arr[], int low, int high) {
    char* pivot = arr[high].hash_value;
    int i = low - 1;
    for (int j = low; j <= high - 1; j++) {
        if (strcmp(arr[j].hash_value, pivot) < 0) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return i + 1;
}

void quicksort(struct node arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

// struct node* mycreate(char* hash_value) {
//     struct node* new_node = (struct node*) malloc(sizeof(struct node));
//     new_node->hash_value = (char*) malloc(strlen(hash_value) + 1);
//     strcpy(new_node->hash_value, hash_value);
//     new_node->exist = 1;
//     return new_node;
// }

int set_value(struct node* arr, int val){
    arr->exist += val;
    return arr->exist;
}

void myinsert(char* hash_value) {
    // struct node* new_node = mycreate(hash_value);
    struct node new_node;
    new_node.hash_value = (char*) malloc(strlen(hash_value) + 1);
    strcpy(new_node.hash_value, hash_value);
    new_node.exist = 1;
    if(root == NULL){
        root = (struct node*) malloc(sizeof(struct node));
        root[size_of_root] = new_node;
        size_of_root++;
        return;
    }
    root = (struct node*) realloc(root, (size_of_root + 1) * sizeof(struct node));
    root[size_of_root] = new_node;
    size_of_root++;
    quicksort(root, 0, size_of_root - 1);
}

struct node* myfind(char* hash_value) {
    int left = 0;
    int right = size_of_root - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(root[mid].hash_value, hash_value);
        if (cmp == 0) {
            return &root[mid];
        } else if (cmp < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return NULL;
}

void mydelete(char* hash_value) {
    int left = 0;
    int right = size_of_root - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(root[mid].hash_value, hash_value);
        if (cmp == 0) {
            free(root[mid].hash_value);
            for (int j = mid; j < size_of_root - 1; j++) {
                root[j] = root[j + 1];
            }
            size_of_root--;
            if(size_of_root == 0){
                root = NULL;    
                return;
            }
            break;
        } else if (cmp < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    root = (struct node*) realloc(root, (size_of_root) * sizeof(struct node));
}

void myinit() {
    root = NULL;
    size_of_root = 0;
}

void mydestroy() {
    for (int i = 0; i < size_of_root; i++) {
        free(root[i].hash_value);
    }
    free(root);
    root = NULL;
    size_of_root = 0;
}


//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void bb_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, BB_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will
				    // break here

    log_msg("    bb_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
	    BB_DATA->rootdir, path, fpath);
}

///  The fuse operations are defined in the fuse_operations structure.
void myload(){
    char path[PATH_MAX];
    bb_fullpath(path, LOAD);
    FILE* fd = fopen(path, "r");
    log_msg("load file: %s\n", path);
    char hash[SHA_DIGEST_LENGTH * 2 + 1] = "";
    int exist;
    while(log_syscall("read", fscanf(fd,"%s %d", hash, &exist), 0) != 0){
        hash[SHA_DIGEST_LENGTH * 2] = '\0';
        myfinsert(hash, exist);
    }
    fclose(fd);
}

void mysave(){
    char path[PATH_MAX];
    bb_fullpath(path, LOAD);
    FILE* fd = fopen(path, "w");
    log_msg("save file: %s\n", path);
    for(int i = 0; i < size_of_root; i++){
        log_syscall("write", fprintf(fd, "%s %d\n", root[i].hash_value, root[i].exist), 0);
    }
    fclose(fd);
}


///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come from /usr/include/fuse.h
//
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int bb_getattr(const char *path, struct stat *statbuf)
{
    int retstat;
    unsigned char block_hash_value[SHA_DIGEST_LENGTH * 2 + 1] = "";
    char fpath[PATH_MAX];
    char meta_path[PATH_MAX] = "";
    char meta_full_path[PATH_MAX] = "";
    char block_path[PATH_MAX] = "";
    char block_full_path[PATH_MAX] = "";
    int meta_fd;
    struct stat temp_statbuff;
    int i;


    log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n", path, statbuf);
    bb_fullpath(fpath, path);

    retstat = log_syscall("lstat", lstat(fpath, statbuf), 0);
    
    log_msg("retstat is %d\n", retstat);
    
    if(retstat != 0){
        return retstat;
    }

    //Add to "rootdir/.meta/{path}"
    strcpy(meta_path, METADATA);
    strcat(meta_path, path);
    bb_fullpath(meta_full_path, meta_path);

    // Read Meta File
    meta_fd = log_syscall("open meta", open(meta_full_path, O_RDWR, 0666), 0);
    if(meta_fd < 0){
        //File Al
        return retstat; 
    }

    while(log_syscall("read meta hash", read(meta_fd, (void *) block_hash_value, SHA_DIGEST_LENGTH * 2), 0) != 0){
        block_hash_value[SHA_DIGEST_LENGTH * 2] = '\0';

        memset(block_path, 0, PATH_MAX);
        strcpy(block_path, BLOCKS);
        strcat(block_path, block_hash_value);
        bb_fullpath(block_full_path, block_path);

        retstat = log_syscall("lstat on blocks", lstat(block_full_path, &temp_statbuff), 0);
        statbuf->st_size += temp_statbuff.st_size;
        statbuf->st_blocks += temp_statbuff.st_blocks;

        log_msg("Statbuff Size: %d\n", statbuf->st_size);
    }
    
    log_syscall("close", close(meta_fd), 0);
    log_stat(statbuf);

    return retstat;
}

// int bb_getattr(const char *path, struct stat *statbuf)
// {
//     int retstat;
//     char fpath[PATH_MAX];
    
//     log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",
// 	  path, statbuf);
//     bb_fullpath(fpath, path);

//     retstat = log_syscall("lstat", lstat(fpath, statbuf), 0);
    
//     log_stat(statbuf);
    
//     return retstat;
// }

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
// Note the system readlink() will truncate and lose the terminating
// null.  So, the size passed to to the system readlink() must be one
// less than the size passed to bb_readlink()
// bb_readlink() code by Bernardo F Costa (thanks!)
int bb_readlink(const char *path, char *link, size_t size)
{
    int retstat;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
	  path, link, size);
    bb_fullpath(fpath, path);

    retstat = log_syscall("readlink", readlink(fpath, link, size - 1), 0);
    if (retstat >= 0) {
	link[retstat] = '\0';
	retstat = 0;
	log_msg("    link=\"%s\"\n", link);
    }
    
    return retstat;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
// shouldn't that comment be "if" there is no.... ?
int bb_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
	  path, mode, dev);
    bb_fullpath(fpath, path);
    
    // On Linux this could just be 'mknod(path, mode, dev)' but this
    // tries to be be more portable by honoring the quote in the Linux
    // mknod man page stating the only portable use of mknod() is to
    // make a fifo, but saying it should never actually be used for
    // that.
    if (S_ISREG(mode)) {
	retstat = log_syscall("open", open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode), 0);
	if (retstat >= 0)
	    retstat = log_syscall("close", close(retstat), 0);
    } else
	if (S_ISFIFO(mode))
	    retstat = log_syscall("mkfifo", mkfifo(fpath, mode), 0);
	else
	    retstat = log_syscall("mknod", mknod(fpath, mode, dev), 0);
    
    return retstat;
}

/** Create a directory */
int bb_mkdir(const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
    bb_fullpath(fpath, path);

    return log_syscall("mkdir", mkdir(fpath, mode), 0);
}

/* Remove a file */
int bb_unlink(const char *path)
{
    char fpath[PATH_MAX];
    char meta_path[PATH_MAX] = "";
    char meta_full_path[PATH_MAX] = "";
    char block_path[PATH_MAX] = "";
    char block_full_path[PATH_MAX] = "";
    char tmp_full_path[PATH_MAX] = "";
    int meta_fd;
    int block_fd;
    unsigned char block_hash_value[SHA_DIGEST_LENGTH * 2 + 1] = "";
    struct node* find_block;
    int found = 0;

    log_msg("bb_unlink(path=\"%s\")\n", path);
    
    strcpy(meta_path, METADATA);
    strcat(meta_path, path);
    bb_fullpath(meta_full_path, meta_path);

    // Read Meta File
    meta_fd = log_syscall("open meta", open(meta_full_path, O_RDWR, 0666), 0);
    if(meta_fd < 0){
        //File Al
        return BLOCKSIZE; 
    }

    while (log_syscall("read meta hash", read(meta_fd, (void *) block_hash_value, SHA_DIGEST_LENGTH * 2), 0) != 0){
        block_hash_value[SHA_DIGEST_LENGTH * 2] = '\0';
        find_block = myfind(block_hash_value);
        found = set_value(find_block, -1);
        if(found == 0){ 
            mydelete(block_hash_value);
            strcpy(block_path, BLOCKS);
            strcat(block_path, block_hash_value);
            bb_fullpath(block_full_path, block_path);
            log_syscall("unlink", unlink(block_full_path), 0);
        }
    } 
    close(meta_fd);
    bb_fullpath(tmp_full_path, path);
    log_syscall("unlink", unlink(tmp_full_path), 0);
    return log_syscall("unlink", unlink(meta_full_path), 0);
}

/** Remove a directory */
int bb_rmdir(const char *path)
{
    char fpath[PATH_MAX];
    
    log_msg("bb_rmdir(path=\"%s\")\n",
	    path);
    bb_fullpath(fpath, path);

    return log_syscall("rmdir", rmdir(fpath), 0);
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.
int bb_symlink(const char *path, const char *link)
{
    char flink[PATH_MAX];
    
    log_msg("\nbb_symlink(path=\"%s\", link=\"%s\")\n",
	    path, link);
    bb_fullpath(flink, link);

    return log_syscall("symlink", symlink(path, flink), 0);
}

/** Rename a file */
// both path and newpath are fs-relative
int bb_rename(const char *path, const char *newpath)
{
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];
    
    log_msg("\nbb_rename(fpath=\"%s\", newpath=\"%s\")\n",
	    path, newpath);
    bb_fullpath(fpath, path);
    bb_fullpath(fnewpath, newpath);

    return log_syscall("rename", rename(fpath, fnewpath), 0);
}

/** Create a hard link to a file */
int bb_link(const char *path, const char *newpath)
{
    char fpath[PATH_MAX], fnewpath[PATH_MAX];
    
    log_msg("\nbb_link(path=\"%s\", newpath=\"%s\")\n",
	    path, newpath);
    bb_fullpath(fpath, path);
    bb_fullpath(fnewpath, newpath);

    return log_syscall("link", link(fpath, fnewpath), 0);
}

/** Change the permission bits of a file */
int bb_chmod(const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chmod(fpath=\"%s\", mode=0%03o)\n",
	    path, mode);
    bb_fullpath(fpath, path);

    return log_syscall("chmod", chmod(fpath, mode), 0);
}

/** Change the owner and group of a file */
int bb_chown(const char *path, uid_t uid, gid_t gid)
  
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chown(path=\"%s\", uid=%d, gid=%d)\n",
	    path, uid, gid);
    bb_fullpath(fpath, path);

    return log_syscall("chown", chown(fpath, uid, gid), 0);
}

/** Change the size of a file */
int bb_truncate(const char *path, off_t newsize)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_truncate(path=\"%s\", newsize=%lld)\n",
	    path, newsize);
    bb_fullpath(fpath, path);

    return log_syscall("truncate", truncate(fpath, newsize), 0);
}

/** Change the access and/or modification times of a file */
/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int bb_utime(const char *path, struct utimbuf *ubuf)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_utime(path=\"%s\", ubuf=0x%08x)\n",
	    path, ubuf);
    bb_fullpath(fpath, path);

    return log_syscall("utime", utime(fpath, ubuf), 0);
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int bb_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);
    bb_fullpath(fpath, path);
    
    // if the open call succeeds, my retstat is the file descriptor,
    // else it's -errno.  I'm making sure that in that case the saved
    // file descriptor is exactly -1.
    fd = log_syscall("open", open(fpath, fi->flags), 0);
    if (fd < 0)
	retstat = log_error("open");
	
    fi->fh = fd;

    log_fi(fi);
    
    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.

int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    char meta_path[PATH_MAX] = "";
    char meta_full_path[PATH_MAX] = "";
    char block_path[PATH_MAX] = "";
    char block_full_path[PATH_MAX] = "";
    int meta_fd;
    int block_fd;
    unsigned char block_hash_value[SHA_DIGEST_LENGTH * 2 + 1] = "";
       
    int i = 0;
    
    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path

    //Add to "rootdir/.meta/{path}"
    strcpy(meta_path, METADATA);
    strcat(meta_path, path);
    bb_fullpath(meta_full_path, meta_path);

    // Read Meta File
    meta_fd = log_syscall("open meta", open(meta_full_path, O_RDWR, 0666), 0);
    if(meta_fd < 0){
        //File Al
        return 0; 
    }

    log_syscall("lseek meta", lseek(meta_fd, (offset / BLOCKSIZE) * (SHA_DIGEST_LENGTH * 2), SEEK_SET), 0);

    i = 0;
    while(log_syscall("read meta hash", read(meta_fd, (void *) block_hash_value, (SHA_DIGEST_LENGTH * 2)), 0) != 0){
        log_msg("block_hash_value: %s\n", block_hash_value);
        block_hash_value[SHA_DIGEST_LENGTH * 2] = '\0';
            
        //At "rootdir/.blocks/{hash}"
        // Read Block
        memset(block_path, 0, PATH_MAX);
        strcpy(block_path, BLOCKS);
        strcat(block_path, block_hash_value);
        bb_fullpath(block_full_path, block_path);

        block_fd = log_syscall("open block for read", open(block_full_path, O_RDWR, 0666), 0);
        log_syscall("read block", read(block_fd, (void*) buf + (i * BLOCKSIZE), BLOCKSIZE), 0);

        //Close File
        log_syscall("close block", close(block_fd), 0);
        i++;
    }

    log_msg("buf: %s\n", buf);

    log_fi(fi);
    log_syscall("close meta", close(meta_fd), 0);

    return size;
}

/*
int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    return log_syscall("pread", pread(fi->fh, buf, size, offset), 0);
}
*/

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int bb_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    int retstat = 0, i = 0, found = 0, length, x;
    unsigned char hash[SHA_DIGEST_LENGTH] = "";
    unsigned char old_hash[SHA_DIGEST_LENGTH * 2 + 1] = "";
    char fpath[PATH_MAX];
    char meta_path[PATH_MAX] = "";
    char block_path[PATH_MAX] = "";
    char block_full_path[PATH_MAX] = ""; 
    char meta_full_path[PATH_MAX] = "";
    int block_fd, meta_fd;
    char raw_name[SHA_DIGEST_LENGTH * 2];
    char old_block_path[PATH_MAX] = "";
    char old_block_full_path[PATH_MAX] = "";
    bool endfile = false;
    struct node* find_block;
    
    //Log Write    
    log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);

    //Add to "rootdir/meta/{path}"
    strcpy(meta_path, METADATA); 
    strcat(meta_path, path);
    bb_fullpath(meta_full_path, meta_path);

    if(access(meta_full_path,F_OK) == -1){
        endfile = true;
    } 


    // Create Meta File
    meta_fd = log_syscall("open meta", open(meta_full_path, O_CREAT | O_RDWR, 0666), 0);
    if(meta_fd < 0){
        //File Al
        return 0; 
    }

    log_syscall("lseek meta", lseek(meta_fd, (offset / BLOCKSIZE) * (SHA_DIGEST_LENGTH * 2), SEEK_SET), 0); 

    while( size/BLOCKSIZE != i ){
        // read meta file
        length = log_syscall("read meta", read(meta_fd, (void *) old_hash, SHA_DIGEST_LENGTH * 2), 0);
        if (length == 0)
        {
            endfile = true;
            break;
        }

        //Create Hash
        SHA1((unsigned char*) buf + (i * BLOCKSIZE), BLOCKSIZE, hash);
        for (int j=0; j < SHA_DIGEST_LENGTH; j++) {
            sprintf((char*)&(raw_name[j*2]), "%02x", hash[j]);
        }

        //Add to "rootdir/.blocks/{hash}"
        strcpy(block_path, BLOCKS);
        strcat(block_path, raw_name);
        bb_fullpath(block_full_path, block_path);

        // Create Storage File
        block_fd = log_syscall("open storage", open(block_full_path, O_CREAT | O_RDWR, 0666), 0);
        if(block_fd < 0){
            //File Al
            return 0; 
        }

        // Write to Storage File
        log_syscall("write storage", write(block_fd, (void*) buf + (i * BLOCKSIZE), BLOCKSIZE), 0);

        // Close Storage File
        log_syscall("close block", close(block_fd), 0);

        if( strcmp(old_hash, raw_name) != 0){
            find_block = myfind(old_hash);
            found = set_value(find_block, -1);
            if(found == 0){
                mydelete(old_hash);
                strcpy(old_block_path, BLOCKS);
                strcat(old_block_path, old_hash);
                bb_fullpath(old_block_full_path, old_block_path);
                log_syscall("unlink", unlink(old_block_full_path), 0);
            }
            find_block = myfind(raw_name);
            if(find_block == NULL){
                myinsert(raw_name);
            }else{
                set_value(find_block, 1);
            }
        } else {
            find_block = myfind(raw_name);
            set_value(find_block, 1);
        }        

        // Write to Meta File
        log_syscall("lseek meta", lseek(meta_fd, -(SHA_DIGEST_LENGTH * 2), SEEK_CUR), 0);
        log_syscall("write meta", write(meta_fd, (void *) raw_name, SHA_DIGEST_LENGTH*2), 0);
        memset(old_hash, 0, SHA_DIGEST_LENGTH * 2 + 1);
        i++;
    }
    
    while( size/BLOCKSIZE != i && endfile){
        //Create Hash
        SHA1((unsigned char*) buf + (i * BLOCKSIZE), BLOCKSIZE, hash);
        for (int j=0; j < SHA_DIGEST_LENGTH; j++) {
            sprintf((char*)&(raw_name[j*2]), "%02x", hash[j]);
        }

        //Add to "rootdir/.blocks/{hash}"
        strcpy(block_path, BLOCKS);
        strcat(block_path, raw_name);
        bb_fullpath(block_full_path, block_path);

        // Create Storage File
        block_fd = log_syscall("open storage", open(block_full_path, O_CREAT | O_RDWR, 0666), 0);
        if(block_fd < 0){
            //File Al
            return 0; 
        }

        // Write to Storage File
        log_syscall("write storage", write(block_fd, (void*) buf + (i * BLOCKSIZE), BLOCKSIZE), 0);

        // Close Storage File
        log_syscall("close block", close(block_fd), 0);

        find_block = myfind(raw_name);
        if (find_block == NULL)
            myinsert(raw_name);
        else{        
            set_value(find_block, 1);
        }        

        // Write to Meta File
        log_syscall("write meta", write(meta_fd, (void *) raw_name, SHA_DIGEST_LENGTH*2), 0);
        i++;
    }

    if( size % BLOCKSIZE != 0 && endfile){
        SHA1((unsigned char*) buf + (i * BLOCKSIZE), BLOCKSIZE, hash);
        for (int j=0; j < SHA_DIGEST_LENGTH; j++) {
            sprintf((char*)&(raw_name[j*2]), "%02x", hash[j]);
        }

        //Add to "rootdir/.blocks/{hash}"
        strcpy(block_path, BLOCKS);
        strcat(block_path, raw_name);
        bb_fullpath(block_full_path, block_path);

        // Create Storage File
        block_fd = log_syscall("open storage", open(block_full_path, O_CREAT | O_RDWR, 0666), 0);
        if(block_fd < 0){
            //File Al
            return 0; 
        }

        // Write to Storage File
        log_syscall("write storage", write(block_fd, (void*) buf + (i * BLOCKSIZE), size % BLOCKSIZE), 0);

        // Close Storage File
        log_syscall("close block", close(block_fd), 0);

        
        find_block = myfind(raw_name);
        if (find_block == NULL)
            myinsert(raw_name);
        else{        
            set_value(find_block, 1);
        }              

        // Write to Meta File
        log_syscall("write meta", write(meta_fd, (void *) raw_name, SHA_DIGEST_LENGTH*2), 0);
        i++;
    }
   
    // Close Meta File
    log_syscall("close meta", close(meta_fd), 0);
    
    return size;
}


/*
int bb_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi
	    );
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    return log_syscall("pwrite", pwrite(fi->fh, buf, size, offset), 0);
}
*/

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int bb_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_statfs(path=\"%s\", statv=0x%08x)\n",
	    path, statv);
    bb_fullpath(fpath, path);
    
    // get stats for underlying filesystem
    retstat = log_syscall("statvfs", statvfs(fpath, statv), 0);
    
    log_statvfs(statv);
    
    return retstat;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
// this is a no-op in BBFS.  It just logs the call and returns success
int bb_flush(const char *path, struct fuse_file_info *fi)
{
    log_msg("\nbb_flush(path=\"%s\", fi=0x%08x)\n", path, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
	
    return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int bb_release(const char *path, struct fuse_file_info *fi)
{
    log_msg("\nbb_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    log_fi(fi);

    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    return log_syscall("close", close(fi->fh), 0);
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int bb_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    log_msg("\nbb_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);
    
    // some unix-like systems (notably freebsd) don't have a datasync call
#ifdef HAVE_FDATASYNC
    if (datasync)
	return log_syscall("fdatasync", fdatasync(fi->fh), 0);
    else
#endif	
	return log_syscall("fsync", fsync(fi->fh), 0);
}

#ifdef HAVE_SYS_XATTR_H
/** Note that my implementations of the various xattr functions use
    the 'l-' versions of the functions (eg bb_setxattr() calls
    lsetxattr() not setxattr(), etc).  This is because it appears any
    symbolic links are resolved before the actual call takes place, so
    I only need to use the system-provided calls that don't follow
    them */

/** Set extended attributes */
int bb_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_setxattr(path=\"%s\", name=\"%s\", value=\"%s\", size=%d, flags=0x%08x)\n",
	    path, name, value, size, flags);
    bb_fullpath(fpath, path);

    return log_syscall("lsetxattr", lsetxattr(fpath, name, value, size, flags), 0);
}

/** Get extended attributes */
int bb_getxattr(const char *path, const char *name, char *value, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_getxattr(path = \"%s\", name = \"%s\", value = 0x%08x, size = %d)\n",
	    path, name, value, size);
    bb_fullpath(fpath, path);

    retstat = log_syscall("lgetxattr", lgetxattr(fpath, name, value, size), 0);
    if (retstat >= 0)
	log_msg("    value = \"%s\"\n", value);
    
    return retstat;
}

/** List extended attributes */
int bb_listxattr(const char *path, char *list, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char *ptr;
    
    log_msg("\nbb_listxattr(path=\"%s\", list=0x%08x, size=%d)\n",
	    path, list, size
	    );
    bb_fullpath(fpath, path);

    retstat = log_syscall("llistxattr", llistxattr(fpath, list, size), 0);
    if (retstat >= 0) {
	log_msg("    returned attributes (length %d):\n", retstat);
	if (list != NULL)
	    for (ptr = list; ptr < list + retstat; ptr += strlen(ptr)+1)
		log_msg("    \"%s\"\n", ptr);
	else
	    log_msg("    (null)\n");
    }
    
    return retstat;
}

/** Remove extended attributes */
int bb_removexattr(const char *path, const char *name)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_removexattr(path=\"%s\", name=\"%s\")\n",
	    path, name);
    bb_fullpath(fpath, path);

    return log_syscall("lremovexattr", lremovexattr(fpath, name), 0);
}
#endif

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int bb_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_opendir(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    bb_fullpath(fpath, path);

    // since opendir returns a pointer, takes some custom handling of
    // return status.
    dp = opendir(fpath);
    log_msg("    opendir returned 0x%p\n", dp);
    if (dp == NULL)
	retstat = log_error("bb_opendir opendir");
    
    fi->fh = (intptr_t) dp;
    
    log_fi(fi);
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */

int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    int retstat = 0;
    DIR *dp;
    struct dirent *de;
    
    log_msg("\nbb_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
	    path, buf, filler, offset, fi);
    // once again, no need for fullpath -- but note that I need to cast fi->fh
    dp = (DIR *) (uintptr_t) fi->fh;

    // Every directory contains at least two entries: . and ..  If my
    // first call to the system readdir() returns NULL I've got an
    // error; near as I can tell, that's the only condition under
    // which I can get an error from readdir()
    de = readdir(dp);
    log_msg("    readdir returned 0x%p\n", de);
    if (de == 0) {
	retstat = log_error("bb_readdir readdir");
	return retstat;
    }

    // This will copy the entire directory into the buffer.  The loop exits
    // when either the system readdir() returns NULL, or filler()
    // returns something non-zero.  The first case just means I've
    // read the whole directory; the second means the buffer is full.
    do {
	log_msg("calling filler with name %s\n", de->d_name);
	if (filler(buf, de->d_name, NULL, 0) != 0) {
	    log_msg("    ERROR bb_readdir filler:  buffer full");
	    return -ENOMEM;
	}
    } while ((de = readdir(dp)) != NULL);
    
    log_fi(fi);
    
    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int bb_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_releasedir(path=\"%s\", fi=0x%08x)\n",
	    path, fi);
    log_fi(fi);
    
    closedir((DIR *) (uintptr_t) fi->fh);
    
    return retstat;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
// when exactly is this called?  when a user calls fsync and it
// happens to be a directory? ??? >>> I need to implement this...
int bb_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fsyncdir(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);
    
    return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
// Undocumented but extraordinarily useful fact:  the fuse_context is
// set up before this function is called, and
// fuse_get_context()->private_data returns the user_data passed to
// fuse_main().  Really seems like either it should be a third
// parameter coming in here, or else the fact should be documented
// (and this might as well return void, as it did in older versions of
// FUSE).
void *bb_init(struct fuse_conn_info *conn)
{
    char path[PATH_MAX];
    log_msg("\nbb_init()\n");

    //Create a directory to store the blocks
    bb_fullpath(path, BLOCKS);
    blocks_fd = log_syscall("Creating Blocks directory", mkdir(path, S_IRWXU), 0);
    
    //Create a directory to store the metadata
    bb_fullpath(path, METADATA);
    metadata_fd = log_syscall("Creating Metadata directory", mkdir(path, S_IRWXU), 0);
    
    //Create a directory to store the hash values
    myinit();
    log_msg("Init the array");

    bb_fullpath(path, LOAD);
    if(access(path, F_OK) != -1)
    {
        log_msg("Load the array");
        myload();
        unlink(path);
    }

    //Create a directory to store the hash values

    log_conn(conn);
    log_fuse_context(fuse_get_context());
    
    return BB_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void bb_destroy(void *userdata)
{
    log_msg("\nbb_destroy(userdata=0x%08x)\n", userdata);
    char path[PATH_MAX];

    if(size_of_root > 0)
    {
        log_msg("Save the array");
        mysave();
    }

    mydestroy();
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int bb_access(const char *path, int mask)
{
    int retstat = 0;
    char fpath[PATH_MAX];
   
    log_msg("\nbb_access(path=\"%s\", mask=0%o)\n",
	    path, mask);
    bb_fullpath(fpath, path);
    
    retstat = access(fpath, mask);
    
    if (retstat < 0)
	retstat = log_error("bb_access access");
    
    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
// Not implemented.  I had a version that used creat() to create and
// open the file, which it turned out opened the file write-only.

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int bb_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_ftruncate(path=\"%s\", offset=%lld, fi=0x%08x)\n",
	    path, offset, fi);
    log_fi(fi);
    
    retstat = ftruncate(fi->fh, offset);
    if (retstat < 0)
	retstat = log_error("bb_ftruncate ftruncate");
    
    return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fgetattr(path=\"%s\", statbuf=0x%08x, fi=0x%08x)\n",
	    path, statbuf, fi);
    log_fi(fi);

    // On FreeBSD, trying to do anything with the mountpoint ends up
    // opening it, and then using the FD for an fgetattr.  So in the
    // special case of a path of "/", I need to do a getattr on the
    // underlying root directory instead of doing the fgetattr().
    if (!strcmp(path, "/"))
	return bb_getattr(path, statbuf);
    
    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0)
	retstat = log_error("bb_fgetattr fstat");
    
    log_stat(statbuf);
    
    return retstat;
}

struct fuse_operations bb_oper = {
  .getattr = bb_getattr,
  .readlink = bb_readlink,
  // no .getdir -- that's deprecated
  .getdir = NULL,
  .mknod = bb_mknod,
  .mkdir = bb_mkdir,
  .unlink = bb_unlink,
  .rmdir = bb_rmdir,
  .symlink = bb_symlink,
  .rename = bb_rename,
  .link = bb_link,
  .chmod = bb_chmod,
  .chown = bb_chown,
  .truncate = bb_truncate,
  .utime = bb_utime,
  .open = bb_open,
  .read = bb_read,
  .write = bb_write,
  /** Just a placeholder, don't set */ // huh???
  .statfs = bb_statfs,
  .flush = bb_flush,
  .release = bb_release,
  .fsync = bb_fsync,
  
#ifdef HAVE_SYS_XATTR_H
  .setxattr = bb_setxattr,
  .getxattr = bb_getxattr,
  .listxattr = bb_listxattr,
  .removexattr = bb_removexattr,
#endif
  
  .opendir = bb_opendir,
  .readdir = bb_readdir,
  .releasedir = bb_releasedir,
  .fsyncdir = bb_fsyncdir,
  .init = bb_init,
  .destroy = bb_destroy,
  .access = bb_access,
  .ftruncate = bb_ftruncate,
  .fgetattr = bb_fgetattr
};

void bb_usage()
{
    fprintf(stderr, "usage:  bbfs [FUSE and mount options] rootDir mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct bb_state *bb_data;

    // bbfs doesn't do any access checking on its own (the comment
    // blocks in fuse.h mention some of the functions that need
    // accesses checked -- but note there are other functions, like
    // chown(), that also need checking!).  Since running bbfs as root
    // will therefore open Metrodome-sized holes in the system
    // security, we'll check if root is trying to mount the filesystem
    // and refuse if it is.  The somewhat smaller hole of an ordinary
    // user doing it with the allow_other flag is still there because
    // I don't want to parse the options string.
    if ((getuid() == 0) || (geteuid() == 0)) {
    	fprintf(stderr, "Running BBFS as root opens unnacceptable security holes\n");
    	return 1;
    }

    // See which version of fuse we're running
    fprintf(stderr, "Fuse library version %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);
    
    // Perform some sanity checking on the command line:  make sure
    // there are enough arguments, and that neither of the last two
    // start with a hyphen (this will break if you actually have a
    // rootpoint or mountpoint whose name starts with a hyphen, but so
    // will a zillion other programs)
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	bb_usage();

    bb_data = malloc(sizeof(struct bb_state));
    if (bb_data == NULL) {
	perror("main calloc");
	abort();
    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    bb_data->rootdir = realpath(argv[argc-2], NULL);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    bb_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &bb_oper, bb_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}