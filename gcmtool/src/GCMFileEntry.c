#include "GCMFileEntry.h"
#include "GCMutils.h"
#include "GCMextras.h"
#include <stdlib.h>

GCMFileEntryStruct *GCMRawFileEntryToStruct(char *rawEntry) {
	/*
	**  converts the rawEntry into a struct.
	**  doesn't fetch the filename or data (to save memory/speed things up)
	**  use GCMFetchFilenameForFileEntry() to get the filename...
	**
	**  Format of a raw file entry:
	**  start__|__size__|__Description
	**   0x00     1		   flags; 0: file, 1: directory
	**   0x01     3		   filename_offset (relative to string table)
	**   0x04     4        file: file_offset, dir: parent_offset
	**   0x08     4        file: file_length, dir: next_offset
	*/

	if (!rawEntry) {
		return NULL;
	}
	
	GCMFileEntryStruct *fe = (GCMFileEntryStruct*)malloc(sizeof(GCMFileEntryStruct));
	
	//it's a file if the first byte is 1, directory if 0
	fe->isDir = *rawEntry;
	
	//then get the filename_offset
	unsigned long *l;
	l = (unsigned long*)rawEntry;
	if (rawEntry[0] != 0)
		*l -= 0x01000000;
	fe->filenameOffset = ntohl(*l);
	
	//get the file_offset/parent_offset
	l = (unsigned long*)rawEntry + 1;
	fe->offset = ntohl(*l);
	
	//get the file_length/next_offset
	l = (unsigned long*)rawEntry + 2;
	fe->length = ntohl(*l);
	
	return fe;
}

void GCMFetchFilenameForFileEntry(FILE *ifile, GCMFileEntryStruct *entry) {
	/*
	**  inspects entry, looks up the filename and
	**  sets entry->filename to the filename for that entry as found in ifile...
	*/

	if (!ifile || !entry) {
		return;
	}
	
	fseek(ifile, GCMGetStringTableOffset(ifile) + entry->filenameOffset, SEEK_SET);
	char *buf = (char*)malloc(MAXFILENAMESIZE);
	if (fread(buf, 1, MAXFILENAMESIZE, ifile) != MAXFILENAMESIZE) {
		free(buf);
		return;
	}
	
	//to save memory, just allocate enough memory for the filename + the \0
	entry->filename = (char*)malloc(strlen(buf) + 1);
	strcpy(entry->filename, buf);
	free(buf);
}

void GCMFetchDataForFileEntry(FILE *ifile, GCMFileEntryStruct *entry) {
	/*
	**  looks up the offset for the file data by inspecting entry
	**  then sets entry->data to the file's data.
	**  look at entry->length to see what the filesize is (in bytes)
	*/

	if (!ifile || !entry || entry->isDir) {
		return;
	}
	
	fseek(ifile, entry->offset, SEEK_SET);
	char *buf = (char*)malloc(entry->length);
	if (fread(buf, 1, entry->length, ifile) != entry->length) {
		free(buf);
		return;
	}
	
	entry->data = buf;
}

void GCMGetFullPathForFileEntry(FILE *ifile, GCMFileEntryStruct *entry, char *buf) {
	/*
	**  set buf to the full path of file entry
	**  since each directory entry has the fileEntryOffset of its parent (stored in offset)
	**  you can just recursively jump up the tree until you get to an entry whose parent_offset is 0.
	**
	**  The only gotcha is that if you want to get the full path of an actual file, you have to 
	**  remember what its parent is... ug...
	**
	**  buf should be allocaed with enough space to store the full path. 512 should be more than enough...
	*/
	
	if (!ifile || !entry || !entry->isDir || !buf) return;
	
	char fullPath[1024] = ""; //allocate 1024, just to be safe...
	
	GCMFetchFilenameForFileEntry(ifile, entry);
	
	strcpy(fullPath, entry->filename);
	
	GCMFileEntryStruct *e = entry; // = GCMGetNthFileEntry(ifile, entry->offset);
	
	while (e->offset) {
		e = GCMGetNthFileEntry(ifile, e->offset);
		GCMFetchFilenameForFileEntry(ifile, e);
		
		char s[1024] = "";
		strcpy(s, fullPath);
		sprintf(fullPath, "%s/%s", e->filename, s);
	}
	
	strcpy(buf, "/"); //leading / on absolute path...
	strcat(buf, fullPath); //append the fullPath...
}

void GCMFreeFileEntryStruct(GCMFileEntryStruct *fe) {
	/*
	**  convenience method for freeing a fileEntryStruct
	**  not really implemented too well...
	*/

	if (!fe) return;
	
//	if (fe->data != NULL)
//		free(fe->data);
//	if (fe->filename != NULL)
//		free(fe->filename);
		
	free(fe);
}
