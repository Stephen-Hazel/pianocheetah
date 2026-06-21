// pcheetah.cpp - PianoCheetah - Steve's weird midi sequencer

#include "pcheetah.h"

TStr Kick;

UCmdDef UCmd [] = {
// done by gui/pcheetah
   {"song<",     "song",       "previous song"},
   {"songRand",  "",           "random song"},
   {"song>",     "",           "next song"},
   {"songKill",  "",           "DELETE song  (be careful:)"},
   {"exit",      "",           "quit PianoCheetah"},
   {"fullScr",   "",           "view:  full screen / tracks"},
// done by thread/song/me
   {"timeBar1",  "time",       "restart"},                           // 6
   {"time<",     "",           "previous bar"},
   {"time>",     "",           "next bar"},
   {"time<<",    "",           "previous page / loop"},
   {"time>>",    "",           "next page / loop"},                  // 10
   {"timePoz",   "",           "play / pause"},
   {"timeBug",   "",           "hop to loop with most bugs"},
   {"tempoHop",  "tempo",      "hop tempo:  60% / 80% / 100%"},
   {"tempo<",    "",           "decrease tempo"},
   {"tempo>",    "",           "increase tempo"},                    // 15
   {"tran<",     "transpose",  "transpose down"},
   {"tran>",     "",           "transpose up"},
   {"learn",     "recording",  "learn:  hear / play / practice"},
   {"recWipe",   "",           "wipe recording  (be careful:)"},
   {"recSave",   "",           "save recording"},                    // 20
   {"color",     "",           "color:  scale / velocity / track"},
   {"hearRec",   "",           "Hear your recording"},
   {"hearLoop",  "",           "Hear the notes to learn"}            // 23
};
ubyte NUCmd = BITS (UCmd);

bool Bye = false;                      // song thread's Quit is slow :/


static void UCmdLoad ()
// load .nt n .ky from device/keycmd.txt for given .cmd
{ TStr fn, s, k;
  ulong i, c;
  StrArr t (CC("keycmd"), 50, 50*sizeof(TStr));
   App.Path (fn, 'd');   StrAp (fn, "/device/keycmd.txt");   t.Load (fn);
   for (i = 0;  i < t.num;  i++) {
      StrCp (s, t.str [i]);   if (*s == '#')  continue;
     ColSep ss (s, 3);
      for (c = 0;  c < NUCmd;  c++)
         if (! StrCm (ss.Col [0], UCmd [c].cmd))  break;
      if (c >= NUCmd) {
DBG("device/keycmd.txt is broke - no cmd='`s'", ss.Col [0]);
         continue;
      }
      StrCp (UCmd [c].nt, ss.Col [1]);
      StrCp (UCmd [c].ky, ss.Col [2]);      // one weirdo: spc=>' '
      if (! StrCm (UCmd [c].ky, "spc"))  StrCp (UCmd [c].ky, " ");
   }
}

static char *UCmdS (const char *cmd)   // return tooltip as "desc  [key note]"
{ static TStr o, k, n;
   *k = *n = '\0';
   for (ubyte i = 0;  i < NUCmd;  i++)  if (! StrCm (cmd, UCmd [i].cmd)) {
//DBG("cmd=`s desc=`s", cmd, UCmd [i].desc);
      StrCp (k, UCmd [i].ky);   if (*k == ' ')  StrCp (k, "space");
      StrCp (n, UCmd [i].nt);   if (*n == '.')  *n = '\0';
      StrCp (o, UCmd [i].desc);
      if (*k || *n) {
         StrAp (o, "  [");
         StrAp (o, k);   if (*k && *n)  StrAp (o, " ");   StrAp (o, n);
         StrAp (o, "]");
      }
//DBG("o=`s", o);
      return o;
   }
   return CC("");
}

void PCheetah::Trak ()
{ QSplitter *sp = ui->spl;
  QList<int> sz = sp->sizes ();
  int w  = width ();
  int wt = _tr.W ();   if (wt == 0)  wt = 30;
DBG("sz0=`d sz1=`d w=`d wt=`d", sz[0], sz[1], w, wt);
   if (sz [0])
        {Gui.FullSc (true);   sp->setSizes (QList<int>() << 0  <<  w    );}
   else {Gui.FullSc (false);  sp->setSizes (QList<int>() << wt << (w-wt));}
}

void PCheetah::GCfg ()  {_dCfg->Open ();}
void PCheetah::MCfg ()  {DBG("gonna midicfg");
                         StrCp (Kick, "midicfg");   Gui.Quit ();}
void PCheetah::LoadGo ()
{ TStr s;   emit sgCmd (StrFmt (s, "load `s", FL.lst [FL.pos]));  }

void PCheetah::Load ()  {emit sgCmd ("wipe");   _dFL->Open ();}


static char *LstLen (char *ln, ubyt2 len, ubyt4 pos, void *ptr)
{  if (MemCm (ln, "DIDXX ", 6)) FL.xLen++;   return nullptr;  }

static File  Fw;

static char *LstGet (char *ln, ubyt2 len, ubyt4 pos, void *ptr)
{ TStr o;
  static ubyt4 n;
   StrFmt (o, "`s\n", ln);
   if (pos == 0)  n = 0;
   if (MemCm (o, "DIDXX ", 6))
      if (n++ == FL.xPos)  {StrCp (FL.xFn, ln);   StrFmt (o, "DIDXX `s\n", ln);}
   Fw.Put (o);   return nullptr;
}

static void XLoad ()
{ ubyt4 ln, i;
  TStr  dMid, fnC, fnW, dFnd, s;
  File  f;
  Path  d;
   App.CfgGet (CC("DlgFL_dir"), dMid);
   ln = f.Size (StrFmt (fnC,  "`s/_midicache.txt", dMid));

   if (Fw.Open (StrFmt (fnW, "`s/__midicache.txt", dMid), "w"))
      {f.DoText (fnC, nullptr, LstGet);   Fw.Shut ();}
   f.ReNm (fnW, fnC);
//TODO recache if FL.xLen hits 0
   if (--FL.xLen == 0)  {Gui.Hey ("_midicache has 0 left - repick midi dir");
                         return;}
TRC("   xPos=`d xLen=`d xFn=`s dMid=`s", FL.xPos, FL.xLen, FL.xFn, dMid);
// recreate dest dir
   StrFmt (dFnd, "`s/4_queue/found", App.Path (s, 'd'));
   d.Kill (dFnd);   d.Make (dFnd);

// ok copy it to 4_queue/found
  TStr fr, to, frP, toP, cmd;
   StrCp  (to, FL.xFn);   StrAp (to, "", 4);   FnFix (to);
   StrFmt (frP, "`s/`s",          dMid, FL.xFn);
   StrFmt (toP, "`s/`s/path.txt", dFnd, to);   f.Save (toP, frP, StrLn (frP));
   StrFmt (toP, "`s/`s/a.mid",    dFnd, to);   f.Copy (frP, toP);
   App.Run (StrFmt (cmd, "mid2song `p", toP));

// relist and move pos to 4_queue/found
   FL.Load ();   FL.pos = 0;
   for (ln = StrLn (dFnd), i = 0;  i < FL.lst.Ln;  i++)
      if (! MemCm (dFnd, FL.lst [i], ln))  {FL.pos = i;   break;}
}


void PCheetah::SongNxt ()
{  emit sgCmd ("wipe");
   if (FL.ext) {if (FL.xPos+1 < FL.xLen)   FL.xPos++;
                XLoad ();}
   else         if (FL.pos+1 < FL.lst.Ln)  FL.pos++;
   LoadGo ();
}


void PCheetah::SongPrv ()
{  emit sgCmd ("wipe");
   if (FL.ext) {if (FL.xPos)  FL.xPos--;
                XLoad ();}
   else         if (FL.pos)   FL.pos--;
   LoadGo ();
}


void PCheetah::SongRand ()
{ ubyt4 ln;
  TStr  dMid, fnC, c;
  File  f;
   emit sgCmd ("wipe");
   ln = 0;
   App.CfgGet (CC("DlgFL_dir"), dMid);
   if (*dMid)  ln = f.Size (StrFmt (fnC, "`s/_midicache.txt", dMid));
   if ((*dMid == '\0') || (! ln)) {
      if (*dMid == '\0')  StrCp (dMid, getenv ("HOME"));
      if (! Gui.AskDir (dMid, "pick dir to search for midis in (NOT / please)"))
         return;
      App.CfgPut (CC("DlgFL_dir"), dMid);
   }
   StrFmt (fnC, "`s/_midicache.txt", dMid);
   if (! f.Size (fnC)) {               // no cache yet so start makin one
TRC("no _midicache.txt for `s", dMid);
      App.Run (StrFmt (c, "ll midi `p &", dMid));
      Gui.Hey ("Making midi cache in that dir...\n"
               "Come back when __midicache.txt turns to _midicache.txt");
      return;
   }

// count # undid undeld
   FL.ext = true;   FL.xLen = 0;   f.DoText (fnC, nullptr, LstLen);
TRC("   init FL.xLen=`d", FL.xLen);
   FL.xPos = Rand (FL.xLen);
   XLoad ();
   LoadGo ();
}


void PCheetah::SongKill ()
{ ubyt4 p;
  TStr  dr, t;
  Path  d;
   if ((p = FL.pos) >= FL.lst.Ln)  return;
TRC("SongKill `s", FL.lst [FL.pos]);
   StrCp (dr, FL.lst [p]);   App.Path (t, 'd');   StrAp (t, "/4_queue/");
   if (MemCm (dr, t, StrLn (t)))
      {Gui.Hey ("songKill only works in 4_queue dir");   return;}

//TODO gotta path.txt?  rm that .mid too
   emit sgCmd ("wipe");   Zzz (750);   // give it 3/4 sec
   FL.lst.Del (p);   d.Kill (dr);   if (p == FL.lst.Ln)  p--;
   FL.pos = p;   LoadGo ();
}


//______________________________________________________________________________
// _tr (ui->tr) stuph...

char TrPop (char *ls, ubyt2 r, ubyte c)
// pop HT, sounddir, sound CtlList
{  *ls = '\0';
TRC("TrPop r=`d c=`d", r, c);
   if ((c == 1) && (Up.trk [r].lrn [0] == 'l') && (! Up.trk [r].drm)) {
      MemCp (ls, CC("4RH\0"
                    "3LH\0"
                    "1LH\0"
                    "2LH\0"
                    "5RH\0"
                    "6RH\0"
                    "7RH\0"), 7*4+1);
      return 'l';
   }
   if (c == 2) {
      if (! Up.trk [r].drm)  return 'e';    // edit melodic track name
     StrArr l (CC("drum.txt"), 256, 128*sizeof(TStr));
TRC("dvt=`d name=`s", Up.trk [r].dvt, Up.dvt [Up.trk [r].dvt].Name ());
     TStr   s, s2;                     // see if devtype has a drum.txt
      l.Load (StrFmt (s, "`s/device/`s/drum.txt",
         App.Path (s2, 'd'), Up.dvt [Up.trk [r].dvt].Name ()));
      if (l.NRow ())  {l.SetZZ (ls);   return 'l';}
   }                                   // else fall thru to readonly
   if ((c == 3) || (c == 4)) {
     ubyte dvt = Up.trk [r].dvt;
      if (Up.trk [r].drm && StrCm (Up.dvt [Up.trk [r].dvt].Name (), "syn"))
         return '\0';                  // no sound editin on nonsyn drums
      if (c == 3)  Up.dvt [dvt].SGrp (ls,                 Up.trk [r].drm);
      if (c == 4)  Up.dvt [dvt].SNam (ls, Up.trk [r].grp, Up.trk [r].drm);
      return 'l';
   }
   if (c == 5) {
     ubyte d = 0;
     TStr  n, t, x;
      StrCp (ls, "+");   ls += 2;
      while (Midi.GetPos ('o', d++, n, t, x, x))  if (StrCm (t, "OFF"))
         {StrCp (ls, n);   ls += (StrLn (n)+1);}
      *ls = '\0';
      return 'l';
   }
   return '\0';
}

void PCheetah::TrClk ()
{ ubyt2 r = _tr.CurRow ();
  ubyte c = _tr.CurCol ();
TRC("TrClk(L) r=`d c=`d", r, c);
   Up.eTrk = (ubyte)r;
   if      (c == 0)  emit sgCmd ("prac");
   else if (c == 1)  emit sgCmd ("htype x");
}

void PCheetah::TrClkR (const QPoint &pos)
{ ubyt2 r = _tr.CurRow ();   (void)pos;
  ubyte c = _tr.CurCol ();
TRC("TrClkR r=`d c=`d", r, c);
   Up.eTrk = (ubyte)r;
   if      (c == 0)  emit sgCmd ("mute");
   else if (c == 1)  emit sgCmd ("showAll");
}


void PCheetah::TrUpd ()
{ ubyt2 r = _tr.CurRow ();
  ubyte c = _tr.CurCol ();
  TStr  s;
   Up.eTrk = (ubyte)r;
TRC("TrUpd r=`d c=`d", r, c);
   switch (c) {
      case 1:                          // hand type  (\0,1..7,x)
         emit sgCmd (StrFmt (s, "htype `s", _tr.Get (r, 1)));   break;
      case 2:                          // track name/drum in
         if (Up.trk [r].drm)
              {emit sgCmd (StrFmt (s, "drmap `s", _tr.Get (r, 2)));   break;}
         else {emit sgCmd (StrFmt (s, "trk `s",   _tr.Get (r, 2)));   break;}
      case 3:                          // sound group
         emit sgCmd (StrFmt (s, "grp `s",   _tr.Get (r, 3)));   break;
      case 4:                          // sound name
         emit sgCmd (StrFmt (s, "snd `s",   _tr.Get (r, 4)));   break;
      case 5:                          // device.channel
         emit sgCmd (StrFmt (s, "dev `s",   _tr.Get (r, 5)));   break;
   }
}


//______________________________________________________________________________
void PCheetah::Upd (QString upd)
{ TStr  u, s;
  ubyte i;
   StrCp (u, UnQS (upd));
//TRC("Upd `s", u);
   for (i = 0;  i < NUCmd;  i++)  if (! StrCm (u, UCmd [i].cmd))  break;
   if (i < NUCmd) {
      if (i > 5)  emit sgCmd (upd);
      else  switch (i) {
         case 0:  SongPrv  ();   break;     // song<
         case 1:  SongRand ();   break;     // songRand
         case 2:  SongNxt  ();   break;     // song>
         case 3:  SongKill ();   break;     // songKill
         case 4:  Gui.Quit ();   break;     // exit
         case 5:  Trak     ();   break;     // fullScr
      }
      return;
   }

   if (! StrCm (u, "bye"))  {Bye = true;   return;}   // song got done closin'

   if (! StrCm (u, "ttl"))  _tb.Act (29)->setText (Up.ttl);

   if (! StrCm (u, "FLdn")) {if (FL.pos < FL.lst.Ln-1)
                                          {++FL.pos;   _dFL->ReDo ();}}
   if (! StrCm (u, "FLup")) {if (FL.pos)  {--FL.pos;   _dFL->ReDo ();}}
   if (! StrCm (u, "FLgo")) {_dFL->Shut ();   LoadGo ();}
   if (! StrCm (u, "FLex")) {_dFL->Shut ();   Gui.Quit ();}

   if (! StrCm (u, "nt"))     _nt->update ();
   if (! StrCm (u, "ntCur"))  _nt->Cur ();
   if (! StrCm (u, "dTDr"))   _dTDr->Open ();
   if (! StrCm (u, "dCue"))   _dCue->Open ();
   if (! StrCm (u, "dChd"))   _dChd->Open ();
   if (! StrCm (u, "dCtl"))   _dCtl->Open ();
   if (! StrCm (u, "dTpo"))   _dTpo->Open ();
   if (! StrCm (u, "dTSg"))   _dTSg->Open ();
   if (! StrCm (u, "dKSg"))   _dKSg->Open ();
   if (! StrCm (u, "dQua"))   _dQua->Open ();
   if (! StrCm (u, "dMov"))   _dMov->Open ();
   if (! StrCm (u, "dHlpO"))  _dHlp->Open ();
   if (! StrCm (u, "dHlpS"))  _dHlp->Shut ();

   if (! StrCm (u, "time")) {
     TStr s;
      StrCp (s, Up.time);
     bool poz = (*s == 'X');
      if (poz)  StrCp (s, & s [1]);
     CtlLabl l (ui->time);
      l.SetBold (poz);
      l.SetFg (poz ? Color ("fgSel") : Color ("fg"));
      l.SetBg (poz ? CScl [1][0]     : Color ("bg"));
      l.Set (s);
   }
   if (! StrCm (u, "bars"))  { CtlLabl l (ui->bars);   l.Set (Up.bars);}
   if (! StrCm (u, "tmpo"))  { CtlLabl l (ui->tmpo);   l.Set (Up.tmpo);}
   if (! StrCm (u, "tsig"))  { CtlLabl l (ui->tSig);   l.Set (Up.tsig);}
   if (! StrCm (u, "lyr"))   { CtlLabl l (ui->lyr );   l.Set (Up.lyr );}
   if (! StrCm (u, "tbPoz"))  _tb.Set (14, Up.uPoz ? 1 : 0);
   if (! StrCm (u, "tbLrn"))  _tb.Set (9,  PLAY ? 1 : (PRAC ? 2 : 0));

   if (! StrCm (u, "trk")) {
     char *rp [32];
     BStr  tip;
      _tr.Open ();   rp [6] = nullptr;
      for (ubyte i = 0, tc = 0;  i < Up.trk.Ln;  i++) {
         rp [0] = Up.trk [i].lrn;      rp [1] = Up.trk [i].ht;
         rp [2] = Up.trk [i].name;     rp [3] = Up.trk [i].grp;
         rp [4] = Up.trk [i].snd;      rp [5] = Up.trk [i].dev;
         _tr.Put (rp,                           Up.trk [i].tip);
         if ((Cfg.ntCo == 2) &&        // color by track (if lrn/show non drum)
             ((rp [0][0] == 'l') || (rp [1][0] == 'S')) &&
             (! Up.trk [i].drm))
            _tr.SetColor (i, 0, CMap (tc++));
      }
      _tr.Shut ();   _tr.HopTo (Up.eTrk, 0);

     int w  = width ();
     int wt = _tr.W ();
     QSplitter *sp = ui->spl;
      sp->setSizes (QList<int>() << wt << (w-wt));
   }

   if (! MemCm (u, "hey ", 4))   Gui.Hey (& u [4]);
   if (! MemCm (u, "die ", 4))  {Gui.Hey (& u [4]);   Gui.Quit ();}
}


//______________________________________________________________________________
void PCheetah::keyPressEvent (QKeyEvent *e)
{ KeyMap km;
  ubyte i;
  key   k;
  TStr  s;
//DBG("keyPressEvent raw mod=`d key=`d", e->modifiers (), e->key ());
   if (! (k = km.Map (e->modifiers (), e->key ())))  return;

   StrCp (s, km.Str (k));
//DBG("   keystr='`s'", s);
   for (i = 0;  i < NUCmd;  i++)  if (! StrCm (s, UCmd [i].ky))  break;
   if      (i < NUCmd)               Upd (UCmd [i].cmd);
   else if (! StrCm (s, "d"))    emit sgCmd ("dump");
   else if (! StrCm (s, "f01"))  _dHlp->Open ();
   else QMainWindow::keyPressEvent (e);
}


void PCheetah::changeEvent (QEvent *ev)
{  QMainWindow::changeEvent (ev);
   if (ev->type () == QEvent::ApplicationPaletteChange ||
       ev->type () == QEvent::PaletteChange) {
DBG("dark/light change");
      Gui.ReIco ();   _tb.ReDo ();   _nt->update ();
DBG("dark/light done");
   }
}


void PCheetah::SetPix ()               // all dem bitmaps
{  Up.bug     = new QPixmap (":/note/bug");
   Up.cue     = new QPixmap (":/note/cue");
   Up.dot     = new QPixmap (":/note/dot");
   Up.fade    = new QPixmap (":/note/fade");
   Up.now     = new QPixmap (":/note/now");
   Up.oct     = new QPixmap (":/note/oct");
   Up.bg  [0] = new QPixmap (":/note/bg");
   Up.bg  [1] = new QPixmap (":/note/bg_d");
   Up.bg2 [0] = new QPixmap (":/note/bg2");
   Up.bg2 [1] = new QPixmap (":/note/bg2_d");
   Up.tpm = new QPixmap ((32+88)*W_NT, M_WHOLE*4);
}


void PCheetah::SetTBar ()              // toolbars take some messin with
{  _tb.Init (this, "app");
TRC(" tbar init");

// global-y
   _tb.Btn (0, UCmdS ("fullScr"));
   _tb.Btn (1, "configure midi devices", "d");
   _tb.Btn (2, "settings and junk");
   connect (_tb.Act (0), & QAction::triggered,  this, & PCheetah::Trak);
   connect (_tb.Act (1), & QAction::triggered,  this, & PCheetah::MCfg);
   connect (_tb.Act (2), & QAction::triggered,  this, & PCheetah::GCfg);
   _tb.Sep (3);

// song pickin
   _tb.Btn (4, "pick from song list");
   _tb.Btn (5, UCmdS ("song<"));
   _tb.Btn (6, UCmdS ("song>"));
   _tb.Btn (7, UCmdS ("songRand"), "*^??");
   connect (_tb.Act (4), & QAction::triggered,  this, & PCheetah::Load);
   connect (_tb.Act (5), & QAction::triggered,  this, & PCheetah::SongPrv);
   connect (_tb.Act (6), & QAction::triggered,  this, & PCheetah::SongNxt);
   connect (_tb.Act (7), & QAction::triggered,  this, & PCheetah::SongRand);
   _tb.Sep (8);

// learn mode
   _tb.Btn (9, UCmdS ("learn"));
   _tb.Ico (9, 1);
   _tb.Ico (9, 2);
   connect (_tb.Act (9), & QAction::triggered,
                         this, [this]() {emit sgCmd ("learn");});
   _tb.Sep (10);

// transport - play/pause/etc
   _tb.Btn (11, UCmdS ("timeBar1"));
   _tb.Btn (12, UCmdS ("time<<"));
   _tb.Btn (13, UCmdS ("time<"));
   _tb.Btn (14, UCmdS ("timePoz"));
   _tb.Ico (14, 1);
   _tb.Btn (15, UCmdS ("time>"));
   _tb.Btn (16, UCmdS ("time>>"));
   connect (_tb.Act (11), & QAction::triggered,
                          this, [this]() {emit sgCmd ("timeBar1");});
   connect (_tb.Act (12), & QAction::triggered,
                          this, [this]() {emit sgCmd ("time<<"  );});
   connect (_tb.Act (13), & QAction::triggered,
                          this, [this]() {emit sgCmd ("time<"   );});
   connect (_tb.Act (14), & QAction::triggered,
                          this, [this]() {emit sgCmd ("timePoz" );});
   connect (_tb.Act (15), & QAction::triggered,
                          this, [this]() {emit sgCmd ("time>"   );});
   connect (_tb.Act (16), & QAction::triggered,
                          this, [this]() {emit sgCmd ("time>>"  );});
   _tb.Sep (17);

// tempo
   _tb.Btn (18, UCmdS ("tempo<"  ), "d");
   _tb.Btn (19, UCmdS ("tempoHop"), "d");
   _tb.Btn (20, UCmdS ("tempo<"  ), "d");
   connect (_tb.Act (18), & QAction::triggered,
                          this, [this]() {emit sgCmd ("tempo<"  );});
   connect (_tb.Act (19), & QAction::triggered,
                          this, [this]() {emit sgCmd ("tempoHop");});
   connect (_tb.Act (20), & QAction::triggered,
                          this, [this]() {emit sgCmd ("tempo<"  );});
   _tb.Sep (21);

// editing stuff
   _tb.Btn (22, "scoot track up", "d");
   _tb.Btn (23, "scoot track down", "d");
   _tb.Btn (24, "insert track", "d");
   _tb.Btn (25, "delete track", "d");
   _tb.Btn (26, "split the learn track (3E and below) into new LH track", "d");
   _tb.Btn (27, "make drum track from clips", "d");
// "time scaling - for { to } => } scales to ^"
//    this, [this]() {emit sgCmd ("trkEd *");});
// "time offsetting - for { to end => { moves to ^"
//    this, [this]() {emit sgCmd ("trkEd -");});
   connect (_tb.Act (22), & QAction::triggered,
                          this, [this]() {emit sgCmd ("trkEd u");});
   connect (_tb.Act (23), & QAction::triggered,
                          this, [this]() {emit sgCmd ("trkEd d");});
   connect (_tb.Act (24), & QAction::triggered,
                          this, [this]() {emit sgCmd ("trkEd +");});
   connect (_tb.Act (25), & QAction::triggered,
                          this, [this]() {emit sgCmd ("trkEd x");});
   connect (_tb.Act (26), & QAction::triggered,
                          this, [this]() {emit sgCmd ("trkEd sp");});
   connect (_tb.Act (27), & QAction::triggered,
                          this, [this]() {emit sgCmd ("preTDr");});
   _tb.Sep (28);

   _tb.Btn (29, "(I just show the song filename in fullscreen)", "*...");
   connect (_tb.Act (29), & QAction::triggered,  this, & PCheetah::Trak);
}


void PCheetah::Init ()
{ TStr fn;
  File f;
TRC("Init");
   *Kick = '\0';
   _s    = nullptr;
   _dFL  = nullptr;   _dCfg = nullptr;   _dTDr = nullptr;
   _dCue = nullptr;   _dChd = nullptr;   _dCtl = nullptr;
   _dTpo = nullptr;   _dTSg = nullptr;   _dKSg = nullptr;
   _dQua = nullptr;   _dMov = nullptr;   _dHlp = nullptr;

   App.Path (fn, 'd');   StrAp (fn, "/device/device.txt");
   if (! f.Size (fn))  return MCfg ();      // ain't no point in goin on

TRC(" got device.txt");
   Midi.Load ();   MCCInit ();
  ubyte i = 0;
  TStr  nm, ty, ds, dv;
   while (Midi.GetPos ('o', i++, nm, ty, ds, dv))
      if (StrCm (ty, "OFF") && (*dv == '?'))
         {Gui.Hey (StrFmt (dv, "Hey! `s: `s (`s)  is off, pal...",
                               nm, ty, ds));   break;}
   UCmdLoad ();
TRC(" song init");
   _s = new Song;                      // git song worker thread goin

   CInit ();                           // init all them thar colors
   Gui.WinLoad (ui->spl);
   Up.cnv.SetFont ("Noto Sans", 12);   Up.tcnv.SetFont ("Noto Sans", 12);
   Up.txH = Up.cnv.FontH ();

   Gui.A ()->setStyleSheet ("QToolTip {font: 15pt monospace;}");

   SetPix ();

   _s->moveToThread (& _thrSong);
   connect (this,       & PCheetah::sgCmd,   _s,   & Song::Cmd);
   connect (_s,         & Song::sgUpd,       this, & PCheetah::Upd);
   connect (& _thrSong, & QThread::finished, _s,   & QObject::deleteLater);
   _thrSong.start ();
   emit sgCmd ("init");

   setFocusPolicy (Qt::StrongFocus);        // so we get keyPressEvent()s

   SetTBar ();

TRC(" tr,nt control init");
   _tr.Init (ui->tr, "trak",
      "+Lrn\0"
     "*_Hand\0"
      "_Track\0"
      "_SnGrp\0"
      "_Sound\0"
      "_Dev.Ch\0", TrPop, "none", "row");
   _tr.SetRowH (32);

   ui->tr->setContextMenuPolicy (Qt::CustomContextMenu);
   ui->tr->setSizeAdjustPolicy (QAbstractScrollArea::AdjustToContents);
   connect (ui->tr, & QTableWidget::itemClicked, this, & PCheetah::TrClk);
   connect (ui->tr, & QTableWidget::customContextMenuRequested,
                                                 this, & PCheetah::TrClkR);
   connect (ui->tr, & QTableWidget::itemChanged, this, & PCheetah::TrUpd);

   _nt = ui->nt;
   connect (_nt, & CtlNt::sgReSz, _s, & Song::ReSz);
   connect (_nt, & CtlNt::sgMsDn, _s, & Song::MsDn);
   connect (_nt, & CtlNt::sgMsMv, _s, & Song::MsMv);
   connect (_nt, & CtlNt::sgMsUp, _s, & Song::MsUp);
   _nt->Init (ui->nt->width (), ui->nt->height ());

TRC(" dlg init");
   _dFL   = new DlgFL  (this);    _dFL->Init ();
   _dCfg  = new DlgCfg (this);   _dCfg->Init ();
   _dTDr  = new DlgTDr (this);   _dTDr->Init ();
   _dCue  = new DlgCue (this);   _dCue->Init ();
   _dChd  = new DlgChd (this);   _dChd->Init ();
   _dCtl  = new DlgCtl (this);   _dCtl->Init ();
   _dTpo  = new DlgTpo (this);   _dTpo->Init ();
   _dTSg  = new DlgTSg (this);   _dTSg->Init ();
   _dKSg  = new DlgKSg (this);   _dKSg->Init ();
   _dQua  = new DlgQua (this);   _dQua->Init ();
   _dMov  = new DlgMov (this);   _dMov->Init ();
   _dHlp  = new DlgHlp (this);   _dHlp->Init ();
   connect (_dCfg, & DlgCfg::sgCmd, this, [this](char *s)  {emit sgCmd (s);});
   connect (_dTDr, & DlgTDr::sgCmd, this, [this](char *s)  {emit sgCmd (s);});
   connect (_dCue, & DlgCue::sgCmd, this, [this](char *s)  {emit sgCmd (s);});
   connect (_dChd, & DlgChd::sgCmd, this, [this](char *s)  {emit sgCmd (s);});
   connect (_dTpo, & DlgTpo::sgCmd, this, [this](char *s)  {emit sgCmd (s);});
   connect (_dTSg, & DlgTSg::sgCmd, this, [this](char *s)  {emit sgCmd (s);});
   connect (_dKSg, & DlgKSg::sgCmd, this, [this](char *s)  {emit sgCmd (s);});
   connect (_dQua, & DlgQua::sgCmd, this, [this](char *s)  {emit sgCmd (s);});
   connect (_dCtl, & DlgCtl::sgCmd, this, [this](char *s)  {emit sgCmd (s);});
   connect (_dFL,  & QDialog::accepted, this, & PCheetah::LoadGo);
   connect (_dFL,  & QDialog::rejected, this, & PCheetah::LoadGo);
   connect (_dMov, & QDialog::accepted, this, [this]() {emit sgCmd ("mov");});

// parse cmdline arg:  try to load song in dir or turn fn into song to do
  bool ld = false;
  TStr a;
  FDir d;
   StrCp (a, Gui.Arg (0));
TRC(" arg=`s", a);
   if (*a) ld = d.Got (a) ? FL.DoDir (a) : FL.DoFN (a);
   if (ld) LoadGo ();   else Load ();
TRC("Init end");
}


void PCheetah::Quit ()
{
TRC("Quit bgn");
   if (_s != nullptr)  emit sgCmd (CC("quit"));
   if (_dMov != nullptr) {
      Gui.WinSave (ui->spl);
      _dFL->Quit ();    _dCfg->Quit ();   _dTDr->Quit ();
      _dCue->Quit ();   _dChd->Quit ();   _dCtl->Quit ();
      _dTpo->Quit ();   _dTSg->Quit ();   _dKSg->Quit ();
      _dQua->Quit ();   _dMov->Quit ();   _dHlp->Quit ();
      delete _dFL;    delete _dCfg;   delete _dTDr;
      delete _dCue;   delete _dChd;   delete _dCtl;
      delete _dTpo;   delete _dTSg;   delete _dKSg;
      delete _dQua;   delete _dMov;   delete _dHlp;
   }
   if (_s != nullptr) {
      while (! Bye)  usleep (5);
      _thrSong.quit ();
      _thrSong.wait ();
   }
   if (*Kick)  App.Spinoff (Kick);
TRC("Quit end");
}


int main (int argc, char *argv [])
{  DBGTH ("PcGui");
  QApplication app (argc, argv);
  PCheetah     win;
   App.Init ();                        //TODO scrsaver off.  limit one instance?
   Gui.Init (& app, & win, "PianoCheetah");   win.Init ();   RandInit ();
   qRegisterMetaType<ubyte>("ubyte");
   qRegisterMetaType<sbyt2>("sbyt2");
   qRegisterMetaType<Qt::MouseButton >("Qt::MouseButton" );
   qRegisterMetaType<Qt::MouseButtons>("Qt::MouseButtons");
  int rc = Gui.Loop ();                       win.Quit ();
   return rc;
}
