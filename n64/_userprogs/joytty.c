/*
 * joytty: input chars by joystick
 * Copyright 2018 Murachue <murachue+github@gmail.com>
 * License: MIT
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/joystick.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NELEM(v) (sizeof(v)/sizeof(*(v)))

typedef int16_t s16;

enum {
	FUNC_NOP   = 0,
	FUNC_0TO7  = 1,
	FUNC_8TOF  = 2,
	FUNC_GTON  = 3,
	FUNC_VSHFT = 4,
	FUNC_SHIFT = 5,
	FUNC_ARROW = 6,
	FUNC_RAWIN = 7,
	FUNC_MAX
};

/* n64si@20180929: a b z l r st du dd dl dr cl cr cu cd */
int button2func[16] = {
	FUNC_8TOF, FUNC_0TO7, FUNC_SHIFT, FUNC_NOP, FUNC_VSHFT, FUNC_RAWIN,
	FUNC_NOP, FUNC_NOP, FUNC_NOP, FUNC_NOP,
	FUNC_ARROW, FUNC_NOP, FUNC_NOP, FUNC_GTON,
};

int bit2button[8] = {1, 0, 10, 13, 12, 11, 4, 2}; /* b a cl cd cu cr r z */

/* assumes joystick is calibrated. */
int eight(s16 x, s16 y) {
	s16 ax = (x < 0) ? -x : x;
	s16 ay = (y < 0) ? -y : y;

	if(11000 < y) {
		if(11000 < x) {
			return 1;
		} else if(-11000 < x) {
			return 0;
		} else {
			return 7;
		}
	} else if(y < -11000) {
		if(11000 < x) {
			return 3;
		} else if(-11000 < x) {
			return 4;
		} else {
			return 5;
		}
	} else {
		if(11000 < x) {
			return 2;
		} else if(x < -11000) {
			return 6;
		}
	}
	return -1;
}
int simin(int fd, char c) {
	return ioctl(fd, TIOCSTI, &c);
}
int simins(int fd, const char *s) {
	if(!s) {
		return 0;
	}
	for(; *s; s++) {
		int r;
		if((r = simin(fd, *s)) < 0) {
			return r;
		}
	}
	return 0;
}
int ineight(int fd, const char *(*sel)[8], s16 x, s16 y) {
	int c = eight(x, y);
	if(c < 0) {
		return 0;
	}
	return simins(fd, (*sel)[c]);
}

void geteq(char *p, unsigned long *k, unsigned long *v) {
	char *q;
	*k = strtoul(p, &p, 10);
	if(p == optarg) {
		puts("arg error: invalid index");
		exit(1);
	}
	if(*p != '=') {
		puts("arg error: invalid separator");
		exit(1);
	}
	q = p;
	*v = strtoul(p, &p, 10);
	if(p == q) {
		puts("arg error: invalid assign");
		exit(1);
	}
}

int main(int argc, char **argv) {
	char *joypath;
	char *ttypath;

	{
		int r;

		while((r = getopt(argc, argv, "b:")) != -1) {
			switch(r) {
			case 'b':
				{
					unsigned long btn, fun;
					geteq(optarg, &btn, &fun);

					if(16 <= btn) {
						puts("arg error: too large button index");
						exit(1);
					}
					if(FUNC_MAX <= fun) {
						puts("arg error: too large function number");
						exit(1);
					}

					button2func[btn] = fun;
				}
				break;
			case 'r':
				{
					unsigned long bit, btn;
					geteq(optarg, &bit, &btn);

					if(8 <= bit) {
						puts("arg error: too large bit index");
						exit(1);
					}
					if(16 <= btn) {
						puts("arg error: too large button index");
						exit(1);
					}

					bit2button[bit] = btn;
				}
				break;
			default:
				exit(1);
			}
		}

		argc -= optind;
		argv += optind;

		if(*argv == NULL) {
			puts("arg error: joystick device path required");
		}
		joypath = *argv;

		argc--;
		argv++;
		if(*argv == NULL) {
			ttypath = "/dev/tty";
		} else {
			ttypath = *argv;
		}
	}

	// http://d.hatena.ne.jp/aki-yam/20130729/1375097014
	{
		int jfd, tfd;

		if((jfd = open(joypath, O_RDONLY)) < 0) {
			puts("couldn't open joystick");
			exit(1);
		}
		if((tfd = open(ttypath, O_WRONLY)) < 0) {
			puts("couldn't open tty");
			exit(1);
		}

		/*
		ioctl(joy_fd, JSIOCGAXES, &num_of_axis);
		ioctl(joy_fd, JSIOCGBUTTONS, &num_of_buttons);
		ioctl(joy_fd, JSIOCGNAME(80), &name_of_joystick);
		*/

		{
			s16 axis[2];
			char buttons[16]; // for rawin
			char shift, vshift;
			const char *map[13][8] = {
				{ "a", "b", "c", "d", "e", "f", "g", "h" },
				{ "i", "j", "k", "l", "m", "n", "o", "p" },
				{ "q", "r", "s", "t", "u", "v", "w", "x" },
				{ "y", "z", "-", ".", " ", "/", "\010", "," },
				{ "0", "1", "2", "3", "4", "5", "6", "7" },
				{ "8", "9", "*", "?", "_", "&", "\"", "'" },
				/*shift*/
				{ "A", "B", "C", "D", "E", "F", "G", "H" },
				{ "I", "J", "K", "L", "M", "N", "O", "P" },
				{ "Q", "R", "S", "T", "U", "V", "W", "X" },
				{ "Y", "Z", ">", "<", "|", 0, "\r", 0 },
				{ 0 },
				{ 0 },
				/*arrow*/
				{ "\033[A", 0, "\033[C", 0, "\033[B", 0, "\033[D", 0 },
			};

			for(;;) {
				struct js_event js;

				if(read(jfd, &js, sizeof(js)) < sizeof(js)) {
					puts("joystick short read");
					exit(1);
				}

				switch(js.type & ~JS_EVENT_INIT) {
				case JS_EVENT_AXIS:
					if(js.number < NELEM(axis)) {
						axis[js.number] = js.value;
					}
					break;
				case JS_EVENT_BUTTON:
					if(js.number < NELEM(button2func)) {
						switch(button2func[js.number]) {
						case FUNC_0TO7:
							if(js.value) {
								ineight(tfd, &map[shift*6+vshift*3+0], axis[0], axis[1]);
							}
							break;
						case FUNC_8TOF:
							if(js.value) {
								ineight(tfd, &map[shift*6+vshift*3+1], axis[0], axis[1]);
							}
							break;
						case FUNC_GTON:
							if(js.value) {
								ineight(tfd, &map[shift*6+vshift*3+2], axis[0], axis[1]);
							}
							break;
						case FUNC_VSHFT:
							vshift = js.value;
							break;
						case FUNC_SHIFT:
							shift = js.value;
							break;
						case FUNC_ARROW:
							if(js.value) {
								ineight(tfd, &map[12], axis[0], axis[1]);
							}
							break;
						case FUNC_RAWIN:
							if(js.value) {
								unsigned char bits = 0;
								int i;
								for(i = 0; i < 8; i++) {
									bits >>= 1;
									bits |= !!(buttons[bit2button[i]]) << 7;
								}
								simin(tfd, bits);
							}
							break;
						}
					}
					if(js.number < NELEM(buttons)) {
						buttons[js.number] = js.value;
					}
					break;
				}
			}
		}
	}

	return 0;
}
