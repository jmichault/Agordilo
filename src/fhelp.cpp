#include <QtGui>
#include "fhelp.h"


FHelp::FHelp()
	    : QMainWindow()
{
	    ui.setupUi(this);
}

void FHelp::on_toolButton2_clicked()
{
  ui.TBHelp->home();
}


void FHelp::on_toolButton2_2_clicked()
{
  ui.TBHelp->backward();
}


void FHelp::on_toolButton2_3_clicked()
{
  ui.TBHelp->forward();
}
