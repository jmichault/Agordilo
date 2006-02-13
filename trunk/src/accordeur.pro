unix:LIBS	+= lib/portaudio/pa_unix_oss/libportaudio.a
win32:LIBS        += ./lib/portaudio/pa_win_wmme/libportaudio.dll.a

INCLUDEPATH	+= lib/portaudio/pa_common
QT +=  qt3support 
HEADERS       = form1.h fabout.h fhelp.h
SOURCES	+= FFT.cpp \
	Spectrum.cpp \
	audiostreams.cpp \
	main.cpp \
	form1.cpp fabout.cpp fhelp.cpp
RESOURCES     = accordeur.qrc
FORMS=form1.ui fabout.ui fhelp.ui
UI_DIR = ui
MOC_DIR = moc
OBJECTS_DIR = obj
TRANSLATIONS = accordeur_fr.ts accordeur_ge.ts
unix:libportaudio.target = lib/portaudio/pa_unix_oss/libportaudio.a
win32:libportaudio.target = .\lib\portaudio\pa_win_wmme\libportaudio.dll.a
unix:libportaudio.commands = cd lib/portaudio;make
win32:libportaudio.commands = mingw32-make -C lib/portaudio -f Makefile.mingw32
libportaudio.depends =  ./lib/portaudio/pa_common/pa_lib.c ./lib/portaudio/pa_unix_oss/pa_unix_oss.c

QMAKE_EXTRA_TARGETS += libportaudio
unix:PRE_TARGETDEPS += lib/portaudio/pa_unix_oss/libportaudio.a
