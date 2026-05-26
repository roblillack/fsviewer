#ifndef _DND_H_
#define _DND_H_

#include <WINGs/WINGs.h>

WMDragDestinationProcs* ShelfDragDestinationProcs();
WMDragDestinationProcs* FolderDragDestinationProcs();
WMArray* SupportedDataTypes();
WMDragSourceProcs* FileViewDragSourceProcs();
WMDragSourceProcs* PathViewDragSourceProcs();

#endif
