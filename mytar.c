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
#include <time.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>

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
	char name[101];
	char mode[9];
	char uid[9];
	char gid[9];
	char size[13];
	char mtime[13];
	char chksum[9];
	char typeflag;
	char linkname[101];
	char magic[7];
	char version[3];
	char uname[33];
	char gname[33];
	char devmajor[9];
	char devminor[9];
	char prefix[156];
};

 
int isError(int argc, char *argv[])
{
	if (argc < 3){
		fprintf(stderr, 
			"usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
		return 1;
	}
	return 0;
}

/**************************Helper******************************/

/*void set_Check(header){
		unsigned char runningTotal = 0;
		unsigned char *initialize = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
		int x;
		runningTotal = runningTotal + header->typeflag[0];
		for(x = 0; x < 8; x++){
					runningTotal = runningTotal + initialize[x];
		}
		for(x = 0; x < strlen(header->name); x++){
					runningTotal = runningTotal + header->name[x];
		}
		for(x = 0; x < strlen(header->mode; x++){
					runningtotal = runningTotal + header->mode[x];
		}
		for(x = 0; x < strlen(header->uid; x++){
					runningtotal = runningTotal + header->uid[x];
		}
		for(x = 0; x < strlen(header->gid; x++){
					runningtotal = runningTotal + header->gid[x];
		}
		for(x = 0; x < strlen(header->size; x++){
					runningtotal = runningTotal + header->size[x];
		}
		for(x = 0; x < strlen(header->mtime; x++){
					runningtotal = runningTotal + header->mtime[x];
		}
		for(x = 0; x < strlen(header->linkname; x++){
					runningtotal = runningTotal + header->linkname[x];
		}
		for(x = 0; x < strlen(header->magic; x++){
					runningtotal = runningTotal + header->magic[x];
		}
		for(x = 0; x < strlen(header->version; x++){
					runningtotal = runningTotal + header->version[x];
		}
		for(x = 0; x < strlen(header->uname; x++){
					runningtotal = runningTotal + header->uname[x];
		}
		for(x = 0; x < strlen(header->gname; x++){
					runningtotal = runningTotal + header->gname[x];
		}
		for(x = 0; x < strlen(header->devmajor; x++){
					runningtotal = runningTotal + header->devmajor[x];
		}
		for(x = 0; x < strlen(header->devminor; x++){
					runningtotal = runningTotal + header->devminor[x];
		}
		for(x = 0; x < strlen(header->prefix; x++){
					runningtotal = runningTotal + header->prefix[x];
		}
		sprintf(header->chksum, "%o", runningtotal;
}*/


char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}


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

void getPath(struct THeader *header, char path[])
{
	if (strlen(header->prefix) == 0) {
		strcpy(path, header->name);
		path[100] = '\0';
		return;
	}
	strcpy(path, header->prefix);
	strcat(path, "/");
	strcat(path, header->name);
	return;	
}


int myrmdir(char *path) {
	DIR *dir = opendir(path);
	int pathlen = strlen(path);
	int len;
	int rm1 = -1;
	int rm2;
	char *buffer;

	struct dirent *dp;
	struct stat sb;


	if (dir) {
		rm1 = 0;
		while((dp = readdir(dir)) && !rm1) {
			if (!strcmp(dp->d_name, ".") ||
				!strcmp(dp->d_name, "..")) 
				continue;
			len = pathlen + strlen(dp->d_name) + 2;
			buffer = malloc(len);

			if(buffer) {
				snprintf(buffer, len, "%s/%s",
					path, dp->d_name);

				if(!stat(buffer, &sb)) {
					if(S_ISDIR(sb.st_mode))
						rm2 = myrmdir(buffer);
					else
						rm2 = unlink(buffer);
				}
				free(buffer);
			}
			rm1 = rm2;
		}
		closedir(dir);
	}
	if (!rm1) 
		rm1 = rmdir(path);
	return rm1;
}

/**************************end Helper************************/

/****************************Header**************************/

/*testing function*/
void displayHeader(struct THeader *header)
{
	int i;
	printf("name:		");
	for (i = 0; i < 100; i++)
		printf("%c", header->name[i]);
	printf("\n");
	printf("mode:		%s\n", header->mode);
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

struct THeader *initHeader()
{
	struct THeader *header = malloc(sizeof(struct THeader));
	int i;

	for (i = 0; i < 100; i++) {
		header->name[i] = '\0';
	}
	header->name[i] = '\0';

	for (i = 0; i < 8; i++) {
		header->mode[i] = '\0';
	}
	header->mode[i] = '\0';

	for (i = 0; i < 8; i++) {
		header->uid[i] = '\0';
	}
	header->uid[i] = '\0';

	for (i = 0; i < 8; i++) {
		header->gid[i] = '\0';
	}
	header->gid[i] = '\0';

	for (i = 0; i < 12; i++) {
		header->size[i] = '\0';
	}
	header->size[i] = '\0';

	for (i = 0; i < 12; i++) {
		header->mtime[i] = '\0';
	}
	header->mtime[i] = '\0';

	for (i = 0; i < 8; i++) {
		header->chksum[i] = '\0';
	}
	header->chksum[i] = '\0';

	header->typeflag = '\0';

	for (i = 0; i < 100; i++) {
		header->linkname[i] = '\0';
	}
	header->linkname[i] = '\0';

	for (i = 0; i < 6; i++) {
		header->magic[i] = '\0';
	}
	header->magic[i] = '\0';

	for (i = 0; i < 2; i++) {
		header->version[i] = '\0';
	}
	header->version[i] = '\0';

	for (i = 0; i < 32; i++) {
		header->uname[i] = '\0';
	}
	header->uname[i] = '\0';

	for (i = 0; i < 32; i++) {
		header->gname[i] = '\0';
	}
	header->gname[i] = '\0';

	for (i = 0; i < 8; i++) {
		header->devmajor[i] = '\0';
	}
	header->devmajor[i] = '\0';
	/*fprintf(stdout, "test: devminor is %d\n", (int) octToDec(buffer));*/

	for (i = 0; i < 8; i++) {
		header->devminor[i] = '\0';
	}
	header->devminor[i] = '\0';
	/*fprintf(stdout, "test: devmajor is %s\n", header->devmajor);*/

	for (i = 0; i < 155; i++) {
		header->prefix[i] = '\0';
	}
	header->prefix[i] = '\0';

	return header;

}

int writeHeader(struct THeader *header, int fd, int curBlock)
{
	char buffer[BLOCK];
	int i;
	int count = 0;

	for (i = 0; i < BLOCK; i++) 
		buffer[i] = '\0';

	for (i = 0; i < 100; i++) {
		buffer[count] = header->name[i];
		count ++;
	}

	for (i = 0; i < 8; i++) {
		buffer[count] = header->mode[i];
		count ++;
	}

	for (i = 0; i < 8; i++) {
		buffer[count] = header->uid[i];
		count ++;
	}

	for (i = 0; i < 8; i++) {
		buffer[count] = header->gid[i];
		count ++;
	}

	for (i = 0; i < 12; i++) {
		buffer[count] = header->size[i];
		count ++;
	}

	for (i = 0; i < 12; i++) {
		buffer[count] = header->mtime[i];
		count ++;
	}

	for (i = 0; i < 8; i++) {
		buffer[count] = header->chksum[i];
		count ++;
	}

	buffer[count] = header->typeflag;
	count ++;

	for (i = 0; i < 100; i++) {
		buffer[count] = header->linkname[i];
		count ++;
	}

	for (i = 0; i < 6; i++) {
		buffer[count] = header->magic[i];
		count ++;
	}

	for (i = 0; i < 2; i++) {
		buffer[count] = header->version[i];
		count ++;
	}

	for (i = 0; i < 32; i++) {
		buffer[count] = header->uname[i];
		count ++;
	}

	for (i = 0; i < 32; i++) {
		buffer[count] = header->gname[i];
		count ++;
	}

	for (i = 0; i < 8; i++) {
		buffer[count] = header->devmajor[i];
		count ++;
	}
	/*fprintf(stdout, "test: devminor is %d\n", (int) octToDec(buffer));*/

	for (i = 0; i < 8; i++) {
		buffer[count] = header->devminor[i];
		count ++;
	}
	/*fprintf(stdout, "test: devmajor is %s\n", header->devmajor);*/

	for (i = 0; i < 155; i++) {
		buffer[count] = header->prefix[i];
		count ++;
	}

	write(fd, &buffer, BLOCK);
	curBlock +=1;
	return curBlock;
}

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
	header->name[i] = '\0';

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->mode[i] = buffer[i];
	}
	header->mode[i] = '\0';

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->uid[i] = buffer[i];
	}
	header->uid[i] = '\0';

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->gid[i] = buffer[i];
	}
	header->gid[i] = '\0';

	read(fd, &buffer, 12);
	for (i = 0; i < 12; i++) {
		header->size[i] = buffer[i];
	}
	header->size[i] = '\0';

	read(fd, &buffer, 12);
	for (i = 0; i < 12; i++) {
		header->mtime[i] = buffer[i];
	}
	header->mtime[i] = '\0';

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->chksum[i] = buffer[i];
	}
	header->chksum[i] = '\0';

	read(fd, &buffer, 1);
	header->typeflag = buffer[0];

	read(fd, &buffer, 100);
	for (i = 0; i < 100; i++) {
		header->linkname[i] = buffer[i];
	}
	header->linkname[i] = '\0';

	read(fd, &buffer, 6);
	for (i = 0; i < 6; i++) {
		header->magic[i] = buffer[i];
	}
	header->magic[i] = '\0';

	read(fd, &buffer, 2);
	for (i = 0; i < 2; i++) {
		header->version[i] = buffer[i];
	}
	header->version[i] = '\0';

	read(fd, &buffer, 32);
	for (i = 0; i < 32; i++) {
		header->uname[i] = buffer[i];
	}
	header->uname[i] = '\0';

	read(fd, &buffer, 32);
	for (i = 0; i < 32; i++) {
		header->gname[i] = buffer[i];
	}
	header->gname[i] = '\0';

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->devmajor[i] = buffer[i];
	}
	header->devmajor[i] = '\0';
	/*fprintf(stdout, "test: devminor is %d\n", (int) octToDec(buffer));*/

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->devminor[i] = buffer[i];
	}
	header->devminor[i] = '\0';
	/*fprintf(stdout, "test: devmajor is %s\n", header->devmajor);*/

	read(fd, &buffer, 155);
	for (i = 0; i < 155; i++) {
		header->prefix[i] = buffer[i];
	}
	header->prefix[i] = '\0';

	return header;

}
/*******************end Header***********************/

/*****************execute and display******************/

void extractPath(char *path, int mode)
{
	char **tokens;
	DIR *dir;
	int i = 0;

	tokens = str_split(path, '/');

	if(!tokens)
		return;

	strcpy(path, *(tokens + i));
	strcat(path, "/");
	dir = opendir(path);
	if (!dir)
		mkdir(path, S_IRUSR | S_IWUSR |
				S_IXUSR);
	closedir(dir);
	free(*(tokens + i));
	i ++;

	do {
		strcat(path, *(tokens + i));
		strcat(path, "/");
		dir = opendir(path);
		if (!dir)
			mkdir(path, S_IRUSR | S_IWUSR |
				S_IXUSR);
		printf("[%s]", *(tokens + i));
		closedir(dir);
		free(*(tokens + i));
		i++;
	} while(*(tokens + i + 1));

	printf("file: [%s]\n", *(tokens + i));
	printf("path: %s\n", path);
	strcat(path, *(tokens + i));
	free(*(tokens + i));

	free(tokens);

}

void extractDir(char *path, int mode)
{
	DIR *dir = opendir(path);

	if(dir) {
		closedir(dir);
		myrmdir(path);
	}
	mkdir(path, mode);
}

int extractFile(int fd, int curBlock, 
	struct THeader *header, char path[])
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
	if (outfd == -1) {
		extractPath(path, (int)octToDec(header->mode));
		outfd = open(path, O_WRONLY | O_CREAT,
		 (int)octToDec(header->mode));
		if (outfd == -1) {
			printf("COULDN'T OPEN FILE");
			exit(EXIT_FAILURE);
		}
	}
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

int skipFile(int fd, int curBlock, struct THeader *header, char path[])
{
	/*MASOOD. This
	is literally a copy of the extract file function
	so my code doesn't break. 
	Change it how you will, but it behaves exactly like extract
	But you can use this function here to display
	the function. NOTE that you should only have one print statement 
	here since this is called in a while loop that goes to
	 each header in the
	program so long as you RETURN WHAT YOUR CURENT BLOCK IS*/
	int loop;
	int fSize;

	/*copy file into outfd (keeping track of blocks traversed)*/
	fSize = (int) octToDec(header->size);
	loop = (int) fSize/ BLOCK;
	curBlock += loop + 1;
	lseek(fd, BLOCK * curBlock, SEEK_SET);

	return curBlock;
}

void displayFile(char *name, struct THeader *header)
{
	struct stat sb;
	int i;
	int uLen = strlen(header->uname);
	int gLen = strlen(header->gname);
	char owner[50];
	time_t mtime = (int)octToDec(header->mtime);
	char mtimeStr[20];
	struct tm *timeInfo;

	/*format owner name*/
	for (i = 0; i < uLen; i++)
		owner[i] = header->uname[i];
	owner[i] = '/';
	for (i = 0; i < gLen; i++)
		owner[i + uLen + 1] = header->gname[i];
	owner[i + uLen + 1] = '\0';

	/*formating mtime*/
	timeInfo = localtime(&mtime);
	strftime(mtimeStr, sizeof(mtimeStr), "%G-%m-%d %H:%M", timeInfo);

	stat(name, &sb);

	switch(header->typeflag) {
		case '5':
			printf("d");
			break;
		case '2':
			printf("l");
			break;
		default:
			printf("-");
			break;
	}

	/*print permissions*/
    printf(((int)octToDec(header->mode) & S_IRUSR) ? "r" : "-") ;
    printf(((int)octToDec(header->mode) & S_IWUSR) ? "w" : "-");
    printf(((int)octToDec(header->mode) & S_IXUSR) ? "x" : "-");
    printf(((int)octToDec(header->mode) & S_IRGRP) ? "r" : "-");
    printf(((int)octToDec(header->mode) & S_IWGRP) ? "w" : "-");
    printf(((int)octToDec(header->mode) & S_IXGRP) ? "x" : "-");
    printf(((int)octToDec(header->mode) & S_IROTH) ? "r" : "-");
    printf(((int)octToDec(header->mode) & S_IWOTH) ? "w" : "-");
    printf(((int)octToDec(header->mode) & S_IXOTH) ? "x" : "-");

    /*printf("uid: %s, gid: %s, uname: %s, gname: %s\n",
    	header->uid, header->gid, header->uname, header->gname);*/
    
    printf(" %17s %8d", owner, (int)octToDec(header->size));
    printf(" %16s %s\n", 
    	mtimeStr, name);


}

int displayNamedChildren(int curBlock, char *dirName, int start,
	bool verbosity, int fd, int argc, char *argv[])
{
	bool done = false;
	char newPath[400];
	char path[255];
	struct THeader *header;
	char type;
	char buffer[BLOCK];
	int i;

	/*init string buffers*/
	for (i = 0; i < 255; i++) {
		path[i] = '\0';
	}
	for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}

	while(!done) {
		header = getHeader(fd, curBlock);
		curBlock += 1;
		/*ensure we are past header*/
		lseek(fd, BLOCK * curBlock, SEEK_SET);
		/*figure out type*/
		type = header->typeflag;
		getPath(header, path);

		printf("path: %s\n", path);

		strcpy(newPath, path + start - 1);

		printf("newPath: %s\n", newPath);

		if (strstr(newPath, dirName) != NULL) {
			displayFile(newPath, header);
			if(type != '5') {
				curBlock = skipFile(fd, curBlock,
					header, path);
			}
		}
		else
			done = true;

		/*check if next block is empty and therefore end of tarfile*/
		lseek(fd, BLOCK * curBlock, SEEK_SET);
		read(fd, &buffer, BLOCK);
		if(isEmptyBlock(buffer)) {
			done = true;
			lseek(fd, BLOCK * curBlock, SEEK_SET);
			free(header);
			exit(EXIT_SUCCESS);
		}


		/*free header*/
		free(header);
	}

	return curBlock;
}

int isNamed(char *path, int argc, char *argv[], bool verbosity,
	struct THeader *header, bool isDir)
{
	int i;
	for (i = 3; i < argc; i++) {
		if(strstr(path, argv[i]) != NULL) {
			if(!verbosity && isDir)
				printf("%s\n", path);
			else if (verbosity && isDir)
				displayFile(path, header);
			if (!isDir)
				return true;
		}
	}
	return false;
}


/*Function used for extract and display.
Loops through the headers and files of the tar
and either extracts or displays them*/
void travTar (int argc, char *argv[], 
	bool verbosity, bool strict, bool isDis)
{
	int fd = open(argv[2], O_RDONLY);
	struct THeader *header;
	char buffer[BLOCK];
	int i;
	char path[255];
	int curBlock = 0;
	char type;
	bool done = false;
	int hCount = 0;
	bool named = false;

	/*for (i = 0; i < 300; i++) {
		tempPath[i] = '\0';
	}*/
	i = 0;

	/*init string buffers*/
	for (i = 0; i < 255; i++) {
		path[i] = '\0';
	}
	for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}

	/*traverse tar tree*/
	while(!done) {
		header = getHeader(fd, curBlock);
		curBlock += 1;
		hCount += 1;
		/*ensure we are past header*/
		lseek(fd, BLOCK * curBlock, SEEK_SET);
		/*figure out type*/
		type = header->typeflag;
		getPath(header, path);

		/*display files if "xv" or "t(v)"*/
		if(/*(verbosity && !isDis ) || */(isDis)) {
			if (isDis && (argc <= 3) && !verbosity)
				fprintf(stdout, "%s\n", path);
			else if (isDis && verbosity && (argc <= 3))
				displayFile(path, header);
			else if (isDis && (argc > 3))
				isNamed(path, argc, argv, 
					verbosity, header, true);
			/*else if (!isDis)
				fprintf(stdout, "%s\n", path);*/

		}

		/*handle each type different*/
		/*MASOOD!!!!!! You have a header for a directory or a file*/
		if (isDis) {
			/*printf("count: %d\n", hCount);*/
			switch(type) {
				case '0':  /*If it is a file*/
					curBlock = skipFile(fd, curBlock, 
						header, path);
					break;
				case '5': /*If it is a directory*/
					mkdir(path,
					 (int)octToDec(header->mode));
				case '\0': /*if it is a sym link*/
				default:  /*if it is anything else*/
					break;
			}
		}
		/*END MASOOD*/
		if(!isDis && argc <= 3) {
			if (verbosity)
				fprintf(stdout, "%s\n", path);
			switch(type) {
				case '0':
					curBlock = 
					extractFile(fd, curBlock,
					 header, path);
					break;
				case '2':
					symlink(header->linkname,
					 header->name);
				case '5':
					extractDir(path, 
					(int)octToDec(header->mode));
					/*mkdir(path,
				 (int)octToDec(header->mode));*/
				default:
					break;
			}
		}

		if(!isDis && argc > 3) {
			named = isNamed(path, argc, argv,
				verbosity, header, false);
			if (!named) {
				switch(type) {
					case '0':  /*If it is a file*/
						curBlock = skipFile(fd, 
							curBlock, 
							header, path);
						break;
					case '5': /*If it is a directory*/
						/*do nothing*/
					case '\0': /*if it is a sym link*/
					default:  /*if it is anything else*/
						break;
				}
			}
			else {
				switch(type) {
					case '0':
						/*strcpy(tempPath, path);*/
						/*extractPath(tempPath,
						(int)octToDec(header->mode));*/
						curBlock = 
						extractFile(fd, curBlock,
						 header, path);
						break;
					case '5':
						extractDir(path, 
						(int)octToDec(header->mode));
						/*mkdir(path,
					 (int)octToDec(header->mode));*/
					default:
						break;
				}
			}
			if (verbosity && named)
				fprintf(stdout, "%s\n", path);
			/*else if(verbosity && !named)
				fprintf(stdout, "FUCK: %s\n", path);*/

		}
		/*if(!isDis && argc > 3) {
			named = isNamed(path, argc, argv, 
					verbosity, header, false);
			if (named && verbosity)
				fprintf(stdout, "%s\n", path);
			switch(type) {
				case '0':
					if (named)
					curBlock = 
					extractFile(fd, curBlock,
					 header, path);
					else
					curBlock = skipFile(fd, curBlock, 
						header, path);
					break;
				case '5':
					if(named)
					extractDir(path, 
					(int)octToDec(header->mode));
				default:
					break;
			}
		}*/
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

/*********************end execute and display********************/

/**********************create***************************/

/* Helper Function to clear the prefix */
void clear_Prefix(struct THeader *header){
        int x;
        for(x = 0; x < strlen(header->prefix); x++){
                header->prefix[x] = '\0';
        }
}

/*TODO: Potential Edge case, file name over 200
 characters that includes a /. */
void makeHeader(char *name, struct THeader *header)
{
        /*open stat of name*/
        /*get a stat and get information*/
        /*gets all the information from file*/
        /*make a DIR and a dirent and a stat*/
        struct stat sb;
        /*int x = 0, MAX = 100, length = strlen(name), y = 0;*/
		/*bool found = false;*/
		/*bool cat = false;*/
		char prefix[300];
		char cpName[300];
		int countlen = 0;
		int namelen;
		int preflen;
		int i;
		char **tokens;

		for (i = 0; i < 300; i++) {
			prefix[i] = '\0';
			cpName[i] = '\0';
		}


		lstat(name, &sb);
		/* Set the Name */
		/* 
		 * If we have something in the prefix, include it in the name.
         * Max gets updated 
		 */

		if (strlen(name) >= 100) {
			tokens = str_split(name, '/');
			i = 0;
			countlen = strlen(*(tokens));
			i ++;
			while (*(tokens + i)) {
				countlen += strlen(*(tokens + i));
				if (countlen >= 100) {
					namelen = i - 1;
				}
				i++;
			}
			preflen = i - namelen;
			for (i = 0; i < preflen; i++) {
				strcat(prefix, *(tokens + i));
				strcat(prefix, "/");
				free(*(tokens + i));
			}
			for(i = preflen; i < preflen + namelen - 1; i++) {
				strcat(cpName, *(tokens + i));
				strcat(cpName, "/");
				free(*(tokens + i));
			}
			strcat(cpName, *(tokens + i));
			strcpy(header->name, cpName);
			strcpy(header->prefix, prefix);
			free (tokens);

		}
		else {
			strcpy(header->name, name);
		}

		printf("NAME: %s\n", header->name);
		printf("PREFIX: %s\n", header->prefix);
        /*if(strlen(header->prefix) > 0){
                strcpy(header->name, 
                	header->prefix);
                printf("NAME:");
                MAX = MAX - length;
				clear_Prefix(header);
				cat = true;
        }
		length--;
		while(x != MAX){
				if(name[length] == '/'){
						strncpy(header->prefix,
							name, length);
						name = name + length;
						found = true;
						break;
				}
				x++;
				length--;
			}
		if(found){strcpy(header->name, name);}
		else{
			if(strlen(name) > MAX){
				fprintf(stderr, 
					"File %s is too large! Skipping...\n", 
					name);
			}
			else{strcpy(header->name, name);}
		}*/
		
		/*MODE, UID, GID, SIZE, MTIME, */
		sprintf(header->mode, "%07o", (int)sb.st_mode);
		sprintf(header->uid, "%7o", (int)sb.st_uid);
		sprintf(header->gid, "%7o", (int)sb.st_gid);
		sprintf(header->size, "%011o", (int)sb.st_size);
		sprintf(header->mtime, "%11o", (int)sb.st_mtime);
		printf("MODE: %s\n", header->mode);
		printf("uid:  %s\n", header->uid);
		printf("GID:  %s\n", header->gid);
		printf("SIZE: %s\n", header->size);
		printf("MTIME:%s\n", header->mtime);
		/*TODO: CHKSUM
		TYPE FLAG - '0' Reg, '\0' Alt-Reg, '2' Sym, '5' direc 
		TODO: MAKE SURE TO CHECK HOW
		 TO HANDLE REGULAR ALTERNATE FILES */
		if(S_ISLNK(sb.st_mode)){
				header->typeflag = '2';
				/*TODO Set SYMLINK Value here.*/
		}
		else if(S_ISDIR(sb.st_mode)){header->typeflag = '5';}
		else if(S_ISREG(sb.st_mode)){header->typeflag = '0';}
		strcpy(header->magic, "ustar");
		strcpy(header->version, "00");
		strcpy(header->uname, getpwuid(sb.st_uid)->pw_name);
		strcpy(header->gname, getgrgid(sb.st_gid)->gr_name);
		/*TODO DevMajor && DevMinor*/
		
		
		
		
}

int putFile(int tarfd, int curBlock, struct stat *sb, char *name)
{
	int fd;
	char buffer[BLOCK];
	int loop;
	int remainder;
	int i;
	int fSize;
	struct THeader *header = initHeader();

	/*initialize buffer string*/
	for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}

	/*write blank header*/
	makeHeader(name, header);
	curBlock = writeHeader(header, tarfd, curBlock);
	lseek(tarfd, BLOCK * curBlock, SEEK_SET	);
	free(header);
	


	/*open file TODO: figure out how to handle a -1 error*/
	fd = open(name, O_RDONLY);
	if (fd == -1)
		exit(EXIT_FAILURE); /*TODO*/

	/*copy file into tarfd (keeping track of blocks traversed)*/
	fSize = sb->st_size;
	loop = (int) fSize/ BLOCK;
	remainder = (int) fSize % BLOCK;

	for (i = 0; i < loop; i++) {
		read(fd, &buffer, BLOCK);
		write(tarfd, &buffer, BLOCK);
		curBlock += 1;
	}

	/*write the non null characters in last block*/
	read(fd, &buffer, remainder);  
	write(tarfd, &buffer, remainder);
	curBlock += 1;

	/*free space*/
	close(fd);

	return curBlock;
}

int putDir(int tarfd, int curBlock, char *name)
{
	/*char buffer[BLOCK];*/
	DIR *dir;
	struct dirent *dp;
	struct stat sb;
	char path[103];
	struct THeader *header = initHeader();


	/*initialize buffer string*/
	/* for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}*/

	/*open file TODO: figure out how to handle a -1 error*/
	if (!(dir = opendir(name))) {
		return curBlock; /*TODO*/
	}

	/*insert header for directory*/
	/*lseek(tarfd, BLOCK * curBlock, SEEK_SET);
	curBlock += 1;*/
	/*write blank header*/
	makeHeader(name, header);
	curBlock = writeHeader(header, tarfd, curBlock);
	lseek(tarfd, BLOCK * curBlock, SEEK_SET	);
	free(header);

	while((dp = readdir(dir)) != NULL) {
		/*ignore current and parent directories*/
		if (strcmp(dp->d_name, ".") == 0 || 
			strcmp(dp->d_name, "..") == 0)
			continue;

		/*get path and stat*/
		snprintf(path, sizeof(path), "%s/%s", name, dp->d_name);
		if(stat(path, &sb) == -1)
			continue;

		/*make blank header*/
		lseek(tarfd, BLOCK * curBlock, SEEK_SET);
		curBlock += 1;

		/*if dir, recurse*/
		if (S_ISDIR(sb.st_mode)) {
			putDir(tarfd, curBlock, path);
		}
		else if (S_ISREG(sb.st_mode)) {
			curBlock = putFile(tarfd, curBlock, &sb, path);
		}
	}
	closedir(dir);
	return curBlock;
}

/*FOR MASOOD*/
/*struct THeader *makeHeader(char *name)
{*/
	/*open stat of name*/
	/*get a stat and get information*/
	/*check if it is a directory or symlink or file*/
	/*gets all the information from file*/
	/*make a DIR and a dirent and a stat*/

	/*return header;
}*/


/*NOTE: this is an incomplete create. It currently will just write the
files to the tar and make blank space for the header. TODO, make a 
createHeader function that takes in a file and constructs a buffer
to write to the file. TODO, rename current getHeader function so it
is more descernable between getHeader and createHeader because it
is currently very confusing*/

/*TODO: make sure to include verbosity and strict. They currently 
aren't in affect right now*/
void makeTar(int argc, char *argv[], bool verbosity, bool strict)
{
	printf("MADE IT TO MAKETAR\n");
	int tarfd = open(argv[2], 
		O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	/*char buffer[BLOCK];*/
	int i;
	struct stat sb;
	int curBlock = 0;
	
	/*loop through each given file*/
	for (i = 3; i < argc; i++) {

		printf("...handling %s\n", argv[i]);

		/*get status of given file/directory*/
		if(stat(argv[i], &sb) == -1) {
			printf("stat broke\n");
			printf("name is: %s", argv[i]);
			break;
		}

		/*if file, print file to tar*/
		if (S_ISREG(sb.st_mode)){
			printf("isregfile... going to putfile\n");
			curBlock = putFile(tarfd, curBlock, &sb, argv[i]);
		}

		/*if directory, print write directory to tar file*/
		else if (S_ISDIR(sb.st_mode)) {
			printf("isDir... going to putdir\n");
			curBlock = putDir(tarfd, curBlock, argv[i]);
		}


	}

	/*Add two empty blocks for end of file*/
	/*NOTE:: I'm not sure this is how a file is terminated*/
	/*Pleas double check, it is currently late and I will
	address this tomorrow*/
	lseek(tarfd, BLOCK * curBlock, SEEK_SET);
	addBlock(tarfd);
	curBlock += 1;
	lseek(tarfd, BLOCK * curBlock, SEEK_SET);
	addBlock(tarfd);

	/*free header*/
	/*free(header);*/

}

/******************************end create**************************/

int main(int argc, char *argv[])
{
	int options;
	int execute = 0;
	bool verbosity = false;
	bool strict = false;
	int i;
	int len;

	char myDir[200];

	for (i = 0; i < 200; i++)
		myDir[i] = '\0';

	strcpy(myDir, "Elle/");


	/*printf("__________________________________________\n");
	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n_____________________________________________\n");*/

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
			fprintf(stderr, 
			"usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
			exit(EXIT_FAILURE);
		}
	}

	/*make sure that an execute was given*/
	if (execute == 0) {
		printf("b\n");
		fprintf(stderr,
		 "usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
		exit(EXIT_FAILURE);
	}

	/*execute code*/
	switch(execute) {
		case CREATE:
			makeTar(argc, argv, verbosity, strict);
			break;
		case DISPLAY:
			travTar(argc, argv, verbosity, strict, true);
			break;
		case EXTRACT:
			/*for (i = 0; i < argc; i++)
				printf("%s ", argv[i]);
			printf("\n");*/
			travTar(argc, argv, verbosity, strict, false);
			break;
	}


	return 0;
}