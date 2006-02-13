#include "ui_form1.h"

class Form1 : public QMainWindow
{
Q_OBJECT

public:
 Form1();
 ~Form1();
void timerEvent( QTimerEvent *e );

private slots:

void on_helpContentsAction_activated();
void on_SPOctave_valueChanged( int newval);
void on_CBNote_activated( int note);
void on_SNote_valueChanged( int note);
void on_PBBruit_clicked();
void on_BGStrings_pressed( int id);
void on_tabWidget2_currentChanged( QWidget * );
void on_fileExitAction_activated();
void on_helpAboutAction_activated();
void on_helpIndexAction_activated();
void on_pBA4_clicked();
void on_CBInstrument_activated( int numInstr);
void on_CBTemperament_activated( int numTemp );

private:
 Ui::Form1 ui;
void init();
void destroy();
void adjWindow( int newPitch);

};
