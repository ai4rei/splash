# Splash Screen
This is self-contained code that will display a bitmap splash screen for a Windows GUI application until the main window appears. The code is provided as a DLL, but can also be used as part of an application under the license terms (attribution).

## System requirements
* Windows* 98 or later (if you remove `AnimateWindow` function and install Active Accesibility 1.3 RDK, you can target Windows* 95)
* Microsoft* Visual C++ 6.0 or later (with NMAKE)
* Windows SDK (preferably Windows XP PSDK or later)

## Building
The code can be built on Visual C++ command line with `nmake`. For building as part of an application consult dependencies in the `Makefile`. Both 32-bit and 64-bit builds are supported.
