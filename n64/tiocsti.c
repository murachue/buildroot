#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h> /* exit */

int main(int argc, char **argv) {
	int fd;
	if(argc < 3) {
		puts("arg count");
		exit(1);
	}
	if((fd = open(argv[1], O_WRONLY)) < 0) {
		perror("could not open tty");
		exit(2);
	}
	for(argv += 2; *argv; argv++){
		char ch = strtoul(*argv, NULL, 10);
		if(ioctl(fd, TIOCSTI, &ch) < 0) {
			perror("could not TIOCSTI");
			exit(3);
		}
	}
	close(fd);
	return 0;
}
