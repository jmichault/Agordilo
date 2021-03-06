#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#include <QtGui>
#include <QLabel>
#include <QTextBrowser>
#include <q3canvas.h>

#include "portaudio.h"
#include "audiostreams.h"
#include "Spectrum.h"

#include "fabout.h"
#include "form1.h"
#include "fhelp.h"

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
 QString locale = QLocale::system().name();
 char loc[20];
  strcpy(loc, locale.toLocal8Bit().data());
  sprintf(cfic,"/usr/share/doc/accordeur/help_%s.html",loc);
  if ( ! stat(cfic, &buf))
  {
     sprintf(cfic,"help_%s.html",locale.toLocal8Bit().data());
      nomfic=cfic;
  }
  else
  {
   char * finloc=strchr(loc,'_');
    if (finloc) finloc[0]=0;
    sprintf(cfic,"/usr/share/doc/accordeur/help_%s.html",loc);
    if ( ! stat(cfic, &buf))
    {
      sprintf(cfic,"help_%s.html",loc);
      nomfic=cfic;
    }
    else
    {
      sprintf(cfic,"doc/help_%s.html",loc);
      if ( ! stat(cfic, &buf))
      {
        sprintf(cfic,"help_%s.html",loc);
        nomfic=cfic;
      }
      else
      {
        nomfic="help.html";
      }
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
//playStream pstream;
int Pitch=69;
static const int MIN_DB = -96;
static const int BACKGROUND_DB = -54; 
static const int INF_DB = (MIN_DB - 1);
extern float * autocorr;
extern int autocorrLen;
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
  ui.CBNote->setCurrentItem(9);
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
    /*
  if (!pstream.open(init)) {
    fprintf(stderr, "Error opening playing stream: %s\n",
            pstream.getErrStr());
    exit(2);
  }
  */
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
 FILE *ficIni=fopen("accordeur.ini","r");
  if(ficIni)
  {
   char line[MAX_LINE_LENGTH+1];
   int opt;
    while(fgets(line,MAX_LINE_LENGTH,ficIni))
    {
      if(line[0]=='#') continue;
      char *p0,*p1;
      p0=line;
      while(*p0==' ')	p0++;
      p1=p0;
      while(*p1 != '=' && *p1!=' ' && *p1!='\t' && *p1)
	      p1++;
      *p1=0;
      p1++;
      while(*p1==' ')	p1++;
      if (!strcmp(p0,"ActiveTab"))
      {
	opt=atoi(p1);
        ui.tabWidget2->setCurrentIndex(opt);
	printf("ActiveTab = %d\n",opt);
      }
      else if (!strcmp(p0,"ChromaticSelect"))
      {
	opt=atoi(p1);
	printf("ChromaticSelect = %d\n",opt);
	if(opt) ui.CBAuto->setCheckState(Qt::Checked);
	else    ui.CBAuto->setCheckState(Qt::Unchecked);
      }
      else if (!strcmp(p0,"ChromaticPitch"))
      {
	opt=atoi(p1);
	printf("ChromaticPitch = %d\n",opt);
	ui.CBNote->setCurrentItem(opt);
      }
      else if (!strcmp(p0,"ChromaticOctave"))
      {
	opt=atoi(p1);
	printf("ChromaticOctave = %d\n",opt);
        ui.SPOctave->setValue( opt );
      }
      else if (!strcmp(p0,"OtherSelect"))
      {
	opt=atoi(p1);
	printf("OtherSelect = %d\n",opt);
	switch(opt)
	{
	 case 1:
	  ui.CBAuto_3->setChecked(TRUE);
	  break;
	 case 2:
	  ui.CBAuto_2->setChecked(TRUE);
	  break;
	 default:
	  ui.CBAuto_Manual->setChecked(TRUE);
	  break;
	}
      }
      else if (!strcmp(p0,"OtherInstrument"))
      {
	opt=atoi(p1);
        ui.CBInstrument->setCurrentItem( opt );
        on_CBInstrument_activated( opt );
	printf("OtherInstrument = %d\n",opt);
      }
      else if (!strcmp(p0,"OtherString"))
      {
	opt=atoi(p1);
	qbuts[opt]->setChecked(true);
	on_qbut_clicked( );
	printf("OtherString = %d\n",opt);
      }
      else if (!strcmp(p0,"Temperament"))
      {
	opt=atoi(p1);
        ui.CBTemperament->setCurrentItem( opt );
        on_CBTemperament_activated( opt );
	printf("Temperament = %d\n",opt);
      }
      else if (!strcmp(p0,"A4Correction"))
      {
	sscanf(p1,"%lf",&cor_A4);
	printf("A4Correction = %f\n",cor_A4);
      }
    }
    fclose(ficIni);
  }
  ui.tabWidget2->setCurrentIndex(ui.tabWidget2->currentIndex());
  WindowSize=0;
  adjWindow(Pitch);
}

void Form1::destroy()
{
  rstream.close();
//  pstream.close();
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
/*
 float db = INF_DB;
  if (gotSound) {
      db = level2db(avg_abs);
      gotPitch = gotSound && (db > BACKGROUND_DB);
  }
  */
   float db = INF_DB; 
   static float maxdb = INF_DB; 
   if (gotSound) { 
	   db = level2db(avg_abs); 
	   if(db>maxdb) maxdb=db; 
	   gotPitch = gotSound && (db > maxdb-15); 
   } 
  //  fr�quence id�ale :
  ui.TLFreqId->setText(QString("%1").arg(PitchToFreq(Pitch),6,'f',2));
  //printf("gotPitch=%d, avg_abs=%lf, db=%lf,maxdb=%lf\n",gotPitch,avg_abs,db,maxdb);
    // nettoyage du canevas :
  for (Q3CanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) 
    {
	delete *it;
    }
  if (gotPitch) 
  {
    ui.TLNote->setText (pitchname);
      // There is non-silence, so detect pitch
      // recherche dans le spectre amorti du meilleur pic :
      double bestpeak_x = bestPeak2(autocorr2, autocorrLen, rstream.m_sample_rate);
      if (ui.tabWidget2->currentIndex() != 3) // not helpful with harpsichord
	{
	  int candidat_x=lround(bestpeak_x);
	  float candidat_y =autocorr2[candidat_x];
	  // y a t'il une harmonique 1/2 et 1/3 et pas 2/3?
	  if (  autocorr2[candidat_x*2]>candidat_y/4
		&&autocorr2[candidat_x*3]>candidat_y/4
		&&autocorr2[candidat_x*3/2]<candidat_y/4
		)
	    {// si oui, alors la bonne fr�quence est 1/2 :
	      //printf("correction %lf ",rstream.m_sample_rate/bestpeak_x);
	      bestpeak_x=candidat_x*19/10+bestPeak2(&autocorr2[candidat_x*19/10],candidat_x*2/10, rstream.m_sample_rate);
	      //printf("devient %lf\n",rstream.m_sample_rate/bestpeak_x);
	    }
	  // y a t'il une harmonique 3/5 et 3 et pas 2/3?
	  if (  autocorr2[candidat_x*5/3]>candidat_y/4
		&&autocorr2[candidat_x/3]>candidat_y/4
		&&autocorr2[candidat_x*3/2]<candidat_y/4
		)
	    {// si oui, alors la bonne fr�quence est *3 :
	      //printf("correction %lf ",rstream.m_sample_rate/bestpeak_x);
	      bestpeak_x=candidat_x*8/30+bestPeak2(&autocorr2[candidat_x*8/30],candidat_x*4/30, rstream.m_sample_rate);
	      //printf("devient %lf\n",rstream.m_sample_rate/bestpeak_x);
	    }
	  // y a t'il une harmonique 5 et 5/3 et pas 2/3?
	  if (  autocorr2[candidat_x/5]>candidat_y/4
		&&autocorr2[candidat_x*3/5]>candidat_y/4
		&&autocorr2[candidat_x*3/2]<candidat_y/4
		)
	    {// si oui, alors la bonne fr�quence est *5 :
	      //printf("correction %lf ",rstream.m_sample_rate/bestpeak_x);
	      bestpeak_x=candidat_x*8/50+bestPeak2(&autocorr2[candidat_x*8/50],candidat_x*4/50, rstream.m_sample_rate);
	      //printf("devient %lf\n",rstream.m_sample_rate/bestpeak_x);
	    }
	  //printf("mesure %lf\n",rstream.m_sample_rate/bestpeak_x);
	} // not for harpsichord
      double bestpeak_freq2 = rstream.m_sample_rate / bestpeak_x;
      bestpeak_freq2 /= cor_A4;
      ui.TLFreq->setText(QString("%1").arg(bestpeak_freq2,6,'f',2));
  // s�lection de la meilleure corde :
  if(ui.CBAuto->isChecked() && ui.tabWidget2->currentIndex() == 1
     && db > BACKGROUND_DB+6)
    {
      float rap,bestrap=100;
      int bestString;//TODO=ui.BGStrings->selectedId();
      int pitch;
      float fr;
      static int lastString=-1;
      static int lastString2=-2;
      for(i=0 ; i <PInstr[ui.CBInstrument->currentItem()].nbStrings-1 ; i++)
	  if (qbuts[i]->isChecked()) break;
      bestString=i;
      for( i=0 ; i < PInstr[ui.CBInstrument->currentItem()].nbStrings ; i++)
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
	  //ui.BGStrings->setButton(bestString);
	  qbuts[bestString]->setChecked(true);
	  on_qbut_clicked();
	}
      lastString2=lastString;
      lastString=bestString;
    }
  if(ui.CBAuto_2->isChecked() && ui.tabWidget2->currentIndex() == 0
     && db > BACKGROUND_DB+6)
    {
      int pitch;
      static int lastPitch=-1;
      static int lastPitch2=-2;
      //pitch=lround(Freq2Pitch(bestpeak_freq2));
      pitch=int(Freq2Pitch(bestpeak_freq2)+0.5); // note found this time 
      if( (pitch==lastPitch) && (pitch==lastPitch2))
	{
	  ui.SPOctave->setValue(pitch/12-1);
	  ui.CBNote->setCurrentItem(pitch%12);
	  on_CBNote_activated(pitch%12);
	  adjWindow(Pitch);
	  //printf("note trouvee=%d,%s,%lf.\n",pitch,PitchName(pitch,1),Pitch2Freq(pitch));
	}
      lastPitch2=lastPitch;
      lastPitch=pitch;
    }
  if(ui.CBAuto_3->isChecked() && 
     (ui.tabWidget2->currentIndex() == 0 || ui.tabWidget2->currentIndex() == 3)
     // 0 and 3 are the Chromatic and Harpsichord tabs - this is fragile code!!
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
  if (ui.tabWidget2->currentIndex() != 3) // not helpful with harpsichord
    {
      // recherche dans le spectre amorti du meilleur pic pr�s de la note recherch�e :
      i = lround(rstream.m_sample_rate / PitchToFreq(Pitch+2));
      max = rstream.m_sample_rate / PitchToFreq(Pitch-2);
      bestpeak_x = i+bestPeak2(&autocorr2[i], max-i+1, rstream.m_sample_rate);
      bestpeak_freq2 = rstream.m_sample_rate / bestpeak_x;
      //printf("mesure;%lf",bestpeak_freq2);
      int candidat_x=lround(bestpeak_x);
      float candidat_y =autocorr2[candidat_x];
      // recherche dans le spectre amorti du meilleur pic pr�s de l'harmonique 1/3 de la note recherch�e :
      if ( candidat_x >0 && autocorr2[candidat_x/3]>candidat_y/4)
	{
	  float bestpeak_x3=candidat_x*29/10+bestPeak2(&autocorr2[candidat_x*29/10],candidat_x*2/10, rstream.m_sample_rate);
	  double bestpeak_freq3 = rstream.m_sample_rate / bestpeak_x3;
	  bestpeak_freq2 = (bestpeak_freq2+bestpeak_freq3)/2;
	}
      // recherche dans le spectre amorti du meilleur pic pr�s de l'harmonique 1/5 de la note recherch�e :
      i = lround(rstream.m_sample_rate*5 / PitchToFreq(Pitch+4));
      max = rstream.m_sample_rate*5 / PitchToFreq(Pitch-4);
      double bestpeak_x5 = i+bestPeak2(&autocorr2[i], max-i+1, rstream.m_sample_rate);
      double bestpeak_freq5 = rstream.m_sample_rate / bestpeak_x5;
      //printf(";%lf;%lf;%lf\n",bestpeak_freq3*3,bestpeak_freq5*5,(bestpeak_freq3*3+bestpeak_freq2)/2);
    } // not harpsichord
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
  
  if(ui.tabWidget2->currentIndex() == 3) // large tuning bar needed
    {
      int iCent=lround((2*cent+100)*4);// value 400 is perfect, 410 = 5/2 cents sharp - I've made this a bit more sensitive
      if (iCent < 0) iCent= 0;
      if (iCent >  800) iCent=  800;
      // this is the red/green/blue bar
      if (iCent <360)// more than ten cents flat
	{
	  rect  = new Q3CanvasRectangle(iCent+20,0,359-iCent,120,c);//x,y, width,height, canvas defined in the init() function above up to x=380
	  rect->setPen( QPen(QColor( 250, 20, 20), 1) );
	  tb->setColor(QColor( 250, 20, 20));
	}
      else if (iCent>440) // more than ten cents sharp
	{
	  rect  = new Q3CanvasRectangle(481,0,iCent-420,120,c);//from x=460
	  rect->setPen( QPen(QColor( 20, 20, 250), 1) );
	  tb->setColor(QColor( 20, 20, 250)); // blue for sharp
	}
      else
	{
	  rect  = new Q3CanvasRectangle(iCent+20,0,20,320,c);
	  int colour = 90+abs(iCent-400)*2;// make a color out of it
	  rect->setPen( QPen(QColor( 250, 20, 20), 1) );
	  if(iCent<380)
	    tb->setColor(QColor( 20+colour, 255-colour,  20)); //green -ish
	  else
	    if(iCent>420)
	      tb->setColor(QColor( 20, 255-colour,  20+colour)); //green -ish
	    else
	      tb->setColor(QColor( 0, 255-colour,  0)); //green 	
	}
      rect->setBrush(*tb);
      rect->show();
      // this the centred outline rect for in tune position
      
      rect  = new Q3CanvasRectangle(379,0,102,320,c);//380 to 440
    }  else { // normal sized tuning bar needed
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
    }


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
    
  if(ui.tabWidget2->currentIndex() != 3) {
  // trait rouge de la note :
  line  = new Q3CanvasLine(c);
  i = lround(rstream.m_sample_rate / PitchToFreq(Pitch));
  line->setPen( QPen(QColor( 208, 20, 20), 1) );
  line->setPoints(900-i+min1, 250,
                             900-i+min1, 0);
  line->show();
  // trait vert note pr�c�dente :
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
  // traits gris rep�res :
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

void Form1::on_qbut_clicked()
{
 int id;
  for(id=0 ; id <PInstr[ui.CBInstrument->currentItem()].nbStrings ; id++)
	  if (qbuts[id]->isChecked()) break;
   Pitch =  Name2Pitch(PInstr[ui.CBInstrument->currentItem()].names[id]);
   //printf("sel=%d:%s.\n",id,PInstr[ui.CBInstrument->currentItem()].names[id]);
   adjWindow(Pitch);
}


void Form1::on_tabWidget2_currentChanged( QWidget * )
{
  if (ui.tabWidget2->currentIndex() == 0)
  {
    Pitch = ui.SPOctave->value()*12 + ui.CBNote->currentItem()+12;
  }
  else if (ui.tabWidget2->currentIndex() == 1)
  {
    on_qbut_clicked();
  }
  adjWindow(Pitch);
}


void Form1::on_fileSavePrefs_activated()
{
 FILE *ficInit=fopen("accordeur.ini","w");
  fprintf(ficInit,"ActiveTab=%d\n",ui.tabWidget2->currentIndex());
  fprintf(ficInit,"ChromaticSelect=%d\n",ui.CBAuto->checkState());
  fprintf(ficInit,"ChromaticPitch=%d\n",ui.CBNote->currentItem());
  fprintf(ficInit,"ChromaticOctave=%d\n",ui.SPOctave->value());
  if(ui.CBAuto_3->isChecked())
    fprintf(ficInit,"OtherSelect=1\n");
  if(ui.CBAuto_2->isChecked())
    fprintf(ficInit,"OtherSelect=2\n");
  else
    fprintf(ficInit,"OtherSelect=0\n");
  fprintf(ficInit,"OtherInstrument=%d\n",ui.CBInstrument->currentItem());
  int id;
  for(id=0 ; id <PInstr[ui.CBInstrument->currentItem()].nbStrings ; id++)
	  if (qbuts[id]->isChecked()) break;
  fprintf(ficInit,"OtherString=%d\n",id);
  fprintf(ficInit,"Temperament=%d\n",ui.CBTemperament->currentItem());
  fprintf(ficInit,"A4Correction=%f\n",cor_A4);
  fclose(ficInit);
}

void Form1::on_fileExitAction_activated()
{
    QApplication::exit(0);
}


void Form1::on_helpAboutAction_activated()
{
FAbout f2;    
QString  text0=f2.textLabel1->text();
  text0.replace("x.y.z",VERSION);
  f2.textLabel1->setText(text0);
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
 QString locale = QLocale::system().name();
 char loc[20];
  strcpy(loc, locale.toLocal8Bit().data());
  sprintf(cfic,"/usr/share/doc/accordeur/index_%s.html",loc);
  if ( ! stat(cfic, &buf))
  {
     sprintf(cfic,"index_%s.html",loc);
      nomfic=cfic;
  }
  else
  {
   char * finloc=strchr(loc,'_');
    if (finloc) finloc[0]=0;
    sprintf(cfic,"/usr/share/doc/accordeur/index_%s.html",loc);
    if ( ! stat(cfic, &buf))
    {
      sprintf(cfic,"index_%s.html",loc);
      nomfic=cfic;
    }
    else
    {
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
  killTimer(Timer);
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
	  
	  //ui.BGStrings->insert(qbut,i);
	  connect(qbut,SIGNAL( clicked()),this,SLOT(on_qbut_clicked()));
	  qbut->show();
	  qbuts[i]=qbut;
  }
  qbuts[0]->setChecked(true);
  on_qbut_clicked();
  if(WindowSize >8192)
      Timer=startTimer(400);
  else
      Timer=startTimer(200);
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
