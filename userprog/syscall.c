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

}
//uintptr_t esp
static void pick_argu(int64_t *args, struct intr_frame *f){
	int i;
	int argc = (int) f->R.rdi;
	uint64_t argv = f->R.rsi;
	/*printf("saved argc : %d\n",argc);
	printf("saved argv : %x\n",argv);*/
	ASSERT (1 <= argc && argc <= 3);
	for (i=0; i<argc; i++){
		*(args++) = (argv++);
	}
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {
	// TODO: Your implementation goes here.
	printf ("system call!\n");
	int argc = (int) f->R.rdi;
	uint64_t argv = f->R.rsi;
	printf("saved argc : %d\n",argc);
	printf("saved argv : %x\n",argv);

	int64_t args[3];

	switch (*(int *) f->rsp){
		case SYS_HALT:                   /* Halt the operating system. */
			halt();
			break;
		case SYS_EXIT:                   /* Terminate this process. */
			printf("exit entered\n");
			pick_argu(args, f);
			exit(args[0]);
			break;
		case SYS_FORK:                   /* Clone current process. */
			break;
		case SYS_EXEC:                   /* Switch current process. */
			break;
	 	case SYS_WAIT:                 	  /* Wait for a child process to die. */
	 		pick_argu(args, f);
	 		wait((tid_t) args[0]);
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
			pick_argu(args, f);
			break;
		case SYS_TELL:                   /* Report current position in a file. */
			break;
		case SYS_CLOSE:
			break;
		default:
			printf("default entered\n");
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
