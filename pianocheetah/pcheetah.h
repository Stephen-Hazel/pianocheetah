// pcheetah.h - PianoCheetah - Steve's weird midi sequencer

#pragma once
#include "ui_pcheetah.h"
#include "ui_dlgcfg.h"
#include "ui_dlgfl.h"
#include "ui_dlgtdr.h"
#include "ui_dlgcue.h"
#include "ui_dlgqua.h"
#include "ui_dlgchd.h"
#include "ui_dlgctl.h"
#include "ui_dlgtpo.h"
#include "ui_dlgtsg.h"
#include "ui_dlgksg.h"
#include "ui_dlgmov.h"
#include "ui_dlghlp.h"
#include "ctlNt.h"
#include "song.h"

extern TStr Kick;                      // kick MidiCfg on exit ?

QT_BEGIN_NAMESPACE                     // we gotta lotta dang windowz
namespace Ui { class DlgFL;    class DlgCfg;
               class DlgTDr;   class DlgCue;
               class DlgQua;   class DlgChd;
               class DlgCtl;   class DlgTpo;
               class DlgTSg;   class DlgKSg;
               class DlgMov;   class DlgHlp;
               class PCheetah; }
QT_END_NAMESPACE


class DlgFL: public Dlg {
   Q_OBJECT
public:
   explicit DlgFL (QWidget *parent = nullptr): Dlg (parent, "DlgFL")
   {  ui = new Ui::DlgFL;   ui->setupUi (this);  }
  ~DlgFL ()         {delete ui;}

   void Init () override;
   void Open () override;
   void Shut () override;

public slots:
   void ReDo ();
   void Pik  ();
   void Find ();
   void Up   ();
   void Dn   ();
   void Song2Wav ();
   void Sfz2Syn  ();
   void Mod2Song ();
   void MidImp   ();
   void Brow     ();

private:
   Ui::DlgFL  *ui;
   CtlTablDDr _t;
};


class DlgCfg: public Dlg {
   Q_OBJECT
public:
   explicit DlgCfg (QWidget *parent = nullptr): Dlg (parent, "DlgCfg")
   {  ui = new Ui::DlgCfg;   ui->setupUi (this);  }
  ~DlgCfg ()         {delete ui;}

   void Init () override;
   void Open () override;
   void Shut () override;

signals:
   void sgCmd (char *s);

private:
   Ui::DlgCfg *ui;
};


class DlgTDr: public Dlg {
   Q_OBJECT
public:
   explicit DlgTDr (QWidget *parent = nullptr): Dlg (parent, "DlgTDr")
   {  ui = new Ui::DlgTDr;   ui->setupUi (this);  }
  ~DlgTDr ()         {delete ui;}

   void Init () override;
   void Open () override;

public slots:
   void Cmd ();

signals:
   void sgCmd (char *s);

private:
   Ui::DlgTDr *ui;
   CtlTabl    _t;
};


class DlgCue: public Dlg {
   Q_OBJECT
public:
   explicit DlgCue (QWidget *parent = nullptr): Dlg (parent, "DlgCue")
   {  ui = new Ui::DlgCue;   ui->setupUi (this);  }
  ~DlgCue ()         {delete ui;}

   void Init () override;
   void Open () override;

public slots:
   void Set (const char *s);

signals:
   void sgCmd (char *s);

private:
   Ui::DlgCue *ui;
};


class DlgQua: public Dlg {
   Q_OBJECT
public:
   explicit DlgQua (QWidget *parent = nullptr): Dlg (parent, "DlgQua")
   {  ui = new Ui::DlgQua;   ui->setupUi (this);  }
  ~DlgQua ()         {delete ui;}

   void Init () override;
   void Open () override;

public slots:
   void Set (char f);

signals:
   void sgCmd (char *s);

private:
   TStr _s;
   Ui::DlgQua *ui;
};


class DlgChd: public Dlg {
   Q_OBJECT
public:
   explicit DlgChd (QWidget *parent = nullptr): Dlg (parent, "DlgChd")
   {  ui = new Ui::DlgChd;   ui->setupUi (this);  }
  ~DlgChd ()         {delete ui;}

   void Init () override;
   void Open () override;
   void Shut () override;

public slots:
   void Cmd (char *s);

signals:
   void sgCmd (char *s);

private:
   ulong _cp, _tm, _tm1;
   bool  _got;
   void ReDo ();
   void Set  (char *s);
   void UnDo ();
   void Pop  ();
   Ui::DlgChd *ui;
};


class DlgCtl: public Dlg {
   Q_OBJECT
public:
   explicit DlgCtl (QWidget *parent = nullptr): Dlg (parent, "DlgCtl")
   {  ui = new Ui::DlgCtl;   ui->setupUi (this);  }
  ~DlgCtl ()         {delete ui;}

   void Init () override;
   void Open () override;
   void Shut () override;

public slots:
   void Upd ();

signals:
   void sgCmd (char *s);

private:
   Ui::DlgCtl *ui;
   CtlTabl    _t;
};


class DlgTpo: public Dlg {
   Q_OBJECT
public:
   explicit DlgTpo (QWidget *parent = nullptr): Dlg (parent, "DlgTpo")
   {  ui = new Ui::DlgTpo;   ui->setupUi (this);  }
  ~DlgTpo ()         {delete ui;}

   void Open () override;
   void Shut () override;

signals:
   void sgCmd (char *s);

private:
   Ui::DlgTpo *ui;
   TStr       _s;
};


class DlgTSg: public Dlg {
   Q_OBJECT
public:
   explicit DlgTSg (QWidget *parent = nullptr): Dlg (parent, "DlgTSg")
   {  ui = new Ui::DlgTSg;   ui->setupUi (this);  }
  ~DlgTSg ()         {delete ui;}

   void Open () override;
   void Shut () override;

signals:
   void sgCmd (char *s);

private:
   Ui::DlgTSg *ui;
   TStr       _s;
};


class DlgKSg: public Dlg {
   Q_OBJECT
public:
   explicit DlgKSg (QWidget *parent = nullptr): Dlg (parent, "DlgKSg")
   {  ui = new Ui::DlgKSg;   ui->setupUi (this);  }
  ~DlgKSg ()         {delete ui;}

   void Init () override;
   void Open () override;
   void Shut () override;

signals:
   void sgCmd (char *s);

private:
   Ui::DlgKSg *ui;
   TStr       _s;
};


class DlgMov: public Dlg {
   Q_OBJECT
public:
   explicit DlgMov (QWidget *parent = nullptr): Dlg (parent, "DlgMov")
   {  ui = new Ui::DlgMov;   ui->setupUi (this);  }
  ~DlgMov ()         {delete ui;}

   void Init () override;
   void Open () override;

private:
   Ui::DlgMov *ui;
};


class DlgHlp: public Dlg {
   Q_OBJECT
public:
   explicit DlgHlp (QWidget *parent = nullptr): Dlg (parent, "DlgHlp")
   {  ui = new Ui::DlgHlp;   ui->setupUi (this);  }
  ~DlgHlp ()         {delete ui;}

   void Init () override;
   void Quit () override;
   void Open () override;
   void Shut () override;

private:
   Ui::DlgHlp *ui;
   CtlTabl    _t;
};


class PCheetah: public QMainWindow {
   Q_OBJECT
public:
   PCheetah (QWidget *par = nullptr)
   : QMainWindow (par)
   {  ui = new Ui::PCheetah;   ui->setupUi (this);  }
  ~PCheetah ()         {delete ui;}

   void SetPix (), SetTBar (),   Init (), Quit ();

   Song *_s;

private:
   Ui::PCheetah *ui;
   CtlTBar      _tb;
   CtlTabl      _tr;
   CtlNt       *_nt;
   DlgFL       *_dFL;
   DlgCfg      *_dCfg;
   DlgTDr      *_dTDr;
   DlgCue      *_dCue;
   DlgQua      *_dQua;
   DlgChd      *_dChd;
   DlgCtl      *_dCtl;
   DlgTpo      *_dTpo;
   DlgTSg      *_dTSg;
   DlgKSg      *_dKSg;
   DlgMov      *_dMov;
   DlgHlp      *_dHlp;
   QThread      _thrSong;

   void SongKill ();

protected:
   void keyPressEvent (QKeyEvent *e);
   void changeEvent   (QEvent    *e);

public slots:
   void GCfg   ();
   void MCfg   ();
   void Load   ();
   void LoadGo ();

   void SongPrv  ();
   void SongNxt  ();
   void SongRand ();

   void Trak ();

// CtlTabl _tr stuph - also TrPop() global func
   void TrClk  ();
   void TrClkR (const QPoint &pos);
   void TrUpd  ();

   void Upd (QString upd);             // page, trk(msg_seth), etc

signals:
   void sgCmd (QString cmd);           // shhh, load, draw, cmds, etc
};
