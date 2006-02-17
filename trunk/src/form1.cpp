#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#include <QtGui>
#include <QTextBrowser>
#include <q3canvas.h>

#include "portaudio.h"
#include "audiostreams.h"
#include "Spectrum.h"

#include "fabout.h"
#include "form1.h"
#include "fhelp.h"

#define VERSION "0.9.0"

Form1::Form1()
	    : QMainWindow()
{
    ui.setupUi(this);
    init();
}

Form1::~Form1()
{
	destroy();
}

void Form1::on_helpContentsAction_activated()
{
 FHelp *fh; 
 QString nomfic("help.html");
 QString path("/usr/share/doc/accordeur");
 QStringList paths;
 char cfic[500];
 struct stat buf;
  sprintf(cfic,"/usr/share/doc/accordeur/help_%s.html",QTextCodec::locale());
  if ( ! stat(cfic, &buf))
  {
     sprintf(cfic,"help_%s.html",QTextCodec::locale());
      nomfic=cfic;
  }
  else
  {
   char loc[10];
    strcpy(loc, QTextCodec::locale());
   char * finloc=strchr(loc,'_');
    if (finloc) finloc[0]=0;
    sprintf(cfic,"doc/help_%s.html",loc);
    if ( ! stat(cfic, &buf))
    {
      sprintf(cfic,"help_%s.html",loc);
      nomfic=cfic;
      path="doc";
    }
    else
    {
      nomfic="help.html";
      path="doc";
    }
  }
  fh=new FHelp;
  paths << "." <<"doc" <<"/usr/share/doc/accordeur";
  fh->ui.TBHelp->setSearchPaths(paths);
  fh->ui.TBHelp->setSource(nomfic);
  fh->showMaximized();
}

extern QApplication *pApp;
Q3Canvas *c;
Q3CanvasView *cv;
recordStream rstream;
playStream pstream;
int Pitch=69;
static const int MIN_DB = -96;
static const int BACKGROUND_DB = -54; // This seemed to work okay
static const int INF_DB = (MIN_DB - 1);
extern float * autocorr,autocorrLen;
extern unsigned long WindowSize;
float *Pitches;
float *Temp;
bool UseTemperament=FALSE;
float Pitch2Freq(float );
float *autocorr2;
float *bruit;

int calib_A4=0;
double cor_A4=1.0;
double cor_A4_tmp[4];

struct instrument
{
  int nbStrings;
  char names[20][4];
} *PInstr;
QRadioButton *qbuts[20];

struct instrument Instr0[]=
{
    {6,{"E2","A2","D3","G3","B3","E4"}}
};

int Name2Pitch(char * name);
float PitchToFreq(float pitch)
{
 if (UseTemperament) return Pitches[(int)pitch];
 else return Pitch2Freq(pitch);
}
long Timer=0;
void Form1::init()
{
 int i;
  for(i=0 ; i<20 ; i++)
    qbuts[i]=0;
  c= new Q3Canvas(1050,560);
  cv = new Q3CanvasView(c,ui.frameGraph);  
  cv->setFixedSize(1050,570);
  cv->show();
  PortAudioInit init;
    if (!rstream.open(init)) {
    fprintf(stderr, "Error opening recording stream: %s\n",
            rstream.getErrStr());
    exit(1);
  }
  if (!pstream.open(init)) {
    fprintf(stderr, "Error opening playing stream: %s\n",
            pstream.getErrStr());
    exit(2);
  }
  autocorr = new float[16384];
  autocorr2 = new float[16384];
  bruit = new float[16384];
  for (int i=0 ; i < 16384 ; i++)
  {
      autocorr2[i]=bruit[i]=0;
  }
  FILE * ficinstr=fopen("instruments.txt","r");
  if(!ficinstr)
   ficinstr=fopen("/usr/share/accordeur/instruments.txt","r");
  if(ficinstr)
  {
 #define MAX_LINE_LENGTH 100
   char line[MAX_LINE_LENGTH+1];
   int nameLength;
   int nbInstr=0;
    while(fgets(line,MAX_LINE_LENGTH,ficinstr))
    {
      if (
          ( line[0] == 'I' ) &&
          ( line[1] == ' ' )
         )
      {
          nameLength = strlen( line );
          while(nameLength >0 && line[nameLength-1]=='\n')
               line[--nameLength]=0;
          if(!nbInstr) ui.CBInstrument->clear();
  	nbInstr++;
  
          ui.CBInstrument->insertItem(pApp->translate("Instruments",line+2));
      }
    }
    if(nbInstr >0)
      {
      PInstr=(struct instrument*)malloc(sizeof(struct instrument)*nbInstr);
      memset(PInstr,0,sizeof(struct instrument)*nbInstr);
      fseek(ficinstr,0,0);
      int numInstr=-1;
      while(fgets(line,MAX_LINE_LENGTH,ficinstr))
      {
        if (
            ( line[1] != ' ' ) &&
            ( line[1] != '\0' ) &&
            ( line[1] != 10 )
           )
            continue;
        switch(line[0])
        {
          case 'I':
    	numInstr++;
    	PInstr[numInstr].nbStrings=0;
            break;
          case 'S':
            nameLength = strlen( line );
            while(nameLength >0
    		&& ( (line[nameLength-1]=='\n')
    			||(line[nameLength-1]==' ')))
                 line[--nameLength]=0;
    	strncpy(PInstr[numInstr].names[ PInstr[numInstr].nbStrings],line+2,4);
    	PInstr[numInstr].nbStrings++;
            break;
        }
      }
    }
    else PInstr=Instr0;
    fclose(ficinstr);
  }
  else PInstr=Instr0;
  FILE * fictemp=fopen("temperaments.txt","r");
  if(!fictemp)
   fictemp=fopen("/usr/share/accordeur/temperaments.txt","r");
  if(fictemp)
  {
   char line[MAX_LINE_LENGTH+1];
   int nameLength;
   int nbTemp=0;
    while(fgets(line,MAX_LINE_LENGTH,fictemp))
    {
      if (
          ( line[0] == 'T' ) &&
          ( (line[1] == ' ' ||line[1]=='\t') )
         )
      {
          nameLength = strlen( line );
          while(nameLength >0 && line[nameLength-1]=='\n')
               line[--nameLength]=0;
          //if(!nbTemp) CBTemperament->clear();
  	  nbTemp++;
          ui.CBTemperament->insertItem(pApp->translate("Temperaments",line+2));
      }
    }
    if(nbTemp >0)
      {
      Temp=(float *)malloc(sizeof(float)*nbTemp*120);
      memset(Temp,0,sizeof(float)*nbTemp);
      fseek(fictemp,0,0);
      int numTemp=-1;
      while(fgets(line,MAX_LINE_LENGTH,fictemp))
      {
        if (
            ( line[1] != ' ' ) &&
            ( line[1] != '\0' ) &&
            ( line[1] != '\t' ) &&
            ( line[1] != 10 )
           )
            continue;
        switch(line[0])
        {
          case 'T':
    	    numTemp++;
            break;
          case 'P':
	   char *p1=line+2;
	   char pitchName[5];
	   int i;
	    for(i=0; i<5 && (*p1!=' ') && (*p1 != '\t') && *p1 ; )
		    pitchName[i++]=*(p1++);
	    pitchName[i]=0;
           int pitch =  Name2Pitch(pitchName);
	   while(*p1==' ' || *p1=='\t') p1++;
	   float freq=strtof(p1,NULL);
	    // set this pitch :
	    Temp[numTemp*120+pitch]=freq;
	   float freq2=freq;
	   int pitch2;
	    // if not already sets, sets other octaves :
	    for(pitch2=pitch-12 ; pitch2>0 ; pitch2 -=12)
	    {
	      freq2 /=2.0;
	      if( Temp[numTemp*120+pitch2]<=0.1)
	        Temp[numTemp*120+pitch2]= freq2;
	    }
	    freq2=freq;
	    for(pitch2=pitch+12 ; pitch2 < 120 ; pitch2 +=12)
	    {
	      freq2 *=2.0;
	      if( Temp[numTemp*120+pitch2]<=0.1)
	        Temp[numTemp*120+pitch2]= freq2;
	    }
            break;
        }
      }
    }
    fclose(fictemp);
  }
  ui.CBInstrument->setCurrentItem( 0 );
  on_CBInstrument_activated( 0);
  Timer=startTimer(200);
}

void Form1::destroy()
{
  rstream.close();
  pstream.close();
}

static float level2db(SAMPLE level) {
  const SAMPLE maxLevel = 1;
  //  wxASSERT((0 < level) && (level <= maxLevel));
  return 20 * log10(level / maxLevel);
}

void Form1::timerEvent( QTimerEvent * )
{
 Q3CanvasLine* line;
 Q3CanvasRectangle* rect;
 Q3CanvasText *t;
 QBrush *tb = new QBrush( Qt::red );
 Q3CanvasItemList l=c->allItems();
 int i;
 float max2=0;
 int min1;
  i = lround(rstream.m_sample_rate / PitchToFreq(Pitch));
  min1 = i-450;
  if(min1<1) min1=1;
  // Analyze the audio
  SAMPLE max, avg_abs;
 float window_ms, bestpeak_freq, perfect_freq, centsOff;
 char * pitchname;

 bool gotSound = rstream.
      audioSoFar(window_ms, max, avg_abs,
                 bestpeak_freq, perfect_freq, centsOff,
                 pitchname);

  // calcul du spectre "amorti" :
#define PARTDERNIER 0.20
  for (i=0; i < WindowSize ; i++)
  {
    autocorr2[i] = (autocorr[i]-bruit[i])*PARTDERNIER
	           +autocorr2[i]*(1-PARTDERNIER);
  }
  for (; i < 16384 ; i++)
  {
    autocorr2[i] = autocorr2[i] *(1-PARTDERNIER);
  }
  // Print results
 bool gotPitch = false;

 float db = INF_DB;
  if (gotSound) {
      db = level2db(avg_abs);
      gotPitch = gotSound && (db > BACKGROUND_DB);
  }
  //  fréquence idéale :
  ui.TLFreqId->setText(QString("%1").arg(PitchToFreq(Pitch),6,'f',2));
  ui.TLNote->setText (pitchname);
  if (gotPitch) 
  {
    // There is non-silence, so detect pitch
    // recherche dans le spectre amorti du meilleur pic :
    double bestpeak_x = min1+1+bestPeak2(&autocorr2[min1+1], 850, rstream.m_sample_rate);
    int candidat_x=lround(bestpeak_x);
    float candidat_y =autocorr2[candidat_x];
    // y a t'il une harmonique 3/5 et 3 et pas 2/3?
    if (  autocorr2[candidat_x*5/3]>candidat_y/4
	&&autocorr2[candidat_x/3]>candidat_y/4
	&&autocorr2[candidat_x*3/2]<candidat_y/4
	)
    {// si oui, alors la bonne fréquence est *3 :
      printf("correction %lf ",rstream.m_sample_rate/bestpeak_x);
      bestpeak_x=candidat_x*8/30+bestPeak2(&autocorr2[candidat_x*8/30],candidat_x*4/30, rstream.m_sample_rate);
      printf("devient %lf\n",rstream.m_sample_rate/bestpeak_x);
    }
    //printf("mesure %lf\n",rstream.m_sample_rate/bestpeak_x);
    double bestpeak_freq2 = rstream.m_sample_rate / bestpeak_x;
    bestpeak_freq2 /= cor_A4;
    ui.TLFreq->setText(QString("%1").arg(bestpeak_freq2,6,'f',2));
    // sélection de la meilleure corde :
    if(ui.CBAuto->isChecked() && ui.tabWidget2->currentPageIndex() == 1
		    && db > BACKGROUND_DB+6)
    {
     float rap,bestrap=100;
     int bestString=ui.BGStrings->selectedId();
     int pitch;
     float fr;
     static int lastString=-1;
     static int lastString2=-2;
      for( i=0 ; i< ui.BGStrings->count() ; i++)
      {
        pitch =  Name2Pitch(PInstr[ui.CBInstrument->currentItem()].names[i]);
        fr = PitchToFreq(pitch);
	if (fr >bestpeak_freq2) rap = fr/bestpeak_freq2;
	else if(fr>1) rap = bestpeak_freq2/fr;
	else rap=200;
	if(rap <bestrap)
	{
	  bestrap=rap;
	  bestString=i;
	}
      }
      if((bestrap <3) && (bestString==lastString) && (bestString==lastString2))
      {
        ui.BGStrings->setButton(bestString);
        on_BGStrings_pressed(bestString);
      }
      lastString2=lastString;
      lastString=bestString;
    }
    if(ui.CBAuto_2->isChecked() && ui.tabWidget2->currentPageIndex() == 0
		    && db > BACKGROUND_DB+6)
    {
     int pitch;
     static int lastPitch=-1;
     static int lastPitch2=-2;
      pitch=lround(Freq2Pitch(bestpeak_freq2));
      if( (pitch==lastPitch) && (pitch==lastPitch2))
      {
	ui.SPOctave->setValue(pitch/12-1);
	ui.CBNote->setCurrentItem(pitch%12);
	on_CBNote_activated(pitch%12);
	adjWindow(Pitch);
	//printf("note trouvee=%d,%s.\n",pitch,PitchName(pitch,1));
      }
      lastPitch2=lastPitch;
      lastPitch=pitch;
    }
    if(ui.CBAuto_3->isChecked() && ui.tabWidget2->currentPageIndex() == 0
		    && db > BACKGROUND_DB+6)
    { 
     int pitch=int(Freq2Pitch(bestpeak_freq2)+0.5); // note found this time 
     int note = pitch%12; 
     int oct = pitch/12 -1; 
     static int last_pitch=-1; // to check if same note found 
     static int confidence; // repeated finding of same note builds confidence 
     if(pitch == last_pitch)
     { 
       if( Pitch-pitch == 1 || Pitch-pitch == -1 ) 
       { 
          confidence++; 
	  if(confidence>2)
	  { 
	    confidence = 0; 
	    Pitch = note+(oct+1)*12; 
	    ui.CBNote->setCurrentItem(note);
	    ui.SPOctave->setValue(oct);
	    on_CBNote_activated(pitch%12);
	    adjWindow(Pitch);
	  } 
	} 
	else 
	  confidence = 0; 
      } 
      last_pitch = pitch; 
    }
//  float bestpeak_x = bestPeak2(autocorr2, 8192, rstream.m_sample_rate);
    // recherche dans le spectre amorti du meilleur pic près de la note recherchée :
    i = lround(rstream.m_sample_rate / PitchToFreq(Pitch+2));
    max = rstream.m_sample_rate / PitchToFreq(Pitch-2);
    bestpeak_x = i+bestPeak2(&autocorr2[i], max-i+1, rstream.m_sample_rate);
    bestpeak_freq2 = rstream.m_sample_rate / bestpeak_x;
    //printf("mesure;%lf",bestpeak_freq2);
    // recherche dans le spectre amorti du meilleur pic près de l'harmonique 1/3 de la note recherchée :
    i = lround(rstream.m_sample_rate*3 / PitchToFreq(Pitch+3));
    max = rstream.m_sample_rate*3 / PitchToFreq(Pitch-3);
    double bestpeak_x3 = i+bestPeak2(&autocorr2[i], max-i+1, rstream.m_sample_rate);
    double bestpeak_freq3 = rstream.m_sample_rate / bestpeak_x3;
    // recherche dans le spectre amorti du meilleur pic près de l'harmonique 1/5 de la note recherchée :
    i = lround(rstream.m_sample_rate*5 / PitchToFreq(Pitch+4));
    max = rstream.m_sample_rate*5 / PitchToFreq(Pitch-4);
    double bestpeak_x5 = i+bestPeak2(&autocorr2[i], max-i+1, rstream.m_sample_rate);
    double bestpeak_freq5 = rstream.m_sample_rate / bestpeak_x5;
    //printf(";%lf;%lf;%lf\n",bestpeak_freq3*3,bestpeak_freq5*5,(bestpeak_freq3*3+bestpeak_freq2)/2);

    if(calib_A4 >0)
    {
      calib_A4--;
      cor_A4_tmp[calib_A4]=bestpeak_freq2/440.0;
	printf("cor_A4_tmp=%f\n",bestpeak_freq2);
      if(!calib_A4)
      {
	cor_A4= (cor_A4_tmp[0]+cor_A4_tmp[1]+cor_A4_tmp[1]+cor_A4_tmp[1])/4.0;
	printf("cor_A4=%f\n",cor_A4);
      }
    }
    
    bestpeak_freq2 /= cor_A4;
    
    ui.TLFreq3->setText(QString("%1").arg(bestpeak_freq2,6,'f',2));
    double cent = log10f(bestpeak_freq2 / PitchToFreq(Pitch))*1200/log10f(2);
    ui.TLCent->setText(QString("%1").arg(cent,5,'f',2));
    int iCent=lround((cent+100)*4);
    if (iCent < 0) iCent= 0;
    if (iCent >  800) iCent=  800;
    if (iCent <380)
    {
       rect  = new Q3CanvasRectangle(iCent,280,399-iCent,20,c);
       rect->setPen( QPen(QColor( 250, 20, 20), 1) );
       tb->setColor(QColor( 250, 20, 20));
    }
    else if (iCent>420)
    {
       rect  = new Q3CanvasRectangle(421,280,iCent-420,20,c);
       rect->setPen( QPen(QColor( 250, 20, 20), 1) );
       tb->setColor(QColor( 250, 20, 20));
    }
    else
    {
       rect  = new Q3CanvasRectangle(iCent,280,20,20,c);
       iCent = 90+abs(iCent-400)*4;
       rect->setPen( QPen(QColor( 250, 20, 20), 1) );
       tb->setColor(QColor( 20, 255-iCent,  20));
    }
    rect->setBrush(*tb);
    rect->show();
    rect  = new Q3CanvasRectangle(399,279,22,22,c);
    rect->setPen( QPen(QColor( 0, 0, 0), 1) );
    rect->show();
  /*
    // harmonique 1/3 TODO
    i = (rstream.m_sample_rate*3) / PitchToFreq(Pitch+2);
    max = (rstream.m_sample_rate*3) / PitchToFreq(Pitch-2);
    bestpeak_x = i+bestPeak2(&autocorr2[i], max-i+1, rstream.m_sample_rate);
    bestpeak_freq2 = rstream.m_sample_rate / bestpeak_x;
    // harmonique 1/5 TODO
    i = (rstream.m_sample_rate*5) / PitchToFreq(Pitch+2);
    max = (rstream.m_sample_rate*5) / PitchToFreq(Pitch-2);
    bestpeak_x = i+bestPeak2(&autocorr2[i], max-i+1, rstream.m_sample_rate);
    bestpeak_freq2 = rstream.m_sample_rate / bestpeak_x;
    */
  }
    
    // nettoyage du canevas :
  for (Q3CanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) 
    {
	delete *it;
    }
  // trait rouge de la note :
  line  = new Q3CanvasLine(c);
  i = lround(rstream.m_sample_rate / PitchToFreq(Pitch));
  line->setPen( QPen(QColor( 208, 20, 20), 1) );
  line->setPoints(900-i+min1, 250,
                             900-i+min1, 0);
  line->show();
  // trait vert note précédente :
  i = lround(rstream.m_sample_rate / PitchToFreq(Pitch-1));
  line  = new Q3CanvasLine(c);
  line->setPen( QPen(QColor( 20, 208, 20), 1) );
  line->setPoints(900-i+min1, 250,
                             900-i+min1, 0);
  line->show();
  // trait vert note suivante :
  i = lround(rstream.m_sample_rate / PitchToFreq(Pitch+1));
  line  = new Q3CanvasLine(c);
  line->setPen( QPen(QColor( 20, 208, 20), 1) );
  line->setPoints(900-i+min1, 250,
                             900-i+min1, 0);
  line->show();
  // trait rouge harmoniques 1/3 1/5 ... :
  for (int j=3 ; j <= 15 ; j+=2)
  {
    i = lround(rstream.m_sample_rate / PitchToFreq(Pitch)*j);
    if ((900-i+min1)>0)
    {
      line  = new Q3CanvasLine(c);
      line->setPen( QPen(QColor( 208, 20, 20), 1) );
      line->setPoints(900-i+min1, 250,
                               900-i+min1, 0);
      line->show();
    }
  }
  for ( int i=min1 ; i <min1+890;i++)
  {
      if (autocorr2[i]>max2) max2=autocorr2[i];
  }
  for ( int i=min1 ; i <min1+890;i++)
  {
    if(autocorr2[i]>0)
    {
      line  = new Q3CanvasLine(c);
      line->setPen( QPen(QColor( 20, 20, 208), 1) );
      line->setPoints(900-i+min1, 250,
                             900-i+min1, lround(250 - autocorr2[i]/max2*180));
      line->show();
    }
  }
  // traits gris repères :
float freq;
  for ( int i=min1 ; i <min1+890;i += 50)
  {
    freq=rstream.m_sample_rate/i;
    t = new Q3CanvasText(c);
    t->setText(QString("%1").arg(freq,5,'f',0));
    t->move( 880-i+min1 , 252);
    t->show();
      line  = new Q3CanvasLine(c);
      line->setPen( QPen(QColor( 20, 20, 20), 1) );
      line->setPoints(900-i+min1, 255,
                             900-i+min1, 250);
      line->show();
  }
  c->update();
}

void Form1::adjWindow( int newPitch)
{
 int oldWindowSize = WindowSize;
  WindowSize = 16384 >>  ((newPitch-12)/12);
  if(WindowSize <1024) WindowSize=1024;  
  if (oldWindowSize <=8192 && WindowSize >8192)
  {
      killTimer(Timer);
      Timer=startTimer(400);
  }
  else if (oldWindowSize > 8192 && WindowSize <= 8192)
  {
      killTimer(Timer);
      Timer=startTimer(200);
  }
}

void Form1::on_SPOctave_valueChanged( int newval)
{
  Pitch = newval*12 + ui.CBNote->currentItem()+12;
  adjWindow(Pitch);
}




void Form1::on_CBNote_activated( int note)
{
  Pitch = ui.SPOctave->value()*12 + note+12;  
  ui.SNote->setValue(note);
}


void Form1::on_SNote_valueChanged( int note)
{
  ui.CBNote->setCurrentItem(note);
  Pitch = ui.SPOctave->value()*12 + note+12;  
}


void Form1::on_PBBruit_clicked()
{
 unsigned long oldWindowSize=WindowSize;
 SAMPLE max, avg_abs;
 float window_ms, bestpeak_freq, perfect_freq, centsOff;
 char * pitchname;
 int i;
  killTimer(Timer);
  WindowSize=16384;
  rstream.
      audioSoFar(window_ms, max, avg_abs,
                 bestpeak_freq, perfect_freq, centsOff,
                 pitchname);
  for ( i = 0 ; i <16384 ; i++)
  {
    if( autocorr[i] >0)
      bruit[i] = autocorr[i];
    else
      bruit[i] = 0;
  }
  WindowSize=oldWindowSize;
  if(WindowSize >8192)
      Timer=startTimer(400);
  else
      Timer=startTimer(200);
}

int Name2Pitch(char * name)
{
  int pitch=0;
    if(!name) return 0;
   switch ( name[0] )
   {
     case 'C':  pitch = 0; break;
     case 'D':  pitch = 2; break;
     case 'E':  pitch = 4; break;
     case 'F':  pitch = 5; break;
     case 'G':  pitch = 7; break;
     case 'A':  pitch = 9;  break;
     case 'B':  pitch = 11; break;
   }
   name++;
   switch ( name[0] )
   {
      case '#' : pitch++; name++; break;
      case 'b' : pitch--; name++; break;
   }
   pitch += atoi(name)*12+12;
   return pitch;
}

void Form1::on_BGStrings_pressed( int id)
{
//   Pitch =  Name2Pitch(strings[CBInstrument->currentItem()][id]);
   Pitch =  Name2Pitch(PInstr[ui.CBInstrument->currentItem()].names[id]);
//   printf("sel=%d:%s.\n",id,PInstr[CBInstrument->currentItem()].names[id]);
   adjWindow(Pitch);
//    printf("bouton %d,instr %d, nom %s, pitch %d\n",id,CBInstrument->currentItem(), strings[CBInstrument->currentItem()][id], Pitch);
}


void Form1::on_tabWidget2_currentChanged( QWidget * )
{
  if (ui.tabWidget2->currentPageIndex() == 0)
  {
    Pitch = ui.SPOctave->value()*12 + ui.CBNote->currentItem()+12;
  }
  else if (ui.tabWidget2->currentPageIndex() == 1)
  {
    ui.BGStrings->setButton(0);
    //Pitch =  Name2Pitch(strings[CBInstrument->currentItem()][0]);
   Pitch =  Name2Pitch(PInstr[ui.CBInstrument->currentItem()].names[0]);
  }
  adjWindow(Pitch);
}


void Form1::on_fileExitAction_activated()
{
    QApplication::exit(0);
}


void Form1::on_helpAboutAction_activated()
{
FAbout f2;    
QString  text0=f2.ui.textLabel1->text();
  text0.replace("x.y.z",VERSION);
  f2.ui.textLabel1->setText(text0);
  f2.exec();
}

void Form1::on_helpIndexAction_activated()
{
 FHelp *fh; 
 QString nomfic("index.html");
 QString path("/usr/share/doc/accordeur");
 QStringList paths;
 char cfic[500];
 struct stat buf;
  sprintf(cfic,"/usr/share/doc/accordeur/index_%s.html",QTextCodec::locale());
  if ( ! stat(cfic, &buf))
  {
     sprintf(cfic,"index_%s.html",QTextCodec::locale());
      nomfic=cfic;
  }
  else
  {
   char loc[10];
    strcpy(loc, QTextCodec::locale());
   char * finloc=strchr(loc,'_');
    if (finloc) finloc[0]=0;
    sprintf(cfic,"doc/index_%s.html",loc);
    if ( ! stat(cfic, &buf))
    {
      sprintf(cfic,"index_%s.html",loc);
      nomfic=cfic;
      path="doc";
    }
    else
    {
      nomfic="index.html";
      path="doc";
    }
  }
  fh=new FHelp;
  paths << "." <<"doc" <<"/usr/share/doc/accordeur";
  fh->ui.TBHelp->setSearchPaths(paths);
  fh->ui.TBHelp->setSource(nomfic);
  fh->showMaximized();
}


void Form1::on_pBA4_clicked()
{
  ui.SPOctave->setValue(4);
  ui.CBNote->setCurrentItem(9);
  Pitch = ui.SPOctave->value()*12 + ui.CBNote->currentItem()+12;
  adjWindow(Pitch);
 calib_A4=4;
}


void Form1::on_CBInstrument_activated( int numInstr)
{
// new instrument :
 QRadioButton *qbut;
 int i;
  for(i=0 ; i<20 ; i++)
  {
    if(qbuts[i])
    {
      delete qbuts[i];
      qbuts[i]=NULL;
    }
  }
  for(i=0;i<PInstr[numInstr].nbStrings ; i++)
  {
	  qbut=new QRadioButton(PInstr[numInstr].names[i],ui.BGStrings);
	  qbut->setGeometry(10,((ui.BGStrings->height()-10)/(PInstr[numInstr].nbStrings+1))*(i+1),171,20);
	  
	  ui.BGStrings->insert(qbut,i);
	  qbut->show();
	  qbuts[i]=qbut;
  }

  
}


void Form1::on_CBTemperament_activated( int numTemp )
{
  if(numTemp==0) UseTemperament=FALSE;
  else
  {
      UseTemperament=TRUE;
      Pitches = &Temp[(numTemp-1)*120];
  }
}
