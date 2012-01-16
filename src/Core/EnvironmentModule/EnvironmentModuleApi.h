// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(ENVIRONMENT_MODULE_EXPORTS) 
#define ENVIRONMENT_MODULE_API __declspec(dllexport)
#else
#define ENVIRONMENT_MODULE_API __declspec(dllimport) 
#endif
#else
#define ENVIRONMENT_MODULE_API
#endif

