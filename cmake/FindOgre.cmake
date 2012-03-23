
# Finds ogre for the Tundra build system.
# UNIX/MAC: Uses sagase/find_library in linux as it works there mighty well when its installed on the system.
# WINDOWS: Uses some more custom logic on windows to find things correct either from Tunda deps, Ogre SDK or Ogre source repo clone.

if (NOT WIN32 AND NOT APPLE)
# TODO: Remove configure_ogre and replace it with a use_package_ogre() and link_package_ogre()
macro(configure_ogre)
  find_path(OGRE_LIBRARY_DIR NAMES lib/libOgreMain.so
    HINTS ${ENV_OGRE_HOME} ${ENV_NAALI_DEP_PATH})

  find_path(OGRE_INCLUDE_DIR Ogre.h
    HINTS ${ENV_OGRE_HOME}/include ${ENV_NAALI_DEP_PATH}/include
    PATH_SUFFIXES OGRE)

  find_library(OGRE_LIBRARY OgreMain
    HINTS ${ENV_OGRE_HOME}/lib ${ENV_NAALI_DEP_PATH}/lib)

  include_directories(${OGRE_INCLUDE_DIR})
  link_directories(${OGRE_LIBRARY_DIR})

endmacro()
    
else() # Windows Ogre lookup.

# TODO: Remove configure_ogre and replace it with a use_package_ogre() and link_package_ogre()
macro(configure_ogre)
    # Find and use DirectX if enabled in the build config
    if (ENABLE_DIRECTX)
        configure_directx()
        link_directx()
    else()
        message(STATUS "DirectX disabled from the build")
    endif()

    # Ogre lookup rules:
    # 1. Use the predefined OGRE_DIR CMake variable if it was set.
    # 2. Otherwise, use the OGRE_HOME environment variable if it was set.
    # 3. Otherwise, use Ogre from Tundra deps directory.

    if ("${OGRE_DIR}" STREQUAL "")
        set(OGRE_DIR $ENV{OGRE_HOME})
    endif()

    # On Apple, Ogre comes in the form of a Framework. The user has to have this manually installed.
    if (APPLE)
        if ("${OGRE_DIR}" STREQUAL "" OR NOT IS_DIRECTORY ${OGRE_DIR}/lib)
            find_library(OGRE_LIBRARY Ogre)
            set(OGRE_DIR ${OGRE_LIBRARY})
        else()
 #           set(OGRE_DIR ${OGRE_DIR}/lib/Ogre.framework) # User specified custom Ogre directory pointing to Ogre Hg trunk directory.
            set(OGRE_BUILD_CONFIG "relwithdebinfo") # TODO: We would like to link to debug in debug mode, release in release etc, not always fixed to this.
            set(OGRE_LIBRARY ${OGRE_DIR}/lib/relwithdebinfo/Ogre.framework)
        endif()
    endif()
         
    # Finally, if no Ogre found, assume the deps path.
    if ("${OGRE_DIR}" STREQUAL "")
        set(OGRE_DIR ${ENV_TUNDRA_DEP_PATH}/Ogre)
    endif()

    # The desired Ogre path is set in OGRE_DIR. The Ogre source tree comes in two flavors:
    # 1. If you cloned and built the Ogre Hg repository, OGRE_DIR can point to Hg root directory.
    # 2. If you are using an installed Ogre SDK, OGRE_DIR can point to the SDK root directory.
    # We want to support both so that one can do active development on the Ogre Hg repository, without
    # having to always do the intermediate SDK installation/deployment step.
    
    if (APPLE)# AND IS_DIRECTORY ${OGRE_DIR}/Headers) # OGRE_DIR points to a manually installed Ogre.framework?
        if (IS_DIRECTORY ${OGRE_DIR}/lib)
            include_directories(${OGRE_DIR}/lib/relwithdebinfo/Ogre.framework/Headers)
            link_directories(${OGRE_DIR}/lib/relwithdebinfo/Ogre.framework)
        else()
            include_directories(${OGRE_DIR}/Headers)
            link_directories(${OGRE_DIR})
        endif()
        message(STATUS "Using Ogre from directory " ${OGRE_DIR})
    elseif (IS_DIRECTORY ${OGRE_DIR}/OgreMain) # Ogre path points to #1 above.    
        include_directories(${OGRE_DIR}/include)
        include_directories(${OGRE_DIR}/OgreMain/include)
	if (WIN32)
       	    include_directories(${OGRE_DIR}/RenderSystems/Direct3D9/include)
        endif()
        link_directories(${OGRE_DIR}/lib)
        message(STATUS "Using Ogre from Mercurial trunk directory " ${OGRE_DIR})
    elseif (IS_DIRECTORY ${OGRE_DIR}/include/OGRE) # If include/OGRE exists, then OGRE_DIR points to the SDK (#2 above)
        include_directories(${OGRE_DIR}/include)
        include_directories(${OGRE_DIR}/include/OGRE)
        link_directories(${OGRE_DIR}/lib)
        if (WIN32)
            include_directories(${OGRE_DIR}/include/OGRE/RenderSystems/Direct3D9)        
            link_directories(${OGRE_DIR}/lib/$(OutDir)/opt)
        endif()
        message(STATUS "Using Ogre from SDK directory " ${OGRE_DIR})
    else()
        message(FATAL_ERROR "When looking for Ogre, the path ${OGRE_DIR} does not point to a valid Ogre directory!")
    endif()
endmacro()

endif()

macro(link_ogre)
    if (WIN32)
        target_link_libraries(${TARGET_NAME} debug OgreMain_d debug RenderSystem_Direct3D9_d)
        target_link_libraries(${TARGET_NAME} optimized OgreMain optimized RenderSystem_Direct3D9)
    else()
        target_link_libraries(${TARGET_NAME} ${OGRE_LIBRARY})
    endif()
endmacro()