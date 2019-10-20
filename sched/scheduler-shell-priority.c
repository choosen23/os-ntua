#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include "stdbool.h"
#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */
typedef enum {LOW, HIGH} priority;
typedef struct Node{
        pid_t pid_of_process;
        char * name;
	int id;
        struct Node * next;
	priority pr;
} Node_t;

Node_t * head, * current;
pid_t current_pid,shells_pid;
int elementsinQueue;
int ida;
void insert(pid_t pid,char * name){
        Node_t * newnode= malloc(sizeof(Node_t));
        newnode->pid_of_process= pid;
	newnode->id=++ida;
	newnode->pr =LOW;
        Node_t * tmp = head;
        while (tmp->next!= NULL) tmp=tmp->next;
        tmp->next=newnode;
        newnode->name=strdup(name); //dimiourgei enan pointer se duplicate char 
        elementsinQueue++;
}

void remove1 (pid_t pid){
       	Node_t * tmp=head;
        while (tmp->next->pid_of_process != pid ) tmp=tmp->next;
        Node_t  * todelete=tmp->next;
	tmp->next=todelete->next;
        free(todelete);
        elementsinQueue--;
        if (elementsinQueue==0){
                 printf("All processes terminated\n");
                 exit(1);
        }
}
	
void find_next_node(void){
	Node_t *tmp=current;
	while (tmp->next != NULL && tmp->next->pr != HIGH ) tmp=tmp->next;
	if (tmp->next== NULL){
		tmp=head;
		while (tmp->next != NULL && tmp->next->pr != HIGH ){
			if (tmp==current){
				 break;
			}
			tmp=tmp->next;
		}
		if (tmp->next == NULL ) {
			current=head->next;
			current_pid=current->pid_of_process;
			return;
		}
	}
	current=tmp->next;
	current_pid=current->pid_of_process; 
}
	
		
const char* getPriority( priority pr){
	switch (pr){
		case LOW: return "LOW";
		case HIGH: return "HIGH";
	}
}	
	
/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
	Node_t *tmp;
	int i;
	for ( tmp=head->next, i=0; i< elementsinQueue; tmp=tmp->next,i++){
		printf("Name: %s \t ID: %d PID: %ld Priority: %s\n", tmp->name,tmp->id,(long)tmp->pid_of_process, getPriority(tmp->pr));
		if ( tmp->pid_of_process== current_pid) printf (" CURRENT");
		puts ( "\n");
	}
}


/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{	if (id==1) {
		printf("Press q to terminate shell\n");
		return 1;
	}
	Node_t *tmp=head;
	while (tmp->next != NULL && tmp->next->id !=id ) tmp=tmp->next;
	if (tmp->next == NULL) return 1;
	kill(tmp->next->pid_of_process,SIGKILL);
	remove1(tmp->next->pid_of_process);
	printf("Killed process with id: %d\n", id);
	return 0;
}


/* Create a new task.  */
static void
sched_create_task(char *executable)
{
	pid_t p=fork();
	if (p>0) insert(p,executable);
	if (p==0){
		char *newargv[]={executable,NULL,NULL,NULL};
		raise(SIGSTOP);
		execve(executable,newargv,NULL);
	}

}

static void sched_set_low(int id){
	Node_t *tmp= head;
	while (tmp->next != NULL && tmp->next->id!= id ) tmp=tmp->next;
	if (tmp->next!=NULL) tmp->next->pr = LOW;
}

static void sched_set_high(int id){
	Node_t *tmp=head;
	while (tmp->next!= NULL && tmp->next->id != id) tmp=tmp->next;
	if (tmp != NULL ) tmp->next->pr= HIGH;
}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;
		
		case REQ_HIGH_TASK:
			sched_set_high(rq->task_arg);
			return 0;
		
		case REQ_LOW_TASK:
			sched_set_low(rq->task_arg);
			return 0;
		
		default:
			return -ENOSYS;
	}
}

/* 
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum){
	if  (kill(current_pid,SIGSTOP)<0) perror("sigalrm error");

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
		if (WIFEXITED(status)|| WIFSIGNALED(status) ) {
			if ( p==current_pid){
		                remove1(current_pid);
				find_next_node();
				if(kill(current->pid_of_process,SIGCONT) < 0) perror("continues error 1");
                                alarm(SCHED_TQ_SEC);
			}
                }
                if (WIFSTOPPED(status)){
			find_next_node();
			if (kill(current->pid_of_process,SIGCONT) < 0) perror("continues error 2");
                        alarm(SCHED_TQ_SEC);
                }

        }

}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
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
	sa.sa_mask = sigset;
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
		perror("signal: sigpipe");
		exit(1);
	}
}

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static pid_t
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];
	return p;
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int nproc;
	ida=0;
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;
	/* Create the shell. */
	shells_pid=sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
	/* TODO: add the shell to the scheduler's tasks */

	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

	nproc = argc-1; /* number of proccesses goes here */
	head=malloc(sizeof(Node_t));
        head->next= NULL;
	insert(shells_pid,SHELL_EXECUTABLE_NAME);
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
        
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc+1);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();
	current=head;
	find_next_node();
	if (kill(current->pid_of_process,SIGCONT)<0) perror("continues error");
	
	alarm(SCHED_TQ_SEC);

	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
