#include <stdio.h>

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

	copied= fcopy_with_align(wfp, rfp, align2, prompos, psize_aligned);

	fclose(rfp);

	return copied;
}

int main(int argc, char *argv[]) {
	FILE *rfp, *wfp;
	long bootsize, kernpos, kernsize, rootpos, rootsize;

	if(argc < 4) {
		fprintf(stderr, "usage: %s output.bin n64load.bin vmlinux.bin [rootfs.bin]\n", argv[0]);
		return 1;
	}

	if((wfp = fopen(argv[1], "wb")) == NULL) {
		fprintf(stderr, "E: could not open %s for writing a output file.\n");
		return 1;
	}

	fcopy_by_name_with_align(wfp, argv[2], 3, NULL, &bootsize, "a boot loader");

	fcopy_by_name_with_align(wfp, argv[3], 3, &kernpos, &kernsize, "a linux kernel");
	{
#define bswap32(n) ( \
	(((n) >> 24) & 0x000000FF) | \
	(((n) >>  8) & 0x0000FF00) | \
	(((n) <<  8) & 0x00FF0000) | \
	(((n) << 24) & 0xFF000000) \
	)
		/* note: PI_CART_ADDR cart base is 0x10000000. */
		long kernpos2_be = bswap32(kernpos + 0x10000000);
		/* note: PI_WR_LEN is to-be-copied minus 1. */
		long kernsize2_be = bswap32(kernsize - 1);

		long tell = ftell(wfp);
		fseek(wfp, 0x10, SEEK_SET);
		fwrite(&kernpos2_be, 4, 1, wfp);
		fwrite(&kernsize2_be, 4, 1, wfp);
		fseek(wfp, tell, SEEK_SET);
	}

	if(4 < argc) {
		/* note: currently n64cart driver supports only 512-bytes sector, so 2^9 align required. */
		fcopy_by_name_with_align(wfp, argv[4], 9, &rootpos, &rootsize, "a rootfs image");
	}

	fclose(wfp);

	printf("boot 0x%x, kernel 0x%x@0x%x", bootsize, kernsize, kernpos);
	if(4 < argc) {
		printf(", rootfs 0x%x@0x%x", rootsize, rootpos);
	}
	printf(".\n");

	return 0;
}
