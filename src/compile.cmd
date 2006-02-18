set QTDIR=c:\qt\4.1.0
set MINGW=c:\mingw
set PATH=%QTDIR%\bin;%MINGW%\bin;%PATH%
set QMAKESPEC=win32-g++
qmake
cd lib\portaudio
mingw32-make -f Makefile.mingw32
copy pa_win_wmme\portaudio.dll ..\..
cd ..\..
mingw32-make
