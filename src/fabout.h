#include <QDialog>
#include "ui_fabout.h"

class FAbout : public QDialog, public Ui::FAbout
{
Q_OBJECT

public:
 FAbout();
private slots:
void on_pushButton4_clicked();

};
