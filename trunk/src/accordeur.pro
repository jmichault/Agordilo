TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release thread

unix:LIBS	+= lib/portaudio/pa_unix_oss/libportaudio.a

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
