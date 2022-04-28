#include <WINGs/WINGsP.h>
#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>

#include "DnD.h"
#include "FSFileButton.h"
#include "FSFileView.h"
#include "files.h"

void BeganDrag(WMView* self, WMPoint* point)
{
    wwarning("Began drag");
}

void EndedDrag(WMView* self, WMPoint* point, Bool deposited)
{
    if (deposited) {
        // Ok, successful drag. Receiver will take care.
        return;
    }

    FSFileButton* btn = (FSFileButton*)WMWidgetOfView(self);
    if (WMWidgetClass(btn) != FileButtonWidgetClass()) {
        return;
    }

    // TODO: We (wrongly) assume this is a shelf FSFileButton
    //       that has its FSFileView attached. For FilePath's
    //       buttons, this is not true.
    FSFileView* fView = (FSFileView*)WMGetHangedData(btn);
    if (!fView) {
        return;
    }

    FSFileViewRemoveFileButtonFromShelf(fView, btn);
}

WMData* FetchDragData(WMView* self, char* type)
{
    wwarning("Somebody requested drag data of type %s", type);

    if (!self) {
        return NULL;
    }

    WMWidget* btn = WMWidgetOfView(self);
    if (!btn) {
        return NULL;
    }

    FileInfo* fileInfo = FSGetFileButtonFileInfo(btn);
    if (!fileInfo) {
        return NULL;
    }

    // TODO
    char buf[2048 + 1];
    memset((void*)buf, 0, 2048 + 1);
    snprintf(buf, 2048, "file://%s%s\r\n", fileInfo->path, fileInfo->name);

    wwarning(buf);

    return WMCreateDataWithBytes(buf, strlen(buf));
}

static WMArray* dataTypes = NULL;
WMArray* SupportedDataTypes()
{
    if (!dataTypes) {
        dataTypes = WMCreateArray(1);
        WMSetInArray(dataTypes, 0, &("text/uri-list"));
    }
    return dataTypes;
}

WMArray* DropDataTypes(WMView* self)
{
    wwarning("supported data types requested");
    return SupportedDataTypes();
}

Bool AcceptDropOperation(WMView* self, WMDragOperationType operation)
{
    return True;
}

WMDragOperationType WantedDropOperation(WMView* self)
{
    return WDOperationCopy;
}

WMDragSourceProcs* CreateDragSourceProcs()
{
    WMDragSourceProcs* r = wmalloc(sizeof(WMDragSourceProcs));
    memset((void*)r, 0, sizeof(WMDragSourceProcs));
    r->acceptDropOperation = AcceptDropOperation;
    // Can be NULL if we don't return WDOAskedOperations in wantedDropOperation
    r->askedOperations = NULL;
    r->beganDrag = BeganDrag;
    r->dropDataTypes = DropDataTypes;
    r->endedDrag = EndedDrag;
    r->fetchDragData = FetchDragData;
    r->wantedDropOperation = WantedDropOperation;

    return r;
}

void PrepareForDragOperation(WMView* self)
{
    wwarning("DROPZONE: Prepare for drag");
}

WMArray* RequiredDataTypes(WMView* self, WMDragOperationType request,
    WMArray* sourceDataTypes)
{
    wwarning("DROPZONE: supported data types requested");
    return SupportedDataTypes();
}

WMDragOperationType AllowedDropOperation(WMView* self,
    WMDragOperationType request,
    WMArray* sourceDataTypes)
{
    wwarning("DROPZONE: allowed op %d? We'll answer with %d", request, WDOperationLink);

    return WDOperationLink;
}

Bool InspectDropData(WMView* self, WMArray* dropData)
{
    wwarning("DROPZONE: Inspecting drop data");
    return True;
}

WMData* FirstNonNullDataItem(WMArray* array)
{
    WMArrayIterator iter = 0;
    WMData* data;

    WM_ITERATE_ARRAY(array, data, iter)
    {
        if (data) {
            return data;
        }
    };

    return NULL;
}

int Index(WMData* data, char ch)
{
    char* bytes = WMDataBytes(data);
    unsigned int len = WMGetDataLength(data);

    for (int i = 0; i < len; i++) {
        if (*(bytes + i) == ch) {
            return i;
        }
    }

    return -1;
}

char* FirstPath(WMData* data)
{
    char* bytes = WMDataBytes(data);
    unsigned int len = WMGetDataLength(data);

    if (len < 7) {
        return NULL;
    }

    if (strncmp(bytes, "file://", 7) != 0) {
        return NULL;
    }

    int pos = Index(data, '\r');
    if (pos < 7) {
        if ((pos = Index(data, '\r')) < 7) {
            return NULL;
        }
    }

    char* r = (char*)wmalloc(pos - 7 + 1);
    strncpy(r, bytes + 7, pos - 7);
    r[pos - 7] = '\0';
    return r;
}

void PerformDragOperation(WMView* self, WMArray* dropData, WMArray* operations, WMPoint* dropLocation)
{
    wwarning("DROPZONE: Performing drop operation");

    WMWidget* widget = WMWidgetOfView(self);
    FSFileView* fView = WMGetHangedData(widget);

    if (!fView) {
        return;
    }

    // Dropped onto the free space of the shelf
    if (WMWidgetClass(widget) == WC_Frame) {
        WMData* data = FirstNonNullDataItem(dropData);
        if (!data) {
            return;
        }

        // TODO: Iterate through all paths
        char* d = FirstPath(data);
        if (!d) {
            return;
        }

        FileInfo* fileInfo = FSGetFileInfo(d);
        if (fileInfo) {
            if (FSAddFileViewShelfItemIntoProplist(fView, fileInfo)) {
                FSAddFileViewShelfItem(fView, fileInfo);
            }
        }
        wfree(d);
    }
}

void ConcludeDragOperation(WMView* self)
{
    wwarning("DROPZONE: Conclude drag");
}

WMDragDestinationProcs* CreateDragDestinationProcs()
{
    WMDragDestinationProcs* r = wmalloc(sizeof(WMDragDestinationProcs));
    memset((void*)r, 0, sizeof(WMDragDestinationProcs));

    r->prepareForDragOperation = PrepareForDragOperation;
    r->requiredDataTypes = RequiredDataTypes;
    r->allowedOperation = AllowedDropOperation;
    r->inspectDropData = InspectDropData;
    r->performDragOperation = PerformDragOperation;
    r->concludeDragOperation = ConcludeDragOperation;

    return r;
}