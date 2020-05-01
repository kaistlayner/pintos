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
#include "filesys/filesys.h"

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
struct lock file_lock;

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
	//uint64_t four = f->R.r10;

	switch ((int)f->R.rax){
		case SYS_HALT:                   /* Halt the operating system. */
			halt();
			break;
		case SYS_EXIT:                   /* Terminate this process. */
			exit(one);
			break;
		case SYS_FORK:                   /* Clone current process. */
			f->R.rax = fork2((const char *)one, f);
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
			remove ((const char *)one);
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
  size = file_write (f, buffer, size);
  lock_release (&file_lock);
  return size;
}

static tid_t fork2(const char *thread_name, struct intr_frame *if_){
	return process_fork(thread_name, if_);
}

static int exec (const char *cmd_line){
	tid_t tid = process_exec(cmd_line);
	struct thread *ch = child_thread(tid);

	sema_down(&ch->exec_wait);

	return tid;
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
	result = process_add_file (f);
	lock_release (&file_lock);
	return result;
}
static int filesize (int fd){
	struct file *f = process_get_file (fd);
  	if (f == NULL) return -1;
	return file_length (f);
}

// 메모리 유효 검사 추가 필요
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
			return 1;
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
