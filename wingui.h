#ifndef WINGUI_H
#define WINGUI_H

#ifdef USEWINGUI
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#include "resource.h"
#include "pmdx2obj.h"
#include "dxFileIO.h"

int GuiCreateWindow();
#endif

#endif /* WINGUI_H */
