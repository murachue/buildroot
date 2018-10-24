// http://d.hatena.ne.jp/aki-yam/20130729/1375097014
// jstest output is too much.

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <malloc.h>

int main(int argc, char **argv) {
	int fd;
	__u8 naxis, nbuttons;
	__s16 *axis;
	char *buttons;

	if(argc < 2) {
		puts("arg count");
		return 1;
	}

	if((fd = open(argv[1], O_RDONLY)) < 0) {
		perror("could not open path");
		return 2;
	}

	if(ioctl(fd, JSIOCGAXES, &naxis) < 0) {
		perror("could not get axises");
		return 3;
	}
	if(ioctl(fd, JSIOCGBUTTONS, &nbuttons) < 0) {
		perror("could not get buttons");
		return 4;
	}

	printf("%d axes %d buttons\n", naxis, nbuttons);

	axis = malloc(sizeof(*axis) * naxis);
	buttons = malloc(sizeof(*buttons) * nbuttons);

	for(;;) {
		struct js_event js;
		int i;

		if(read(fd, &js, sizeof(js)) < sizeof(js)) {
			perror("short read");
			return 5;
		}

		switch(js.type & ~JS_EVENT_INIT) { /* orly? */
		case JS_EVENT_AXIS:
			if(js.number < naxis) {
				axis[js.number] = js.value;
			} else {
				printf("?axis %d\n", js.number);
			}
			break;
		case JS_EVENT_BUTTON:
			if(js.number < nbuttons) {
				buttons[js.number] = js.value;
			} else {
				printf("?button %d\n", js.number);
			}
			break;
		}

		printf("a:");
		for(i = 0; i < naxis; i++) {
			if(0 < i) {
				printf(",");
			}
			printf("%6d", axis[i]);
		}
		printf(" b:");
		for(i = 0; i < nbuttons; i++) {
			printf("%d", buttons[i]);
		}
		puts("");
	}

	close(fd);

	return 0;
}
