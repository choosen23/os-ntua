#include <stdio.h> 
#include <unistd.h>
#include "zing.h"

void zing (const char *argv) {

	printf("hello voithe! Oi dimiourgoi tis askisis einai oi: %s \n" ,getlogin() );
}
