#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>

#define MAXPATH 256
#define BLOCK 512
#define ASCII2NUM(x) ((x) - 48)

/*execute type*/
#define ERROR -1
#define CREATE 1
#define EXTRACT 2
#define DISPLAY 3

struct THeader 
{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
};

 
int isError(int argc, char *argv[])
{
	if (argc < 3){
		fprintf(stderr, "usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
		return 1;
	}
	return 0;
}

void listdir(const char *name, int indent)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char path[1024];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            printf("%*s[%s]\n", indent, "", entry->d_name);
            listdir(path, indent + 2);
        } else {
            printf("%*s- %s\n", indent, "", entry->d_name);
        }
    }
    closedir(dir);
}

/*Helper*/
void addBlock(int fd)
{
	char buffer[BLOCK];
	int i;
	for (i = 0; i < BLOCK; i++) 
		buffer[i] = '\0';

	write(fd, buffer, BLOCK);
}

bool isEmptyBlock(char buffer[BLOCK])
{
	int i;
	bool res = true;
	for (i = 0; i < BLOCK; i++) {
		if (buffer[i] !='\0') {
			res = false;
			i = BLOCK;
		} 
	}
	return res;
}

uint64_t octToDec(char *oct) {
	int i;
	int len = strlen(oct);
	uint64_t res = 0;
	uint64_t octMultiplier = 1;
	for (i = len - 1; i >= 0; i--) {
		res += ASCII2NUM(oct[i]) * octMultiplier;
		octMultiplier *= 8;
	}
	return res;
}
/*end Helper*/

/*Header*/

/*testing function*/
void displayHeader(struct THeader *header)
{
	int i;
	printf("name:		");
	for (i = 0; i < 100; i++)
		printf("%c", header->name[i]);
	printf("\n");
	printf("mode:		%s (%d)\n", header->mode, (int) octToDec(header->mode));
	printf("uid:		%s\n", header->uid);
	printf("gid:		%s\n", header->gid);
	printf("size:		%s\n", header->size);
	printf("mtime:		%s\n", header->mtime);
	printf("chksum:		%s\n", header->chksum);
	printf("typeflag:	%c\n", header->typeflag);
	printf("linkname: 	%s\n", header->linkname);
	printf("magic: 		%s\n", header->magic);
	printf("version: 	%s\n", header->version);
	printf("uname: 		%s\n", header->uname);
	printf("gname: 		%s\n", header->gname);
	/*printf("devmajor: 	%s\n", header->devmajor);
	printf("devminor: 	%s\n", header->devminor);*/
	printf("prefix: 	%s\n", header->prefix);
}
/*end Header*/


/*Takes curent block and structures it into a header*/
struct THeader *getHeader(int fd, int curBlock) {
	struct THeader *header = malloc(sizeof(struct THeader));
	char buffer[BLOCK];
	int i;

	lseek(fd, BLOCK * curBlock, SEEK_SET);

	read(fd, &buffer, 100);
	for (i = 0; i < 100; i++) {
		header->name[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->mode[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->uid[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->gid[i] = buffer[i];
	}

	read(fd, &buffer, 12);
	for (i = 0; i < 12; i++) {
		header->size[i] = buffer[i];
	}

	read(fd, &buffer, 12);
	for (i = 0; i < 12; i++) {
		header->mtime[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->chksum[i] = buffer[i];
	}

	read(fd, &buffer, 1);
	header->typeflag = buffer[0];

	read(fd, &buffer, 100);
	for (i = 0; i < 100; i++) {
		header->linkname[i] = buffer[i];
	}

	read(fd, &buffer, 6);
	for (i = 0; i < 6; i++) {
		header->magic[i] = buffer[i];
	}

	read(fd, &buffer, 2);
	for (i = 0; i < 2; i++) {
		header->version[i] = buffer[i];
	}

	read(fd, &buffer, 32);
	for (i = 0; i < 32; i++) {
		header->uname[i] = buffer[i];
	}

	read(fd, &buffer, 32);
	for (i = 0; i < 32; i++) {
		header->gname[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->devmajor[i] = buffer[i];
	}
	/*fprintf(stdout, "test: devminor is %d\n", (int) octToDec(buffer));*/

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->devminor[i] = buffer[i];
	}
	/*fprintf(stdout, "test: devmajor is %s\n", header->devmajor);*/

	read(fd, &buffer, 155);
	for (i = 0; i < 155; i++) {
		header->prefix[i] = buffer[i];
	}

	return header;

}

/*get path of file or directory*/
char *getPath(int argc, char *argv[], char *name, char path[])
{
	int i;
	int pathLen;
	int nameLen;

	/*if no destination was given, don't add path to name*/
	if(argc <= 3)
		return name;

	/*else add path to beginning of name*/
	pathLen = strlen(argv[3]);
	nameLen = strlen(name);

	for (i = 0; i < pathLen; i++) {
		path[i] = argv[3][i];
	}

	/*make sure the path ends in '/'*/
	if (path[i - 1] != '/')
		path[i] = '/';

	for (i = 0; i < nameLen; i++) { 
		path[i + pathLen] = name[i];
	}
	path[i + pathLen + 1] = '\0';

	return path;
}

int extractFile(int fd, int curBlock, struct THeader *header, char path[])
{
	int outfd;
	char buffer[BLOCK];
	int loop;
	int remainder;
	int i;
	int fSize;

	/*initialize buffer string*/
	for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}

	/*create new file, exit if failure*/
	outfd = open(path, O_WRONLY | O_CREAT, (int)octToDec(header->mode));
	if (outfd == -1)
		exit(EXIT_FAILURE);

	/*copy file into outfd (keeping track of blocks traversed)*/
	fSize = (int) octToDec(header->size);
	loop = (int) fSize/ BLOCK;
	remainder = (int) fSize % BLOCK;

	for (i = 0; i < loop; i++) {
		read(fd, &buffer, BLOCK);
		write(outfd, &buffer, BLOCK);
		curBlock += 1;
	}

	/*write the non null characters in last block*/
	read(fd, &buffer, remainder);  
	write(outfd, &buffer, remainder);
	curBlock += 1;

	/*free space*/
	close(outfd);

	return curBlock;
}

int displayTar(int fd, int curBlock, struct THeader *header, char path[])
{
	/*MASOOD. This
	is literally a copy of the extract file function
	so my code doesn't break. 
	Change it how you will, but it behaves exactly like extract
	But you can use this function here to display
	the function. NOTE that you should only have one print statement 
	here since this is called in a while loop that goes to each header in the
	program so long as you RETURN WHAT YOUR CURENT BLOCK IS*/
	int outfd;
	char buffer[BLOCK];
	int loop;
	int remainder;
	int i;
	int fSize;

	/*initialize buffer string*/
	for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}

		/*create new file, exit if failure (MASOOD: or directory, the extract function is only called when
	it is a file. You cannot call this for display)*/
	outfd = open(path, O_WRONLY | O_CREAT, (int)octToDec(header->mode));
	if (outfd == -1)
		exit(EXIT_FAILURE);

	/*copy file into outfd (keeping track of blocks traversed)*/
	fSize = (int) octToDec(header->size);
	loop = (int) fSize/ BLOCK;
	remainder = (int) fSize % BLOCK;
	for (i = 0; i < loop; i++) {
		read(fd, &buffer, BLOCK);
		write(outfd, &buffer, BLOCK);
		curBlock += 1;
	}

	/*write the non null characters in last block*/
	read(fd, &buffer, remainder);
	write(outfd, &buffer, remainder);
	curBlock += 1;

	/*free space*/
	close(outfd);

	return curBlock;
}

/*Function used for extract and display.
Loops through the headers and files of the tar
and either extracts or displays them*/
void travTar (int argc, char *argv[], bool verbosity, bool strict, bool isDis)
{
	int fd = open(argv[2], O_RDONLY);
	struct THeader *header;
	char buffer[BLOCK];
	int i;
	char path[103];
	int curBlock = 0;
	char type;
	bool done = false;

	/*init string buffers*/
	for (i = 0; i < 103; i++) {
		path[i] = '\0';
	}
	for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}

	/*traverse tar tree*/
	while(!done) {
		header = getHeader(fd, curBlock);
		curBlock += 1;
		/*ensure we are past header*/
		lseek(fd, BLOCK * curBlock, SEEK_SET);
		/*figure out type*/
		type = header->typeflag;
		getPath(argc, argv, header->name, path);
		if(verbosity)
			fprintf(stdout, "%s\n", path);
		/*handle each type different*/
		if (isDis) {
			printf("display mother fuckers\n");
			switch(type) {
				case '0':
					curBlock = extractFile(fd, curBlock, header, path);
					break;
				case '5':
					mkdir(path, (int)octToDec(header->mode));
				default:
					break;
			}
		}
		else {
			switch(type) {
				case '0':
					curBlock = extractFile(fd, curBlock, header, path);
					break;
				case '5':
					mkdir(path, (int)octToDec(header->mode));
				default:
					break;
			}
		}
		/*check if next block is empty and therefore end of tarfile*/
		lseek(fd, BLOCK * curBlock, SEEK_SET);
		read(fd, &buffer, BLOCK);
		if(isEmptyBlock(buffer))
			done = true;
		lseek(fd, BLOCK * curBlock, SEEK_SET);

		/*free header*/
		free(header);
	}
}

int main(int argc, char *argv[])
{
	int options;
	int execute = 0;
	bool verbosity = false;
	bool strict = false;
	int i;
	int len;
	/*determine if input is valid...ish*/
	if (isError(argc, argv)){
		fprintf(stdout, "Not enough input\n");
		return 0;
	}

	/*get command line arguments while still checking for errors*/
	len = strlen (argv[1]);
	for (i = 0; i < len; i++) {
		options = argv[1][i];
		switch(options) {
			case 'f':
				break;
			case 'c':
				execute = (execute == 0) ? CREATE : ERROR;
				break;
			case 't':
				execute = (execute == 0) ? DISPLAY : ERROR;
				break;
			case 'x':
				execute = (execute == 0) ? EXTRACT : ERROR;
				break;
			case 'v':
				verbosity = true;
				break;
			case 'S':
				strict = true;
				break;	
			default:
				execute = ERROR;
		}
		if (execute == ERROR) {
			printf("a\n");
			fprintf(stderr, "usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
			exit(EXIT_FAILURE);
		}
	}

	/*make sure that an execute was given*/
	if (execute == 0) {
		printf("b\n");
		fprintf(stderr, "usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
		exit(EXIT_FAILURE);
	}

	/*execute code*/
	switch(execute) {
		case CREATE:
			printf("CREATE\n");
			break;
		case DISPLAY:
			travTar(argc, argv, verbosity, strict, true);
			break;
		case EXTRACT:
			travTar(argc, argv, verbosity, strict, false);
			break;
	}


	return 0;
}