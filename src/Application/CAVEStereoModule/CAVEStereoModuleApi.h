// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef incl_CAVEStereoModuleApi_h
#define incl_CAVEStereoModuleApi_h

#if defined (_WINDOWS)
#if defined(CAVESTEREO_MODULE_EXPORTS) 
#define CAVESTEREO_MODULE_API __declspec(dllexport)
#else
#define CAVESTEREO_MODULE_API __declspec(dllimport) 
#endif
#else
#define CAVESTEREO_MODULE_API
#endif

#endif
