#include <QtGui>
#include <QLabel>
#include "fabout.h"

FAbout::FAbout()
	    : QDialog()
{
	    ui.setupUi(this);
}

void FAbout::on_pushButton4_clicked()
{
close();
}
