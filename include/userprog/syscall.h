#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void exit(int n);
int isemptydir (int fd, char *name);
#endif /* userprog/syscall.h */
