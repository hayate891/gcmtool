/*
 *  GCMutils.c
 *  gcmtool
 *
 *  Created by spike on Sun Mar 28 2004.
 *  Copyright (c) 2004 Sadistech. All rights reserved.
 *
 */

#include "GCMutils.h"
#include "GCMextras.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <arpa/inet.h>

//for directory enumeration...
#include <dirent.h>
#include <sys/types.h>

//some stuff for temp files... (string table and fstData...)
static FILE *fstTempFile;
static FILE *stringTableTempFile;
static int fst_fd;
static int stringTable_fd;

static void initRecursion();
static int recurseDirectory(char *path, char *buf);

//for string table stuff...
static int writeStringToTempFile(char *string);

void GCMGetDiskHeader(FILE *ifile, char *buf) {
	/* 
	**	sets buf to the diskheader (boot.bin)
	**  use functions in GCMExtras.h to manipulate data in a useful manner
	*/
	
	if (!ifile || !buf) return;
	
	fseek(ifile, GCM_DISK_HEADER_OFFSET, SEEK_SET);
	if (fread(buf, 1, GCM_DISK_HEADER_LENGTH, ifile) != GCM_DISK_HEADER_LENGTH) {
		free(buf);
		return;
	}
}

void GCMGetDiskHeaderInfo(FILE *ifile, char *buf) {
	/*
	**  sets buf to the diskheaderinfo from ifile (bi2.bin)
	**  use functions from GCMExtras.h to manipulate it in a useful manner
	*/
	
	if (!ifile || !buf) return;
	
	fseek(ifile, GCM_DISK_HEADER_INFO_OFFSET, SEEK_SET);
	if (fread(buf, 1, GCM_DISK_HEADER_INFO_LENGTH, ifile) != GCM_DISK_HEADER_INFO_LENGTH) {
		free(buf);
		return;
	}
}

void GCMGetApploader(FILE *ifile, char *buf) {
	/*
	**  sets buf to the apploader (appldr.bin)
	**  use functions from GCMExtras.h to manipulate it in a useful manner
	*/
	
	if (!ifile || !buf) return;
	
	//this could be wrong...
	//currently it's grabbing apploaderSize + 0x0020 starting at the apploader's start
	
	//first we have to get the apploader size.....
	u32 appSize = GCMGetApploaderSize(ifile) + GCM_APPLOADER_CODE_OFFSET;
	
	fseek(ifile, GCM_APPLOADER_OFFSET, SEEK_SET);
	if (fread(buf, 1, appSize, ifile) != appSize) {
		free(buf);
		return;
	}
}

void GCMGetFST(FILE *ifile, char *buf) {
	/*
	**  sets buf to the entire filesystem table (fst.bin)
	**  be sure to use GCMGetFSTSize() to get the size of the FST when you allocate buf.
	**  the FST contains every file offset and filename offset. no actual file data, though.
	**  this function isn't really useful. GCMGetNthFileEntry() is more useful.
	*/
	
	if (!ifile || !buf) return;
	
	//first we need to get the size of the FST...
	u32 fstSize = GCMGetFSTSize(ifile);
	fseek(ifile, GCMGetFSTOffset(ifile), SEEK_SET);
	if (fread(buf, 1, fstSize, ifile) != fstSize) {
		free(buf);
		return;
	}
}

int GCMPutDiskHeader(FILE *ofile, char *buf) {
	/*
	**  takes a diskheader (boot.bin) and replaces the one in ofile with
	**  the new one... ALWAYS WORK ON A BACKUP!
	**  
	**  Since disk headers are always the same size, we can just overwrite the old one...
	**  also, we can assume that buf is GCM_DISK_HEADER_LENGTH bytes long...
	**
	**  returns GCM_ERROR on error
	**  returns GCM_SUCCESS on success
	*/
	
	if (!ofile || !buf) return GCM_ERROR;
	
	fseek(ofile, GCM_DISK_HEADER_OFFSET, SEEK_SET);
	if (fwrite(buf, 1, GCM_DISK_HEADER_LENGTH, ofile) != GCM_DISK_HEADER_LENGTH) {
		return GCM_ERROR;
	}
	
	return GCM_SUCCESS;
}

int GCMPutDiskHeaderInfo(FILE *ofile, char *buf) {
	/*
	**  takes a diskheaderinfo (bi2.bin) and replaces the one in ofile with
	**  the new one... ALWAYS WORK ON A BACKUP!
	**
	**  Since DiskHeaderInfo is always the same size, we can just overwrite the old one...
	**
	**  returns GCM_ERROR on error
	**  returns GCM_SUCCESS on success
	*/
	
	if (!ofile || !buf) return GCM_ERROR;
	
	fseek(ofile, GCM_DISK_HEADER_INFO_OFFSET, SEEK_SET);
	if (fwrite(buf, 1, GCM_DISK_HEADER_INFO_LENGTH, ofile) != GCM_DISK_HEADER_INFO_LENGTH) {
		return GCM_ERROR;
	}
	
	return GCM_SUCCESS;
}

int GCMPutApploader(FILE *ofile, char *buf, u32 length) {
	/*
	**  takes an apploader (appldr.bin) and replaces the one in ofile with
	**  the new one... ALWAYS WORK ON A BACKUP!
	**  returns GCM_ERROR on error
	**  returns GCM_SUCCESS on success
	**  THIS IS NOT IMPLEMENTED YET (obviously). MAY TAKE A WHILE.
	*/
	
	// first, calculate the size difference...
	// the old apploader and new one might be different sizes, so you may have to shift
	// the rest of the file...
	
	long lengthDiff = length - GCMGetApploaderSize(ofile);
	
	
	
	return GCM_ERROR;
}

char GCMGetSystemID(FILE *ifile) {
	/*
	**  returns the system ID as stored in the diskheader
	**  see the system ID Codes section of GCMExtras.h for possible values
	**  returns 0 on error.
	*/
	
	if (!ifile) {
		return 0;
	}
	
	fseek(ifile, GCM_DISK_HEADER_OFFSET + GCM_SYSTEM_ID_OFFSET, SEEK_SET);
	
	return (char)fgetc(ifile);
}

void GCMGetGameID(FILE *ifile, char *buf) {
	/*
	**  sets buf to the 2 character gameID as stored in the diskheader
	**  returns 0 on error
	*/
	
	if (!ifile || !buf) {
		return;
	}
	
	fseek(ifile, GCM_DISK_HEADER_OFFSET + GCM_GAME_ID_OFFSET, SEEK_SET);
	
	if (fread(buf, 1, GCM_GAME_ID_LENGTH, ifile) != GCM_GAME_ID_LENGTH) {
		free(buf);
		return;
	}
}

char GCMGetRegionCode(FILE *ifile) {
	/*
	**  returns the region code as stored in the disk header.
	**  see GCMExtras.h for possible values
	**  Determines if the game is PAL or NTSC, Japanese or not.
	**  returns 0 on error
	*/
	
	if (!ifile) {
		return 0;
	}
	
	fseek(ifile, GCM_DISK_HEADER_OFFSET + GCM_REGION_CODE_OFFSET, SEEK_SET);
	
	return fgetc(ifile);
}

void GCMGetMakerCode(FILE *ifile, char *buf) {
	/*
	**  sets buf to the 2-character maker code of the game as stored in the diskheader
	**  see GCMExtras.h for possible values
	**  returns 0 on error
	**  returns GCM_MAKER_CODE_LENGTH on success.
	*/
	
	if (!ifile || !buf) {
		return;
	}
	
	fseek(ifile, GCM_DISK_HEADER_OFFSET + GCM_MAKER_CODE_OFFSET, SEEK_SET);
	
	//let's zero out buf just to be sure it's still null terminated. It should be GCM_MAKER_CODE_LENGTH + 1 bytes.
	bzero(buf, GCM_MAKER_CODE_LENGTH + 1);
	
	if (fread(buf, 1, GCM_MAKER_CODE_LENGTH, ifile) != GCM_MAKER_CODE_LENGTH) {
		free(buf);
		return;
	}
}

void GCMGetGameName(FILE *ifile, char *buf) {
	/*
	**  sets buf to the game name as stored in the diskheader
	**  reads all of the extra zeros, you may want to trim the excess off at the end, but that's up to you.
	*/
	
	if (!ifile || !buf) {
		return;
	}
	
	fseek(ifile, GCM_DISK_HEADER_OFFSET + GCM_GAME_NAME_OFFSET, SEEK_SET);
	
	if (fread(buf, 1, GCM_GAME_NAME_LENGTH, ifile) != GCM_GAME_NAME_LENGTH) {
		free(buf);
		return;
	}
}

u32 GCMGetDolOffset(FILE *ifile) {
	/*
	**  returns the offset of the main executable DOL (bootfile) file found in the diskheader
	**  returns 0 on error.
	*/
	
	if (!ifile) {
		return 0;
	}
	
	fseek(ifile, GCM_DOL_OFFSET_OFFSET, SEEK_SET);
	u32 *buf = (u32*)malloc(sizeof(u32));
	if (fread(buf, 1, GCM_DOL_OFFSET_LENGTH, ifile) != GCM_DOL_OFFSET_LENGTH) {
		free(buf);
		return 0;
	}
	u32 offset = *buf;
	offset = ntohl(offset); //flip bytes for endian...
	free(buf);
	
	return offset;
}

u32 GCMGetApploaderSize(FILE *ifile) {
	/*
	**  returns the apploader size (varies game to game) (found in the apploader)
	**  returns 0 on error
	*/
	
	if (!ifile) return 0;
	
	//returns the size of the code for the apploader...
	
	fseek(ifile, GCM_APPLOADER_OFFSET + GCM_APPLOADER_SIZE_OFFSET, SEEK_SET);
	u32 *buf = (u32*)malloc(sizeof(u32));
	if (fread(buf, 1, GCM_APPLOADER_SIZE_LENGTH, ifile) != GCM_APPLOADER_SIZE_LENGTH) {
		free(buf);
		return 0;
	}
	u32 appSize = ntohl(*buf);
	free(buf);
	
	return appSize;
}

u32 GCMGetFSTOffset(FILE *ifile) {
	/*
	**  returns the FST offset (found in the diskheader)
	**  returns 0 on error
	*/
	
	if (!ifile) {
		return 0;
	}
	
	fseek(ifile, GCM_DISK_HEADER_OFFSET + GCM_FST_OFFSET_OFFSET, SEEK_SET);
	u32 *buf = (u32*)malloc(sizeof(u32));
	if (fread(buf, 1, GCM_FST_OFFSET_LENGTH, ifile) != GCM_FST_OFFSET_LENGTH) {
		free(buf);
		return 0;
	}
	u32 offset = ntohl(*buf);
	free(buf);
	
	return offset;
}

u32 GCMGetFSTSize(FILE *ifile) {
	/*
	**  returns the size of the FST (found in the diskheader)
	**  returns 0 on error
	*/
	
	if (!ifile) {
		return 0;
	}
	
	fseek(ifile, GCM_DISK_HEADER_OFFSET + GCM_FST_SIZE_OFFSET, SEEK_SET);
	u32 *buf = (u32*)malloc(sizeof(u32));
	if (fread(buf, 1, GCM_FST_SIZE_LENGTH, ifile) != GCM_FST_SIZE_LENGTH) {
		free(buf);
		return 0;
	}
	u32 size = ntohl(*buf);
	free(buf);
	
	return size;
}

void GCMGetNthRawFileEntry(FILE *ifile, int n, char *buf) {
	/*
	**  sets buf to the nth raw file entry.
	**  use functions in GCMExtras.h to do something useful with it
	*/
	
	if (!ifile || n < 0 || !buf) {
		return;
	}
	
	fseek(ifile, GCMGetFSTOffset(ifile) + (GCM_FST_ENTRY_LENGTH * n), SEEK_SET);
	if (fread(buf, 1, GCM_FST_ENTRY_LENGTH, ifile) != GCM_FST_ENTRY_LENGTH) {
		free(buf);
		return;
	}
}

void GCMReplaceFilesystem(FILE *ifile, char *fsRootPath) {
	initRecursion(); //gotta set up some temp vars...

	int count =	recurseDirectory(fsRootPath, NULL);
	
	//allocated enough memory for all of the entries + 1 (for the root entry)
	char *fst = (char*)malloc((count + 1) * GCM_FST_ENTRY_LENGTH);
	char *fstStart = fst;
	
	GCMFileEntryStruct *root= (GCMFileEntryStruct*)malloc(sizeof(GCMFileEntryStruct));
	
	//initialize the entry...
	root->isDir = 1;
	root->filename = ""; //because it's root, it don't mattah!
	root->filenameOffset = 0; //no filename, damnit!
	root->offset = 0; //root has no parent offset...
	root->length = (u32)(count + 1); //file count + 1
	
	char *rawRoot = (char*)malloc(GCM_FST_ENTRY_LENGTH);
	GCMFileEntryStructToRaw(root, rawRoot); //convert the struct into something that can be written to a file
	memcpy(fst, rawRoot, GCM_FST_ENTRY_LENGTH);

	//we don't need these anymore...
	free(root);
	free(rawRoot);
	
	fst += GCM_FST_ENTRY_LENGTH; //move the thing so we can write after it when we recurse!
	
	recurseDirectory(fsRootPath, fst);
	
	fst = fstStart; //reset fst
	
	fclose(fstTempFile);
	fclose(stringTableTempFile);
}

static void initRecursion() {
	fst_fd = -1;
	stringTable_fd = -1;
	
	//init the temp file...
	char fstTempFilename[] = "fstData.tmp.XXXXXX";
	if ((fst_fd = mkstemp(fstTempFilename) == -1) || !(fstTempFile = fdopen(fst_fd, "w+"))) {
		perror("ERROR OPENING TEMP FILE");
		exit(1);
	}
	
	char stringTableTempFilename[] = "stringtable.tmp.XXXXXX";
	if ((stringTable_fd = mkstemp(stringTableTempFilename) == -1) || !(stringTableTempFile = fdopen(stringTable_fd, "w+"))) {
		perror("ERROR OPENING TEMP FILE!");
		exit(1);
	}
}

static int recurseDirectory(char *path, char *buf) {
	/*
	**  returns the count of entries in buf...
	*/
	
	if (!path) return 0;
	
	DIR *d = NULL;
	struct dirent *de = NULL;
	
	int count = 0;
	
	if (!(d = opendir(path))) {
		printf("error opening directory! Doesn't exist?\n");
		exit(1);
	}
	
	int i = 0;
	
	for (i = 0; (de = readdir(d)) != NULL; i++) {
		if (de->d_name[0] == '.') { //skip invisible files and . and ..
			i--;
			continue; 
		}
		
		GCMFileEntryStruct *e = (GCMFileEntryStruct*)malloc(sizeof(GCMFileEntryStruct));
		
		if (de->d_type == DT_DIR) {
			e->isDir = 1;
			e->filenameOffset = writeStringToTempFile(de->d_name);
			e->offset = 0; //FIX THIS!
			e->length = 0; //FIX THIS!
			
			printf("dir : %s\n", de->d_name);
		
			char newPath[255] = "";
			strcpy(newPath, path);
			strcat(newPath, "/");
			strcat(newPath, de->d_name);
			
			count = recurseDirectory(newPath, NULL);
		//	printf("%d (%s)\n", count, de->d_name);
			i += count;
		} else if (de->d_type == DT_REG) {
			e->isDir = 0;
			e->filenameOffset = writeStringToTempFile(de->d_name);
			e->offset = 0; //FIX THIS!
			e->length = 0; //FIX THIS!
			
			printf("file: %s\n", de->d_name);
		} else {
			printf("unknown filetype! (%d)\n", de->d_type);
			exit(1);
		}
	}
	
	closedir(d);
	
	return i;
}

static int writeStringToTempFile(char *string) {
	int len = strlen(string) + 1;
	
	printf("%d\t%s\n", len, string);

	if (fwrite(string, 1, len, stringTableTempFile) != len) {
		perror("Error writing to string table!\n");
		exit(1);
	}
	
	return (int)ftell(stringTableTempFile);
}
