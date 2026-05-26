#include <WINGs/WINGs.h>
#include <WINGs/WUtil.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "FSUtils.h"
#include "FSViewer.h"
#include "misc.h"

static Bool DISPLAY_HIDDEN_FILES = False;
static Bool SORT_TOGGLE = True;

static int copy(ino_t* inodes, int n_inodes, char* oldpath, char* newpath);
static int copyfile(char* oldpath, char* newpath);
static int copydir(ino_t* inodes, int n_inodes, struct stat* oldstats,
    char* oldpath, char* newpath);

/*
** Here's an example of how to sort a singly-linked list.  I think it
** can be modified to sort a doubly-linked list, but it would get a bit
** more complicated.  Note that this is a recursive method, but the
** recursion depth is limited to be proportional to the base 2 log of
** the length of the list, so it won't "run away" and blow the stack.
**
**  10/21/93 rdg  Fixed bug -- function was okay, but called incorrectly.
** linked list sort -- public domain by Ray Gardner  5/90
*/

/*
   Taken from the code snippet website (sorry no URL)
   Functions modified on 98/10/14 by George Clernon (clernong@tinet.ie)
   for Workplace.app list type.
*/

/* returns < 0 if *p sorts lower than *q */
int keycmp(FileInfo* p, FileInfo* q)
{
    if (FSGetIntegerForName("SortOrder"))
        return strcasecmp(p->name, q->name);
    else
        return strcasecmp(q->name, p->name);
}

/* merge 2 lists under dummy head item */
FileInfo* lmerge(FileInfo* p, FileInfo* q)
{
    FileInfo *r, head;

    for (r = &head; p && q;) {
        if (keycmp(p, q) < 0) {
            r = r->next = p;
            p = p->next;
        } else {
            r = r->next = q;
            q = q->next;
        }
    }
    r->next = (p ? p : q);

    return head.next;
}

/* split list into 2 parts, sort each recursively, merge */
FileInfo* lsort(FileInfo* p)
{
    FileInfo *q, *r;

    if (p) {
        q = p;
        for (r = q->next; r && (r = r->next) != NULL; r = r->next)
            q = q->next;

        r = q->next;
        q->next = NULL;

        if (r)
            p = lmerge(lsort(r), lsort(p));
    }
    return p;
}

/* FileInfo *GetDirList(char *path) */
/* { */
/*     char           *fullPath; */
/*     FileInfo       *top; */
/*     FileInfo       *fileList; */
/*     struct dirent  *dirent; */
/*     DIR            *dir; */

/*     fullPath = wexpandpath(path); */

/*     dir = opendir(fullPath); */
/*     if (!dir) */
/*     { */
/* 	printf("Unable to open dir: %s\n", path); */
/* 	return NULL; */
/*     } */

/*     fileList = (FileInfo *) wmalloc(sizeof(FileInfo)); */
/*     fileList->next = NULL; */
/*     fileList->name = NULL; */
/*     fileList->fileType = NORMAL; */
/*     fileList->imgName = NULL; */
/*     fileList->abbrev = NULL; */
/*     top = fileList; */

/*     dirent = readdir(dir); */
/*     while(dirent)  */
/*     { */
/* 	GetFileInfo(fullPath, dirent->d_name, fileList); */

/* 	dirent = readdir(dir); */

/* 	if(dirent) */
/* 	{ */
/* 	    fileList->next = (FileInfo *) wmalloc(sizeof(FileInfo)); */
/* 	    fileList = fileList->next; */
/* 	    fileList->next = NULL; */
/* 	} */
/*     } */

/*     closedir(dir); */

/*     if(fullPath) */
/* 	free(fullPath); */

/*     return lsort(top); */
/* } */

FileInfo* GetDirList(char* path)
{
    Bool mixed = False;
    Bool dirFirst = False;
    char* fullPath = NULL;
    FileInfo* topDir = NULL;
    FileInfo* topFile = NULL;
    FileInfo* dirList = NULL;
    FileInfo* fileList = NULL;
    FileInfo* fileInfo = NULL;
    struct dirent* dirent = NULL;
    DIR* dir = NULL;

    fullPath = wexpandpath(path);

    dir = opendir(fullPath);
    if (!dir) {
        /* 	printf("Unable to open dir: %s\n", path); */
        return NULL;
    }

    if (FSGetIntegerForName("SortDisplay") == 0)
        mixed = True;
    else if (FSGetIntegerForName("SortDisplay") == 2)
        dirFirst = True;

    while ((dirent = readdir(dir)) != NULL) {
        fileInfo = FSCreateFileInfo();

        GetFileInfo(fullPath, dirent->d_name, fileInfo);

        if (!mixed && isDirectory(fileInfo->fileType)) {
            if (dirList) {
                dirList->next = fileInfo;
                dirList = dirList->next;
            } else {
                dirList = fileInfo;
                topDir = dirList;
            }
        } else {
            if (fileList) {
                fileList->next = fileInfo;
                fileList = fileList->next;
            } else {
                fileList = fileInfo;
                topFile = fileList;
            }
        }
    }

    closedir(dir);

    if (fullPath)
        free(fullPath);

    if (topFile)
        fileList = lsort(topFile);
    if (topDir)
        dirList = lsort(topDir);

    if (dirFirst) {
        if (dirList) {
            topFile = dirList;
            while (dirList->next != NULL)
                dirList = dirList->next;
            dirList->next = fileList;
        } else
            topFile = fileList;
    } else {
        if (fileList) {
            topFile = fileList;
            while (fileList->next != NULL)
                fileList = fileList->next;
            fileList->next = dirList;
        } else
            topFile = dirList;
    }

    return topFile;
}

void GetFileInfo(char* path, char* name, FileInfo* fileInfo)
{
    struct stat* st;
    char* pathname;
    char* fileName;
    char* dirPath;

    st = (struct stat*)wmalloc(sizeof(struct stat));

    /* make abosolute pathname */
    size_t pathname_size = strlen(path) + strlen(name) + 1;
    pathname = (char*)wmalloc(pathname_size);
    strlcpy(pathname, path, pathname_size);
    strlcat(pathname, name, pathname_size);

    /* get  information of a file represented by pathname */
    if (stat(pathname, st) == -1)
        wwarning(_("%s GetFileInfo %d: Stat Error for %s\n"),
            __FILE__, __LINE__, pathname);

    if (strcmp("/", pathname) == 0) {
        fileInfo->fileType = ROOT;
        fileName = FSNodeName();
    } else if (strcmp(FSGetHomeDir(), pathname) == 0) {
        fileInfo->fileType = HOME;
        size_t fileName_size = strlen(name) + 1;
        fileName = (char*)wmalloc(fileName_size);
        strlcpy(fileName, name, fileName_size);
    } else {
        if (S_ISDIR(st->st_mode))
            fileInfo->fileType = DIRECTORY;
        else
            fileInfo->fileType = NORMAL;

        size_t fileName_size = strlen(name) + 1;
        fileName = (char*)wmalloc(fileName_size);
        strlcpy(fileName, name, fileName_size);
    }

    size_t dirPath_size = strlen(path) + 1;
    dirPath = (char*)wmalloc(dirPath_size);
    strlcpy(dirPath, path, dirPath_size);

    fileInfo->name = fileName;
    fileInfo->path = dirPath;

    if (fileInfo->fileType == ROOT)
        fileInfo->extn = wstrdup("ROOT");
    else
        fileInfo->extn = wstrdup(GetFileExtn(fileInfo->name));

    fileInfo->imgName = GetFileImgName(GetFileExtn(fileName),
        fileInfo->fileType);
    fileInfo->abbrev = GetFileAbbrev(fileName);

    fileInfo->st = st;

    /*     if (st) */
    /*     { */
    /* 	free(st); */
    /* 	st = NULL; */
    /*     } */

    if (pathname) {
        free(pathname);
        pathname = NULL;
    }
}

char* RemoveFileExtension(char* filename)
{
    char* tmp;
    char* str;
    int len;

    tmp = strrchr(filename, '.');

    len = strlen(filename) - strlen(tmp);

    str = (char*)wmalloc(len + 1);
    strncpy(str, filename, len);
    str[len] = '\0';

    return str;
}

char* GetFileExtn(char* filename)
{
    char* extn;

    if (FSStringMatch("*.tar.gz", filename))
        return wstrdup(".tar.gz");

    extn = strrchr(filename, '.');

    if (extn == NULL) {
        /* 	extn = (char *) wmalloc(strlen(filename)+1); */
        /* 	strcpy(extn, filename); */
        extn = wstrdup(filename);
    }

    return extn;
}

char* GetFileExtnOrNull(char* filename)
{
    if (FSStringMatch("*.tar.gz", filename)) {
        return ".tar.gz";
    }

    char* ext = strrchr(filename, '.');
    if (ext == NULL) {
        return NULL;
    }

    for (int i = 0; i < strlen(ext); i++) {
        if ((uint)ext[i] > 0x7f) {
            // We don't allow non-ASCII characters in file extensions
            // see: https://github.com/roblillack/fsviewer/issues/20
            return NULL;
        }
    }

    return ext;
}

char* GetFileAbbrev(char* fileName)
{
    char* abbrev;
    /*     int i; */

    size_t abbrev_size = strlen(fileName) + 1;
    abbrev = (char*)wmalloc(abbrev_size);

    if (strlen(fileName) > 19) {
        strncpy(abbrev, fileName, 16);
        abbrev[16] = '\0';
        strlcat(abbrev, "...", abbrev_size);

    } else {
        strlcpy(abbrev, fileName, abbrev_size);
    }

    return abbrev;
}

char* GetFileImgName(char* fileName, enum FileType fileType)
{
    /*WMPropList* dict, value = NULL;*/
    char* extn = NULL;
    char* icon = NULL;
    char* name = NULL;

    /*
     * I'm not sure what should happen here
     * Cos the event should really arise.
     * Need to improve error handling in functions
     * that call this one
     */
    if (fileName == NULL)
        return wstrdup(DEFAULT_STR);

    if (fileType == ROOT) {
        extn = "ROOT";
    } else {
        extn = GetFileExtnOrNull(fileName);
    }

    if (extn) {
        name = FSGetStringForNameKey(extn, "icon");
    }

    if (name == NULL) {
        if (fileType == DIRECTORY)
            name = FSGetStringForName(DIR_STR);
        else if (fileType == ROOT)
            name = FSGetStringForNameKey(ROOT_STR, "icon");
        else if (fileType == HOME)
            name = FSGetStringForName(HOME_STR);
        else
            name = FSGetStringForName(DEFAULT_STR);
    }

    icon = LocateImage(name);
    if (name)
        free(name);

    if (icon)
        return icon;
    else
        return wstrdup(DEFAULT_STR);

    /*     dict = PLGetDictionaryEntry(filesDB, PLMakeString(extn)); */
    /*     if(dict && PLIsDictionary(dict)) */
    /*     { */
    /* 	value = PLGetDictionaryEntry(dict, PLMakeString("icon")); */
    /*     } */

    /*     if(value == NULL) */
    /*     { */
    /* 	if(fileType == DIRECTORY) */
    /* 	    value = PLGetDictionaryEntry(filesDB, PLMakeString(DIR_STR)); */
    /* 	else if(fileType == ROOT) */
    /* 	    value = PLGetDictionaryEntry(filesDB, PLMakeString(ROOT_STR)); */
    /* 	else if(fileType == HOME) */
    /* 	    value = PLGetDictionaryEntry(filesDB, PLMakeString(HOME_STR)); */
    /* 	else */
    /* 	{ */
    /* 	    value = PLGetDictionaryEntry(filesDB, PLMakeString(DEFAULT_STR)); */
    /* 	} */
    /*     }	     */

    /*     if(extn) */
    /* 	free(extn); */

    /* No Icons at all!! */
    /*     if(value == NULL) */
    /*     { */
    /* 	return DEFAULT_STR; */
    /*     } */

    /*     return PLGetString(value); */
}

char* GetNameFromPathname(char* pathname)
{
    char* str;

    str = strrchr(pathname, '/');
    str++;

    return wstrdup(str);
}

char* GetPathFromPathname(char* pathname)
{
    int lenTot;
    int lenName;
    int lenPath;
    char* path;

    lenTot = strlen(pathname);
    lenName = strlen(GetNameFromPathname(pathname));
    lenPath = lenTot - lenName;

    path = (char*)wmalloc(lenPath + 1);
    strncpy(path, pathname, lenPath);
    path[lenPath] = '\0';

    return path;
}

char* GetPathnameFromPathName(char* path, char* name)
{
    int len;
    char* pathname;

    len = strlen(GetNameFromPathname(path));
    if (len) {
        size_t pathname_size = strlen(path) + strlen(name) + 2;
        pathname = (char*)wmalloc(pathname_size);
        strlcpy(pathname, path, pathname_size);
        strlcat(pathname, "/", pathname_size);
        strlcat(pathname, name, pathname_size);
    } else {
        size_t pathname_size = strlen(path) + strlen(name) + 1;
        pathname = (char*)wmalloc(pathname_size);
        strlcpy(pathname, path, pathname_size);
        strlcat(pathname, name, pathname_size);
    }

    return pathname;
}

Bool isDirectory(enum FileType fileType)
{

    if (fileType == DIRECTORY || fileType == ROOT || fileType == HOME)
        return True;

    return False;
}

Bool DisplayFile(char* str, char* filter, FileType fileType)
{

    /*     if(str[0] != '.') */
    /*     { */
    /* 	return True; */
    /*     } */
    /*     else if(DISPLAY_HIDDEN_FILES) */
    /*     { */
    /* 	if(str[1] != '.' && str[1] != '\0') */
    /* 	    return True; */
    /*     } */

    /*     return False; */

    if (!strcmp(".", str) || !strcmp("..", str))
        return False;
    else if (filter && !isDirectory(fileType))
        if (!FSStringMatch(filter, str))
            return False;

    if (str[0] == '.')
        return FSGetIntegerForName("DisplayDotFiles"); /* DISPLAY_HIDDEN_FILES; */

    return True;
}

void FSToggleDisplayHiddenFiles()
{
    /*     DISPLAY_HIDDEN_FILES = !DISPLAY_HIDDEN_FILES; */
    /*    if( FSGetIntegerForName("DisplayDotFiles") )
            FSSetIntegerForName("DisplayDotFiles", 0);
        else
            FSSetIntegerForName("DisplayDotFiles", 1);*/
    FSSetIntegerForName("DisplayDotFiles",
        !FSGetIntegerForName("DisplayDotFiles"));
}

void FSToggleSort()
{
    /*     SORT_TOGGLE = !SORT_TOGGLE; */
    /*    if( FSGetIntegerForName("SortOrder") )
            FSSetIntegerForName("SortOrder", 0);
        else
            FSSetIntegerForName("SortOrder", 1);*/
    FSSetIntegerForName("SortOrder",
        !FSGetIntegerForName("SortOrder"));
}

FileInfo*
FSCreateFileInfo()
{

    FileInfo* fileInfo;

    fileInfo = (FileInfo*)wmalloc(sizeof(FileInfo));
    memset(fileInfo, 0, sizeof(FileInfo));
    /*     fileInfo->next = NULL; */
    /*     fileInfo->name = NULL; */
    /*     fileInfo->path = NULL; */
    fileInfo->fileType = NORMAL;
    fileInfo->imgName = GetFileImgName(NULL_STR, NORMAL);
    /*     fileInfo->abbrev = NULL; */
    /*     fileInfo->extn   = NULL; */
    /*     fileInfo->st     = NULL; */

    return fileInfo;
}

void FSCopyFileInfo(FileInfo* src, FileInfo* dest)
{

    if (src == NULL || dest == NULL)
        return;

    if (src->name) {
        size_t name_size = strlen(src->name) + 1;
        dest->name = (char*)wrealloc(dest->name, name_size);
        strlcpy(dest->name, src->name, name_size);
    } else
        dest->name = NULL;

    if (src->path) {
        size_t path_size = strlen(src->path) + 1;
        dest->path = (char*)wrealloc(dest->path, path_size);
        strlcpy(dest->path, src->path, path_size);
    } else
        dest->path = NULL;

    if (src->extn) {
        size_t extn_size = strlen(src->extn) + 1;
        dest->extn = (char*)wrealloc(dest->extn, extn_size);
        strlcpy(dest->extn, src->extn, extn_size);
    } else
        dest->extn = NULL;

    if (src->abbrev) {
        size_t abbrev_size = strlen(src->abbrev) + 1;
        dest->abbrev = (char*)wrealloc(dest->abbrev, abbrev_size);
        strlcpy(dest->abbrev, src->abbrev, abbrev_size);
    } else
        dest->abbrev = NULL;

    if (src->imgName) {
        size_t imgName_size = strlen(src->imgName) + 1;
        dest->imgName = (char*)wrealloc(dest->imgName, imgName_size);
        strlcpy(dest->imgName, src->imgName, imgName_size);
    } else
        dest->imgName = NULL;

    dest->fileType = src->fileType;

    /*
     * Not sure if I really want to do this!!!
     * It should be done though, I think I'll call back later
     */
    /*     if(src->next) */
    /*     { */
    /* 	dest->next = FSCreateFileInfo(); */
    /* 	FSCopyFileInfo(src->next, dest->next); */
    /*     } */
}

void FSFreeFileInfo(FileInfo* fileInfo)
{
    if (!fileInfo)
        return;

    if (fileInfo->name) {
        free(fileInfo->name);
        fileInfo->name = NULL;
    }
    if (fileInfo->path) {
        free(fileInfo->path);
        fileInfo->path = NULL;
    }
    if (fileInfo->imgName) {
        free(fileInfo->imgName);
        fileInfo->imgName = NULL;
    }
    if (fileInfo->extn) {
        free(fileInfo->extn);
        fileInfo->extn = NULL;
    }
    if (fileInfo->abbrev) {
        free(fileInfo->abbrev);
        fileInfo->abbrev = NULL;
    }
    if (fileInfo->st) {
        free(fileInfo->st);
        fileInfo->st = NULL;
    }

    if (fileInfo->next)
        FSFreeFileInfo(fileInfo->next);

    free(fileInfo);
    fileInfo = NULL;
}

/* check whether a file exists */
int FSFileExists(char* path)
{
    struct stat stats;

    return (!lstat(path, &stats));
}

FileInfo*
FSGetFileInfo(char* pathname)
{
    char* path;
    char* name;
    FileInfo* fileInfo = FSCreateFileInfo();

    name = GetNameFromPathname(pathname);
    path = GetPathFromPathname(pathname);

    GetFileInfo(path, name, fileInfo);

    return fileInfo;
}

/* create a new directory */
int FSCreateNewFile(char* path, mode_t mode)
{
    int file = open(path, O_WRONLY | O_CREAT | O_EXCL, mode);

    if (file == -1 || close(file))
        return -1;
    else
        return 0;
}

/* create a new file */
int FSCreateNewDirectory(char* path, mode_t mode)
{
    return mkdir(path, mode);

    /*     if (mkdir(path, mode))  */
    /*     { */
    /* 	char s[0xff]; */

    /* 	sprintf(s, "Error creating folder %s:", path); */
    /* 	FSErrorDialog("File Operation Error", s); */
    /*     } */
}

/*-------------------------------------------------------------------------*/

/* recursive copy operation */

int FSRCopy(char* oldpath, char* newpath)
{
    return copy((ino_t*)NULL, 0, oldpath, newpath);
}

static int
copyfile(char* oldpath, char* newpath)
{
    struct stat stats;
    int src = -1, dest = -1, n, errno_ret;
    char buf[BUFSIZ];

    if ((src = open(oldpath, O_RDONLY)) == -1 || stat(oldpath, &stats))
        goto err;
    else if ((dest = creat(newpath, stats.st_mode)) == -1)
        goto err;

    while ((n = read(src, buf, BUFSIZ)) != 0)
        if (n == -1 || write(dest, buf, n) != n)
            goto err;

    if (close(src)) {
        src = -1;
        goto err;
    } else
        return close(dest);

err:
    errno_ret = errno;
    if (src != -1)
        close(src);
    if (dest != -1)
        close(dest);
    errno = errno_ret;
    return -1;
}

static int
copydir(ino_t* inodes, int n_inodes, struct stat* oldstats,
    char* oldpath, char* newpath)
{
    DIR* dir;
    struct dirent* entry;
    int i, ol = strlen(oldpath), nl = strlen(newpath);
    struct stat newstats;
    mode_t umask = FSGetUMask();

    for (i = n_inodes - 1; i >= 0; i--)
        if (inodes[i] == oldstats->st_ino) {
            errno = EINVAL;
            return -1;
        }

    if ((mkdir(newpath, umask & 0777) < 0 && errno != EEXIST) || lstat(newpath, &newstats) || !(dir = opendir(oldpath)))
        return -1;

    inodes = (ino_t*)wrealloc(inodes, (n_inodes + 1) * sizeof(ino_t));
    inodes[n_inodes++] = newstats.st_ino;

    for (i = 0; (entry = readdir(dir)); i++)
        if (entry->d_name[0] != '.' || (entry->d_name[1] != '\0' && (entry->d_name[1] != '.' || entry->d_name[2] != '\0'))) {
            int ol1 = ol, nl1 = nl, l = strlen(entry->d_name);
            size_t oldpath1_size = ol1 + l + 2;
            size_t newpath1_size = nl1 + l + 2;
            char* oldpath1 = (char*)alloca(oldpath1_size);
            char* newpath1 = (char*)alloca(newpath1_size);

            strlcpy(oldpath1, oldpath, oldpath1_size);
            strlcpy(newpath1, newpath, newpath1_size);
            if (oldpath1[ol1 - 1] != '/')
                oldpath1[ol1++] = '/';
            if (newpath1[nl1 - 1] != '/')
                newpath1[nl1++] = '/';
            strlcpy(oldpath1 + ol1, entry->d_name, oldpath1_size - ol1);
            strlcpy(newpath1 + nl1, entry->d_name, newpath1_size - nl1);
            if (copy(inodes, n_inodes, oldpath1, newpath1)) {
                /* take care of recursive errors */
                char s[0xff];
                snprintf(s, sizeof(s), _("Error copying %s:"), oldpath1);
                FSErrorDialog(_("File Operation Error"), s);
            }
        }

    inodes = (ino_t*)wrealloc(inodes, (n_inodes - 1) * sizeof(ino_t));
    return closedir(dir);
}

static int
copy(ino_t* inodes, int n_inodes, char* oldpath, char* newpath)
{
    struct stat stats;
    mode_t umask = FSGetUMask();

    if (lstat(oldpath, &stats))
        return -1;

    /* Directory: copy recursively */
    if (S_ISDIR(stats.st_mode))
        return copydir(inodes, n_inodes, &stats, oldpath, newpath);

    /* Regular file: copy block by block */
    else if (S_ISREG(stats.st_mode))
        return copyfile(oldpath, newpath);

    /* Fifo: make a new one */
    else if (S_ISFIFO(stats.st_mode))
        return mkfifo(newpath, umask & 0666);

    /* Device: make a new one */
    else if (S_ISBLK(stats.st_mode) || S_ISCHR(stats.st_mode) || S_ISSOCK(stats.st_mode))
        return mknod(newpath, umask & 0666, stats.st_rdev);

    /* Symbolic link: make a new one */
    else if (S_ISLNK(stats.st_mode)) {
        char lnk[MAX_LEN + 1];
        int l = readlink(oldpath, lnk, MAX_LEN);

        if (l < 0)
            return -1;
        lnk[l] = '\0';
        return (symlink(lnk, newpath));
    }

    /* This shouldn't happen */
    else {
        char s[0xff];
        snprintf(s, sizeof(s), _("Unrecognized File type: %s"), oldpath);
        FSErrorDialog(_("File Operation Error"), s);

        return 0;
    }
}

/*-------------------------------------------------------------------------*/

/* recursive delete */

int FSRDel(char* path)
{
    struct stat stats;

    if (lstat(path, &stats))
        return -1;

    if (S_ISDIR(stats.st_mode)) {
        DIR* dir;
        struct dirent* entry;
        int i, pl = strlen(path);

        if (!(dir = opendir(path)))
            return -1;

        for (i = 0; (entry = readdir(dir)); i++)
            if (entry->d_name[0] != '.' || (entry->d_name[1] != '\0' && (entry->d_name[1] != '.' || entry->d_name[2] != '\0'))) {
                int pl1 = pl, l = strlen(entry->d_name);
                size_t path1_size = pl1 + l + 2;
                char* path1 = (char*)alloca(path1_size);

                strlcpy(path1, path, path1_size);
                if (path1[pl1 - 1] != '/')
                    path1[pl1++] = '/';
                strlcpy(path1 + pl1, entry->d_name, path1_size - pl1);
                if (FSRDel(path1)) {
                    /* take care of recursive errors */
                    char s[0xff];
                    snprintf(s, sizeof(s), _("Error deleting %s:"), path);
                    FSErrorDialog(_("File Operation Error"), s);
                }
            }

        if (closedir(dir))
            return -1;
        else
            return rmdir(path);
    } else
        return unlink(path);
}

void FSCopy(FileInfo* src, FileInfo* dest)
{
    struct stat stats;
    int i, toi, n_copied = 0;
    char* from = NULL;
    char to[MAX_LEN];

    from = GetPathnameFromPathName(src->path, src->name);
    strlcpy(to, GetPathnameFromPathName(dest->path, dest->name), sizeof(to));

    if (access(to, W_OK) && isDirectory(dest->fileType)) {
        char s[0xff];

        snprintf(s, sizeof(s), _("No write access to %s"), to);
        FSErrorDialog(_("File Operation Error"), s);
        if (from)
            free(from);
        return;
    }

    /*
       if target exists and is a directory, copy the source
       into that directory
    */
    if (!stat(to, &stats) && S_ISDIR(stats.st_mode)) {
        if (!strcmp(from, to)) {
            FSErrorDialog(_("File Operation Error"),
                _("Copy: Source and destination are identical"));
            if (from)
                free(from);
            return;
        }

        toi = strlen(to);
        if (to[toi - 1] != '/') {
            to[toi++] = '/';
            to[toi] = '\0';
        }

        /*
          Should do a loop here that goes through each element
          of the FileInfo list, checks and copies it
        */
        strlcpy(to + toi, src->name, sizeof(to) - toi);
        if (FSFileExists(to) /* && resources.confirm_overwrite */) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Copy: file %s already exists at destination"),
                src->name);
            if (FSConfirmationDialog(_("Overwrite?"), s)) {
                if (from)
                    free(from);
                return;
            }
        }

        if (FSRCopy(from, to)) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Error copying %s:"), from);
            FSErrorDialog(_("File Operation Error"), s);
        } else
            n_copied++;

    }
    /*
       otherwise only a single file may be selected;
       copy it to the target file
    */
    /*     else if (popups.fw->n_selections > 1)  */
    /*     { */
    /* 	error("Copy: target for multiple files", "must be a folder"); */
    /* 	goto out; */
    /*     }  */
    else {
        struct stat stats1;

        if (!stat(from, &stats1) && S_ISDIR(stats1.st_mode)) {
            FSErrorDialog(_("File Operation Error"),
                _("Cannot copy a directory to a file"));
            if (from)
                free(from);
            return;
        }

        if (!lstat(to, &stats) && !lstat(from, &stats1) && stats.st_ino == stats1.st_ino) {
            FSErrorDialog(_("File Operation Error"),
                _("Copy: Source and destination are identical"));
            if (from)
                free(from);
            return;
        }

        if (FSFileExists(to) /* && resources.confirm_overwrite */) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Copy: file %s already exists"), to);
            if (FSConfirmationDialog(_("Overwrite?"), s)) {
                if (from)
                    free(from);
                return;
            }
        }

        if (FSRCopy(from, to)) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Error copying %s:"), from);
            FSErrorDialog(_("File Operation Error"), s);
        } else
            n_copied = 1;
    }
    if (n_copied)
        FSUpdateFileView(FileCopy, src, dest);

    if (from)
        free(from);
}

void FSDelete(FileInfo* item)
{
    char s[0xff];
    char* itemName;
    int i, n_deleted = 0;

    itemName = GetPathnameFromPathName(item->path, item->name);

    if (isDirectory(item->fileType))
        if (!strcmp(item->name, ".") || !strcmp(item->name, "..")) {
            FSErrorDialog(_("File Operation Error"), _("Cannot delete . or .."));
            if (itemName)
                free(itemName);
            return;
        }
    /*       else if (resources.confirm_delete_folder) */
    /*       { */

    /*     sprintf(s, "Do you REALLY wish to delete folder %s"\ */
    /* 	    "and ALL items contained in it?", itemName); */

    snprintf(s, sizeof(s), _("Do you REALLY wish to delete %s"), itemName);

    if (FSConfirmationDialog(_("File Operation"), s)) {
        if (itemName)
            free(itemName);
        return;
    }
    /*       } */

    if (FSRDel(itemName)) {
        snprintf(s, sizeof(s), _("Error deleting %s:"), itemName);
        FSErrorDialog(_("File Operation Error"), s);
    } else
        n_deleted++;

    if (n_deleted)
        FSUpdateFileView(FileDelete, item, NULL);

    if (itemName)
        free(itemName);
}

void FSMove(FileInfo* src, FileInfo* dest)
{
    struct stat stats;
    int i, toi, n_moved = 0;
    char* from = NULL;
    char to[MAX_LEN];

    from = GetPathnameFromPathName(src->path, src->name);
    strlcpy(to, GetPathnameFromPathName(dest->path, dest->name), sizeof(to));

    if (access(to, W_OK) && isDirectory(dest->fileType)) {
        char s[0xff];

        snprintf(s, sizeof(s), _("No write access to %s"), to);
        FSErrorDialog(_("File Operation Error"), s);
        if (from)
            free(from);
        return;
    }

    /*
       if target exists and is a directory, move the source
       into that directory
    */
    if (!stat(to, &stats) && S_ISDIR(stats.st_mode)) {
        if (!strcmp(from, to)) {
            FSErrorDialog(_("File Operation Error"),
                _("Move: Source and destination are identical"));
            if (from)
                free(from);
            return;
        }

        toi = strlen(to);
        if (to[toi - 1] != '/') {
            to[toi++] = '/';
            to[toi] = '\0';
        }

        /*
          Should do a loop here that goes through each element
          of the FileInfo list, checks and moves it
        */
        strlcpy(to + toi, src->name, sizeof(to) - toi);
        if (FSFileExists(to) /* && resources.confirm_overwrite */) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Move: file %s already exists at destination"),
                src->name);
            if (FSConfirmationDialog(_("Overwrite?"), s)) {
                if (from)
                    free(from);
            }
            return;
        }

        if (rename(from, to)) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Error copying %s:"), from);
            FSErrorDialog(_("File Operation Error"), s);
            if (from)
                free(from);
        } else
            n_moved++;
    }
    /*
       otherwise only a single file may be selected;
       copy it to the target file
    */
    /*     else if (popups.fw->n_selections > 1)  */
    /*     { */
    /* 	error("Copy: target for multiple files", "must be a folder"); */
    /* 	goto out; */
    /*     }  */
    else {
        struct stat stats1;

        if (!stat(from, &stats1) && S_ISDIR(stats1.st_mode)) {
            FSErrorDialog(_("File Operation Error"),
                _("Cannot move a directory to a file"));
            if (from)
                free(from);
            return;
        }

        if (!lstat(to, &stats) && !lstat(from, &stats1) && stats.st_ino == stats1.st_ino) {
            FSErrorDialog(_("File Operation Error"),
                _("Move: Source and destination are identical"));
            if (from)
                free(from);
            return;
        }

        if (FSFileExists(to) /* && resources.confirm_overwrite */) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Move: file %s already exists"), to);
            if (FSConfirmationDialog(_("Overwrite?"), s)) {
                if (from)
                    free(from);
                return;
            }
        }

        if (rename(from, to)) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Error renaming %s:"), from);
            FSErrorDialog(_("File Operation Error"), s);
        } else
            n_moved = 1;
    }

    if (n_moved)
        FSUpdateFileView(FileMove, src, dest);

    if (from)
        free(from);
}

void FSRename(FileInfo* src, FileInfo* dest)
{
    struct stat stats;
    struct stat stats1;
    int i, toi, n_renamed = 0;
    char* from = NULL;
    char* to = NULL;

    from = GetPathnameFromPathName(src->path, src->name);
    to = GetPathnameFromPathName(dest->path, dest->name);

    if (!lstat(to, &stats) && !lstat(from, &stats1) && stats.st_ino == stats1.st_ino) {
        FSErrorDialog(_("File Operation Error"),
            _("Rename: Source and destination are identical"));
        if (from)
            free(from);
        if (to)
            free(to);
        return;
    }

    if (FSFileExists(to) /* && resources.confirm_overwrite */) {
        char s[0xff];
        snprintf(s, sizeof(s), _("Rename: file %s already exists"), to);
        if (FSConfirmationDialog(_("Overwrite?"), s)) {
            if (from)
                free(from);
            if (to)
                free(to);
            return;
        }
    }

    if (rename(from, to)) {
        char s[0xff];
        snprintf(s, sizeof(s), _("Error renaming %s:"), from);
        FSErrorDialog(_("File Operation Error"), s);
    } else
        n_renamed = 1;

    if (n_renamed)
        FSUpdateFileView(FileRename, src, dest);

    if (from)
        free(from);
    if (to)
        free(to);
}

void FSLink(FileInfo* src, FileInfo* dest)
{
    struct stat stats;
    int i, toi, n_linked = 0;
    char* from = NULL;
    char* to = NULL;

    from = GetPathnameFromPathName(src->path, src->name);
    to = GetPathnameFromPathName(dest->path, dest->name);
    size_t to_size = strlen(dest->path) + strlen(dest->name) + 2;

    /*
       if target exists and is a directory,
       link the source into that directory
    */
    if (!stat(to, &stats) && S_ISDIR(stats.st_mode)) {
        if (!strcmp(from, to)) {
            FSErrorDialog(_("File Operation Error"),
                _("Copy: Source and destination are identical"));
            if (from)
                free(from);
            if (to)
                free(to);
            return;
        }

        toi = strlen(to);
        if (to[toi - 1] != '/') {
            to[toi++] = '/';
            to[toi] = '\0';
        }

        /*
          Should do a loop here that goes through each element
          of the FileInfo list, checks and copies it
        */
        strlcpy(to + toi, src->name, to_size - toi);
        if (FSFileExists(to) /* && resources.confirm_overwrite */) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Link: file %s already exists at destination"),
                src->name);
            if (FSConfirmationDialog(_("Overwrite?"), s)) {
                if (from)
                    free(from);
                if (to)
                    free(to);
                return;
            }
        }

        if (symlink(from, to)) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Error linking %s to %s"), from, to);
            FSErrorDialog(_("File Operation Error"), s);
        } else
            n_linked++;
    }
    /* otherwise only a single file may be selected;
       link it to the target file */
    /*     else if (popups.fw->n_selections > 1) { */

    /*       error("Link: target for multiple files", "must be a folder"); */
    /*       goto out; */

    /*     } */
    else {
        struct stat stats1;

        if (!lstat(to, &stats) && !lstat(from, &stats1) && stats.st_ino == stats1.st_ino) {
            FSErrorDialog(_("File Operation Error"),
                _("Link: Source and destination are identical"));
            if (from)
                free(from);
            if (to)
                free(to);
            return;
        }

        if (FSFileExists(to) /* && resources.confirm_overwrite */) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Link: file %s already exists"), to);
            if (FSConfirmationDialog(_("Overwrite?"), s)) {
                if (from)
                    free(from);
                if (to)
                    free(to);
                return;
            }
        }

        if (symlink(from, to)) {
            char s[0xff];
            snprintf(s, sizeof(s), _("Error linking %s to %s"), from, to);
            FSErrorDialog(_("File Operation Error"), s);
        } else
            n_linked = 1;
    }

    if (n_linked)
        FSUpdateFileView(FileLink, src, dest);

    if (from)
        free(from);
    if (to)
        free(to);
}
