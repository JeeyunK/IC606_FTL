#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct user_request {
	uint32_t lba;
	int op;
	int io_size;
} user_request;

static off_t fdlength(int fd)
{
	struct stat st;
	off_t cur, ret;

	if (!fstat(fd, &st) && S_ISREG(st.st_mode))
		return st.st_size;

	cur = lseek(fd, 0, SEEK_CUR);
	ret = lseek(fd, 0, SEEK_END);
	lseek(fd, cur, SEEK_SET);

	return ret;
}

static inline long parse(const off_t read_len, const char * const buf, const bool op)
{
	static long cur = 0, parsed = 0;
	long last, ret;
	char tmp[32];
	bool eof;

	last = cur;
	while (!(eof = cur > read_len) && buf[cur] != ',' && buf[cur] != '\n')
		cur++;
	if (eof)
		return -1;

	memcpy(&tmp, buf + last, cur - last);
	tmp[cur - last] = '\0';

	//puts(tmp);
	cur++;
	if (op) {
		switch (tmp[0]) {
		case 'W':
			ret = 1;
			break;
		case 'R':
			ret = 0;
			break;
		case 'D':
			ret = 3;
			break;
		case 'F':
			ret = -2;
			break;
		default:
			fprintf(stderr, "Unrecognized op: \"%s\"\n", tmp);
			exit(1);
			break;
		}
	} else {
		ret = atol(tmp);
	}

	if (++parsed % 10000 == 0) {
		printf("\r%.2f%%", (float)cur * 100 / read_len);
	}

	return ret;
}

int main(int argc, char *argv[])
{
	int readfd;
	FILE *wr_f; // Use stdio's FILE so that writes are buffered automatically
	off_t read_len;
	char *buf;
	user_request req;
	long count = 0;
	static char wrbuf[4 * 1024 * 1024];

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	if (argc != 3) {
		fprintf(stderr, "Usage: %s /path/to/old_trace /path/to/new_trace\n", argv[0]);
		exit(1);
	}

	readfd = open(argv[1], O_RDONLY);
	if (readfd < 0) {
		perror("Failed to open read file");
		exit(1);
	}

	read_len = fdlength(readfd);
	buf = (char*)mmap(NULL, read_len, PROT_READ, MAP_SHARED, readfd, 0);
	if (buf == MAP_FAILED) {
		perror("Failed to mmap read file");
		exit(1);
	}
	close(readfd);

	madvise(buf, read_len, MADV_SEQUENTIAL | MADV_WILLNEED);

	wr_f = fopen(argv[2], "w");
	if (wr_f == NULL) {
		perror("Failed to open write file");
		exit(1);
	}

	setvbuf(wr_f, wrbuf, _IOFBF, sizeof(wrbuf));

	// Write a null long for line count
	fwrite(&count, sizeof(count), 1, wr_f);

	// Start parsing
	for (;;) {
		req.op = parse(read_len, buf, true);
		req.lba = parse(read_len, buf, false) / 8;
		req.io_size = parse(read_len, buf, false);
		if (req.op == -1)
			break;
		if (req.op == -2)
			continue;
		if (req.io_size == 0)
			continue;
		if (req.lba >= 128ULL * 1024 * 1024 * 1024 / 4096) {
			printf("count: %ld, op: %d, lba: %u, size: %d is out-of-range, skipping\n", count, req.op, req.lba, req.io_size);
			continue; // EOF
		}

//		req.lba %= 32LL * 1024 * 1024 * 1024 / 4096;

		assert(0 <= req.op && req.op <= 3);
		//printf("op: %d, lba: %u, size: %d\n", req.op, req.lba, req.io_size);
		fwrite(&req, sizeof(req), 1, wr_f);

		count++;
	}
	printf("\n");

	munmap(buf, read_len);

	// Rewind wr_f to write line count
	fseek(wr_f, 0, SEEK_SET);
	fwrite(&count, sizeof(count), 1, wr_f);

	fclose(wr_f);

	return 0;
}
