#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */

typedef struct Node{
        pid_t pid_of_process;
        char * name;
        int id;
	struct Node * next;
} Node_t;

Node_t * head, * tail;
pid_t current_pid;
int elementsinQueue,ids;

void insert(pid_t pid,char * name){
        Node_t * newnode= malloc(sizeof(Node_t));
        newnode->pid_of_process= pid;
        Node_t * tmp = head;
        while (tmp->next!= NULL) tmp=tmp->next;
        tmp->next=newnode;
        newnode->name=strdup(name); //dimiourgei enan pointer se duplicate char 
       	newnode->id=++ids;
	tail=newnode;
        elementsinQueue++;
}

void remove1 (pid_t pid){
        Node_t * tmp=head;
        while (tmp->next->pid_of_process != pid ) tmp=tmp->next;
        Node_t  * todelete=tmp->next;
        tmp->next= tmp->next->next;
        free(todelete);
        elementsinQueue--;
        if (elementsinQueue==0){
		 printf("All processes terminated\n");
		 exit(1);
	}
}

/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum){
	if ( kill(current_pid,SIGSTOP)<0) perror("stop error");
}
/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
	int p,status;
	for (;;) {
		p=waitpid(-1,&status,WUNTRACED | WNOHANG);// i waitpid perimenei me -1 opoiodipote paidi na termatistei.wstoso me to wuntraced,enimerwnei to status akomi kai an auto to paidi egine stopped apo kapoio sima. to whoang allazei katastasi an kanena paidi den egine exit

	        if (p==0) // kanena paidi dn  allazei katastasi kai epistrefetai amesws to 0
	        	break;
		explain_wait_status(p,status);
		if (WIFEXITED(status) || WIFSIGNALED(status) ) {
			remove1(current_pid);
			if (kill(head->pid_of_process,SIGCONT) < 0) perror("continues error 1");
			current_pid= head->pid_of_process;
			head=head->next;
			tail=tail->next;
			alarm(SCHED_TQ_SEC);
		}
		if (WIFSTOPPED(status)){
			if (kill(head->pid_of_process,SIGCONT) < 0) perror("continues error 2");
			current_pid=head->pid_of_process;
			head=head->next;	
			tail=tail->next;
			alarm(SCHED_TQ_SEC);
		}
		
	}
		
		
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset; // dimiourgei mia maska,wste oso ginetai execute tou handler auti na ginetai block me to sima pou piastike kai na min mporei na piasei allo sima pou brisketai sto sigset  mexri na teleiwsei o handler tin leitourgia 
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal:t sigpipe");
		exit(1);
	}
}


int main(int argc, char *argv[])
{
	int nproc;
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	ids=0;
	nproc = argc-1; /* number of proccesses goes here */
	head=malloc(sizeof(Node_t));
	head->next= NULL;
	int i;	
	for ( i=1; i<= nproc; i++){
		pid_t pid= fork();
		if (pid>0) insert(pid,argv[i]);
		else if (pid==0){
			char *newargv[]={argv[i],NULL,NULL,NULL};
			raise(SIGSTOP);
			execve(argv[i],newargv,NULL);
		}
	}
	if (nproc == 0) {
                fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
                exit(1);
        }

	head=head->next;
	free(tail->next);
	tail->next=head;	
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);
		
	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();
	if ( kill(head->pid_of_process,SIGCONT)<0){
		perror("continue error");
	}
	current_pid=head->pid_of_process;
	head=head->next;
	tail= tail ->next;	
	alarm(SCHED_TQ_SEC);

	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
