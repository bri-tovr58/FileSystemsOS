/*
  MyFS: a tiny file-system written for educational purposes
  MyFS is 
  Copyright 2018-21 by
  University of Alaska Anchorage, College of Engineering.
  Copyright 2022
  University of Texas at El Paso, Department of Computer Science.
  Contributors: Christoph Lauter
                Roxxanne White
                Alan Ochoa and
                Natasha Rovelli
  and based on 
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
  gcc -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs
*/

#include <stddef.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>


/* The filesystem you implement must support all the 13 operations
   stubbed out below. There need not be support for access rights,
   links, symbolic links. There needs to be support for access and
   modification times and information for statfs.
   The filesystem must run in memory, using the memory of size 
   fssize pointed to by fsptr. The memory comes from mmap and 
   is backed with a file if a backup-file is indicated. When
   the filesystem is unmounted, the memory is written back to 
   that backup-file. When the filesystem is mounted again from
   the backup-file, the same memory appears at the newly mapped
   in virtual address. The filesystem datastructures hence must not
   store any pointer directly to the memory pointed to by fsptr; it
   must rather store offsets from the beginning of the memory region.
   When a filesystem is mounted for the first time, the whole memory
   region of size fssize pointed to by fsptr reads as zero-bytes. When
   a backup-file is used and the filesystem is mounted again, certain
   parts of the memory, which have previously been written, may read
   as non-zero bytes. The size of the memory region is at least 2048
   bytes.
   CAUTION:
   * You MUST NOT use any global variables in your program for reasons
   due to the way FUSE is designed.
   You can find ways to store a structure containing all "global" data
   at the start of the memory region representing the filesystem.
   * You MUST NOT store (the value of) pointers into the memory region
   that represents the filesystem. Pointers are virtual memory
   addresses and these addresses are ephemeral. Everything will seem
   okay UNTIL you remount the filesystem again.
   You may store offsets/indices (of type size_t) into the
   filesystem. These offsets/indices are like pointers: instead of
   storing the pointer, you store how far it is away from the start of
   the memory region. You may want to define a type for your offsets
   and to write two functions that can convert from pointers to
   offsets and vice versa.
   * You may use any function out of libc for your filesystem,
   including (but not limited to) malloc, calloc, free, strdup,
   strlen, strncpy, strchr, strrchr, memset, memcpy. However, your
   filesystem MUST NOT depend on memory outside of the filesystem
   memory region. Only this part of the virtual memory address space
   gets saved into the backup-file. As a matter of course, your FUSE
   process, which implements the filesystem, MUST NOT leak memory: be
   careful in particular not to leak tiny amounts of memory that
   accumulate over time. In a working setup, a FUSE process is
   supposed to run for a long time!
   It is possible to check for memory leaks by running the FUSE
   process inside valgrind:
   valgrind --leak-check=full ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f
   However, the analysis of the leak indications displayed by valgrind
   is difficult as libfuse contains some small memory leaks (which do
   not accumulate over time). We cannot (easily) fix these memory
   leaks inside libfuse.
   * Avoid putting debug messages into the code. You may use fprintf
   for debugging purposes but they should all go away in the final
   version of the code. Using gdb is more professional, though.
   * You MUST NOT fail with exit(1) in case of an error. All the
   functions you have to implement have ways to indicated failure
   cases. Use these, mapping your internal errors intelligently onto
   the POSIX error conditions.
   * And of course: your code MUST NOT SEGFAULT!
   It is reasonable to proceed in the following order:
   (1)   Design and implement a mechanism that initializes a filesystem
         whenever the memory space is fresh. That mechanism can be
         implemented in the form of a filesystem handle into which the
         filesystem raw memory pointer and sizes are translated.
         Check that the filesystem does not get reinitialized at mount
         time if you initialized it once and unmounted it but that all
         pieces of information (in the handle) get read back correctly
         from the backup-file. 
   (2)   Design and implement functions to find and allocate free memory
         regions inside the filesystem memory space. There need to be 
         functions to free these regions again, too. Any "global" variable
         goes into the handle structure the mechanism designed at step (1) 
         provides.
   (3)   Carefully design a data structure able to represent all the
         pieces of information that are needed for files and
         (sub-)directories.  You need to store the location of the
         root directory in a "global" variable that, again, goes into the 
         handle designed at step (1).
          
   (4)   Write __myfs_getattr_implem and debug it thoroughly, as best as
         you can with a filesystem that is reduced to one
         function. Writing this function will make you write helper
         functions to traverse paths, following the appropriate
         subdirectories inside the file system. Strive for modularity for
         these filesystem traversal functions.
   (5)   Design and implement __myfs_readdir_implem. You cannot test it
         besides by listing your root directory with ls -la and looking
         at the date of last access/modification of the directory (.). 
         Be sure to understand the signature of that function and use
         caution not to provoke segfaults nor to leak memory.
   (6)   Design and implement __myfs_mknod_implem. You can now touch files 
         with 
         touch foo
         and check that they start to exist (with the appropriate
         access/modification times) with ls -la.
   (7)   Design and implement __myfs_mkdir_implem. Test as above.
   (8)   Design and implement __myfs_truncate_implem. You can now 
         create files filled with zeros:
         truncate -s 1024 foo
   (9)   Design and implement __myfs_statfs_implem. Test by running
         df before and after the truncation of a file to various lengths. 
         The free "disk" space must change accordingly.
   (10)  Design, implement and test __myfs_utimens_implem. You can now 
         touch files at different dates (in the past, in the future).
   (11)  Design and implement __myfs_open_implem. The function can 
         only be tested once __myfs_read_implem and __myfs_write_implem are
         implemented.
   (12)  Design, implement and test __myfs_read_implem and
         __myfs_write_implem. You can now write to files and read the data 
         back:
         echo "Hello world" > foo
         echo "Hallo ihr da" >> foo
         cat foo
         Be sure to test the case when you unmount and remount the
         filesystem: the files must still be there, contain the same
         information and have the same access and/or modification
         times.
   (13)  Design, implement and test __myfs_unlink_implem. You can now
         remove files.
   (14)  Design, implement and test __myfs_unlink_implem. You can now
         remove directories.
   (15)  Design, implement and test __myfs_rename_implem. This function
         is extremely complicated to implement. Be sure to cover all 
         cases that are documented in man 2 rename. The case when the 
         new path exists already is really hard to implement. Be sure to 
         never leave the filessystem in a bad state! Test thoroughly 
         using mv on (filled and empty) directories and files onto 
         inexistant and already existing directories and files.
   (16)  Design, implement and test any function that your instructor
         might have left out from this list. There are 13 functions 
         __myfs_XXX_implem you have to write.
   (17)  Go over all functions again, testing them one-by-one, trying
         to exercise all special conditions (error conditions): set
         breakpoints in gdb and use a sequence of bash commands inside
         your mounted filesystem to trigger these special cases. Be
         sure to cover all funny cases that arise when the filesystem
         is full but files are supposed to get written to or truncated
         to longer length. There must not be any segfault; the user
         space program using your filesystem just has to report an
         error. Also be sure to unmount and remount your filesystem,
         in order to be sure that it contents do not change by
         unmounting and remounting. Try to mount two of your
         filesystems at different places and copy and move (rename!)
         (heavy) files (your favorite movie or song, an image of a cat
         etc.) from one mount-point to the other. None of the two FUSE
         processes must provoke errors. Find ways to test the case
         when files have holes as the process that wrote them seeked
         beyond the end of the file several times. Your filesystem must
         support these operations at least by making the holes explicit 
         zeros (use dd to test this aspect).
   (18)  Run some heavy testing: copy your favorite movie into your
         filesystem and try to watch it out of the filesystem.
*/

/* Helper types and functions */

// Macros ------------------------------------------
#define MYFS_MAXIMUM_NAME_LENGTH (256)
#define MYFS_STATIC_PATH_BYF_SIZE (8192)
#define MYFS_TRUNCATE_SMALL_ALLOCATE ((size_t) 512)
#define MYFS_MAGIC ((uint32_t) (UINT32_C(OxCAFEBABE)))



// Offsets  ----------------------------------------
typedef size_t __myfs_off_t;

static inline void * offset_to_ptr(void *fsptr, __myfs_off_t off) {
      if (off == ((__myfs_off_t) 0)) return NULL;
      return fsptr + off;
}


static inline __myfs_off_t ptr_to_offset(void *fsptr, void *ptr) {
      if (ptr == NULL) return ((__myfs_off_t)0);
      if (ptr <= fsptr) return ((__myfs_off_t)0);
      return (__myfs_off_t)(ptr - fsptr);
}
// ----------------------------------------------



// Memory block structure -------------------------
struct __myfs_mem_block_struct_t {
	size_t size;
	size_t user_size;
	__myfs_off_t next;
};
typedef struct __myfs_mem_block_struct_t *__myfs_handle_t;



// Handler structure -------------------------
struct __myfs_handle_struct_t {
	uint32_t magic;
	__myfs_off_t free_memory;
	__myfs_off_t root_dir;
	size_t size;
};
typedef struct __myfs_handle_struct_t *__myfs_handle_t;



/* types for inode entries */
typedef enum __myfs_inode_type_enum_t __myfs_inode_type_t;
enum __myfs_inode_type_enum_t {
	DIRECTORY,
	REG_FILE
};



/* inode file */
typedef struct __myfs_inode_file_struct_t __myfs_inode_file_t;
struct __myfs_inode_file_struct_t {
	size_t size;
	__myfs_off_t first_block;
};



/* File Block Structure */ 
typedef struct __myfs_file_block_t __myfs_file_block_t;
struct __myfs_file_block_t{
	size_t size;
	size_t allocated;
	__myfs_off_t next;
	__myfs_off_t data;
};



/* Directory Structure */ 
typedef struct __myfs_inode_directory_struct_t __myfs_inode_directory_t;
struct __myfs_inode_directory_struct_t{
	size_t number_children;
	__myfs_off_t children;
};



/* Incomplete */ 
typedef struct __myfs_inode_struct_t __myfs_inode_t;
struct __myfs_inode_struct_t {
	__myfs_inode_type_t type;
	char name[MYFS_MAXIMUM_NAME_LENGTH];
	struct timespec times[2];
	union {
		__myfs_inode_file_t file;
            __myfs_inode_directory_t directory;
		// incomplete
      } type;
} 


static __myfs_inode_t * __myfs_path_resolve(__myfs_handle_t handle, const char *path); 
static __myfs_inode_t * __myfs_path_resolve_one_step(__myfs_handle_t handle, __myfs_inode_t *curr, const char *path); //incomplete

static void __myfs_set_filename(char *dest, const char *src){
  dest[MYFS_MAXIMUM_NAME_LENGTH -1] = '/0';
  strncpy(dest, src, MYFS_MAXIMUM_NAME_LENGTH -1);
}

static void __myfs_set_curr_time(); //incomplete 

/* End of helper functions */

/* Implements an emulation of the stat system call on the filesystem 
   of size fssize pointed to by fsptr. 
   
   If path can be followed and describes a file or directory 
   that exists and is accessable, the access information is 
   put into stbuf. 
   On success, 0 is returned. On failure, -1 is returned and 
   the appropriate error code is put into *errnoptr.
   man 2 stat documents all possible error codes and gives more detail
   on what fields of stbuf need to be filled in. Essentially, only the
   following fields need to be supported:
   st_uid      the value passed in argument
   st_gid      the value passed in argument
   st_mode     (as fixed values S_IFDIR | 0755 for directories,
                                S_IFREG | 0755 for files)
   st_nlink    (as many as there are subdirectories (not files) for directories
                (including . and ..),
                1 for files)
   st_size     (supported only for files, where it is the real file size)
   st_atim
   st_mtim
*/
int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr,
                          uid_t uid, gid_t gid,
                          const char *path, struct stat *stbuf) {
  __myfs_handle_t handle;
  __myfs_inode_t *node;
  
  handle = __myfs_get_handle(fsptr, fssize);
  
  if (handle == NULL){
	  errnoptr = EFAULT;
	  return -1;
  }
  
  node = __myfs_path_resolve(handle, path);

  if (node == NULL){
	  *errnoptr = ENOENT;
	  return -1;
  }
  
  memset(stbuf, 0, sizeof(struct stat));
  
  stbuf->st_uid = uid; 
  stbuf->st_gid = gid;
  // still incomplete

  return -1;
}

/* Implements an emulation of the readdir system call on the filesystem 
   of size fssize pointed to by fsptr. 
   If path can be followed and describes a directory that exists and
   is accessable, the names of the subdirectories and files 
   contained in that directory are output into *namesptr. The . and ..
   directories must not be included in that listing.
   If it needs to output file and subdirectory names, the function
   starts by allocating (with calloc) an array of pointers to
   characters of the right size (n entries for n names). Sets
   *namesptr to that pointer. It then goes over all entries
   in that array and allocates, for each of them an array of
   characters of the right size (to hold the i-th name, together 
   with the appropriate '\0' terminator). It puts the pointer
   into that i-th array entry and fills the allocated array
   of characters with the appropriate name. The calling function
   will call free on each of the entries of *namesptr and 
   on *namesptr.
   The function returns the number of names that have been 
   put into namesptr. 
   If no name needs to be reported because the directory does
   not contain any file or subdirectory besides . and .., 0 is 
   returned and no allocation takes place.
   On failure, -1 is returned and the *errnoptr is set to 
   the appropriate error code. 
   The error codes are documented in man 2 readdir.
   In the case memory allocation with malloc/calloc fails, failure is
   indicated by returning -1 and setting *errnoptr to EINVAL.
*/
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, char ***namesptr) {
  /* check handle 
  resolve path, must be dir 
  return # children, excluding . and .. (return 0) 
  allocate array of pointers to chars using malloc calloc 
  each child is a char * 
  EINVAL 
  copy strings 
  free names 
  return pointer to the array(of child arrays) we allocated 
		must allocate array and each child array (main array has no \0, number should match children count)
  */
  
  __myfs_handle_t handle;
  __myfs_inode_t *node;
  char **names;
  size_t i,k;
  off_t *kids;
  //inode_t *tmp; UNSURE IF ACTUALLY INODE_T 
  char *name; 

  //names = calloc(node->value.directory, number_children, sizeof(char*));
  
  if (names == NULL) {
      errnoptr = EINVAL;
      return -1;
  }

  //kids = (myfs_offset_t *)



 // name = strdup(tmp->name);
 // if (name == NULL)  
  return -1;
}

/* Implements an emulation of the mknod system call for regular files
   on the filesystem of size fssize pointed to by fsptr.
   This function is called only for the creation of regular files.
   If a file gets created, it is of size zero and has default
   ownership and mode bits.
   The call creates the file indicated by path.
   On success, 0 is returned.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The error codes are documented in man 2 mknod.
*/
int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  char *parent, *newfile;
  __myfs_handle_t handle;
  __myfs_off_t *node, prev;
  __myfs_inode_t *new;
  //__myfs_off_t 
  
  if (path == NULL) {
      *errnoptr = ENOENT;
      return -1;
  }

  parent = NULL;
  newfile = NULL;

  if (node->type != DIR) {
      free(parent);
      free(newfile);
  }


  if (prev != NULL) {

  }

  if (strlen(newfile) > MYFS_MAXIMUM_NAME_LENGTH) {

  }

  if (strchr(newfile , '/')!= NULL) {

  }


  new = (__myfs_inode_t*) __myfs_offset_to_ptr; 
  // create file 
  // check for name too long 
  // check for slashes 
  // allocate memory 
  // if no memory say no space 
  // new child in child's directory 
  // if no space, free empty file again 
  // reallocate 
  // free 
  
  return -1;
}

/* Implements an emulation of the unlink system call for regular files
   on the filesystem of size fssize pointed to by fsptr.
   This function is called only for the deletion of regular files.
   On success, 0 is returned.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The error codes are documented in man 2 unlink.
*/
int __myfs_unlink_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  // destroys a file 
  /* if path = null, error 
	get handle, if no file, error 
	can not delete directory as regular file 
	find the parent path, modify # kids 
	if deleting name too long, error
	can't delete file with slash
	set current time of modification 
	if no more kids deallocate kids array 
	free memory */

      if (path == NULL) {
            *errnoptr = ENOENT;
            return -1;
      }

      handle = get_handle();
      
      if (handle = NULL) {
            errnoptr = EFAULT;
            return -1;
      }

      node = __myfs_path_resolve
      if (node->type != something) {
      
      }


      //myfs_cut_path_parent_address_base
      if (parent == NULL){}
      if (strchr(newfile , '/')!= NULL) {
            free(parent);
            free(file);
            *errnoptr = ENOTDIR;
            return -1;
      }

      //if (node_off ==)
  return -1;
}

/* Implements an emulation of the rmdir system call on the filesystem 
   of size fssize pointed to by fsptr. 
   The call deletes the directory indicated by path.
   On success, 0 is returned.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The function call must fail when the directory indicated by path is
   not empty (if there are files or subdirectories other than . and ..).
   The error codes are documented in man 2 rmdir.
*/
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* path null, error
  can't do if empty 
  can't find parent path error 
  name too long error 
  name has slash error 
  free */
    __myfs_handle_t handle;
    //__myfs_inode_t *node,
    // path = NULL;
    // handler = NULL
    // node = NULL 
  
    //  if (node->value)

  return -1;
}

/* Implements an emulation of the mkdir system call on the filesystem 
   of size fssize pointed to by fsptr. 
   The call creates the directory indicated by path.
   On success, 0 is returned.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The error codes are documented in man 2 mkdir.
*/
int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* create dir inside dir 
	if path null, error
	already exists error 
	no memory error */
      
      char *parent, *newDir;
      __myfs_handle_t handle;
      __myfs_inode_t *node;
  
      return -1;
}

/* Implements an emulation of the rename system call on the filesystem 
   of size fssize pointed to by fsptr. 
   The call moves the file or directory indicated by from to to.
   On success, 0 is returned.
   On failure, -1 is returned and *errnoptr is set appropriately.
   Caution: the function does more than what is hinted to by its name.
   In cases the from and to paths differ, the file is moved out of 
   the from path and added to the to path.
   The error codes are documented in man 2 rename.
*/
int __myfs_rename_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *from, const char *to) {
      __myfs_handle_t handle;
      __myfs_inode_t *fromNode, *fromDir;
  /* STUB */
  return -1;
}

/* Implements an emulation of the truncate system call on the filesystem 
   of size fssize pointed to by fsptr. 
   The call changes the size of the file indicated by path to offset
   bytes.
   When the file becomes smaller due to the call, the extending bytes are
   removed. When it becomes larger, zeros are appended.
   On success, 0 is returned.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The error codes are documented in man 2 truncate.
*/
int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr,
                           const char *path, off_t offset) {
  /* make file smaller or bigger 
  smaller = byte removed 
  bigger = 0's appended 
  find handle
  find node
  if not file error
  neg size error 
  new size = old size, only set current time 
  smaller, find block where to cut 
  free memory 
  bigger, allocate more space for 0s 
  if can't use *2, or create new block, or 'no spaces left'
  fill 0s using memset */
  
  __myfs_handle_t handle;
  __myfs_inode_t *node;
  //size_t size;
  //off_t off
  handle = __myfs_get_handle(fsptr,fssize);
  node = __myfs_path_resolve();

      // (node == null) 
      // node->type != REG_FILE

      //new_size = (size_t) offset;
      // off = (off_t) realSize;
      // if (off != offset){} 

      // alloc = new_size;
      // ms = allocate_mem

      // memset(ms,struct,0,ms_len)
  return -1;
}

/* Implements an emulation of the open system call on the filesystem 
   of size fssize pointed to by fsptr, without actually performing the opening
   of the file (no file descriptor is returned).
   The call just checks if the file (or directory) indicated by path
   can be accessed, i.e. if the path can be followed to an existing
   object for which the access rights are granted.
   On success, 0 is returned.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The two only interesting error codes are 
   * EFAULT: the filesystem is in a bad state, we can't do anything
   * ENOENT: the file that we are supposed to open doesn't exist (or a
             subpath).
   It is possible to restrict ourselves to only these two error
   conditions. It is also possible to implement more detailed error
   condition answers.
   The error codes are documented in man 2 open.
*/
int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path) {
  /* open file, check if file 
  get handle 
  get node 
  return 0 */
  
  __myfs_handle_t handle;
  __myfs_inode_t *node;

  handle = __myfs_get_handle == NULL;
  if (handle == NULL) {
      *errnoptr = EFAULT;
      return -1;
  }

  return -1;
}

/* Implements an emulation of the read system call on the filesystem 
   of size fssize pointed to by fsptr.
   The call copies up to size bytes from the file indicated by 
   path into the buffer, starting to read at offset. See the man page
   for read for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes read into the buffer is
   returned. The value zero is returned on an end-of-file condition.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The error codes are documented in man 2 read.
*/
int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path, char *buf, size_t size, off_t offset) {
  /* read certain amt bytes out of file starting at offset 
  find hande, node 
  must be file 
  must be positive integer, not beyond lengths of file 
  read 0 bytes, give 0 
  otherwise, find data blocks and copy from memory to read
  memccpy
  15 bytes out of 10, return 10 bytes
  */

      int res;
      //__myfs_handle_t
      //size_t off,cpy;
      //off_t
      //void *
  
      res = -1;
      //handle = __myfs_get_handle() 

      //sizeof(off_t <  sizeof(size_t))
      //if (offset >=((off_t) (node->file_size)) 
      //else 


      // cpy 
      // memcpy(buf + already_read, mc_start, mc_len)
      // already_read += mc_len;
      // to_read -= mc_len;

  return -1;
}

/* Implements an emulation of the write system call on the filesystem 
   of size fssize pointed to by fsptr.
   The call copies up to size bytes to the file indicated by 
   path into the buffer, starting to write at offset. See the man page
   for write for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes written into the file is
   returned. The value zero is returned on an end-of-file condition.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The error codes are documented in man 2 write.
*/
int __myfs_write_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path, const char *buf, size_t size, off_t offset) {
  /* write beyond file, at precicely the end, make file longer 
  write middle of file, modify file
  recycle truncate to make file right length 
  write chunks to file 
  memccpy from buffer to file
  free */ 
  
  // Professor suggested a recycling of truncate to use for write 

      int res;
      __myfs_handle_t handle;
      __myfs_inode_t *node;
      //size_t off, minSize

      //if (node->value file_size < minSize)
      //mc_start = __myfs_offset_to_ptr(handle, ((__myfs_file_block_t *) __myfs_offset_to_ptr(handle,curr))->data);
      //mc_len



  
  return -1;
}

/* Implements an emulation of the utimensat system call on the filesystem 
   of size fssize pointed to by fsptr.
   The call changes the access and modification times of the file
   or directory indicated by path to the values in ts.
   On success, 0 is returned.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The error codes are documented in man 2 utimensat.
*/
int __myfs_utimens_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, const struct timespec ts[2]) {
  /* get handle
  get path
  change node times 
  */
 __myfs_handle_t handle;
 __myfs_inode_t *node;

 handle = __myfs_get_handle(fsptr, fssize);
 node = __myfs_path_resolve(fsptr, path);
 
 node->time[0] = ts[0];
 node->time[1] = ts[1];

 return 0;   
}

/* Implements an emulation of the statfs system call on the filesystem 
   of size fssize pointed to by fsptr.
   The call gets information of the filesystem usage and puts in 
   into stbuf.
   On success, 0 is returned.
   On failure, -1 is returned and *errnoptr is set appropriately.
   The error codes are documented in man 2 statfs.
   Essentially, only the following fields of struct statvfs need to be
   supported:
   f_bsize   fill with what you call a block (typically 1024 bytes)
   f_blocks  fill with the total number of blocks in the filesystem
   f_bfree   fill with the free number of blocks in the filesystem
   f_bavail  fill with same value as f_bfree
   f_namemax fill with your maximum file/directory name, if your
             filesystem has such a maximum
*/
int __myfs_statfs_implem(void *fsptr, size_t fssize, int *errnoptr,
                         struct statvfs* stbuf) {
  /* get handle
  how many blocks are our size 1024(1 byte, can use other sizes)
  blocks = systemsize/ size 
  free blocks 
  available blocks 
  name max length */ 

  __myfs_handle_t handle;
  handle = __myfs_get_handle(fsptr,fssize);
  
  if (handle == NULL) {
      errnoptr = EFAULT;
      return -1;
  }
  
  memset(stbuf,0,sizeof(struct statvfs));

  stbuf->f_bsize;
  stbuf->f_blocks = fsblkcnt_t( (handle->size) / ((size_t stbuf->f_bsize) /* SOMETHING MISSING HERE */ );
  // stbuf->f_bfree(fsblkcnt_t(( /*var*/ ((a_size)), ((size_t stbuf->f_bsize));
  stbuf->f_bavail = stbuf->f_bfree;
  stbuf->f_namemax = (unsigned long) MYFS_MAXIMUM_NAME_LENGTH; 
  return 0;
}