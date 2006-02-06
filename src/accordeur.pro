TEMPLATE	= app
LANGUAGE	= C++

CONFIG  += qt warn_on thread

unix:LIBS	+= lib/portaudio/pa_unix_oss/libportaudio.a
win32:LIBS        += ./lib/portaudio/pa_win_wmme/libportaudio.dll.a

INCLUDEPATH	+= lib/portaudio/pa_common

SOURCES	+= FFT.cpp \
	Spectrum.cpp \
	audiostreams.cpp \
	main.cpp

FORMS	= form1.ui \
	fabout.ui \
	fhelp.ui

IMAGES	= images/1leftarrow.png \
	images/1rightarrow.png \
	images/gohome.png

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
TRANSLATIONS = accordeur_fr.ts accordeur_ge.ts
unix:libportaudio.target = lib/portaudio/pa_unix_oss/libportaudio.a
win32:libportaudio.target = .\lib\portaudio\pa_win_wmme\libportaudio.dll.a
unix:libportaudio.commands = cd lib/portaudio;make
win32:libportaudio.commands = mingw32-make -C lib/portaudio -f Makefile.mingw32
libportaudio.depends =  ./lib/portaudio/pa_common/pa_lib.c ./lib/portaudio/pa_unix_oss/pa_unix_oss.c

QMAKE_EXTRA_TARGETS += libportaudio
unix:PRE_TARGETDEPS += lib/portaudio/pa_unix_oss/libportaudio.a
win32:PRE_TARGETDEPS += "./lib/portaudio/pa_win_wmme/libportaudio.dll.a"

#The following line was inserted by qt3to4
QT +=  qt3support 
#The following line was inserted by qt3to4
CONFIG += uic3

