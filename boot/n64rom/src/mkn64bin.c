#include <stdio.h>
#include <string.h>
#include <stdint.h>

long fsize(FILE *fp) {
	long tell, size;
	tell = ftell(fp);
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, tell, SEEK_SET);
	return size;
}

long fcopy(FILE *wfp, FILE *rfp) {
	char buf[1024];
	int r;
	long rtotal = 0;
	while(0 < (r = fread(buf, 1, 1024, rfp))) {
		fwrite(buf, 1, r, wfp);
		rtotal += r;
	}
	return rtotal;
}

long fcopy_with_align(FILE *wfp, FILE *rfp, int align2, long *prompos, long *psize_aligned) {
	long copied;
	static const char zeroes[512] = {0};
	long align = 1 << align2;
	long align1 = align - 1;

	/* currently over 2^9(=512) is not supported. */
	if(9 < align2) {
		return -1;
	}

	fwrite(zeroes, 1, (align - (ftell(wfp) & align1)) & align1, wfp);
	if(prompos) {
		/* note: user program starts at 0x1000. */
		*prompos = ftell(wfp) + 0x1000;
	}
	copied = fcopy(wfp, rfp);
	fwrite(zeroes, 1, (align - (copied & align1)) & align1, wfp);
	if(psize_aligned) {
		*psize_aligned = (copied + align1) & ~align1;
	}
	return copied;
}

long fcopy_by_name_with_align(FILE *wfp, const char *fname, int align2, long *prompos, long *psize_aligned, const char *descr) {
	FILE *rfp;
	long copied;

	if((rfp = fopen(fname, "rb")) == NULL) {
		fprintf(stderr, "E: could not open %s for reading %s.\n", fname, descr);
		return -1;
	}

	copied = fcopy_with_align(wfp, rfp, align2, prompos, psize_aligned);

	fclose(rfp);

	return copied;
}

int find_file_in_iso9660(FILE *fp, const char *path, long *ppos, long *psize) {
	const long offset = 0x1000;
	unsigned char buf[0x21+37];
	uint32_t sect, limit;

	fseek(fp, 16 * 2048 - offset, SEEK_SET);
	if(fread(buf, 6, 1, fp) != 1) {
		fprintf(stderr, "E: could not read a PVD signature.\n");
		return -1;
	}
	if(memcmp(buf, "\001CD001", 6)) {
		fprintf(stderr, "E: could not find a valid PVD.\n");
		return -1;
	}

	fseek(fp, 16 * 2048 + 0x9C - offset, SEEK_SET);
	if(fread(buf, 0x22, 1, fp) != 1) {
		fprintf(stderr, "E: could not read a root directory entry in PVD.\n");
		return -1;
	}
	if(buf[0] != 0x22) {
		fprintf(stderr, "E: unexpected root directory entry length 0x%02x.\n", buf[0]);
		return -1;
	}
	// TODO portability: should use 2-byte access.
	sect = *(uint32_t *)(buf + 2);
	limit = *(uint32_t *)(buf + 10);

	for(;;) {
		long readbytes = 0;
		fseek(fp, sect * 2048 - offset, SEEK_SET);

		// wow, because buf[0] is filled in the block, ok to use at 3rd expr of "for".
		// FIXME trusting "directory entry must be in the limit, not truncated."
		//       this may false in ISO9660 Level3, but this program does not support that at all.
		for(; readbytes < limit; readbytes += buf[0]) {
			int i;
			long curpos = ftell(fp);

			if(fread(buf, 0x21, 1, fp) != 1) {
				fprintf(stderr, "E: could not read a directory entry, pos=0x%lx.\n", curpos);
				return -1;
			}
			if(buf[0] == 0) {
				fprintf(stderr, "E: could not find a entry. (pos=0x%lx)\n", curpos);
				return -1;
			}
			if(37 < buf[32]) {
				fprintf(stderr, "E: too long name, pos=0x%lx.\n", curpos);
				return -1;
			}
			if(fread(buf + 0x21, buf[32], 1, fp) != 1) {
				fprintf(stderr, "E: could not read a name in a directory entry, pos=0x%lx.\n", curpos);
				return -1;
			}
			// skipping extra bytes after the file name.
			fseek(fp, buf[0] - 0x21 - buf[32], SEEK_CUR);
			for(i = 0; i < buf[32]; i++) {
				if(buf[33 + i] != path[i]) {
					break;
				}
			}
			if(i == buf[32]) {
				if(path[i] == '/') {
					if((buf[25] & 2) == 0) {
						fprintf(stderr, "E: unexpected type, should dir but file, pos=0x%lx.\n", curpos);
						return -1;
					}
					sect = *(uint32_t *)(buf + 2);
					limit = *(uint32_t *)(buf + 10);
					path += i + 1;
					break; // next-dir
				} else if(path[i] == '\0') {
					if((buf[25] & 2) != 0) {
						fprintf(stderr, "E: unexpected type, should file but dir, pos=0x%lx.\n", curpos);
						return -1;
					}
					if(ppos) {
						*ppos = *(uint32_t *)(buf + 2) * 2048;
					}
					if(psize) {
						*psize = *(uint32_t *)(buf + 10);
					}
					return 0;
				}
				// else, too short file entry. go next entry.
			}
		}
	}
}

// TODO portability: this code assumes the host is LittleEndian.
#define bswap32(n) ( \
	(((n) >> 24) & 0x000000FF) | \
	(((n) >>  8) & 0x0000FF00) | \
	(((n) <<  8) & 0x00FF0000) | \
	(((n) << 24) & 0xFF000000) \
	)

int write_kerninfo(FILE *wfp, uint32_t kernpos, uint32_t kernsize) {
	/* note: PI_CART_ADDR cart base is 0x10000000. */
	uint32_t kernpos2_be = bswap32(kernpos + 0x10000000);
	/* note: PI_WR_LEN is to-be-copied minus 1. */
	uint32_t kernsize2_be = bswap32(kernsize - 1);

	// TODO error check...
	long tell = ftell(wfp);
	fseek(wfp, 0x10, SEEK_SET);
	fwrite(&kernpos2_be, 4, 1, wfp);
	fwrite(&kernsize2_be, 4, 1, wfp);
	fseek(wfp, tell, SEEK_SET);

	return 0;
}


int main(int argc, char *argv[]) {
#ifdef ISO9660
	FILE *rfp, *wfp;
	long bootsize, kernpos, kernsize, rootsize;

	if(argc < 5) {
		fprintf(stderr, "usage: %s output.bin n64load.bin /path/to/vmlinux.bin rootfs.iso\n", argv[0]);
		return 1;
	}

	if((wfp = fopen(argv[1], "w+b")) == NULL) {
		fprintf(stderr, "E: could not open %s for writing a output file.\n", argv[1]);
		return 1;
	}

	/* copy rootfs.iso from 0x1000 offset, to be a valid iso9660 after mkn64rom. */

	if((rfp = fopen(argv[4], "rb")) == NULL) {
		fprintf(stderr, "E: could not open %s for reading %s.\n", argv[4], "the rootfs.iso");
		return 1;
	}

	/* note: currently n64cart driver supports only 512-bytes sector, so 2^9 align required. */
	/*       but iso9660 is 2^11 aligned, so this may redundant. */
	fseek(rfp, 0x1000, SEEK_SET);
	if(fcopy_with_align(wfp, rfp, 9, NULL, &rootsize) < 0) {
		fprintf(stderr, "E: copying the rootfs.iso %s failed.\n", argv[4]);
		return 1;
	}

	fclose(rfp);

	/* overwrite bootloader to the (nearly, 0x1000 offset) top of iso9660 */
	fseek(wfp, 0, SEEK_SET);
	if(fcopy_by_name_with_align(wfp, argv[2], 3, NULL, &bootsize, "a boot loader") < 0) {
		fprintf(stderr, "E: copying a boot loader %s failed.\n", argv[2]);
		return 1;
	}

	if(find_file_in_iso9660(wfp, argv[3], &kernpos, &kernsize) != 0) {
		fprintf(stderr, "E: could not find the kernel %s in rootfs.iso.\n", argv[3]);
		return 1;
	}

	if(write_kerninfo(wfp, kernpos, kernsize) != 0) {
		fprintf(stderr, "E: could not write the kernel info.\n");
		return 1;
	}

	fclose(wfp);

	printf("boot 0x%lx, kernel 0x%lx@0x%lx, rootfs 0x%lx.\n", bootsize, kernsize, kernpos, rootsize);
#else
	FILE *wfp;
	long bootsize, kernpos, kernsize, rootpos, rootsize;

	if(argc < 4) {
		fprintf(stderr, "usage: %s output.bin n64load.bin vmlinux.bin [rootfs.bin]\n", argv[0]);
		return 1;
	}

	if((wfp = fopen(argv[1], "wb")) == NULL) {
		fprintf(stderr, "E: could not open %s for writing a output file.\n", argv[1]);
		return 1;
	}

	if(fcopy_by_name_with_align(wfp, argv[2], 3, NULL, &bootsize, "a boot loader") < 0) {
		fprintf(stderr, "E: copying a boot loader %s failed.\n", argv[2]);
	return 1;
}

	if(fcopy_by_name_with_align(wfp, argv[3], 3, &kernpos, &kernsize, "a linux kernel") < 0) {
		fprintf(stderr, "E: copying a linux kernel %s failed.\n", argv[3]);
		return 1;
	}

	if(write_kerninfo(wfp, kernpos, kernsize) != 0) {
		fprintf(stderr, "E: could not write the kernel info.\n");
		return 1;
	}

	if(4 < argc) {
		/* note: currently n64cart driver supports only 512-bytes sector, so 2^9 align required. */
		if(fcopy_by_name_with_align(wfp, argv[4], 9, &rootpos, &rootsize, "a rootfs image") < 0) {
			fprintf(stderr, "E: copying a rootfs image %s failed.\n", argv[4]);
			return 1;
		}
	}

	fclose(wfp);

	printf("boot 0x%lx, kernel 0x%lx@0x%lx", bootsize, kernsize, kernpos);
	if(4 < argc) {
		printf(", rootfs 0x%lx@0x%lx", rootsize, rootpos);
	}
	printf(".\n");
#endif

	return 0;
}
