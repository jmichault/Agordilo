#include <QDialog>
#include "ui_fabout.h"

class FAbout : public QDialog
{
Q_OBJECT

public:
 FAbout();
 Ui::FAbout ui;
private slots:
void on_pushButton4_clicked();

};
