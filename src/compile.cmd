set QTDIR=c:\qt-3
set MINGW=c:\mingw
set PATH=%QTDIR%\bin;%MINGW%\bin;%PATH%
set QMAKESPEC=win32-g++
cd lib\portaudio
mingw32-make -f Makefile.cygwin
copy pa_win_wmme\portaudio.dll ..\..
cd ..\..
mingw32-make -f Makefile.win