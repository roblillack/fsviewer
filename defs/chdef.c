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

#include "../src/config.h"


static WMPropList* FSGetDBObjectForKey(WMPropList* dict, char *key);
WMPropList* filesDB = NULL;

Bool
InsertArrayElement(WMPropList* array, WMPropList* val)
{
    int i;
    Bool notFound = True;

    if (array && WMIsPLArray(array)) 
    {
	for (i = 0; i < WMGetPropListItemCount(array); i++)
	{
	    if (WMIsPropListEqualTo(val, WMGetFromPLArray(array, i)))
	    {
		notFound = False;
		break;
	    }
	}
	if (notFound)
	     WMAddToPLArray(array, val);
    }
    else
	array = WMCreatePLArray(val, NULL);

    return notFound;
}

void
loadConfigurationFile()
{
    char *home         = NULL;
    char mbuf[512];
    WMPropList* val     = NULL;

    home = wdefaultspathfordomain("FSViewer"); 
    filesDB = WMReadPropListFromFile(home);
    if (filesDB)
    {
        if (!WMIsPLDictionary(filesDB)) 
        {
            WMReleasePropList(filesDB);
            filesDB = NULL;
            snprintf(mbuf, 512, "FSViewer Defaults Domain (%s) is "\
                     "corrupted!", home);
	    fprintf(stderr,"%s %d\n: %s", __FILE__, __LINE__, mbuf);
        }
    } 

    if (home)
        free(home);
}

WMPropList*
GetDictObject(WMPropList* dictKey, WMPropList* valKey)
{
    WMPropList* dict = NULL;
    WMPropList* val  = NULL;

    if (dictKey && WMIsPLString(dictKey))
    {
	dict = WMGetFromPLDictionary(filesDB, dictKey);
	if (dict && WMIsPLDictionary(dict))
	{
	    if (valKey && WMIsPLString(valKey))
	    {
		val = WMGetFromPLDictionary(dict, valKey);
		if (val)
		    return val;
	    }
/* 	    else */
/* 		return dict; */
	}
    }

    return NULL;
} 

WMPropList*
GetCmdForExtn(char *extn, char *cmd)
{
    WMPropList* dictKey = WMCreatePLString(extn);
    WMPropList* cmdKey  = WMCreatePLString(cmd);
    WMPropList* val     = NULL;


    val = GetDictObject(dictKey, cmdKey);

    if (dictKey)
	WMReleasePropList(dictKey);
    if (cmdKey)
	WMReleasePropList(cmdKey);

    return val;
}

char *
GetExecStringForExtn(char *extn)
{
    WMPropList* val = NULL;
    
    val = GetCmdForExtn(extn, "exec");
    
    if (val != NULL)
	return WMGetFromPLString(val);
    else
	return NULL;
}

char *
GetViewerStringForExtn(char *extn)
{
    WMPropList* val = NULL;
    
    val = GetCmdForExtn(extn, "viewer");
    
    if (val != NULL)
	return WMGetFromPLString(val);
    else
	return NULL;
}

char *
GetEditorStringForExtn(char *extn)
{
    WMPropList* val = NULL;
    
    val = GetCmdForExtn(extn, "editor");
    
    if (val)
	return WMGetFromPLString(val);
    else
	return NULL;
}

char *
GetIconStringForExtn(char *extn)
{
    WMPropList* val = NULL;
    
    val = GetCmdForExtn(extn, "icon");
    
    if (val != NULL)
	return WMGetFromPLString(val);
    else
	return NULL;
}

WMPropList*
FSRemoveArrayElement(WMPropList* array, WMPropList* val)
{
    int i;
    Bool notFound = True;

    if (array && WMIsPLArray(array)) 
    {
	for (i = 0; i < WMGetPropListItemCount(array); i++)
	{
	    if (WMIsPropListEqualTo(val, WMGetFromPLArray(array, i)))
	    {
		WMDeleteFromPLArray(array, i);
		WMReleasePropList(val);
		break;
	    }
	}
    }

    return array;
}

void
SetEditorStringForExtn(char *extn, char *execStr)
{
    WMPropList* dict = NULL;
    WMPropList* dictKey = WMCreatePLString(extn);
    WMPropList* val = WMCreatePLString(execStr);
    WMPropList* key = WMCreatePLString("editor");

    dict = WMGetFromPLDictionary(filesDB, dictKey);
    if (dict && WMIsPLDictionary(dict))
    {
	WMPutInPLDictionary(dict, key, val);
    }
    else
	fprintf(stderr,"%s %d\n\n", __FILE__, __LINE__);
	
}

void
SetViewerStringForExtn(char *extn, char *execStr)
{
    WMPropList* dict = NULL;
    WMPropList* dictKey = WMCreatePLString(extn);
    WMPropList* val = WMCreatePLString(execStr);
    WMPropList* key = WMCreatePLString("viewer");

    dict = WMGetFromPLDictionary(filesDB, dictKey);
    if (dict && WMIsPLDictionary(dict))
    {
	WMPutInPLDictionary(dict, key, val);
    }
    else
	fprintf(stderr,"%s %d\n\n", __FILE__, __LINE__);

}

void
SetObjectForKey(WMPropList* object, char *keyName)
{
    WMPropList* key = WMCreatePLString(keyName);

    WMPutInPLDictionary(filesDB, key, object);
    WMReleasePropList(key);    
}

void
SetIntegerForKey(int i, char *key)
{
    WMPropList* object;
    char buffer[128];

    sprintf(buffer, "%i", i);
    object = WMCreatePLString(buffer);

    SetObjectForKey(object, key);
    WMReleasePropList(object);
}

void
SetStringForKey(char *str, char *key)
{
    WMPropList* object;

    object = WMCreatePLString(str);

    SetObjectForKey(object, key);
    WMReleasePropList(object);
}

int 
main(int argc, char **argv)
{
    int result = 0;
    char *icon = NULL;

    loadConfigurationFile();

    if (filesDB)
    {
	int numElem, i;
	int execArrayFound = 0;
	int extnArrayFound = 0;
	WMPropList* key   = NULL;
	WMPropList* val   = NULL;
	WMPropList* tmp   = NULL;
	WMPropList* dict  = NULL;
	WMPropList* array = WMGetPLDictionaryKeys(filesDB);
	WMPropList* extnArray = WMGetFromPLDictionary(filesDB, 
						    WMCreatePLString("EXTN"));
	WMPropList* execArray = WMGetFromPLDictionary(filesDB, 
						    WMCreatePLString("EXE"));
	WMPropList* diskDict  = WMGetFromPLDictionary(filesDB, 
						    WMCreatePLString("DISKS"));

	if (!extnArray || !WMIsPLArray(extnArray))
	{
	    extnArray = WMCreatePLArray(NULL, NULL);
	    extnArrayFound = 0;
	}
	else
	    extnArrayFound = 1;

	if (!execArray || !WMIsPLArray(execArray))
	{
	    execArray = WMCreatePLArray(NULL, NULL);
	    execArrayFound = 0;
	}
	else
	    execArrayFound = 1;

	if (array && WMIsPLArray(array))
	{
	    char *extn = NULL;
	    char *exe  = NULL;
	    char *execStr = NULL;

	    numElem = WMGetPropListItemCount(array);
	    for (i = 0; i < numElem; i++)
	    {
		tmp = WMGetFromPLArray(array, i);
		extn = WMGetPropListDescription(tmp, True);
		if (extn)
		{
		    exe = GetViewerStringForExtn(extn);
		    if (exe)
		    {
			/* Add to extn list */
			InsertArrayElement(extnArray, WMCreatePLString(extn));
			continue;
		    } /* End if (exe) */

		    exe = GetEditorStringForExtn(extn);
		    if (exe)
		    {
			/* Add to extn list */
			InsertArrayElement(extnArray, WMCreatePLString(extn));
			continue;
		    } /* End if (exe) */

		    exe = GetExecStringForExtn(extn);
		    if (exe)
		    {
			/*Add to Exe list*/
			InsertArrayElement(execArray, WMCreatePLString(extn));
			continue;
		    } /* End if (exe) */
		} /* End if (extn) */
	    } /* End for (i;;) */
	}

	if (!extnArrayFound)
	    SetObjectForKey(extnArray, "EXTN");
	if (!execArrayFound)
	    SetObjectForKey(execArray, "EXE");
	/* DISKS setting */
	if (diskDict && WMIsPLDictionary(diskDict))
	{
	    WMPropList* devices = NULL;
	    WMPropList* mount = NULL;
	    WMPropList* umount = NULL;
	    WMPropList* eject = NULL;

	    devices = WMGetFromPLDictionary(diskDict, WMCreatePLString("devices"));
	    mount = WMGetFromPLDictionary(diskDict, WMCreatePLString("mount"));
	    umount = WMGetFromPLDictionary(diskDict, WMCreatePLString("umount"));
	    eject = WMGetFromPLDictionary(diskDict, WMCreatePLString("eject"));

	    if (devices/*  && WMIsPLArray(devices) */)
	    { 
		numElem = WMGetPropListItemCount(devices);
		for (i = 0; i < numElem; i++)
		{
		    tmp = WMGetFromPLArray(devices, i);
		    if (tmp && WMIsPLArray(tmp))
		    {
			InsertArrayElement(tmp, mount);
			InsertArrayElement(tmp, umount);
			InsertArrayElement(tmp, eject);
			InsertArrayElement(tmp, WMCreatePLString("close %s"));
		    }
		}
		SetObjectForKey(devices, "DISCS");
	    }
	}

	SetIntegerForKey(0, "DisplayMCListPixmap");
	
	WMWritePropListToFile(filesDB,
                wdefaultspathfordomain("FSViewer"), True);
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
