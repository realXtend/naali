== Windows NSIS Installer Scripts ==

This folder contains tools, scripts, and third party installers required for creating an NSIS Windows installer for Tundra.

VS2008\
        MakeBuild.bat       - Makes a build folder for VS 2008 SP1 x86 build of Tundra by copying the Tundra's bin folder.
        MakeBuildX64.bat    - Makes a build folder for VS 2008 SP1 x64 build of Tundra by copying the Tundra's bin folder.
        MakeInstall.cmd     - Slave script for deploying RelWithDebInfo x86 Tundra VS 2008 SP1 build to build folder using the INSTALL project.
                              Note: Must be run from the VS 2008 Command Prompt.
        MakeInstallX64.cmd  - Deploys RelWithDebInfo x64 Tundra VS 2008 SP1 build to build folder using the INSTALL project.
                              Note: Must be run from the VS 2008 x64 Command Prompt.
        vcredist_x64.exe    - Installer for Microsoft Visual C++ 2008 SP1 Redistributable Package (x64).
        vcredist_x86.exe    - Installer for Microsoft Visual C++ 2008 SP1 Redistributable Package (x86).

VS2010\
        MakeBuild.bat       - Makes a build folder for VS 2010 SP1 x86 build of Tundra by copying the Tundra's bin folder.
        MakeBuildX64.bat    - Makes a build folder for VS 2010 SP1 x64 build of Tundra by copying the Tundra's bin folder.
        MakeInstall.cmd     - Deploys RelWithDebInfo x86 Tundra VS 2010 SP1 build to build folder using the INSTALL project.
                              Note: Must be run from the VS 2010 Command Prompt.
        MakeInstallX64.cmd  - Deploys RelWithDebInfo x64 Tundra VS 2010 SP1 build to build folder using the INSTALL project.
                              Note: Must be run from the VS 2010 x64 Command Prompt.
        vcredist_x64.exe    - Installer for Microsoft Visual C++ 2010 SP1 Redistributable Package (x64).
        vcredist_x86.exe    - Installer for Microsoft Visual C++ 2010 SP1 Redistributable Package (x86).

dxwebsetup.exe              - Web installer for Microsoft DirectX SDK.
fileassoc.nsh               - Utility script used by the tundra-installer.nsi.
MakeClean.bat               - Cleans up the build folder created by MakeBuild*.bat from typically unneeded files (debug DLLs etc.).
MakeInstall.cmd             - Master script for deploying RelWithDebInfo Tundra build to build folder using the INSTALL project.
                              Note: Must be run from the VS Command Prompt and invokek from the slave scripts in VS2008\ or VS2010\.
oalinst.exe                 - Installer for OpenAL.
RunReinstall.bat            - Reinstalls Tundra, by performing silent uninstallation of possible previous installation.
                              Note: the script needs to be manually configured in order to work!
TundraExtHandler(Debug).reg - Creates Windows context menu items for opening up Tundra scene files in your release (debug) Tundra
                              development build. Note: Before merging the file, you need to replace the file paths in the file
                              manually in order to make it work!
tundra-installer.nsi        - NSIS script for making a Tundra installer. Make sure you have run the right MakeBuild script before running this.
