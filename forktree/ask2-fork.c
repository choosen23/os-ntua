#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(void)
{
	/*
	 * initial process is A.
	 */
	int status;
	change_pname("A");
	printf("A: Sleeping...\n");
	//sleep(SLEEP_PROC_SEC);


	pid_t b = fork();
	if (b < 0) {
		perror("skaei to B ");
		exit(1);
	}
	else if (b == 0 ) {
		change_pname("B");
		printf("B: Sleeping...\n");
		pid_t node1  = fork();
		if ( node1 < 0 ) {
			perror("something wrong");
			exit(1);
		}
		if (node1 == 0) {
			change_pname("D");
			printf("D: Sleeping...\n");
			sleep(SLEEP_PROC_SEC);
			printf("D: Exiting...\n");
			exit(13);
		}

		node1 = wait(&status);
		explain_wait_status(node1,status);
		sleep(SLEEP_PROC_SEC);
		printf("B: Exiting...\n");
		exit(19);
	}

	pid_t node2 = fork();
	if (node2 < 0) {
		perror ("lathos");
		exit(1);
	}
	if ( node2 == 0) {
		change_pname("C");
		printf("C: Sleeping...\n");
		sleep(SLEEP_PROC_SEC);
		printf("C: Exiting...\n");
		exit(17);
	}
	pid_t pid = waitpid(-1, &status,0);
	explain_wait_status(pid,status);

	pid = wait(&status);
	explain_wait_status(pid,status);

	printf("A: Exiting...\n");
	sleep(SLEEP_PROC_SEC);
	exit(16);

}


void child (void) 
{
	compute(10000);
	exit(7);
}
/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(void)
{
	pid_t pid;
	int status;

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs();
		//child();
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	/* wait_for_ready_children(1); */

	/* for ask2-{fork, tree} */
	sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	/* kill(pid, SIGCONT); */

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
