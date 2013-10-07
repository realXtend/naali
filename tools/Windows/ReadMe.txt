== Windows Build Tools and Scripts ==

This folder contains master scripts that should be invoked from slave scripts.
For build scripts specific to certain Visual Studio versions, see the VS<XXXX> folders.
For the typical development work it's recommended to build the dependencies using both
RelWithDebInfo and Debug build configurations.

As a general guideline, .cmd files must be run from the Visual Studio Command Prompt, but
.bat files can be run from the regular Command Prompt or from the File Explorer.

== Directory Structure ==

Installer\                      Tools for creating an NSIS installer for Tundra.
Mods\                           File modifications required by the build scripts.
Utils\                          Utility functionality needed by the build scripts.
VS*\                            Build scripts specific to certain Visual Studio versions.
    BuildDeps_*.cmd             Builds Tundra dependencies as 32-bit.
    BuildDepsX64_*.cmd          Builds Tundra dependencies as 64-bit. NOTE! Remember to run
                                the script from the Visual Studio x64 Command Promt.
BuildAll.cmd                    Master script for building both dependencies and Tundra from
                                command-line (using default generator and build configuration).
BuildDeps.cmd                   Master script for building dependencies from command-line.
BuildTundra_Debug.cmd           Script for building Debug Tundra from command-line.
BuildTundra_RelWithDebInfo.cmd  Script for building RelWithDebInfo Tundra from command-line.
RunCMake.cmd                    Master script for running CMake for Tundra. Should be called
                                with a valid CMage generator string, f.ex.:
                                'RunCMake "Visual Studio 10 Win64"'.
VSConfig.cmd                    Master script containing various utility variables used by
                                the build scripts.
                                Should be called from the build scripts with a valid CMake
                                generator string, f.ex. 'VSConfig "Visual Studio 10 Win64"'.
CleanBuild.bat                  Cleans up after possible previous Tundra build in order to
                                guarantee a fresh build next time.

DEPRECATED:
DeployDeps.cmd                  Script for deploying Tundra dependencies.
FetchPrebuiltDeps.cmd           Script for fetching prebuilt Tundra dependencies.
