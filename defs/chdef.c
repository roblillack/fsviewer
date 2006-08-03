#include <WMaker.h>
#include <WINGs/WINGsP.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <WMaker.h>

#include <wraster.h>

#include <WINGs/WINGs.h>
#include <WINGs/WUtil.h>
#include <WINGs/proplist-compat.h>

#include "../src/config.h"

static proplist_t FSGetDBObjectForKey(proplist_t dict, char *key);
proplist_t filesDB = NULL;

Bool
InsertArrayElement(proplist_t array, proplist_t val)
{
    int i;
    Bool notFound = True;

    if (array && PLIsArray(array)) 
    {
	for(i = 0; i < PLGetNumberOfElements(array); i++)
	{
	    if(PLIsEqual(val, PLGetArrayElement(array, i)))
	    {
		notFound = False;
		break;
	    }
	}
	if(notFound)
	     PLAppendArrayElement(array, val);
    }
    else
	array = PLMakeArrayFromElements(val, NULL);

    return notFound;
}

void
loadConfigurationFile()
{
    char *home         = NULL;
    char mbuf[512];
    proplist_t val     = NULL;

    home = wdefaultspathfordomain("FSViewer"); 
    filesDB = PLGetProplistWithPath(home);
    if(filesDB)
    {
        if (!PLIsDictionary(filesDB)) 
        {
            PLRelease(filesDB);
            filesDB = NULL;
            snprintf(mbuf, 512, "FSViewer Defaults Domain (%s) is "\
                     "corrupted!", home);
	    fprintf(stderr,"%s %d\n: %s", __FILE__, __LINE__, mbuf);
        }
    } 

    if(home)
        free(home);
}

proplist_t
GetDictObject(proplist_t dictKey, proplist_t valKey)
{
    proplist_t dict = NULL;
    proplist_t val  = NULL;

    if(dictKey && PLIsString(dictKey))
    {
	dict = PLGetDictionaryEntry(filesDB, dictKey);
	if(dict && PLIsDictionary(dict))
	{
	    if(valKey && PLIsString(valKey))
	    {
		val = PLGetDictionaryEntry(dict, valKey);
		if(val)
		    return val;
	    }
/* 	    else */
/* 		return dict; */
	}
    }

    return NULL;
} 

proplist_t
GetCmdForExtn(char *extn, char *cmd)
{
    proplist_t dictKey = PLMakeString(extn);
    proplist_t cmdKey  = PLMakeString(cmd);
    proplist_t val     = NULL;


    val = GetDictObject(dictKey, cmdKey);

    if(dictKey)
	PLRelease(dictKey);
    if(cmdKey)
	PLRelease(cmdKey);

    return val;
}

char *
GetExecStringForExtn(char *extn)
{
    proplist_t val = NULL;
    
    val = GetCmdForExtn(extn, "exec");
    
    if(val != NULL)
	return PLGetString(val);
    else
	return NULL;
}

char *
GetViewerStringForExtn(char *extn)
{
    proplist_t val = NULL;
    
    val = GetCmdForExtn(extn, "viewer");
    
    if(val != NULL)
	return PLGetString(val);
    else
	return NULL;
}

char *
GetEditorStringForExtn(char *extn)
{
    proplist_t val = NULL;
    
    val = GetCmdForExtn(extn, "editor");
    
    if(val)
	return PLGetString(val);
    else
	return NULL;
}

char *
GetIconStringForExtn(char *extn)
{
    proplist_t val = NULL;
    
    val = GetCmdForExtn(extn, "icon");
    
    if(val != NULL)
	return PLGetString(val);
    else
	return NULL;
}

proplist_t
FSRemoveArrayElement(proplist_t array, proplist_t val)
{
    int i;
    Bool notFound = True;

    if (array && PLIsArray(array)) 
    {
	for(i = 0; i < PLGetNumberOfElements(array); i++)
	{
	    if(PLIsEqual(val, PLGetArrayElement(array, i)))
	    {
		PLRemoveArrayElement(array, i);
		PLRelease(val);
		break;
	    }
	}
    }

    return array;
}

void
SetEditorStringForExtn(char *extn, char *execStr)
{
    proplist_t dict = NULL;
    proplist_t dictKey = PLMakeString(extn);
    proplist_t val = PLMakeString(execStr);
    proplist_t key = PLMakeString("editor");

    dict = PLGetDictionaryEntry(filesDB, dictKey);
    if (dict && PLIsDictionary(dict))
    {
	PLInsertDictionaryEntry(dict, key, val);
    }
    else
	fprintf(stderr,"%s %d\n\n", __FILE__, __LINE__);
	
}

void
SetViewerStringForExtn(char *extn, char *execStr)
{
    proplist_t dict = NULL;
    proplist_t dictKey = PLMakeString(extn);
    proplist_t val = PLMakeString(execStr);
    proplist_t key = PLMakeString("viewer");

    dict = PLGetDictionaryEntry(filesDB, dictKey);
    if (dict && PLIsDictionary(dict))
    {
	PLInsertDictionaryEntry(dict, key, val);
    }
    else
	fprintf(stderr,"%s %d\n\n", __FILE__, __LINE__);

}

void
SetObjectForKey(proplist_t object, char *keyName)
{
    proplist_t key = PLMakeString(keyName);

    PLInsertDictionaryEntry(filesDB, key, object);
    PLRelease(key);    
}

void
SetIntegerForKey(int i, char *key)
{
    proplist_t object;
    char buffer[128];

    sprintf(buffer, "%i", i);
    object = PLMakeString(buffer);

    SetObjectForKey(object, key);
    PLRelease(object);
}

void
SetStringForKey(char *str, char *key)
{
    proplist_t object;

    object = PLMakeString(str);

    SetObjectForKey(object, key);
    PLRelease(object);
}

int 
main(int argc, char **argv)
{
    int result = 0;
    char *icon = NULL;

    loadConfigurationFile();

    if(filesDB)
    {
	int numElem, i;
	int execArrayFound = 0;
	int extnArrayFound = 0;
	proplist_t key   = NULL;
	proplist_t val   = NULL;
	proplist_t tmp   = NULL;
	proplist_t dict  = NULL;
	proplist_t array = PLGetAllDictionaryKeys(filesDB);
	proplist_t extnArray = PLGetDictionaryEntry(filesDB, 
						    PLMakeString("EXTN"));
	proplist_t execArray = PLGetDictionaryEntry(filesDB, 
						    PLMakeString("EXE"));
	proplist_t diskDict  = PLGetDictionaryEntry(filesDB, 
						    PLMakeString("DISKS"));

	if(!extnArray || !PLIsArray(extnArray))
	{
	    extnArray = PLMakeArrayFromElements(NULL, NULL);
	    extnArrayFound = 0;
	}
	else
	    extnArrayFound = 1;

	if(!execArray || !PLIsArray(execArray))
	{
	    execArray = PLMakeArrayFromElements(NULL, NULL);
	    execArrayFound = 0;
	}
	else
	    execArrayFound = 1;

	if(array && PLIsArray(array))
	{
	    char *extn = NULL;
	    char *exe  = NULL;
	    char *execStr = NULL;

	    numElem = PLGetNumberOfElements(array);
	    for(i = 0; i < numElem; i++)
	    {
		tmp = PLGetArrayElement(array, i);
		extn = PLGetDescription(tmp);
		if(extn)
		{
		    exe = GetViewerStringForExtn(extn);
		    if(exe)
		    {
			/* Add to extn list */
			InsertArrayElement(extnArray, PLMakeString(extn));
			continue;
		    } /* End if(exe) */

		    exe = GetEditorStringForExtn(extn);
		    if(exe)
		    {
			/* Add to extn list */
			InsertArrayElement(extnArray, PLMakeString(extn));
			continue;
		    } /* End if(exe) */

		    exe = GetExecStringForExtn(extn);
		    if(exe)
		    {
			/*Add to Exe list*/
			InsertArrayElement(execArray, PLMakeString(extn));
			continue;
		    } /* End if(exe) */
		} /* End if(extn) */
	    } /* End for(i;;) */
	}

	if(!extnArrayFound)
	    SetObjectForKey(extnArray, "EXTN");
	if(!execArrayFound)
	    SetObjectForKey(execArray, "EXE");
	/* DISKS setting */
	if(diskDict && PLIsDictionary(diskDict))
	{
	    proplist_t devices = NULL;
	    proplist_t mount = NULL;
	    proplist_t umount = NULL;
	    proplist_t eject = NULL;

	    devices = PLGetDictionaryEntry(diskDict, PLMakeString("devices"));
	    mount = PLGetDictionaryEntry(diskDict, PLMakeString("mount"));
	    umount = PLGetDictionaryEntry(diskDict, PLMakeString("umount"));
	    eject = PLGetDictionaryEntry(diskDict, PLMakeString("eject"));

	    if(devices/*  && PLIsArray(devices) */)
	    { 
		numElem = PLGetNumberOfElements(devices);
		for(i = 0; i < numElem; i++)
		{
		    tmp = PLGetArrayElement(devices, i);
		    if(tmp && PLIsArray(tmp))
		    {
			InsertArrayElement(tmp, mount);
			InsertArrayElement(tmp, umount);
			InsertArrayElement(tmp, eject);
			InsertArrayElement(tmp, PLMakeString("close %s"));
		    }
		}
		SetObjectForKey(devices, "DISCS");
	    }
	}

	SetIntegerForKey(0, "DisplayMCListPixmap");
	
	PLSave(filesDB, wdefaultspathfordomain("FSViewer"), YES);
	result = 0;
    }	    
    else
    {
	fprintf(stderr,"%s %d: unable to open user defaults", 
		__FILE__, __LINE__);
	result = 1;
    }

    exit(result);
}
