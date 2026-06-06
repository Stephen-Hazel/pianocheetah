// initme.h - setup fer pcheetah
#pragma once

#include "stv/ui.h"
#include "ui_initme.h"
#include "QThread"

extern TStr  DirPC;
extern char  Buf [800000000];
extern ubyt4 Len;

class Setup: public QThread {
   Q_OBJECT
public:
   Setup () {start ();}
  ~Setup ()  {}

   void run () override
   { TStr dir, fn;
     File f;
      StrCp (dir, DirPC);
DBGTH("InitMe_Downloader");   DBG("dir=`s", dir);
      Len = WGet (Buf, sizeof (Buf),
         CC("https://pianocheetah.app/download/pianocheetah.tar.gz"));
DBG("wget done");
      StrFmt (fn, "`s.tar.gz", dir);
      f.Save (fn, Buf, Len);
DBG("Len=`d size=`d", Len, f.Size (fn));
      Zip (dir, 'x');
DBG("unzip done");
      Gui.Quit ();
DBG("DONE");
   }
};


QT_BEGIN_NAMESPACE
namespace Ui { class InitMe; }
QT_END_NAMESPACE

class InitMe: public QMainWindow {
   Q_OBJECT
private:
   Ui::InitMe *ui;

public:
   InitMe (QWidget *par = nullptr)
   : QMainWindow (par), ui (new Ui::InitMe)  {ui->setupUi (this);}

  ~InitMe ()   {delete ui;}

   void Init (), Quit (), Setup ();
};
