// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						


#include <stdio.h>
#include <tchar.h>


// TODO: reference additional headers your program requires here
#include "IwUtils.h"
#include <ws2tcpip.h>
#include <stdarg.h>
#include "json_spirit.h"
#include "CmdLine.h"


#pragma warning( pop )



