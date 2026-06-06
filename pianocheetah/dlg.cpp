// dlg.cpp - all pcheetah's dang dialogs - a LOT

#include "pcheetah.h"

//______________________________________________________________________________
// DlgCfg - load/save/edit global Cfg biz
//          and per song cfg via CfgInit/CfgLoad/CfgSave

CfgDef Cfg;

void CfgDef::Init ()                   // default the global settings
{  tran   = 0;                         // tran comes from .song
   ntCo   = 0;                         // scale
   barCl  = false;
}

void CfgDef::Load ()                   // load global settings
{ StrArr t (CC("cfg"), 80, 80*sizeof(TStr));
  TStr   fn, s, v;
  char  *p;
  ubyt2  i;
TRC("CfgDef::Load");
   Init ();
   App.Path (fn, 'c');   StrAp (fn, "/cfg.txt");   t.Load (fn);
   for (i = 0;  i < t.num;  i++) {
      StrCp (s, t.str [i]);   if (! (p = StrCh (s, '=')))  continue;
      *p = '\0';   StrCp (v, p+1);
      if (StrSt (s, CC("ntCo"  )))  Cfg.ntCo   = (ubyte)Str2Int (v);
      if (StrSt (s, CC("barCl" )))  Cfg.barCl  = (*v == 'y') ? true:false;
   }
}

void CfgDef::Save ()                   // save global settings
{ BStr buf;
  TStr fn;
  File f;
TRC("CfgDef::Save");
   StrFmt (buf, "ntCo=`d\n"  "barCl=`s\n",
                 ntCo,        barCl?"y":"n");
   App.Path (fn, 'c');   StrAp (fn, "/cfg.txt");
   f.Save (fn, buf, StrLn (buf));
}


//______________________________________________________________________________
void DlgCfg::Open ()
{ TStr ts;
TRC("DlgCfg::Open");
  CtlSpin tr (ui->tran, -12, 12);
  CtlChek b  (ui->barCl), t (ui->trc);
   tr.Set (Cfg.tran);
   b.Set  (Cfg.barCl);   t.Set (App.trc);
   show ();   raise ();   activateWindow ();
}

void DlgCfg::Shut ()                   // set em n save em
{ TStr ts;
TRC("DlgCfg.Shut");
  CtlSpin tr (ui->tran);
  CtlChek b  (ui->barCl), t (ui->trc);
   Cfg.tran  = tr.Get ();
   Cfg.barCl = b.Get ();   App.TrcPut (t.Get ());
   Cfg.Save ();                        // in case we die early :/
   done (true);   lower ();   hide ();
}

void DlgCfg::Init ()
{  Cfg.Load ();        Gui.DlgLoad (this, "DlgCfg");
   connect (ui->quan, & QPushButton::clicked,
            this, [this]() { emit sgCmd (CC("quan")); });
}
void DlgCfg::Quit ()  {Gui.DlgSave (this, "DlgCfg");}


//______________________________________________________________________________
// dlgChd - edit chords

static char *IntvMap = CC("1m2m34d5a6m7");


void DlgChd::Cmd (char *s)
{ TStr u;
   emit sgCmd (StrFmt (u, "chd `s", s));
}


void DlgChd::ReDo ()
{ TStr s, s2, chd;
TRC("DlgChd::ReDo");
// pull our chord picks to make chd string
  CtlList root (ui->root), type (ui->type), bass (ui->bass);
   root.GetS (chd);   StrCp (s, MChd [type.Get ()].lbl);   bass.GetS (s2);
   if (! StrCm (chd, "(none)"))  *chd = '\0';     // no chord at all?
   else {
      StrAp (chd, s);
      if (StrCm (s2, "(none)"))  {StrAp (chd, "/");   StrAp (chd, s2);}
   }
// enable/dis per picks
                 type.Enable (true);    bass.Enable (true);
   if (! *chd)  {type.Enable (false);   bass.Enable (false);
                 type.Set (0);          bass.Set (0);}
// update dat song's _f.chd (ins/del/upd) based on _got,_cp
   Cmd (StrFmt (s, ".`b `d `d `s", _got, _cp, _tm, chd));
   _got = *chd ? true : false;
TRC("DlgChd::ReDo end");
}


void DlgChd::Set (char *s)
{ ubyte i, j;
  TStr  ch;
  char *c;
   StrCp (ch, s);
  CtlList root (ui->root), type (ui->type), bass (ui->bass);
   root.Set (0);   type.Set (0);   bass.Set (0);
   if ((c = StrCh (ch, '/'))) {        // if got bass, chop if off n set ctrl
      *c++ = '\0';
      for (i = 0;  i < 12;  i++)  {if (! StrCm (c, MKeyStr  [i]))  break;
                                   if (! StrCm (c, MKeyStrB [i]))  break;}
      if (i < 12)  bass.Set (1+i);
   }
   if (*ch) {
      i = ((ch [1] == 'b') || (ch [1] == '#')) ? 2 : 1;    // type pos
      c = & ch [i];
      for (j = 0;  j < NMChd;  j++)  if (! StrCm (c, MChd [j].lbl, 'x'))
                                        {type.Set (j);   break;}
      *c = '\0';                       // chop it off
   }
   for (i = 0;  i < 12;  i++)  {if (! StrCm (ch, MKeyStr  [i]))  break;
                                if (! StrCm (ch, MKeyStrB [i]))  break;}
   if (i < 12) root.Set (i+1);
}


void DlgChd::Pop ()
{ TStr s;
  CtlList pop (ui->pop);
   pop.GetS (s);   while (*s == ' ')  StrCp (s, & s [1]);   Set (s);
   ReDo ();   Cmd (StrFmt (s, "@`d", _tm1));
}


void DlgChd::UnDo ()
{ TStr s;
  CtlBttn undo (ui->undo);
   StrCp (s, undo.Get ());   StrCp (s, & s [8]);   Set (s);
   ReDo ();   Cmd (StrFmt (s, "@`d", _tm1));
}


//______________________________________________________________________________
void DlgChd::Init ()
{  Gui.DlgLoad (this, "DlgChd");
  CtlTBar tb;
   tb.Init (this, "chd");
   tb.Btn (0, "guess chords\n"
              "   calculate whole song's chords\n"
              "   using notes of selected practice tracks");
   tb.Btn (1, "pop chords\n"
              "   plop some random happy pop chords into this section");
   tb.Btn (2, "delete all chords  (be carefulll)");
   connect (tb.Act (0), & QAction::triggered,  this, [this]() {Cmd (CC("?"));});
   connect (tb.Act (1), & QAction::triggered,  this, [this]() {Cmd (CC("+"));});
   connect (tb.Act (2), & QAction::triggered,  this, [this]() {Cmd (CC("x"));});
   connect (ui->shhh, & QPushButton::clicked,
                                  this, [this]() {emit sgCmd (CC("timePoz"));});
   connect (ui->undo, & QPushButton::clicked,  this, [this]() {UnDo ();});
   connect (ui->pop,  QOverload<int>::of(& QComboBox::currentIndexChanged),
                                               this, [this]() {Pop  ();});
}


void DlgChd::Open ()
{ ubyt4 i, j;
  ubyte nbtw;
  TStr  *btw, s, it, t, ch, ch1, ch2;
  char  *c;
TRC("DlgChd::Open");
   show ();   raise ();   activateWindow ();
   _got = Up.pos.got ? true : false;
   _cp = Up.pos.cp;   _tm = Up.pos.tm;   _tm1 = Up.pos.tmBt;
   StrCp (ch1, Up.d [0][0]);   StrCp (ch2, Up.d [0][1]);
   nbtw = ChdBtw (& btw, ch1, ch2);
  CtlList pop  (ui->pop);
  CtlBttn undo (ui->undo);
   pop.ClrLs ();
   for (ubyte i = 0;  i < nbtw;  i++) {
     ColSep c (btw [i], 80);
      pop.InsLs (c.Col [0]);
      for (ubyte j = 1;  c.Col [j][0];  j++)
         {StrCp (it, " ");   StrAp (it, c.Col [j]);   pop.InsLs (it);}
   }
   StrCp (ch, Up.pos.str);   StrCp (s, ch);
   if (! nbtw)  {undo.Enable (false);   pop.InsLs (CC("(none)"), 0);}
   else {        undo.Enable (true);
      if (! Up.pos.got)  StrCp (s, "(none)");
      StrCp (it, "restore ");   StrAp (it, s);   undo.Set (it);
   }
   pop.Set (0);

// init lists for chord's root/type/bass
   c = Up.pos.kSg.flt ? CC("(none)\0C\0Db\0D\0Eb\0E\0F\0Gb\0G\0Ab\0A\0Bb\0B\0")
                      : CC("(none)\0C\0C#\0D\0D#\0E\0F\0F#\0G\0G#\0A\0A#\0B\0");
  CtlList root (ui->root, c), type (ui->type), bass (ui->bass, c);
   type.ClrLs ();
   for (i = 0;  i < NMChd;  i++) {
      StrCp (t, "............");
      for (j = 0;  MChd [i].tmp [j] != 'x';  j++)
         t [(ubyte)(MChd [i].tmp [j])] = IntvMap [(ubyte)(MChd [i].tmp [j])];
      StrFmt (s, "`<5s  `s  `s", MChd [i].lbl, t, MChd [i].etc?MChd [i].etc:"");
      type.InsLs (s);
   }

// chord control picks (root, type, bass)  (n disabling)
   Set (ch);   ReDo ();
   Gui.DlgMv (this, Up.gp, "tR");
TRC("} DlgChd::Open");
}


void DlgChd::Shut ()
{ TStr x;
   Cmd (StrFmt (x, "@`d", _tm));
   done (true);   lower ();   hide ();
}


void DlgChd::Quit ()  {Gui.DlgSave (this, "DlgChd");}


//______________________________________________________________________________
// dlgCtl - pick dev,ctlin=>songctl n whether to show/hide/mini the controls

static BStr MLst;

static char CtlPop (char *ls, ubyt2 r, ubyte c)
{  (void)r;   ZZCp (ls, MLst);   return (c == 2) ? 'l' : '\0';
}

void DlgCtl::Upd ()
{ ubyt2 r = _t.CurRow ();
  ubyte c = _t.CurCol ();
  TStr  s;
   if (c != 3)  return;
   StrCp (s, _t.Get (r, 2));   if (! StrCm (s, "-"))  return;

   StrCp (s, _t.Get (r, 3));           // hide/mini/show
   if      (*s == 'h')  StrCp (s, "mini");
   else if (*s == 'm')  StrCp (s, "show");
   else                 StrCp (s, "hide");
   _t.Set (r, 3, s);
}

void DlgCtl::Init ()
{ ubyte i;
   *MLst = '\0';     ZZApS (MLst, CC("-"));
   for (i = 0;  i < NMCC;  i++)  if (! StrCm (MCC [i].s, "keyCmd"))  break;
   while (i < NMCC)  ZZApS (MLst, MCC [i++].s);

   Gui.DlgLoad (this, "DlgCtl");
   _t.Init (ui->t, "Device\0DeviceControl\0_SongControl\0Show?\0", CtlPop);
   connect (ui->t, & QTableWidget::itemClicked, this, & DlgCtl::Upd);
}

void DlgCtl::Open ()
{ ubyte r, c;
  char *ro [5];
  CtlChek ch (ui->chk);
   ch.Set (Up.d [Up.nR][0][0] == 'Y');
   ro [4] = nullptr;
   show ();   raise ();   activateWindow ();
   _t.Open ();
   for (r = 0;  r < Up.nR;  r++) {
      for (c = 0;  c < 4;  c++)  ro [c] = Up.d [r][c];
      _t.Put (ro);
   }
   _t.Shut ();
   Gui.DlgMv (this, Up.gp, "tc");
   _t.HopTo (Up.rHop, 2);
}

void DlgCtl::Shut ()
{ CtlChek ch (ui->chk);
   Up.d [Up.nR][0][0] = ch.Get () ? 'Y':'N';
   for (ubyte r = 0;  r < Up.nR;  r++)
      {StrCp (Up.d [r][2], _t.Get (r,2));   StrCp (Up.d [r][3], _t.Get (r,3));}
   emit sgCmd (CC("ctl"));
   done (true);   lower ();   hide ();
}

void DlgCtl::Quit ()  {Gui.DlgSave (this, "DlgCtl");}


//______________________________________________________________________________
// dlgCue - pick a new drum track per section (patA,patB,fill)

void DlgCue::Set (const char *s)
{ TStr s1, s2;
   if (StrCm (s, "```"))  StrCp (s1, s);
   else {
     CtlLine l (ui->str);
      StrCp (s1, l.Get ());
   }
   emit sgCmd (StrFmt (s2, "cue `s", s1));   Shut ();
}

void DlgCue::Open ()
{ CtlLine l (ui->str);
   l.Set (Up.pos.str);
   show ();   raise ();   activateWindow ();
   Gui.DlgMv (this, Up.gp, "tL");
}

void DlgCue::Shut ()  {done (true);   lower ();   hide ();}

void DlgCue::Init ()
{  Gui.DlgLoad (this, "DlgCue");
  CtlTBar tb;
   tb.Init (this, "cue");
   tb.Btn (0, "redo loops and erase all bug history");
   tb.Btn (1, "delete this cue");
   connect (tb.Act (0), & QAction::triggered,
                        this, [this]() {Set ("loopInit");});
   connect (tb.Act (1), & QAction::triggered,
                        this, [this]() {Set ("");});
   tb.Sep (2);

   tb.Btn (3, "text / non repeating section", "d");
   tb.Btn (4, "verse section", "d");
   tb.Btn (5, "chorus section", "d");
   tb.Btn (6, "break section", "d");
   connect (tb.Act (3), & QAction::triggered,
                        this, [this]() {Set ("```");});
   connect (tb.Act (4), & QAction::triggered,
                        this, [this]() {Set ("(verse");});
   connect (tb.Act (5), & QAction::triggered,
                        this, [this]() {Set ("(chorus");});
   connect (tb.Act (6), & QAction::triggered,
                        this, [this]() {Set ("(break");});
   tb.Sep (7);

   tb.Btn ( 8, "crescendo");
   tb.Btn ( 9, "decrescendo");
   tb.Btn (10, "fermata");
   tb.Btn (11, "tremelo");
   tb.Btn (12, "star");
   tb.Btn (13, "happy");
   tb.Btn (14, "sad");
   tb.Btn (15, "mad");
   tb.Btn (16, "piano x3",    "*ppp");
   tb.Btn (17, "piano x2",    "*pp");
   tb.Btn (18, "piano",       "*p");
   tb.Btn (19, "mezzo piano", "*mp");
   tb.Btn (20, "mezzo forte", "*mf");
   tb.Btn (21, "forte",       "*f");
   tb.Btn (22, "forte x2",    "*ff");
   tb.Btn (23, "forte x3",    "*fff");
   tb.Btn (24, "forzando",    "*Fz");
   connect (tb.Act ( 8), & QAction::triggered,  this, [this]() {Set ("<");});
   connect (tb.Act ( 9), & QAction::triggered,  this, [this]() {Set (">");});
   connect (tb.Act (10), & QAction::triggered,  this, [this]() {Set ("`fer");});
   connect (tb.Act (11), & QAction::triggered,  this, [this]() {Set ("`tre");});
   connect (tb.Act (12), & QAction::triggered,  this, [this]() {Set ("`sta");});
   connect (tb.Act (13), & QAction::triggered,  this, [this]() {Set ("`hap");});
   connect (tb.Act (14), & QAction::triggered,  this, [this]() {Set ("`sad");});
   connect (tb.Act (15), & QAction::triggered,  this, [this]() {Set ("`mad");});
   connect (tb.Act (16), & QAction::triggered,  this, [this]() {Set ("ppp");});
   connect (tb.Act (17), & QAction::triggered,  this, [this]() {Set ("pp");});
   connect (tb.Act (18), & QAction::triggered,  this, [this]() {Set ("p");});
   connect (tb.Act (19), & QAction::triggered,  this, [this]() {Set ("mp");});
   connect (tb.Act (20), & QAction::triggered,  this, [this]() {Set ("mf");});
   connect (tb.Act (21), & QAction::triggered,  this, [this]() {Set ("f");});
   connect (tb.Act (22), & QAction::triggered,  this, [this]() {Set ("ff");});
   connect (tb.Act (23), & QAction::triggered,  this, [this]() {Set ("fff");});
   connect (tb.Act (24), & QAction::triggered,  this, [this]() {Set ("Fz");});
}

void DlgCue::Quit ()  {Gui.DlgSave (this, "DlgCue");}


//______________________________________________________________________________
// dlgfl - PianoCheetah's file list (FL.lst[], FL.pos, FL.ext)
//         and dialog to mess w it

FLstDef FL;

static ubyte NCol;   static char *Col [8];
static ubyt4 NFnd;
static TStr  DirF, DirT;
static File  FFnd;

static bool SongOK (void *ptr, char dfx, char *fn)
// find any a.song files and put em in (StrArr)ptr
{ StrArr *a = (StrArr *) ptr;
  ubyt4   ln = StrLn (fn);
   if ( (dfx == 'f') && (ln > 7) && (! StrCm (& fn [ln-7], "/a.song")) )
      {fn [ln-7] = '\0';   if (! a->Add (fn))  return true;}
   return false;
}

void FLstDef::Load ()
// load last FLst[];  add new files we got;  del gone files
{ ubyt4 i, j, ins1;
  TStr  dr, fn, c;
  File  f;
  StrArr t (CC("FL.Load"), FL.MAX, FL.MAX*sizeof (TStr)/2);
// load prev songlist.txt in cfg dir w order of learn,rep songs
   App.Path (fn, 'c');   StrAp (fn, CC("/songlist.txt"));   t.Load (fn);
   FL.pos = 0;   FL.lst.Ln = t.num;
   for (ubyt4 i = 0;  i < t.num;  i++)
      {StrCp (FL.lst [i], t.str [i]);   FL.lst [i][FL.X] = 'n';}
//DBG("songlist.txt:");for (i=0;i<FL.lst.Ln;i++) DBG("`d `s", i, FL.lst [i]);

// reinit t n list every a.song file in Pianocheetah dir
   App.Path (dr, 'd');
   t.Init (CC("lstS"), FL.MAX, FL.MAX*sizeof (TStr)/2);
   StrFmt (fn, "`s/1_learning",   dr);   f.DoDir (fn, & t, SongOK);
   StrFmt (fn, "`s/2_repertoire", dr);   f.DoDir (fn, & t, SongOK);
   t.Sort ();
//TRC("num 1,2 pc songs=`d", t.num);  t.Dump ();

// if got 2_rep, need 1_learn ins pos
   for (ins1 = 0;  ins1 < FL.lst.Ln;  ins1++)
      if (StrSt (FL.lst [ins1], CC("2_repertoire")))  break;

// upd FL keepin prev order if file still exists (w flag n=>y)
   for (i = 0;  i < t.num;  i++) {
      for (j = 0;  j < FL.lst.Ln;  j++)     // in songlist?  break early
         if (! StrCm (t.str [i], FL.lst [j]))  break;
      if (j >= FL.lst.Ln)  if (! FL.lst.Full ()) {    // new - append ta songlst
         if (StrSt (t.str [i], CC("1_learning")))  j = ins1++;
         if (FL.lst.Full ())  break;
         FL.lst.Ins (j);
         StrCp (FL.lst [j], t.str [i]);
      }
      FL.lst [j][FL.X] = 'y';
   }
   for (j = 0;  j < FL.lst.Ln;)        // del gone ones (still n)
      {if (FL.lst [j][FL.X] == 'n')  FL.lst.Del (j);   else  j++;}

// append done/queue to FLst[]
   t.Init (CC("lstS"), FL.MAX, FL.MAX*sizeof (TStr)/2);
   StrFmt (fn, "`s/3_done",  dr);   f.DoDir (fn, & t, SongOK);
   StrFmt (fn, "`s/4_queue", dr);   f.DoDir (fn, & t, SongOK);
   t.Sort ();
//TRC("num 3,4 pc songs=`d", t.num);  t.Dump ();
   j = FL.lst.Ln;
   for (i = 0;  i < t.num;  i++, j++) {     // FLAG init for rand load
      if (FL.lst.Full ())  break;
      FL.lst.Ins ();
      StrCp (FL.lst [j], t.str [i]);
      FL.lst [j][FL.X] = StrSt (t.str [i], CC("4_queue")) ? 'n' : 'y';
   }
   Save ();
//TRC("FL::Load ln=`d", FL.lst.Ln);
//for(i=0;i<FL.lst.Ln;i++)DBG("`d `s `c", i, FL.lst[i], FL.lst[i][FL.X]);
}


void FLstDef::Save ()
// just dump Learn/Rep (with manual sort) of FLst[] in cfg/songlist.txt
{ TStr fn;
  File f;
   App.Path (fn, 'c');   StrAp (fn, CC("/songlist.txt"));
   if (! f.Open (fn, "w"))  DBG("FL.Save  couldn't write songlist");
   for (ubyt4 r = 0;  r < FL.lst.Ln;  r++) {
      if (StrSt (FL.lst [r], CC("3_done")) ||
          StrSt (FL.lst [r], CC("4_queue")))  break;
      f.Put (FL.lst [r]);  f.Put (CC("\n"));
   }
   f.Shut ();
}


bool FLstDef::DoDir (char *dir)
// if pc dir (dolphin song dir pick), just scoot to it in list
{ ubyt4 i, ln;
  BStr  pc, fn, fne, c;
  File  f;
TRC("FL.DoDir `s", dir);
   FL.pos = 0;
   App.Path (pc, 'd');
   if (MemCm (dir,  pc,  StrLn (pc))) {
      Gui.Hey ("use MidiImport in the song file dialog");
      return false;
   }
   if (! f.Size (StrFmt (fn, "`s/a.song", dir))) {
      if      (f.Size (StrFmt (fn, "`s/a.txt", dir))) {
         App.Run (StrFmt (c, "txt2song `p", fn));
         StrFmt (fne, "`s/RATS.txt", dir);
         if (f.Size (fne)) {
            f.Load (fne, c, sizeof (c), 'z');
            Gui.Hey (c);
            Gui.Quit ();               // eh, just blow the scene, mannn
         }
      }
      else if (f.Size (StrFmt (fn, "`s/a.mid", dir)))
         App.Run (StrFmt (c, "mid2song `p", fn));
      FL.Load ();
   }
   for (ln = StrLn (dir), i = 0;  i < FL.lst.Ln;  i++)
      if (! MemCm (dir, FL.lst [i], ln))
         {FL.pos = i;   return true;}  // got it?  hop to it n go
   return false;
}


bool FLstDef::DoFN (char *fn)          // just do ma dir
{ TStr dir;   StrCp (dir, fn);   Fn2Path (dir);   return DoDir (dir);  }


//______________________________________________________________________________
void DlgFL::Pik ()
{ sbyt2 p;
  TStr  fn, s, bars, tmpo, tsig, ksig, prac, lyr;
  ubyt4 r, i;
  StrArr tb (CC("FLstEtc"), 16000, 6000*sizeof(TStr));
   StrCp (bars, CC("0"));   StrCp (tmpo, CC("120"));   StrCp (tsig, CC("4/4"));
   StrCp (ksig, CC("C"));   StrCp (prac, CC(""));      StrCp (lyr,  CC("0"));

   if ((p = _t.CurRow ()) >= 0)  FL.pos = p;
   FL.ext = false;

// git the .song fn, load for infoz
   StrCp (fn, FL.lst [FL.pos]);   StrAp (fn, CC("/a.song"));
   tb.Load (fn, nullptr, CC("Track:"));

// plow thru top of .song till we hit DrumMap: or Track: and load interestin etc
   for (r = 0;  r < tb.NRow ();  r++) {
      StrCp (s, tb.Get (r));
      if ((! MemCm (s, CC("DrumMap:"), 8)) ||
          (! MemCm (s, CC("Track:"  ), 6)))  break;
      if  (! MemCm (s, CC("info={"), 6)) {
         for (i = r+1;  i < tb.NRow ();  i++) {
            StrCp (s, tb.Get (i));
            if (! MemCm (s, CC("}"), 1))  break;

            if (! MemCm (s, CC("bars" ), 4))  StrCp (bars, & s [7]);
            if (! MemCm (s, CC("tempo"), 5))  StrCp (tmpo, & s [7]);
            if (! MemCm (s, CC("tsig" ), 4))  StrCp (tsig, & s [7]);
            if (! MemCm (s, CC("ksig" ), 4))  StrCp (ksig, & s [7]);
            if (! MemCm (s, CC("prac" ), 4))  StrCp (prac, & s [7]);
            if (! MemCm (s, CC("lyric"), 5))  StrCp (lyr,  & s [7]);
         }
      }
   }
   { CtlLabl l (ui->bars);   l.Set (StrFmt (s, "bars\n`s",     bars));}
   { CtlLabl l (ui->tmpo);   l.Set (StrFmt (s, "tempo\n`s",    tmpo));}
   { CtlLabl l (ui->tsig);   l.Set (StrFmt (s, "timeSig\n`s",  tsig));}
   { CtlLabl l (ui->ksig);   l.Set (StrFmt (s, "keySig\n`s",   ksig));}
   { CtlLabl l (ui->prac);   l.Set (StrFmt (s, "practice\n`s", prac));}
   { CtlLabl l (ui->lyr );   l.Set (StrFmt (s, "lyrics\n`s",   lyr ));}
}


void DlgFL::ReDo ()                    // FL.lst/FL.pos => gui tbl
{ char *ro [10];                       // if lst is huge, learn may not show :/
  TStr  ts, s1, s2;
  ubyt4 i, ln, p;
  bool  all;
  CtlChek c (ui->all);   all = c.Get ();
   _t.Open ();
   if (! (ln = FL.lst.Ln))  {_t.Shut ();   return;}

   App.Path (ts, 'd');   p = StrLn (ts) + 1;
   ro [0] = s1;   ro [1] = s2;   ro [2] = nullptr;
   for (i = 0;  i < ln;  i++) {
      StrCp (ts, & FL.lst [i][p]);
//DBG("i=`d/`d all=`b FL.pos=`d p=`d ts=`s", i, ln, all, FL.pos, p, ts);
      if ((! all) && (*ts >= '3')) {   // not doin all?  done unless FL.pos sez
         if (FL.pos < i)  break;
         Gui.Hey ("pick a learn/rep song to uncheck all");
         all = true;  c.Set (true);
      }
      switch (*ts) {                   // 1_learning/ 2_repertoire/ 3_done/ etc
         case '1':  StrCp (s1, CC("learn"));   StrCp (s2, & ts [11]);   break;
         case '2':  StrCp (s1, CC("rep"));     StrCp (s2, & ts [13]);   break;
         case '3':  StrCp (s1, CC("done"));    StrCp (s2, & ts [ 7]);   break;
         case '4':  StrCp (s1, CC("queue"));   StrCp (s2, & ts [ 8]);   break;
      }
      _t.Put (ro);
   }
   _t.Shut ();
// if (_t.ColW (1) > 600)  _t.SetColW (1, 600);  // or we get scrollin right :(
   _t.HopTo (FL.pos, 0);
   Pik ();
}


//______________________________________________________________________________
/*ubyte FindBuf [2*1024*1024];*/

static char *FLFind (char *fn, ubyt2 len, ubyt4 pos, void *ptr)
// find cached mid files matchin search Col[NCol] - count,make found.txt
{ ubyte i;                     (void)len; (void)pos; (void)ptr;
   for (i = 0;  i < NCol;  i++)  if (! StrSt (fn, Col [i]))  return nullptr;
/* searchin strings inside .mid doesn't seem worth it
**ubyt4 sz;
**File  f;
** {
** // welp, fn doesn't match, check it's strings
**    sz = f.Load (fn, FindBuf, sizeof (FindBuf));
**    for (i = 0;  i < NCol;  i++)  if (! MemSt (FindBuf, Col [i], sz))
**                                     return nullptr;
** }
*/
   NFnd++;
//DBG("FLFind fn=`s", fn);
   FFnd.Put (fn);   FFnd.Put (CC("\n"));   return nullptr;
}


static char *FLCopy (char *fr, ubyt2 len, ubyt4 pos, void *ptr)
// copy and Mid2Song found mids (limit to 500)
{ ubyte i;                     (void)len;            (void)ptr;
  BStr  to, frP, toP, cmd;
  File  f;
   if (pos >= 500)  return CC("enough, pal!");
   StrCp (to, fr);   StrAp (to, CC(""), 4);   FnFix (to);
   StrFmt (frP, "`s/`s",          DirF, fr);
   StrFmt (toP, "`s/`s/path.txt", DirT, to);   f.Save (toP, frP, StrLn (frP));
   StrFmt (toP, "`s/`s/a.mid",    DirT, to);   f.Copy (frP, toP);
   App.Run (StrFmt (cmd, "mid2song `p", toP));
   return nullptr;
}


void DlgFL::Find ()
{ ubyt4 i, ln;
  TStr  srch, dMid, dFnd, fnC, fnF, c;
  File  f;
  Path  d;
// turn srch into Col[],NCol.  get search dir n save.
  CtlLine l (ui->srch);
   StrCp (srch, l.Get ());
  ColSep ss (srch, 8);
   for (NCol = 0;  ss.Col [NCol][0];  NCol++)  Col [NCol] = ss.Col [NCol];
   App.CfgGet (CC("DlgFL_dir"), dMid);
//DBG("a dir=`s", dMid);
   if (*dMid == '\0')  StrCp (dMid, getenv ("HOME"));
//DBG("b dir=`s", dMid);
   if (! Gui.AskDir (dMid, "pick dir to search for songs in (NOT / please)"))
      return;

   App.CfgPut (CC("DlgFL_dir"), dMid);

   StrFmt (fnC, "`s/_midicache.txt", dMid);
   if (! f.Size (fnC)) {               // no cache yet so start makin one
DBG("no _midicache.txt for `s", dMid);
      App.Run (StrFmt (c, "ll midi `p &", dMid));
      Gui.Hey ("Making midi cache in that dir...\n"
               "Come back when __midicache.txt turns to _midicache.txt");
      return;
   }

// wipe n recreate 4_queue/found;  start writin 4_queue/found.txt
   StrFmt (dFnd, "`s/4_queue/found", App.Path (c, 'd'));
   d.Kill (dFnd);   d.Make (dFnd);
   NFnd = 0;   if (! FFnd.Open (StrFmt (fnF, "`s.txt", dFnd), "w"))  return;

// FLFind() each cache fn  n shut found.txt
   f.DoText (fnC, nullptr, FLFind);   FFnd.Shut ();
DBG("found `d", NFnd);
   if (NFnd == 0)   {Gui.Hey ("rats!  got nothin");   return;}
   if (NFnd >= 500)  if (Gui.YNo (
                            "I'm only copyin 500 of the midi files, pal.\n"
                            "wanna view all matched filenames?"))
                        App.Open (fnF);
// ok copy em to 4_queue/found
   StrCp (DirF, dMid);   StrCp (DirT, dFnd);   f.DoText (fnF, nullptr, FLCopy);

// relist and move pos to 4_queue/found
  CtlChek a (ui->all);
   a.Set (true);   FL.Load ();   FL.pos = 0;
   for (ln = StrLn (dFnd), i = 0;  i < FL.lst.Ln;  i++)
      if (! MemCm (dFnd, FL.lst [i], ln))  {FL.pos = i;   break;}
   ReDo ();
}


void DlgFL::Up ()
{ ubyt4 p = FL.pos;
   if (p == 0)  return;
   FL.lst.MvUp (p);   FL.pos--;   FL.Save ();   ReDo ();
}

void DlgFL::Dn ()
{ ubyt4 p = FL.pos;
   if (p >= FL.lst.Ln-1)  return;
   FL.lst.MvDn (p);   FL.pos++;   FL.Save ();   ReDo ();
}

void DlgFL::MidImp ()
{ TStr d;
  BStr c;
   App.CfgGet    (CC("DlgFL_middir"), d);
   if (*d == '\0')  StrCp (d, getenv ("HOME"));
   if (Gui.AskDir (d, "pick dir with midi files")) {
      App.CfgPut (CC("DlgFL_middir"), d);   // remember it
      App.Spinoff (StrFmt (c, "midimp `p", d));
   }
}

void DlgFL::Song2Wav ()
{ TStr fn;
  BStr c;
   StrCp (fn, FL.lst [FL.pos]);   StrAp (fn, CC("/a.song"));
   App.Spinoff (StrFmt (c, "song2wav `p", fn));
}

void DlgFL::Sfz2Syn ()
{ TStr d;
  BStr c;
   App.CfgGet    (CC("DlgFL_sfzdir"), d);
   if (*d == '\0')  StrCp (d, getenv ("HOME"));
   if (Gui.AskDir (d, "pick dir with .sfz files")) {
      App.CfgPut (CC("DlgFL_sfzdir"), d);   // remember it
      App.Spinoff (StrFmt (c, "sfz2syn `p", d));
   }
}

void DlgFL::Mod2Song ()
{ TStr d;
  BStr c;
   App.CfgGet    (CC("DlgFL_moddir"), d);
   if (*d == '\0')  StrCp (d, getenv ("HOME"));
   if (Gui.AskDir (d, "pick dir with .sfz files")) {
      App.CfgPut (CC("DlgFL_moddir"), d);   // remember it
      App.Spinoff (StrFmt (c, "mod2song `p", d));
   }
}

void DlgFL::Brow ()
{ TStr d, dv;
   App.Open (StrFmt (dv, "`s/device", App.Path (d, 'd')));
}


//______________________________________________________________________________
void DlgFL::Open ()  {ReDo ();   show ();   raise ();   activateWindow ();}

void DlgFL::Shut ()
{   FL.pos = _t.CurRow ();   done (true);   lower ();   hide ();  }

void DlgFL::Init ()
{  Gui.DlgLoad (this, "DlgFL");
   FL.Load ();
  CtlTBar tb;
   tb.Init (this, "flst");
   tb.Btn (0, CC("Up\n"
                 "Scoot song up in the list"));
   tb.Btn (1, CC("Down\n"
                 "Scoot song down in the list"));
   tb.Btn (2, CC("Search\n"
                 "Search a big midi file dir for matching search strings\n"
                 "   fill in the search box below THEN click me"));
   tb.Btn (3, CC("MidiImport\n"
                 "Pick a dir tree with midi files to convert to songs\n"
                 "in the 4_queue dir"));
   tb.Btn (4, CC("Song2Wav\n"
                 "Render .song to a .wav file"));
   tb.Btn (5, CC("Sfz2Syn\n"
                 "Pick a dir with .sfz files to add to a Syn sound bank"));
   tb.Btn (6, CC("Mod2Song\n"
                 "Pick a .mod file to convert to\n"
                 "a .song and a Syn sound bank"));
   tb.Btn (7, CC("Browse\n"
                 "Open file browser in PianoCheetah/device directory\n"
                 "   to delete/rename/etc"));
   connect (tb.Act (0), & QAction::triggered,  this, & DlgFL::Up);
   connect (tb.Act (1), & QAction::triggered,  this, & DlgFL::Dn);
   connect (tb.Act (2), & QAction::triggered,  this, & DlgFL::Find);
   connect (tb.Act (3), & QAction::triggered,  this, & DlgFL::MidImp);
   connect (tb.Act (4), & QAction::triggered,  this, & DlgFL::Song2Wav);
   connect (tb.Act (5), & QAction::triggered,  this, & DlgFL::Sfz2Syn);
   connect (tb.Act (6), & QAction::triggered,  this, & DlgFL::Mod2Song);
   connect (tb.Act (7), & QAction::triggered,  this, & DlgFL::Brow);

   _t.Init (ui->fLst, "Stage\0Song\0", nullptr, "single", "row", 'w');
   connect (ui->fLst, &QTableWidget::itemClicked,       this, & DlgFL::Pik);
   connect (ui->fLst, &QTableWidget::itemDoubleClicked, this, & DlgFL::Shut);
   _t.SetColWrapOK (1);

  CtlChek a (ui->all);
  TStr t;
   App.CfgGet (CC("DlgFL_all"), t);
   if (*t)  a.Set ((*t=='y')?true:false);   else a.Set (true);

   connect (ui->all, &QCheckBox::checkStateChanged, this, & DlgFL::ReDo);
}

void DlgFL::Quit ()
{ CtlChek a (ui->all);
  TStr t;
   StrCp (t, CC(a.Get ()?"y":"n"));   App.CfgPut (CC("DlgFL_all"),  t);
   Gui.DlgSave (this, "DlgFL");
}


//______________________________________________________________________________
// dlgQua - quantize note

void DlgQua::Set (char q)
{  emit sgCmd (StrFmt (Up.pos.str, "`s `c", _s, q));   Shut ();  }

void DlgQua::Open ()
{  StrFmt (_s, "qua `d `d", Up.pos.tr, Up.pos.p);
  CtlLabl l (ui->info);
   l.Set (Up.pos.str);
  CtlBttn qp (ui->qp);
  CtlBttn qn (ui->qn);
  TStr s;
   qp.Set (StrFmt (s, "quantPrev: `s", Up.pos.stp));
   qn.Set (StrFmt (s, "quantNext: `s", Up.pos.stn));
   show ();   raise ();   activateWindow ();
   Gui.DlgMv (this, Up.gp, "Tc");
}

void DlgQua::Shut ()  {done (true);   lower ();   hide ();}

void DlgQua::Init ()
{  Gui.DlgLoad (this, "DlgQua");
   connect (ui->qp, & QPushButton::clicked, this, [this]() {Set ('p');});
   connect (ui->qn, & QPushButton::clicked, this, [this]() {Set ('n');});
}

void DlgQua::Quit ()  {Gui.DlgSave (this, "DlgQua");}


//______________________________________________________________________________
// dlgHlp - help meee

void DlgHlp::Open ()
{  setAttribute (Qt::WA_ShowWithoutActivating, true);
   setWindowFlags (windowFlags () |    // doesn't seem to work
      Qt::Tool |                       // maybe cuz flatpak ??
      Qt::WindowStaysOnTopHint |
   // Qt::WindowTransparentForInput |
   //   Qt::FramelessWindowHint |
   //   Qt::NoDropShadowWindowHint |
   //   Qt::X11BypassWindowManagerHint |
      Qt::WindowDoesNotAcceptFocus);
   show ();   raise ();
/* don't steal focus activateWindow (); */  }

void DlgHlp::Shut ()
{  Gui.DlgSave (this, "DlgHlp");
   done (true);   lower ();   hide ();
}

void DlgHlp::Init ()
{ char *ro [10];
   ro [4] = nullptr;
   Gui.DlgLoad (this, "DlgHlp");
   setAttribute (Qt::WA_ShowWithoutActivating, true);
   setWindowFlags (windowFlags () |
      Qt::Tool |
      Qt::WindowStaysOnTopHint |
   // Qt::WindowTransparentForInput |
   //   Qt::FramelessWindowHint |
   //   Qt::NoDropShadowWindowHint |
   //   Qt::X11BypassWindowManagerHint |
      Qt::WindowDoesNotAcceptFocus);
   _t.Init (ui->t, "Group\0Note\0Key\0Command Description\0");
   _t.Open ();
   for (ubyte i = 0;  i < NUCmd;  i++) {
      ro [0] = CC(UCmd [i].grp);   ro [1] = CC(UCmd [i].nt);
      ro [2] = CC(UCmd [i].ky);    ro [3] = CC(UCmd [i].desc);
      _t.Put (ro);
   }
   _t.Shut ();
}

void DlgHlp::Quit ()  {}


//______________________________________________________________________________
// dlgKSg - edit keysig

static char *Ma = CC(
"C   (0b)\0"                  "Db  (5b  also C# below)\0"
"D   (2#)\0"                  "Eb  (3b)\0"
"E   (4#)\0"                  "F   (1b)\0"
"Gb  (6b  also F# below)\0"   "G   (1#)\0"
"Ab  (4b)\0"                  "A   (3#)\0"
"Bb  (2b)\0"                  "B   (5#  also Cb below)\0"
"F#  (6#  also Gb above)\0"   "C#  (7#  also Db above)\0"
"Cb  (7b  also B  above)\0"),
            *Mi = CC(
"A   (0b)\0"                  "Bb  (5b  also A# below)\0"
"B   (2#)\0"                  "C   (3b)\0"
"C#  (4#)\0"                  "D   (1b)\0"
"Eb  (6b  also D# below)\0"   "E   (1#)\0"
"F   (4b)\0"                  "F#  (3#)\0"
"G   (2b)\0"                  "G#  (5#  also Ab below)\0"
"D#  (6#  also Eb above)\0"   "A#  (7#  also Bb above)\0"
"Ab  (7b  also G# above)\0");

void DlgKSg::Open ()
{  show ();   raise ();   activateWindow ();
   StrCp (_s, Up.pos.etc);
  TStr s;
   StrCp (s, Up.pos.str);
  char *m = & s [StrLn (s)-1];
  CtlList maj (ui->maj, CC("Major\0Minor\0")),
          key (ui->key, *m == 'm' ? Mi : Ma);
   maj.Set (*m == 'm' ? 1 : 0);
   if (*m == 'm')  *m = '\0';
   StrAp (s, CC(" "));   key.SetS (s);
   Gui.DlgMv (this, Up.gp, "tR");
}

void DlgKSg::Shut ()
{ TStr s;
  CtlList key (ui->key), maj (ui->maj);
   key.GetS (s);
   if (s [1] == ' ')  s [1] = '\0';   else s [2] = '\0';
   if (maj.Get ())  StrAp (s, CC("m"));
   StrAp (_s, s);
   emit sgCmd (_s);
   done (true);   lower ();   hide ();
}

void DlgKSg::Init ()
{  Gui.DlgLoad (this, "DlgKSg");
   connect (ui->maj, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() {
     ubyt2   p =  ui->key->currentIndex ();
     CtlList key (ui->key, ui->maj->currentIndex () ? Mi : Ma);
      key.Set (p);
   });
}

void DlgKSg::Quit ()  {Gui.DlgSave (this, "DlgKSg");}


//______________________________________________________________________________
// dlgMov - what ta do with our rect o notes

void DlgMov::Open ()
{  show ();   raise ();   activateWindow ();
   Gui.DlgMv (this, Up.gp, "Tc");
}

void DlgMov::Shut ()  {done (true);   lower ();   hide ();}

void DlgMov::Init ()
{  Gui.DlgLoad (this, "DlgMov");
   connect (ui->h2r, & QPushButton::pressed,
            this, [this]() {StrCp (Up.pos.str, CC(">"));   Shut ();});
   connect (ui->h2l, & QPushButton::pressed,
            this, [this]() {StrCp (Up.pos.str, CC("<"));   Shut ();});
   connect (ui->h2b, & QPushButton::pressed,
            this, [this]() {StrCp (Up.pos.str, CC("#"));   Shut ();});
   connect (ui->del, & QPushButton::pressed,
            this, [this]() {StrCp (Up.pos.str, CC("x"));   Shut ();});
}

void DlgMov::Quit ()  {Gui.DlgSave (this, "DlgMov");}


//______________________________________________________________________________
// dlgTDr - pick a new drum track per section (patA,patB,fill)

static BStr ALst, BLst, FLst;

static char TDrPop (char *ls, ubyt2 r, ubyte c)
{  (void)r;   ZZCp (ls, (c==1) ? ALst : ((c==2) ? BLst : FLst));  return 'l';}


void DlgTDr::Cmd ()
{ ubyt2 r = _t.CurRow ();
  ubyte c = _t.CurCol ();
  TStr  s;
TRC("DlgTDr::Upd r=`d c=`d s=`s", r, c, _t.Get (r, c));
   emit sgCmd (StrFmt (s, "tDr `d `d `s", r, c, _t.Get (r, c)));
}


//______________________________________________________________________________
void DlgTDr::Init ()
{ TStr s, fn;
  BStr b;
  StrArr sa;
   Gui.DlgLoad (this, "DlgTDr");
   StrFmt (fn, "`s/clip/drum/main", App.Path (s, 'd'));
   sa.GetDir (fn, 'f', 1024, 'x');
   sa.SetZZ (b);
   *ALst = *BLst = *FLst = '\0';
   ZZAp (ALst, CC("(off)\0(continue)\0"));   ZZAp (ALst, b);
   ZZAp (BLst, CC("(off)\0"));               ZZAp (BLst, b);
   StrAp  (fn,           CC("fill"), 4);
   sa.GetDir (fn, 'f', 1024, 'x');   sa.SetZZ (b);
   ZZAp (FLst, CC("(off)\0"));               ZZAp (FLst, b);
   _t.Init (ui->t, "Section\0_PatA\0_PatB\0_Fill\0", TDrPop);
   connect (ui->t, & QTableWidget::itemChanged, this, & DlgTDr::Cmd);
}


void DlgTDr::Open ()
{ ubyte r, c;
  char *ro [5];
   if (Up.nR == 0) {
      Gui.Hey ("...oops!  Add some Cues!\n" "(for verse/chorus/etc)");
      return;
   }
   show ();   raise ();   activateWindow ();
   ro [4] = nullptr;
   _t.Open ();
   for (r = 0;  r < Up.nR;  r++) {
      for (c = 0;  c < 4;  c++)  ro [c] = Up.d [r][c];
      _t.Put (ro);
   }
   _t.Shut ();
}


void DlgTDr::Shut ()  {done (true);   lower ();   hide ();}
void DlgTDr::Quit ()  {Gui.DlgSave (this, "DlgTDr");}


//______________________________________________________________________________
// dlgTpo - edit tempo

void DlgTpo::Open ()
{  show ();   raise ();   activateWindow ();
   StrCp (_s, Up.pos.etc);
  CtlSpin sp (ui->num, 1, 960);
TRC("DltTpo::Open in=`s etc=`s", Up.pos.str, _s);
   sp.Set (Str2Int (Up.pos.str));
   Gui.DlgMv (this, Up.gp, "Tc");
}

void DlgTpo::Shut ()
{ TStr s;
   StrAp (_s, Int2Str (ui->num->value (), s));
TRC("DltTpo::Shut done=`s", _s);
   emit sgCmd (_s);
   done (true);   lower ();   hide ();
}

void DlgTpo::Init ()  {Gui.DlgLoad (this, "DlgTpo");}
void DlgTpo::Quit ()  {Gui.DlgSave (this, "DlgTpo");}


//______________________________________________________________________________
// dlgTSg - edit timesig

void DlgTSg::Open ()
{  show ();   raise ();   activateWindow ();
   StrCp (_s, Up.pos.etc);
TRC("DlgTSg::Open in=`s etc=`s", Up.pos.str, _s);
  TStr  n;
  char *d, *s;
  ubyte i;
  BStr  ld, ls;
   *ld = *ls = '\0';
   for (i = 0;  i < 16;  i++)  ZZAp (ld, StrFmt (n, "`d", 1 << i));
   for (i = 0;  i <  9;  i++)  ZZAp (ls, StrFmt (n, "`d", i +  1));
   StrCp (n, Up.pos.str);
   d = StrCh (n, '/');   *d++ = '\0';
   s = StrCh (d, '/');   if (s)  *s++ = '\0';  else s = CC("1");
  CtlSpin num (ui->num, 1, 255);
  CtlList den (ui->den, ld),
          sub (ui->sub, ls);
   num.Set (Str2Int (n));   den.SetS (d);   sub.SetS (s);
   Gui.DlgMv (this, Up.gp, "Tc");
}

void DlgTSg::Shut ()
{ TStr s;
  CtlSpin num (ui->num);
  CtlList den (ui->den), sub (ui->sub);
   StrFmt (_s, "`d/`d", num.Get (), den.Get ());
   if (sub.Get ())  StrAp (_s, StrFmt (s, "/`d", sub.Get ()));
   emit sgCmd (_s);
   done (true);   lower ();   hide ();
}

void DlgTSg::Init ()  {Gui.DlgLoad (this, "DlgTSg");}
void DlgTSg::Quit ()  {Gui.DlgSave (this, "DlgTSg");}
