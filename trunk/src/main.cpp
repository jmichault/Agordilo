/*
    copyright (C) 2004 Jean Michault

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <qapplication.h>
#include <qsqldatabase.h> 
#include <qvbox.h>
#include <qworkspace.h>
#include <qtextcodec.h>
#include "form1.h"
#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

#include "audiostreams.h"


Form1 *wMain;
int main( int argc, char ** argv )
{
 QApplication a( argc, argv );
 int ret;
        // translation file for Qt
        QTranslator qt( 0 );
        qt.load( QString( "qt_" ) + QTextCodec::locale(), "." );
        a.installTranslator( &qt );

        // translation file for application strings
        QTranslator myapp( 0 );
        if (!myapp.load( QString( "accordeur_" ) + QTextCodec::locale(), "/usr/share/accordeur" ))
        myapp.load( QString( "accordeur_" ) + QTextCodec::locale(), "." );
        a.installTranslator( &myapp );
 Form1 w;
  wMain = &w;
  w.show();
  a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
  ret = a.exec();
    
  return ret;
}
