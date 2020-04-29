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

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
static void halt (void);

static void exit(int n);
static int wait(tid_t tid);
static int write (int fd, const void * buffer, unsigned size);
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
	//printf ("system call!\n");
	//int argc = (int) f->R.rdi;
	//uint64_t argv = f->R.rsi;
	/*printf("saved stack pointer : %x\n", f->rsp);
	printf("saved argc : %d\n",argc);
	printf("saved argv : %x\n",argv);*/
	uint64_t one = f->R.rdi;
	uint64_t two = f->R.rsi;
	uint64_t three = f->R.rdx;
	uint64_t four = f->R.r10;
	//int64_t args[4];
	//printf("syscall num : %d\n",(int)f->R.rax);
	switch ((int)f->R.rax){
		case SYS_HALT:                   /* Halt the operating system. */
			halt();
			break;
		case SYS_EXIT:                   /* Terminate this process. */
			//printf("exit entered\n");
			//printf("exit entered\n");
			//pick_argu(args, f, 1);
			//printf("return args0 : %s == exit\n",(char *)args[0]);
			exit(one);
			break;
		case SYS_FORK:                   /* Clone current process. */
			break;
		case SYS_EXEC:                   /* Switch current process. */
			break;
	 	case SYS_WAIT:                 	  /* Wait for a child process to die. */
	 		//printf("wait entered\n");
	 		//pick_argu(args, f, 1);
	 		//printf("return args0 : %s == wait\n",(char *)args[0]);
	 		wait((tid_t) one);
			break;
		case SYS_CREATE:                 /* Create a file. */
			break;
		case SYS_REMOVE:                 /* Delete a file. */
			break;
		case SYS_OPEN:                   /* Open a file. */
			break;
		case SYS_FILESIZE:              /* Obtain a file's size. */
			break;
		case SYS_READ:                   /* Read from a file. */
			break;
		case SYS_WRITE:                  /* Write to a file. */
			//printf("write entered\n");
			write((int)one,(const void *)two,(unsigned)three);
			//pick_argu(args, f, 3);
			//printf("return args0 : %s == wait\n",(char *)args[0]);
			break;
		case SYS_TELL:                   /* Report current position in a file. */
			break;
		case SYS_CLOSE:
			break;
		default:
			//printf("default entered\n");
			exit(-1);
	}
}

static void halt (void) {
	power_off();
}

static void exit(int n){
	struct thread *cur = thread_current();
	cur->exit_status = n;
	char *ptr1, *ptr2, *name;
	name = thread_current()->name;
	ptr1 = strtok_r(name, " ", &ptr2);

	printf("%s: exit(%d)\n", ptr1, n);
	thread_exit ();
}

static int
wait(tid_t tid)
{
  return process_wait(tid);
}

static int
write (int fd, const void * buffer, unsigned size)
{
  //struct file *f;
  lock_acquire (&file_lock);
  if (fd == STDOUT_FILENO)
    {
      putbuf (buffer, size);
      lock_release (&file_lock);
      return size;  
    }
   else printf("fd = %d\n",fd);
   return 0;
  /*if ((f = process_get_file (fd)) == NULL)
    {
      lock_release (&file_lock);
      return 0;
    }
  size = file_write (f, buffer, size);
  lock_release (&file_lock);
  return size;*/
}
