#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_NAME_LEN 1024
#define READ_BUF_SIZE (128*1024)

FILE *in_blocks;
int blocksize = 4096;

void skip_file(char *buf)
{
	while (fgets(buf, MAX_NAME_LEN, in_blocks) && buf[0] == ' ');
}

void read_extent(int fd, loff_t off, loff_t len)
{
	char readbuf[READ_BUF_SIZE];
	loff_t toread;

	lseek64(fd, off*blocksize, SEEK_SET);
	while (len) {
		toread = len < READ_BUF_SIZE ? len : READ_BUF_SIZE;

		read(fd, readbuf, toread);
		len -= toread;
	}
}

void load_blocks(char *mnt)
{
	char namebuf[MAX_NAME_LEN];
	char linebuf[MAX_NAME_LEN];
	char *par, *namestart;
	int fd;
	long long start, offset, len;

	strcpy(namebuf, mnt);
	namestart = namebuf+strlen(namebuf);
 	if (!fgets(linebuf, MAX_NAME_LEN, in_blocks))
		return;

	while (!feof(in_blocks)) {
		if (!strncmp(linebuf, "Inode", 5)) {
			skip_file(linebuf);
			continue;
		}
		/* Remove inode number */
		par = strrchr(linebuf, '(');
		if (!par) {
			fprintf(stderr, "Cannot parse input line: %s\n", linebuf);
			exit(1);
		}
		*(par-1) = 0;
		strcpy(namebuf, linebuf);
		if ((fd = open(namebuf, O_RDONLY)) < 0) {
			fprintf(stderr, "Cannot open file %s: %s\n", namebuf, strerror(errno));
			skip_file(linebuf);
			continue;
		}
		while (fgets(linebuf, MAX_NAME_LEN, in_blocks) && linebuf[0] == ' ') {
			if (sscanf(linebuf, "%Ld+%Ld %Ld", &start, &len, &offset) != 3) {
				fprintf(stderr, "Cannot parse line: %s\n", linebuf);
				exit(1);
			}
			if (offset >= 0)
				read_extent(fd, offset, len);
		}
		close(fd);
	} 
}

int main(int argc, char **argv)
{
	
	if (argc != 3) {
		fprintf(stderr, "Usage: preloadblocks <file-with-block> <mountpoint>\n");
		return 1;
	}

	if (!(in_blocks = fopen(argv[1], "r"))) {
		perror("Cannot open file with blocks to load");
		return 1;
	}
	load_blocks(argv[2]);
	return 0;
}
