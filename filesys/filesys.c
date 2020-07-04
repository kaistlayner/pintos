#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/fat.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"
#include "threads/thread.h"

/* The disk that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);

/* Initializes the file system module.
 * If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) {
	filesys_disk = disk_get (0, 1);
	if (filesys_disk == NULL)
		PANIC ("hd0:1 (hdb) not present, file system initialization failed");

	inode_init ();

#ifdef EFILESYS
	fat_init ();

	if (format)
		do_format ();

	fat_open ();
	thread_current ()->working_dir = dir_open_root ();
#else
	/* Original FS */
	free_map_init ();

	if (format)
		do_format ();

	free_map_open ();
#endif
}

/* Shuts down the file system module, writing any unwritten data
 * to disk. */
void
filesys_done (void) {
	/* Original FS */
#ifdef EFILESYS
	fat_close ();
#else
	free_map_close ();
#endif
}

/* Creates a file named NAME with the given INITIAL_SIZE.
 * Returns true if successful, false otherwise.
 * Fails if a file named NAME already exists,
 * or if internal memory allocation fails. */
bool
filesys_create (const char *path, off_t initial_size) {
	disk_sector_t inode_sector = 0;
	// struct dir *dir = dir_open_root ();
	char name[PATH_MAX_LEN + 1];
	struct dir *dir = parse_path (path, name);
	//printf("dir : %x\n", dir);
	bool a, b, c, d;
	a = dir != NULL;
	b = free_map_allocate (1, &inode_sector);
	c = inode_create (inode_sector, initial_size);
	d = dir_add (dir, name, inode_sector);
	bool success = a && b && c && d && inode_sector;
	//printf("filesys_create...\n\tans : %d %d %d %d %d\n", a, b, c, d, inode_sector);
	/*bool success = (dir != NULL
			&& free_map_allocate (1, &inode_sector)
			&& inode_create (inode_sector, initial_size)
			&& dir_add (dir, name, inode_sector));*/
	if (!success && inode_sector != 0)
		free_map_release (inode_sector, 1);
	dir_close (dir);

	return success;
}

/* Opens the file with the given NAME.
 * Returns the new file if successful or a null pointer
 * otherwise.
 * Fails if no file named NAME exists,
 * or if an internal memory allocation fails. */
struct file *
filesys_open (const char *path) {

	char name[PATH_MAX_LEN + 1];
	// struct dir *dir = dir_open_root ();
	struct dir *dir = parse_path(path, name);
	struct inode *inode = NULL;

	if (dir == NULL) return NULL;
	//printf("name : %s\n",name);
	//bool why = dir_lookup (dir, name, &inode);
	//printf("bool : %d\n",why);
	if (!dir_lookup (dir, name, &inode)) return NULL;

	dir_close (dir);

	return file_open (inode);
}

/* Deletes the file named NAME.
 * Returns true if successful, false on failure.
 * Fails if no file named NAME exists,
 * or if an internal memory allocation fails. */
bool
filesys_remove (const char *path) {
	char name[PATH_MAX_LEN + 1];
	// struct dir *dir = dir_open_root ();
	struct dir *dir = parse_path (path, name);
	bool success = dir != NULL && dir_remove (dir, name);
	dir_close (dir);

	return success;
}

/* Formats the file system. */
static void
do_format (void) {
	printf ("Formatting file system...");

#ifdef EFILESYS
	/* Create FAT and save it to the disk. */
	fat_create ();
	if (!dir_create (ROOT_DIR_SECTOR, 16))
		PANIC ("root directory creation failed");
	fat_close ();
#else
	free_map_create ();
	if (!dir_create (ROOT_DIR_SECTOR, 16))
		PANIC ("root directory creation failed");
	free_map_close ();
#endif

	printf ("done.\n");
}

struct dir *
parse_path (const char *path_o, char *file_name)
{
  struct dir *dir = NULL;

  // 기본 예외 처리
  if (!path_o || !file_name)
    return NULL;
  if (strlen (path_o) == 0)
    return NULL;

  char path[PATH_MAX_LEN + 1];
  strlcpy (path, path_o, PATH_MAX_LEN);

  if (path[0] == '/' || thread_current ()->working_dir == NULL) {
	 	//printf("if\n");
	 	dir = dir_open_root ();
	 }
   else {
	 	//printf("else\n");
	 	dir = dir_reopen (thread_current ()->working_dir);
	 }
	//dir = dir_open_root ();

  // // 아이노드가 어떤 이유로 제거되었거나 디렉터리가 아닌 경우
  // if (!inode_is_dir (dir_get_inode (dir)))
  //   return NULL;

  char *token, *next_token, *save_ptr;
  token = strtok_r (path, "/", &save_ptr);
  next_token = strtok_r (NULL, "/", &save_ptr);

  if (token == NULL)
    {
      strlcpy (file_name, ".", PATH_MAX_LEN);
      return dir;
    }

  while (token && next_token)
    {
      struct inode *inode = NULL;
      if (!dir_lookup (dir, token, &inode))
        {
          dir_close (dir);
          return NULL;
        }
      // if (!inode_is_dir (inode))
      //   {
      //     dir_close (dir);
      //     return NULL;
      //   }
      dir_close (dir);
      dir = dir_open (inode);

      token = next_token;
      next_token = strtok_r (NULL, "/", &save_ptr);
    }
  strlcpy (file_name, token, PATH_MAX_LEN);
  return dir;
}

bool
filesys_create_dir (const char *path)
{
  disk_sector_t inode_sector = 0;
  char name[PATH_MAX_LEN + 1];
  struct dir *dir = parse_path (path, name);

  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  // && dir_create (inode_sector, 16)
									&& dir_create (inode_sector, 14)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);

  if (success)
    {
      struct dir *new_dir = dir_open (inode_open (inode_sector));
      dir_add (new_dir, ".", inode_sector);
      dir_add (new_dir, "..", inode_get_inumber (dir_get_inode (dir)));
      dir_close (new_dir);
    }
  dir_close (dir);
  return success;
}
