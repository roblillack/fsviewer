#ifndef FILES_H_
#define FILES_H_

typedef enum FileType {
        S_LINK,
        DIRECTORY,
        NORMAL,
	ROOT,
	HOME,
	NONE
} FileType;

typedef struct SLL {
	char  *data;
	struct SLL *next;
        enum FileType fileType;
} SLL;

typedef enum AppEvent {
        AppExec,
        AppView,
        AppEdit
} AppEvent;

typedef enum FileAction {
        FileCopy,
        FileDelete,
	FileMove,
	FileRename,
	FileLink,
	FileSync
} FileAction;

typedef struct FileInfo 
{
    /* path name */
    char *path;
    
    /* a file name */
    char *name;
    
    /* File Extension */
    char *extn;

    /* an abbreviated file name*/
    char *abbrev;
    
    char *imgName;
    
    /* ?? */
    /*         char *linkname; */

    /* ?? */
    /*         unsigned int linkmode; */
    
    /* ?? */
    /*         int linkok; */
    
    struct stat *st;
    /* file type */
    enum FileType fileType;

    struct FileInfo *next;
    
} FileInfo;

#define DIR_STR                "DIRECTORY"
#define ROOT_STR               "ROOT"
#define HOME_STR               "HOME"
#define DEFAULT_STR            "DEFAULT_IMG"
#define NULL_STR               "NULL"

char     *GetFileAbbrev(char *fileName);
char     *GetPathFromPathname(char* pathname);
char     *GetPathnameFromPathName(char* path, char *name);
char     *GetNameFromPathname(char* pathname);
char     *GetFileImgName(char *fileName, enum FileType fileType);
void      GetFileInfo(char* path, char *name, FileInfo* fileInfo);
FileInfo *GetDirList(char *path);
char     *GetFileExtn(char *filename);
Bool      isDirectory(enum FileType fileType);
char     *RemoveFileExtension(char* filename);
/* Bool      DisplayFile(const char *str); */
Bool      DisplayFile(char *str, char *filter, FileType fileType);
void      FSToggleDisplayHiddenFiles();
void      FSToggleSort();
FileInfo *FSCreateFileInfo();
FileInfo *FSGetFileInfo(char* pathname);
void      FSFreeFileInfo(FileInfo *fileInfo);
void      FSCopyFileInfo(FileInfo *src, FileInfo *dest);
int       FSCreateNewFile(char *path, mode_t mode);
int       FSCreateNewDirectory(char *path, mode_t mode);
int       FSRCopy(char *oldpath, char *newpath);
int       FSRDel(char *path);
void      FSCopy(FileInfo *src, FileInfo *dest);
void      FSDelete(FileInfo *item);
void      FSMove(FileInfo *src, FileInfo *dest);
void      FSRename(FileInfo *src, FileInfo *dest);
void      FSLink(FileInfo *src, FileInfo *dest);

#endif
