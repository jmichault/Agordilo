unix:LIBS	+= lib/portaudio/lib/libportaudio.a -ljack -lasound
win32:LIBS        += ./lib/portaudio/pa_win_wmme/libportaudio.dll.a

INCLUDEPATH	+= lib/portaudio/include
QT +=  qt3support 
HEADERS       = form1.h fabout.h fhelp.h
RELEASE=1.0.3
VERSION=-$$RELEASE
SOURCES	+= FFT.cpp \
	Spectrum.cpp \
	audiostreams.cpp \
	main.cpp \
	form1.cpp fabout.cpp fhelp.cpp
RESOURCES     = accordeur.qrc
FORMS=form1.ui fabout.ui fhelp.ui
UI_DIR = ui
MOC_DIR = moc
DISTFILES=images/*.png
DISTFILES+=instruments.txt
DISTFILES+=temperaments.txt
DISTFILES+=doc/[!CVS]*
DISTFILES+=*.h
DISTFILES+=accordeur.menu
DISTFILES+=accordeur.desktop
DISTFILES+=accordeur*.png
DISTFILES+= `cat lib.lst`
OBJECTS_DIR = obj
TRANSLATIONS = accordeur_fr.ts accordeur_ge.ts
unix:libportaudio.target = lib/portaudio/lib/libportaudio.a
win32:libportaudio.target = .\lib\portaudio\pa_win_wmme\libportaudio.dll.a
unix:libportaudio.commands = cd lib/portaudio;make
win32:libportaudio.commands = mingw32-make -C lib/portaudio -f Makefile.mingw32
libportaudio.depends =  ./lib/portaudio/src/common/pa_stream.c
DEFINES += VERSION=\\\"$$RELEASE\\\"

QMAKE_EXTRA_TARGETS += libportaudio
unix:PRE_TARGETDEPS += lib/portaudio/lib/libportaudio.a
