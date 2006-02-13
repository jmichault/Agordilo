#include <QDialog>
#include "ui_fhelp.h"

class FHelp : public QMainWindow
{
Q_OBJECT

public:
 FHelp();
 Ui::FHelp ui;

private slots:
void on_toolButton2_clicked();
void on_toolButton2_2_clicked();
void on_toolButton2_3_clicked();

private:

};
