#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/types.h>


#include "tree.h"
#include "proc-common.h"

#define SLEEP_PROC_SEC 10
#define SLEEP_TREE_SEC 4



void recursive_create(struct tree_node *node) {
	change_pname(node->name);
	int i;
	for (i = 0; i < node->nr_children; i++) {
		pid_t pid = fork();
		if (pid < 0 ) {
			perror("stin anadromi tis fork\n");
			exit(1);
		}
		if (pid == 0 )
		{
			
			printf("child with %d created \n" , getpid());
			recursive_create(node->children+i);
		}
	}
	while(i--) {
		int status;
		pid_t pid = wait(&status);
		explain_wait_status(pid, status);
	}
	if (node-> nr_children == 0)	sleep(SLEEP_PROC_SEC);
	exit(1);
}



int main(int argc, char *argv[]) {


	struct tree_node *root;

	if (argc != 2) {
		printf("lathos orisma\n");
		exit(1);
	}

	root = get_tree_from_file(argv[1]);
	print_tree(root);

	pid_t pid=fork();


	if ( pid < 0) {
		perror("error stin fork");
		exit(1);
	}
	if (pid == 0 ) {
		recursive_create(root);
		exit(1);
	}

	sleep(SLEEP_TREE_SEC);
	show_pstree(pid);
	int status;
	pid = wait(&status);
	explain_wait_status(pid,status);
	return 0;
}
