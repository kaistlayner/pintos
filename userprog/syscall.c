#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "userprog/process.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "filesys/inode.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

static void halt (void);
void exit(int n);
static tid_t fork2(const char *thread_name, struct intr_frame *if_);
static int exec (const char *cmd_line);
static int wait(tid_t tid);
static bool create (const char *file, unsigned initial_size);
static bool remove (const char *file);
static int open (const char *file);
static int filesize (int fd);
static int read (int fd, void *buffer, unsigned size);
static int write (int fd, const void * buffer, unsigned size);
static void seek (int fd, unsigned position);
static unsigned tell (int fd);
static void close (int fd);
static void *mmap (void *addr, size_t length, int writable, int fd, off_t offset);
static void munmap (void *addr);
static bool mkdir (const char *);
static bool chdir (char *);
static bool isdir (int fd);
static int inumber (int fd);
static bool readdir (int fd, char *name);
int isemptydir (int fd, char *name);
int symlink (const char *target, const char *linkpath);

struct lock file_lock;
struct semaphore fork_sema;

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	lock_init (&file_lock);
	sema_init(&fork_sema, 0);



}
//uintptr_t esp
/*static void pick_argu(int64_t *args, struct intr_frame *f, int n){
	int i;

	int argc = (int) f->R.rdi;
	uint64_t argv = f->R.rsi;
	printf("saved argc : %d\n",argc);
	printf("saved argv : %x\n",argv);
	ASSERT (1 <= n && n <= 4);
	uintptr_t esp = f->rsp;
	for (i=0; i<n; i++){
		*(args+i) = (int64_t)(esp++);
	}
	printf("recall esp : %x\n", f->rsp);
	for (i=0; i<n; i++){

		if(i==1){
			*(args+i) = f->R.rdi;
		}
		else if(i==2){
			*(args+i) = f->R.rsi;
		}
		else if(i==3){
			*(args+i) = f->R.rdx;
		}
		else if(i==4){
			*(args+i) = f->R.r10;
		}
		printf("%dth arg : %s\n",i,(char *)(args+i));
	}
}*/

/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {
	// TODO: Your implementation goes here.

	uint64_t one = f->R.rdi;
	uint64_t two = f->R.rsi;
	uint64_t three = f->R.rdx;
	uint64_t four = f->R.r10;
	uint64_t five = f->R.r8;

	//uint64_t four = f->R.r10;
	uint64_t rax;

	switch ((int)f->R.rax){
		case SYS_HALT:                   /* Halt the operating system. */
			halt();
			break;
		case SYS_EXIT:                   /* Terminate this process. */
			exit(one);
			break;
		case SYS_FORK:                   /* Clone current process. */
			rax = fork2((const char *)one, f);
			//printf("fork return : %d\n",rax);
			f->R.rax = rax;
			break;
		case SYS_EXEC:                   /* Switch current process. */
			f->R.rax = exec ((const char *)one);
			break;
	 	case SYS_WAIT:                 	  /* Wait for a child process to die. */
	 		f->R.rax = wait((tid_t) one);
			break;
		case SYS_CREATE:                 /* Create a file. */
			f->R.rax = create ((const char *)one, (unsigned) two);
			break;
		case SYS_REMOVE:                 /* Delete a file. */
			f->R.rax = remove ((const char *)one);
			break;
		case SYS_OPEN:                   /* Open a file. */
			f->R.rax = open ((const char *)one);
			break;
		case SYS_FILESIZE:              /* Obtain a file's size. */
		 	f->R.rax = filesize ((int) one);
			break;
		case SYS_READ:                   /* Read from a file. */
			f->R.rax = read ((int) one, (void *)two, (unsigned) three);
			break;
		case SYS_WRITE:                  /* Write to a file. */
			f->R.rax = write((int)one,(const void *)two,(unsigned)three);
			break;
		case SYS_SEEK:
			seek ((int) one, (unsigned) two);
			break;
		case SYS_TELL:                   /* Report current position in a file. */
			f->R.rax = tell((int)one);
			break;
		case SYS_CLOSE:
			close((int)one);
			break;
		case SYS_MMAP:
			rax = mmap((void *)one, (size_t)two, (int)three, (int)four, (off_t)five);
			f->R.rax = rax;
			break;
		case SYS_MUNMAP:
			munmap((void *)one);
			break;
		case SYS_MKDIR:
		  rax = mkdir ((const char *) one);
			f->R.rax = rax;
			break;
		case SYS_CHDIR:
			rax = chdir ((const char *) one);
			f->R.rax = rax;
      		break;
		case SYS_ISDIR:
			rax = isdir ((int) one);
			f->R.rax = rax;
			break;
		case SYS_INUMBER:
			rax = inumber ((int) one);
			f->R.rax = rax;
			break;
		case SYS_READDIR:
			rax = readdir ((int) one, (char *) two);
			f->R.rax = rax;
			break;
		case SYS_SYMLINK:
			rax = symlink ((const char *)one, (const char *)two);
			f->R.rax = rax;
			break;
		default:
			exit(-1);
	}
}

static void halt (void) {
	power_off();
}

void exit(int n){
	struct thread *cur = thread_current();
	cur->exit_status = n;
	char *ptr1, *ptr2, *name;
	name = thread_current()->name;
	ptr1 = strtok_r(name, " ", &ptr2);

	printf("%s: exit(%d)\n", ptr1, n);
	thread_exit ();
}

static int wait(tid_t tid)
{
  return process_wait(tid);
}

static int write (int fd, const void * buffer, unsigned size)
{
  struct file *f;
  lock_acquire (&file_lock);
  if (fd == STDOUT_FILENO)
    {
      putbuf (buffer, size);
      lock_release (&file_lock);
      return size;
    }
  if ((f = process_get_file (fd)) == NULL)
    {
      lock_release (&file_lock);
      return 0;
    }
  bool is_dir = is_dir_inode(file_get_inode(f));
  if(is_dir){
  	lock_release (&file_lock);
  	return -1;
  }
  size = file_write (f, buffer, size);
  lock_release (&file_lock);
  return size;
}

static tid_t fork2(const char *thread_name, struct intr_frame *if_){
	tid_t tid = process_fork(thread_name, if_);
	if (tid != thread_current()->tid) {
		thread_yield();
		return tid;
	}
	else if (tid == thread_current()->tid) {
		return 0;
	}
	NOT_REACHED();
	return -1;
}

static int exec (const char *cmd_line){
	//tid_t tid = process_exec(cmd_line);
	//struct thread *ch = child_thread(tid);
	int ret;
	ret = process_exec(cmd_line);
	struct thread* t = thread_current();
	sema_down(&t->exec_wait);
	return ret;
}
static bool create (const char *file, unsigned initial_size){
	if(file==NULL){
		exit(-1);
	}
	return filesys_create(file, initial_size);
}
static bool remove (const char *file){
	return filesys_remove(file);
}

static int open (const char *file){
	int result = -1;
	lock_acquire (&file_lock);
	if (file == NULL || file == "") exit(-1);
	
	struct file *f = filesys_open (file);
	if (!strcmp(thread_current()->name, file)) {
		file_deny_write(f);
	}
	result = process_add_file (f);
	lock_release (&file_lock);
	return result;
}
static int filesize (int fd){
	struct file *f = process_get_file (fd);
  	if (f == NULL) return -1;
	return file_length (f);
}

static int read (int fd, void *buffer, unsigned size){
	int ret;
	lock_acquire(&file_lock);
	struct file *f;
	unsigned count = size;

	if (fd == 0) {
		int i;
		for (i=0; i<size; i++) {
			*((char *)buffer+i) = input_getc();
		}
		lock_release(&file_lock);
		return size;
	}
	else {
		f = process_get_file(fd);
		if (f == NULL) {
			lock_release(&file_lock);
			return -1;
		}
		else {
			size = file_read(f, buffer, size);
			lock_release(&file_lock);
			return size;
		}
	}
}
static void seek (int fd, unsigned position){
	struct file *f = process_get_file (fd);
	if (f == NULL)
		return;
	file_seek (f, position);
}
static unsigned tell (int fd){
	struct file *f = process_get_file (fd);
	if (f == NULL)
		exit (-1);
	return file_tell (f);
}
static void close (int fd){
	process_close_file (fd);
}
static void *mmap (void *addr, size_t length, int writable, int fd, off_t offset){
	struct file *file = process_get_file(fd);

	if (file == NULL || length <= 0 || addr == NULL || addr != pg_round_down(addr) || offset > PGSIZE || !is_user_vaddr(addr) || (size_t)addr >= ((size_t)addr + length)) return NULL;

	return do_mmap (addr, length, writable, file, offset);
}
static void munmap (void *addr){
	do_munmap(addr);
}
static bool
mkdir (const char *dir)
{
  return filesys_create_dir (dir);
}
static bool
chdir (char *path_o)
{
  char path[PATH_MAX_LEN + 1];
  strlcpy (path, path_o, PATH_MAX_LEN);
  strlcat (path, "/0", PATH_MAX_LEN);

  char name[PATH_MAX_LEN + 1];
  struct dir *dir = parse_path (path, name);
  if (!dir)
    return false;
  //dir_close (thread_current ()->working_dir);
  thread_current ()->working_dir = dir;
  return true;
}
static bool
isdir (int fd)
{
  // 파일 디스크립터를 이용하여 파일을 찾습니다.
  struct file *f = process_get_file (fd);
  if (f == NULL)
    exit (-1);
  // 디렉터리인지 계산하여 반환합니다.
  return is_dir_inode (file_get_inode (f));
}
static int
inumber (int fd)
{
  // 파일 디스크립터를 이용하여 파일을 찾습니다.
  struct file *f = process_get_file (fd);
  if (f == NULL)
    exit (-1);
  return (int)get_sector(file_get_inode (f));
}

static bool
readdir (int fd, char *name)
{
  // 파일 디스크립터를 이용하여 파일을 찾습니다.
  struct file *f = process_get_file (fd);
  if (f == NULL)
    exit (-1);
  // 내부 아이노드 가져오기 및 디렉터리 열기
  struct inode *inode = file_get_inode (f);
  if (!inode || !is_dir_inode (inode))
    return false;
  struct dir *dir = dir_open (inode);
  if (!dir)
    return false;
  int i;
  bool result = true;
  off_t *pos = (off_t *)f + 1;
  for (i = 0; i <= *pos && result; i++)
    result = dir_readdir (dir, name);
  if (i <= *pos == false)
    (*pos)++;
  return result;
}

int
isemptydir (int fd, char *name)
{
  // 파일 디스크립터를 이용하여 파일을 찾습니다.
  struct file *f = process_get_file (fd);
  if (f == NULL)
    exit (-1);
  // 내부 아이노드 가져오기 및 디렉터리 열기
  struct inode *inode = file_get_inode (f);
  if (!inode || !is_dir_inode (inode))
    return false;
  struct dir *dir = dir_open (inode);
  if (!dir)
    return false;
  int i;
  bool result = true;
  off_t *pos = (off_t *)f + 1;
  for (i = 0; i <= *pos && result; i++)
    result = dir_readdir (dir, name);
  if (i <= *pos == false)
    (*pos)++;
   printf("result : %d\n", result);
  return result-2;
}

int symlink (const char *target, const char *linkpath){
	
}

