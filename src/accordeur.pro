SOURCES	+= FFT.cpp \
	Spectrum.cpp \
	audiostreams.cpp \
	main.cpp
unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
FORMS	= form1.ui
TEMPLATE	=app
CONFIG	+= qt warn_on release
LANGUAGE	= C++
