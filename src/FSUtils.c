/*
  Need to ifdef these two #include statements.
  I guess they won't work on other systems!!!

  Uncomment if you run Linux and want the RAM total
  to appear in the Info Dialogue Box.
*/
/* #include <linux/kernel.h> */
/* #include <linux/sys.h> */
#include <sys/utsname.h>
/* #include <sys/vfs.h> */
#include <sys/stat.h>
/* #include <mntent.h> */
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <WINGs/WINGsP.h>
#include <wraster.h>
#include <X11/cursorfont.h>

/* change this to your OS */
/* #define __Linux__ */

#if defined(__FreeBSD__)
#include <kvm.h>
#include <limits.h>
#include <osreldate.h>
#include <sys/conf.h>
#include <sys/dkstat.h>
/* #include <sys/rlist.h> */
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/vmmeter.h>
#include <fcntl.h>
typedef unsigned long ulong;
#endif /* __FreeBSD__ */

#if defined(__SunOS__)
#include <kstat.h>
#include <sys/cpuvar.h>
#include <sys/swap.h>
#include <utmp.h>
#endif /*__SunOS__*/

/* #include "config.h" */
#include "files.h"
#include "filebrowser.h"
#include "FSFileButton.h"
#include "FSViewer.h"
#include "FSFileView.h"
#include "FSUtils.h"
#include "FSPanel.h"
#include "DnD.h"
#include "xpm/file_plain.xpm"
#include "xpm/backing2.xpm"
#include "xpm/transparent.xpm"

#if defined(__FreeBSD__)
#define DF_COMMAND "df"
#else
#define DF_COMMAND "df -k"
#endif

const char *formatk(ulong k);

static RContext *rContext = NULL;
static RContextAttributes attributes;

typedef struct  _FSSystem
{
    char            *host;
    /*         char            *cwd; */
    struct passwd   *user;
    Bool             haveUserInfo;
    /*         struct passwd   *pwds; */
    /*         int              npwds; */
    /*         time_t           pwdtime; */
    /*         struct group    *grps; */
    /*         int              ngrps; */
    /*         time_t           grptime; */
    
    /*   
	 Uncomment if you run Linux and want the RAM total
	 to appear in the Info Dialogue Box. 
    */
    /*         struct sysinfo   sysinf; */
    Bool             haveSysinfo;
    struct utsname   uts;
    Bool             haveUname;
    long             size;
    mode_t           umask;

    char **imgTypes;

} _FSSystem;

typedef struct _FSSystem FSSystem;


static FSSystem *info;
static FSViewer *fsViewer;
static char buf[MAX_LEN];
static Cursor cursor;

void
FSInitSystemInfo(FSViewer *fsV)
{

    struct passwd *pwp;

    info = (FSSystem *) malloc(sizeof(FSSystem));
    memset(info, 0, sizeof(FSSystem));

    if( (info->user= getpwuid(getuid())) == NULL) 
	info->haveUserInfo = False;
    else
	info->haveUserInfo = True;
	
    /*   
	 Uncomment if you run Linux and want the RAM total
	 to appear in the Info Dialogue Box. 
    */
/*     if(!sysinfo(&info->sysinf)) */
/* 	info->haveSysinfo = True; */
/*     else */
	info->haveSysinfo = False;

#ifdef __SunOS__
    if(uname(&info->uts) >= 0)
#else
    if(!uname(&info->uts))
#endif
	info->haveUname = True;
    else
	info->haveUname = False;

    info->umask = umask(0);
    umask(info->umask);
    info->umask = 0777777 ^ info->umask;

    info->imgTypes = RSupportedFileFormats();  

    fsViewer = fsV;
}

char *
FSProcessor()
{

    if ((info->haveUname == True))
	snprintf(buf, MAX_LEN, "%s", info->uts.machine); 
    else
	snprintf(buf, MAX_LEN, _("Unavailable"));

    return wstrdup(buf);
}

char *
FSSystemRelease()
{

    if ((info->haveUname == True))
	snprintf(buf, MAX_LEN, "%s %s", info->uts.sysname, info->uts.release);
    else
	snprintf(buf, MAX_LEN, _("Unavailable"));

    return wstrdup(buf);
}

char *
FSNodeName()
{

    if ((info->haveUname == True))
	snprintf(buf, MAX_LEN, "%s", info->uts.nodename);
    else
	snprintf(buf, MAX_LEN, "/");

    return wstrdup(buf);
}

char *
FSMemory()
{

    /*   
	 Uncomment if you run Linux and want the RAM total
	 to appear in the Info Dialogue Box. 
    */
/*     if ((info->haveSysinfo == True)) */
/* 	snprintf(buf, MAX_LEN, "%.1fM", */
/* 		 (float) info->sysinf.totalram / 1048576L); */
/*     else */
/* 	snprintf(buf, MAX_LEN, "Unavailable"); */

/*     return buf; */

    ulong total = 0;
    unsigned long buffers;

#ifdef __Linux__
    FILE	 *fp_meminfo;
    char	  temp[128];
    unsigned long free, shared, used, cached;
#endif /* __linux__ */

#if defined(__FreeBSD__)
    kvm_t	*kvmd = NULL;
    struct nlist nl[] = {
#define N_CNT		0
	{ "_cnt" },
#define N_BUFSPACE	1
	{ "_bufspace" },
#define N_CP_TIME	2
	{ "_cp_time" },
#define N_AVERUN	3
	{ "_averunnable" },
#define VM_SWAPLIST	4
	{ "_swaplist" },
#define VM_SWDEVT	5
	{ "_swdevt" },
#define VM_NSWAP	6
	{ "_nswap" },
#define VM_NSWDEV	7
	{ "_nswdev" },
#define VM_DMMAX	8
	{ "_dmmax" },
#define N_DK_NDRIVE	9
	{ "_dk_ndrive" },
#define N_DK_WDS	10
	{ "_dk_wds" },
	{ "" }
    };
    int	 psize;
    int	 pshift;
    char errbuf[_POSIX2_LINE_MAX];
#endif /* __FreeBSD__ */
    
#ifdef __SunOS__
    long		maxmem;
    int 		PageShift, i;
#endif /*__SunOS__*/

#ifdef __Linux__

    fp_meminfo = fopen("/proc/meminfo", "r");
    while (fgets(temp, 128, fp_meminfo)) 
    {
	if (strstr(temp, "Mem:")) 
	{
	    sscanf(temp, "Mem: %ld %ld %ld %ld %ld %ld",
		   &total, &used, &free, &shared, &buffers, &cached);
	    total >>= 10;
	    break;
	}
    }
    fclose(fp_meminfo);

#endif /* __linux__ */

#ifdef __FreeBSD__

    psize = getpagesize();
    for (pshift = 0, psize = getpagesize(); psize>1; pshift++, psize>>=1);
    pshift -= 10;
    psize = getpagesize();
    
    if (kvmd==NULL) 
	kvmd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf);
    if (kvmd) 
    {
	if (kvm_nlist(kvmd, nl) >= 0) 
	{
	    if (nl[0].n_type != 0) 
	    {
		struct vmmeter sum;
		
		if ((kvm_read(kvmd, nl[N_CNT].n_value, (char *)&sum, sizeof(sum))==sizeof(sum)) &&
		    (kvm_read(kvmd, nl[N_BUFSPACE].n_value, (char *)&buffers, sizeof(buffers))==sizeof(buffers))) 
		{
		    total = (sum.v_page_count - (buffers / psize) - sum.v_wire_count - sum.v_cache_count) << pshift;
		}
	    }
	}
    }
#endif /* __FreeBSD__ */

#ifdef __SunOS__

    i = sysconf(_SC_PAGESIZE); PageShift = 0;
    while ((i >>= 1) > 0) { ++PageShift; }
    PageShift -= 10;


    maxmem = sysconf(_SC_PHYS_PAGES) << PageShift;
    maxmem >>= 10;
    
    /* 
     * maxmem is in megs already
     * formatk assumes it is in kbytes
     * so it has to be scaled to get M
     * instead of K
     */
    total = maxmem*1024;

#endif /*__SunOS__*/

    if(total > 0)
	snprintf(buf, MAX_LEN, "%s", formatk(total));
    else
	snprintf(buf, MAX_LEN, "%s", _("Unavailable"));

    return wstrdup(buf);

}

char *
FSDisk()
{
    ulong total = 0;    /* Total Space */
    FILE* f     = popen(DF_COMMAND, "r");
    
    if (!f) 
    {
	wwarning(_("%s %d: Can't run df, %s\n"), __FILE__, __LINE__, 
		 strerror(errno));
	exit(1);
    }
	
    /* Read in from the pipe until we hit the end */
    for (;;)
    {
	int   n     = 0;     /* number of words */
	char *p     = NULL;
	char *fs    = NULL;  /* File System */
	char *word[10];      /* pointer to each word */
	char  buffer[1024];
	
	/* Read in line by line */
	if (!fgets(buffer, 1024, f)) 
	    break;
	
	/* Strip the whitespace and break up the line */
	for (p = buffer; n < 10;) 
	{
	    // skip leading whitespace:
	    while (*p && isspace(*p)) 
      		p++;
	    if (!*p) 
      		break;
	    // skip over the word:
	    word[n++] = p;
	    
	    while (*p && !isspace(*p)) 
		p++;
	    if (!*p) 
      		break;
	    *p++ = 0;
	}

	/* Get the file system name */
	fs = strdup(word[n-6]);
	/* And make sure it is local */
	if(strncmp(fs, "/dev", 4)) 
	    continue;
	
	/* ok we found a line with a /dev at the start */
	/* Total */
	total += strtol(word[n-5], 0, 10);
		
	if(fs)
	    free(fs);
    }
    pclose(f);

    sprintf(buf, "%s", formatk(total));

    return wstrdup(buf);
}

/* turn number of K into user-friendly text */
const char* 
formatk(ulong k) 
{
    static char buffer[10];
  
    if (k >= 1024*1024) 
	sprintf(buffer,"%.4gG",(double)k/(1024*1024));
    else if(k >= 1024)
	sprintf(buffer,"%.4gM",(double)k/1024);
    else 
	sprintf(buffer,"%ldK",k);
    
    return buffer;
}

char*
LocateImage(char *name)
{
    char *path = NULL;
    char *tmp = NULL;
    char **types;

    if(name)
	tmp = (char *)wmalloc(strlen(name)+8);
    else
	return NULL;

    if(FSImageTypeIsSupported("TIFF"))
    {
	sprintf(tmp, "%s.tiff", name);
	path = WMPathForResourceOfType(tmp, "tiff");
	
	if(!path)
	{
	    sprintf(tmp, "%s.tif", name);
	    path = WMPathForResourceOfType(tmp, "tiff");
	}
    }
    
    /* Uncomment if you want the app to support GIF or JPEG icons */
/*     if(FSImageTypeIsSupported("GIF")) */
/*     { */
/* 	sprintf(tmp, "%s.gif", name); */
/* 	path = WMPathForResourceOfType(tmp, "gif"); */
/*     } */
    
/*     if(FSImageTypeIsSupported("JPEG")) */
/*     { */
/* 	sprintf(tmp, "%s.jpg", name); */
/* 	path = WMPathForResourceOfType(tmp, "jpg"); */
/*     } */
        
    if(!path)
    {
	sprintf(tmp, "%s.xpm", name);
	path = WMPathForResourceOfType(tmp, "xpm");
    }

    if(!path)
	path = WMPathForResourceOfType(name, "");

    free(tmp);

    if (!path) 
	return wstrdup(name);

    return path;
}


char *
FSParseExecString(char *pathname, char *execStr, ...)
{
    va_list ap;
    char *str;
    char *buffer;
    int i, cnt;
    int pathnameLen;
    int initBufferSize;


    cnt = 0;
    pathnameLen = strlen(pathname);
    initBufferSize = strlen(execStr)+1;

    /* This is an inefficient way of doing things */
    buffer = (char *) wmalloc(initBufferSize);
    str = buffer;
    
    va_start(ap, execStr);
    for(; *execStr; ++execStr)
    {
	if (*execStr != '%') 
	{
	    *str++ = *execStr;
	    cnt++;
	    continue;
	}
	++execStr;
	switch(*execStr) 
	{
	case 's':
	    (void)va_arg(ap, char *);
	    
	    buffer = (char *) wrealloc(buffer, initBufferSize+cnt+pathnameLen);
	    str = buffer;
	    str += cnt;
	    for (i = 0; i < pathnameLen; ++i)
	    {
		*str++ = pathname[i];
		cnt++;
	    }
	    continue;
	default : 
	    if (*execStr != '%')
	    {
		*str++ = '%';
		cnt++;
	    }
	    if (*execStr)
	    {
		*str++ = *execStr;
		cnt++;
	    }
	    else
		--execStr;
	    continue;
	}
    }
    va_end(ap);
    *str = '\0';

    return buffer;
}

void
FSLaunchApp(FSViewer *fsViewer, AppEvent event)
{
    char *pathname;
    char *path;
    char *name;
    FileInfo *fileInfo;

    fileInfo = FSCreateFileInfo();

    pathname = FSGetFileViewPath(FSGetFSViewerCurrentView(fsViewer));
    path = GetPathFromPathname(pathname);
    name = GetNameFromPathname(pathname);
    
    GetFileInfo(path, name, fileInfo);

    LaunchApp(fsViewer, fileInfo, event);
}

void      
LaunchApp(FSViewer *fsViewer, FileInfo *fileInfo, AppEvent event)
{
    if(!isDirectory(fileInfo->fileType))
    {
	char *exec = NULL;
	char *extn = NULL;
	char *execStr = NULL;
	char *pathname;

	
	if (chdir(fileInfo->path))
	{
	    char s[0xff];

	    sprintf(s, _("Error changing to %s but it's not that serious!"), 
		    fileInfo->path);
	    FSErrorDialog(_("Error File Operation"), s);
	}

	pathname = GetPathnameFromPathName(fileInfo->path, fileInfo->name);
	extn = GetFileExtn(fileInfo->name);

	if(event == AppExec)
	{
	    if( (exec = FSGetStringForNameKey(extn, "exec")) != NULL)
		execStr = FSParseExecString("", exec);
	    else
		execStr = FSParseExecString("", fileInfo->name);
	}
	else if(event == AppView)
	{
	    if( (exec = FSGetStringForNameKey(extn, "viewer")) != NULL)
		execStr = FSParseExecString(pathname, exec);
	    else
	    {
		char buf[MAX_LEN];

		/* 
		   This is a quick hack, a very quick hack. 
		   The magic files method has some very good  
		   potential, I just need to figure out how 
		   to implement it for the whole app. 
		*/ 
		magic_get_type(pathname, buf);
 
		if( !strcmp("ascii", buf) ||
		    !strcmp("HTML", buf)  ||
		    !strcmp("MAIL", buf) )
		{
		    exec = FSGetStringForNameKey("MAGICASCII", "exec");
		}
		else if( !strcmp("PBM", buf) ||
			 !strcmp("PGM", buf) ||
			 !strcmp("GIF", buf) ||
			 !strcmp("JPG", buf) ||
			 !strcmp("XPM", buf) ||
			 !strcmp("XBM", buf) ||
			 !strcmp("PPM", buf) ||
			 !strcmp("TIFF", buf) )
		{
		    exec = FSGetStringForNameKey("MAGICIMAGE", "exec");
		}
		else if( !strcmp("PS", buf) )
		{
		    exec = FSGetStringForNameKey("MAGICPS", "exec");
		}

		if(exec != NULL)
		    execStr = FSParseExecString(pathname, exec);
		else
		{
		    char *result = NULL;
	    
		    result = FSRunAppInputPanel(fsViewer, fileInfo, 
						_("App Input Window"));
		    if(result)
			execStr = FSParseExecString(pathname, result);
		}
	    }
	}
	else if(event == AppEdit)
	{
	    if( (exec = FSGetStringForNameKey(extn, "editor")) != NULL)
		execStr = FSParseExecString(pathname, exec);

	    if(execStr == NULL)
	    {
		char *result = NULL;
		
		result = FSRunAppInputPanel(fsViewer, fileInfo, 
					    _("App Input Window"));
		if(result)
		    execStr = FSParseExecString(pathname, result);
	    }
	}

	if(execStr)
	{
	    execCommand(execStr);
	    free(execStr);
	}

	if(pathname)
	    free(pathname);
    }
}

void
FSSetButtonImageFromFile(WMButton *btn, char *imgName)
{
    RColor color;
    WMPixmap *pixmap;

    color.red   = 0xae;
    color.green = 0xaa; /* aa ?*/
    color.blue  = 0xae;
    color.alpha = 0;
    if(imgName)
	pixmap = WMCreateBlendedPixmapFromFile(WMWidgetScreen(btn),
					       imgName, &color);
    else
	pixmap = NULL;

    if (!pixmap)
    {
	wwarning(_("%s %d: Could not load icon file %s"), 
		 __FILE__, __LINE__, imgName);
    }
    else
    {
	WMSetButtonImage(btn, pixmap);
	WMReleasePixmap(pixmap);
    }

    color.red   = 0xff;
    color.green = 0xff;
    color.blue  = 0xff;
    color.alpha = 0;
    if(imgName)
/*	pixmap = FSCreatePixmapWithBackingFromFile(WMWidgetScreen(btn),
						   imgName, &color);*/
	pixmap = WMCreateBlendedPixmapFromFile(WMWidgetScreen(btn),
					       imgName, &color);
    else
	pixmap = NULL;

    if (!pixmap)
    {
	wwarning(_("%s %d: Could not load icon file %s"), 
		 __FILE__, __LINE__, imgName);
    }
    else
    {
	WMSetButtonAltImage(btn, pixmap);
	WMReleasePixmap(pixmap);
    }
}

void
FSSetButtonImageFromXPMData(WMButton *btn, char **data)
{
    RColor color;
    WMPixmap *pixmap;

    color.red   = 0xae;
    color.green = 0xaa;
    color.blue  = 0xae;
    color.alpha = 0;
    pixmap = WMCreatePixmapFromXPMData(WMWidgetScreen(btn), data);
    if (!pixmap)
	wwarning(_("%s %d: Could not create Pixmap from Data"),
		 __FILE__, __LINE__);

    WMSetButtonImage(btn, pixmap);

    if (pixmap)
	WMReleasePixmap(pixmap);

    color.red   = 0xff;
    color.green = 0xff;
    color.blue  = 0xff;
    color.alpha = 0;
    pixmap = WMCreatePixmapFromXPMData(WMWidgetScreen(btn), data);
    if (!pixmap)
	wwarning(_("%s %d: Could not create Pixmap from Data"),
		 __FILE__, __LINE__);

    WMSetButtonAltImage(btn, pixmap);

    if (pixmap)
	WMReleasePixmap(pixmap);
}

void
FSLoadIconPaths(WMList *list)
{
    int i;
    char mbuf[MAX_LEN];
    char *path;
    WMPropList* pathList;
    WMPropList* val;
    WMPropList* WindowMakerDB;

    path = wdefaultspathfordomain("WindowMaker");
    
    WindowMakerDB = WMReadPropListFromFile(path);
    if (WindowMakerDB) 
    {
	if (!WMIsPLDictionary(WindowMakerDB)) 
	{
	    WMReleasePropList(WindowMakerDB);
	    pathList = NULL;
	    snprintf(mbuf, MAX_LEN, _("Window Maker domain (%s) is corrupted!"), 
		     path);
	    WMRunAlertPanel(WMWidgetScreen(list), NULL, _("Error"), 
			    mbuf, _("OK"), NULL, NULL);
	}
    } 
    else 
    {
	snprintf(mbuf, MAX_LEN, _("Could not load Window Maker defaults (%s)"), 
		 path);
	WMRunAlertPanel(WMWidgetScreen(list), NULL, _("Error"), 
			mbuf, _("OK"), NULL, NULL);
    }

    pathList = WMGetFromPLDictionary(WindowMakerDB, WMCreatePLString("IconPath"));
    if (pathList && WMIsPLArray(pathList)) 
    {
	for (i=0; i<WMGetPropListItemCount(pathList); i++) 
	{
	    val = WMGetFromPLArray(pathList, i);
	    WMAddListItem(list, WMGetFromPLString(val));
	}
    }

    if(path)
	free(path);
}

static void
FSRenderPixmap(WMScreen *screen, Pixmap d, char **data,
             int width, int height)
{
    int x, y;
    Display *dpy = WMScreenDisplay(screen);
    GC whiteGC   = WMColorGC(WMWhiteColor(screen));
    GC blackGC   = WMColorGC(WMBlackColor(screen));
    GC lightGC   = WMColorGC(WMGrayColor(screen));
    GC darkGC    = WMColorGC(WMDarkGrayColor(screen));


    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            switch (data[y][x]) {
             case ' ':
             case 'w':
                XDrawPoint(dpy, d, whiteGC, x, y);
                break;

             case '=':
             case '.':
             case 'l':
                XDrawPoint(dpy, d, lightGC, x, y);
                break;

             case '%':
             case 'd':
                XDrawPoint(dpy, d, darkGC, x, y);
                break;

             case '#':
             case 'b':
             default:
                XDrawPoint(dpy, d, blackGC, x, y);
                break;
            }
        }
    }
}

WMPixmap*
FSMakePixmap(WMScreen *sPtr, char **data, int width, int height)
{
    Pixmap pixmap;

    pixmap = XCreatePixmap(WMScreenDisplay(sPtr), W_DRAWABLE(sPtr), 
			   width, height, WMScreenDepth(sPtr));

    FSRenderPixmap(sPtr, pixmap, data, width, height);

    return WMCreatePixmapFromXPixmaps(sPtr, pixmap, None, width, height,
                                      WMScreenDepth(sPtr));
}

void
FSErrorDialog(char *title, char *msg)
{
    WMScreen *scr = FSGetFSViewerScreen(fsViewer);

    WMRunAlertPanel(scr, NULL, title, msg, _("OK"), 
		    NULL, NULL);
}

int
FSConfirmationDialog(char *title, char *msg)
{
    WMScreen *scr = FSGetFSViewerScreen(fsViewer);

    return WMRunAlertPanel(scr, NULL, title, msg, _("OK"), 
			   _("Cancel"), NULL);
}

void
FSUpdateFileView(FileAction action, FileInfo *src, FileInfo *dest)
{
    FSUpdateFileViewPath(FSGetFSViewerCurrentView(fsViewer),
			 action, src, dest);

}

mode_t
FSGetUMask()
{
    return info->umask;
}

char *
FSGetHomeDir()
{
    return info->haveUserInfo ? info->user->pw_dir : NULL;
}

/* match a pattern with a filename, returning nonzero if the match was
   correct */

/* Currently only *, ? and [...] (character classes) are recognized, no curly
   braces. An escape mechanism for metacharacters is also missing. This could
   be implemented more efficiently, but the present simple backtracking
   routine does reasonably well for the usual kinds of patterns. -ag */

int 
FSStringMatch(char *pattern, char *fn)
{
    char *start;

    for (;; fn++, pattern++) 
    {
	switch (*pattern) 
	{
	case '?':
	    if (!*fn)
		return 0;
	    break;
	    
	case '*':
	    pattern++;
	    do
		if (FSStringMatch(pattern,fn))
		    return 1;
	    while (*fn++);
	    return 0;

	case '[':
	    start = pattern+1;
	    do 
	    {
	    next:
		pattern++;
		if (*pattern == ']')
		    return 0;
		else if( pattern[0] == '-' && 
			 pattern > start   && 
			 pattern[1] != ']'  )
		{
		    if (pattern[-1] <= *fn && *fn <= pattern[1])
			break;
		}
		else 
		{
		    start = (++pattern)+1;
		    goto next;
		}
	    }
	    while (*fn != *pattern);

	    while (*pattern != ']')
		if (!*pattern++)
		    return 0;
	    break;
	
	default:
	    if (*fn != *pattern)
		return 0;
	}

	if (!*fn)
	    return 1;
    };
}
/*
WMPixmap*
FSCreateBlendedPixmapFromFile(WMScreen *scr, char *fileName, RColor *color)
{
    RImage *image = NULL;
    RImage *clone = NULL;
    WMPixmap *pixmap;
    RColor color1;
    int x,y;

    if(!rContext)
    {
	memset((void *) &attributes, 0, sizeof(RContextAttributes));
	attributes.flags = (RC_RenderMode | RC_ColorsPerChannel);
	attributes.render_mode = RM_DITHER;
	attributes.colors_per_channel = 4;

	rContext = RCreateContext(WMScreenDisplay(scr), 
				  DefaultScreen(WMScreenDisplay(scr)), 
				  &attributes);
    }
    if(fileName)
	image = RLoadImage(rContext, fileName, 0);
    if(!image)
	image = RGetImageFromXPMData(rContext, file_plain);

    color1.red   = 0xa3;
    color1.green = 0xa3;
    color1.blue  = 0xa3;
    color1.alpha = 255;

    RCombineImageWithColor(image, color);

    / * Resize the width to 64x64 pixels * /
    clone = RMakeCenteredImage(image, 64, 64, &color1);
    pixmap = WMCreatePixmapFromRImage(scr, clone, 0);

    RReleaseImage(image);
    RReleaseImage(clone);

    return pixmap;
}
*/

WMPixmap*
FSCreatePixmapWithBackingFromFile(WMScreen *scr, char *fileName, RColor *color)
{
    RImage *image = NULL;
    RImage *clone = NULL;
    int x, y;
    WMPixmap *pixmap = NULL;

    if(!rContext)
    {
	memset((void *) &attributes, 0, sizeof(RContextAttributes));
	attributes.flags = (RC_RenderMode | RC_ColorsPerChannel);
	attributes.render_mode = RM_DITHER;
	attributes.colors_per_channel = 4;
	
	rContext = RCreateContext(WMScreenDisplay(scr), 
				  DefaultScreen(WMScreenDisplay(scr)), 
				  &attributes);
    }

    if(fileName)
	image = RLoadImage(rContext, fileName, 0);

    if(!image)
	image = RGetImageFromXPMData(rContext, file_plain);

    clone = RGetImageFromXPMData(rContext, backing2_xpm);

    RCombineImageWithColor(image, color);

    /*
     * Using (clone->width - image->width)/2
     * gives out gibberish. For whatever reason
     * explicit structure below works
     */
    x = clone->width/2  - image->width/2;
    y = clone->height/2 - image->height/2;

    /* Bad hack to make sure things fit the clone pixmap */
    if(x >= 0 && y >= 0)
    {
      RCombineArea(clone, image, 0, 0, image->width, image->height, x, y);
      pixmap = WMCreatePixmapFromRImage(scr, clone, 0);
    }

    RReleaseImage(clone);
    RReleaseImage(image);

    return pixmap;
}


WMPixmap*
FSCreateBlurredPixmapFromFile(WMScreen *scr, char *fileName)
{
    int x, y;
    RColor color;
    WMPixmap *pixmap;
    RImage   *image = NULL;
    RImage   *clone = NULL;

    if(!rContext)
    {
	memset((void *) &attributes, 0, sizeof(RContextAttributes));
	attributes.flags = (RC_RenderMode | RC_ColorsPerChannel);
	attributes.render_mode = RM_DITHER;
	attributes.colors_per_channel = 4;

	rContext = RCreateContext(WMScreenDisplay(scr), 
				  DefaultScreen(WMScreenDisplay(scr)), 
				  &attributes);
    }
    if(fileName)
	image = RLoadImage(rContext, fileName, 0);
    if(!image)
	image = RGetImageFromXPMData(rContext, file_plain);

    color.red =   0xff;
    color.green = 0xff;
    color.blue =  0xff;
    color.alpha = 200;

    RClearImage(image, &color);
    pixmap = WMCreatePixmapFromRImage(scr, image, 0);

    RReleaseImage(image);

    return pixmap;
}

char *
FSParseCmdField(FileInfo *fileInfo, char *txt, ...)
{
    /*
      Dynamically allocate buf and then return it.
      Use something along the basis of the size of
      *txt, then add in the size of file|path each time
      they are encountered. Use realloc
    */
       
    va_list ap;
    char *str;
    char *file;
    char *path;
    char *buffer;
    int i, cnt;
    int fileLen, pathLen;
    int initBufferSize;


    cnt = 0;
    fileLen = strlen(fileInfo->name);
    file = (char *) wmalloc(fileLen+1);
    strcpy(file, fileInfo->name);

    pathLen = strlen(fileInfo->path);
    path = (char *) wmalloc(pathLen+1);
    strcpy(path, fileInfo->path);

    initBufferSize = strlen(txt)+1;
    /* This is an inefficient way of doing things */
    buffer = (char *) wmalloc(initBufferSize);
    str = buffer;
    
    va_start(ap, txt);
    for(; *txt; ++txt)
    {
	if (*txt != '%') 
	{
	    *str++ = *txt;
	    cnt++;
	    continue;
	}
	++txt;

	switch(*txt) 
	{
	case 'f':
	    (void)va_arg(ap, char *);
	    
	    buffer = (char *) wrealloc(buffer, initBufferSize+cnt+fileLen);
	    str = buffer;
	    str += cnt;
	    for (i = 0; i < fileLen; ++i)
	    {
		*str++ = file[i];
		cnt++;
	    }
	    continue;
	case 'p':
	    (void)va_arg(ap, char *);
	    
	    buffer = (char *) wrealloc(buffer, initBufferSize+cnt+pathLen);
	    str = buffer;
	    str += cnt;
	    for (i = 0; i < pathLen; ++i)
	    {
		*str++ = path[i];
		cnt++;
	    }
	    continue;
	default : 
	    if (*txt != '%')
	    {
		*str++ = '%';
		cnt++;
	    }
	    if (*txt)
	    {
		*str++ = *txt;
		cnt++;
	    }
	    else
		--txt;
	    continue;
	}
    }
    va_end(ap);
    *str = '\0';

    return buffer;
}

void
FSSetBusyCursor(WMWidget *w, Bool state)
{
    Window wnd = WMWidgetXID(w);
    WMScreen *scr = WMWidgetScreen(w);
    Display  *dpy = WMScreenDisplay(scr);

    if(state)
    {
	Cursor cursor;
	
	cursor = XCreateFontCursor(dpy, XC_watch);
	XDefineCursor(dpy, wnd, cursor);
    }
    else
	XUndefineCursor(dpy, wnd);

}

int
FSGetDNDType(FileInfo *fileInfo)
{
    int type = DndFile;
    char buf[MAX_LEN];
    char *pathname = NULL;

    if(fileInfo == NULL)
	return;
    
    switch (fileInfo->fileType)
    {
    case S_LINK    : type=DndLink; 
	             break;
    case DIRECTORY :
    case ROOT      :
    case HOME      : type=DndDir;
	             break;
    default        : pathname = GetPathnameFromPathName(fileInfo->path, 
							fileInfo->name);
                     magic_get_type(pathname, buf);
		     if(strcmp("EXEC", buf) == 0)
			 type = DndExe;
		     else if(strcmp("ascii", buf) == 0)
			 type=DndFile;
		     else if(strcmp("ascii", buf) == 0)
			 type=DndText;
		     else
			 type=DndFile;			 
	             break;
    }

    return type;
}

int
FSExecCommand(char *path, char *execStr)
{
    if (chdir(path))
    {
	char s[0xff];
	
	sprintf(s, _("Error changing to %s, \"%s\" cancelled."), path);
	FSErrorDialog(_("Error File Operation"), s);
	
	return 1;
    }

    if (execCommand(execStr))
	return 0;

    return 1;
}

Bool
FSImageTypeIsSupported(char *imgType)
{
    int i;
    Bool found = False;

    if(imgType == NULL || info->imgTypes[0] == NULL)
	return found;

    for(i=0; info->imgTypes[i]; i++)
    {
	if(strcmp(imgType, info->imgTypes[i]) == 0)
	{
	    found = True;
	    break;
	}
    }
    return found;
}
