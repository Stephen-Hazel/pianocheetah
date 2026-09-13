// initme.h - setup fer pcheetah
#pragma once

#include "stv/ui.h"
#include "ui_initme.h"
#include "QThread"

extern TStr DirPC;

inline bool UnFlac (void *ptr, char dfx, char *fn)
// convert .flac ta .WAV keepin its smpl/loop chunk, then toss the .flac
{ ubyt4 ln = StrLn (fn);
  BStr  wfn, cmd;
  File  f;
   if ( (dfx == 'f') && (ln > 5) && (! StrCm (& fn [ln-5], ".flac")) ) {
      StrCp (wfn, fn);   StrCp (& wfn [ln-5], ".WAV");
      App.Run (StrFmt (cmd,
         "flac -d --keep-foreign-metadata-if-present -f -o `p `p", wfn, fn));
      if (f.Size (wfn))  f.Kill (fn);  // only toss source if .WAV came out ok
DBG("unflac `s => `s", fn, wfn);
   }
   return false;                       // keep on walkin
}


class Setup: public QThread {
   Q_OBJECT
public:
   Setup ()  {start ();}
  ~Setup ()  {}

   void run () override
   { TStr dir, cmd, aria;
     Path p;
     File f;
      StrCp (dir, DirPC);
DBGTH("InitMe_Downloader");   DBG("dir=`s", dir);
      p.Make (dir);                    // make our pc path
      App.Run (StrFmt (cmd,            // stream download+extract
         "wget -qO- `p | tar xz --strip-components=1 -C `p",
         "https://pianocheetah.app/download/pc.tar.gz", dir));
DBG("download+extract done");
      StrFmt  (aria, "`s/device/syn/aria", dir);
      f.DoDir (aria, nullptr, & UnFlac);
DBG("unflac done");
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
