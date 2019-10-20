#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>



#include "tree.h"
#include "proc-common.h"


#define SLEEP_PROC_SEC 10
#define SLEEP_TREE_SEC 3


void recursive_create(struct tree_node *node, int pip) {

	int result,i;
	change_pname(node->name);

	if (node->nr_children == 0 ) { //ftiakse pipe
		result = atoi(node->name); //string to integer
		if ( write(pip,&result,sizeof(result)) != sizeof(result) ) {
			perror("child write");
			exit(1);
		}
		fprintf(stdout,"Eimai o  %s kai exo grapsei : %d \n",node->name,result); 
		exit(0);
	}
	int n[2],status ;

	if (pipe(n) < 0) {
		perror ("new pipe");
		exit(1);
	}

	for ( i = 0; i < 2; i++) {
		pid_t pid = fork();

		if (pid < 0 ) {
			perror ("fork");
			exit(1);
		}
		if (pid == 0 ) {
			recursive_create(node->children+i,n[1]);
		}
	}
	int num[2];
	for ( i = 0; i<2; i++) {
		pid_t pid = wait(&status);
		explain_wait_status(pid,status);
		if ( read(n[0],&num[i],sizeof(int)) != sizeof(int)) {
			perror("diavase apo child pipe");
			exit(1);
		}
		fprintf(stdout,"Eimai o %s kai exo diavasei : %d \n",node->name,num[i]);
	}
	switch(strcmp(node->name,"+")) {
		case 0:
			result = num[1] + num[0];
			break;
		default:
			result = num[1] * num[0];
	}
	
	if ( write(pip, &result, sizeof(result))!= sizeof(result)) {
		perror ("bad arguments" );
		exit(1);
	}
	fprintf(stdout,"Eimai o %s kai egrapsa : %d \n",node->name,result);
	exit (0);

}
int main(int argc, char *argv[])
{
	pid_t pid;
	int status,tmp,fd[2];
	struct tree_node *root;

	if (argc != 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	if (pipe(fd) < 0) {
		perror("pipe");
		exit(1);
	}
	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		close(fd[0]);
		recursive_create(root,fd[1]);
		exit(1);
	}

	pid = wait(&status);
	explain_wait_status(pid, status);


	if (read(fd[0],&tmp,sizeof(int))!=sizeof(int)) {
		perror("diavase pipe");
	}
	fprintf(stdout,"Diavasa :%d \n",tmp);
	fprintf(stdout," %d \n",tmp);

	return 0;
}
