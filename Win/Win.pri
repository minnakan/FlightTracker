#-------------------------------------------------
#  Copyright 2025 ESRI
#
#  All rights reserved under the copyright laws of the United States
#  and applicable international laws, treaties, and conventions.
#
#  You may freely redistribute and use this sample code, with or
#  without modification, provided you include the original copyright
#  notice and use restrictions.
#
#  See the Sample code usage restrictions document for further information.
#-------------------------------------------------

# Ensure we're building a Windows GUI application, not console
CONFIG -= console
CONFIG += windows

# Specify Windows subsystem explicitly
win32-msvc* {
    QMAKE_LFLAGS += /SUBSYSTEM:WINDOWS
}

win32-g++ {
    QMAKE_LFLAGS += -mwindows
}

# Resource file and icon
RC_FILE = $$PWD/Resources.rc

OTHER_FILES += \
    $$PWD/Resources.rc \
    $$PWD/AppIcon.ico

# Required Windows libraries
LIBS += \
    Ole32.lib \
    OleAut32.lib \
    Advapi32.lib \
    User32.lib \
    Gdi32.lib \
    Shell32.lib
