SOURCES	+= FFT.cpp \
	Spectrum.cpp \
	audiostreams.cpp \
	main.cpp
unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
TRANSLATIONS = accordeur_fr.ts
FORMS	= form1.ui
TEMPLATE	=app
CONFIG	+= qt warn_on release thread
INCLUDEPATH	+= lib/portaudio/pa_common
unix:LIBS	+= lib/portaudio/pa_unix_oss/libportaudio.a
LANGUAGE	= C++
