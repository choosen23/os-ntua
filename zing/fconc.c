#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>



void ReadWrite(int fdread, int fdwrite);


int main(int argc,char **argv){
	if (argc<3 || argc>4 ) {
		const char buff[]= "Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n";
		write (STDOUT_FILENO, buff, sizeof(buff)-1);
		exit (1);
	} 
	int fd1,fd2,fd;
	fd1 = open( argv[1],O_RDONLY);
	fd2 = open (argv[2] , O_RDONLY);
	if (fd1==-1){
		perror(argv[1]);
		exit (2);
	}
	else if (fd2==-1){
		perror(argv[2]);
		exit (3);
	}

	if (argc==3){
		fd = open("fconc.out" ,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	}					
	else{
		if ( (strcmp(argv[1] , argv[3])== 0) || (strcmp(argv[2], argv[3]) ==0) ){
			const char buff[]= " You should not give the same file as input and output \n" ;
			write (STDOUT_FILENO , buff , sizeof(buff)-1);
			exit (7);
			}	


		fd = open( argv[3], O_WRONLY| O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	} 

	ReadWrite(fd1,fd);
	ReadWrite(fd2,fd);
	close (fd);
	close (fd1);
	close (fd2);
	return 0;


}
void ReadWrite( int fdread, int fdwrite){
	char buff[1024];
	size_t len,idx;
	ssize_t rcnt,wcnt;
	for (;;){
		rcnt = read(fdread,buff,sizeof(buff)-1);
		if (rcnt == 0) /* end-of-file */
			break;
		if (rcnt == -1){ /* error */
			perror("read");
			exit(4);
		}
		buff[rcnt]= '\0';
		idx = 0;
		len = strlen(buff);
		do {
			wcnt = write(fdwrite,buff + idx, len - idx);
			if (wcnt == -1){ /* error */
				perror("write");
				exit(5);;
			}
			idx += wcnt;
		} while (idx < len);




	}

}




