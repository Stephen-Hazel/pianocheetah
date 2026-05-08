// sCmd.cpp - song thread's main Cmd () for gui thread to use

#include "song.h"

void Song::Cmd (QString s)
{ TStr  c;
  ubyte i;
   StrCp (c, UnQS (s));
DBGTH("PcSng");                        // first time Song is hit :/
DBG("Cmd='`s'", c);
   for (i = 0;  i < NUCmd;  i++)  if (! StrCm (c, CC(UCmd [i].cmd)))  break;
   if (i < 6)     {emit sgUpd (s);   return;}
   if (i < NUCmd)  switch (i) {
      case  6:  EdTime (1);  break;         // timeBar1
      case  7:  EdTime (2);  break;         // time<
      case  8:  EdTime (3);  break;         // time>
      case  9:  EdTime (4);  break;         // time<<
      case 10:  EdTime (5);  break;         // time>>
      case 11:  EdTime (0);  break;         // timePoz
      case 12:  EdTime (6);  break;         // timeBug

      case 13:  EdTmpo (0);  break;         // tempoHop
      case 14:  EdTmpo (1);  break;         // tempo<
      case 15:  EdTmpo (2);  break;         // tempo>
      case 16:  EdTmpo (3);  break;         // tran<
      case 17:  EdTmpo (4);  break;         // tran>

      case 18:  EdLrn  (0);  break;         // learn
      case 19:  RecWipe ();  break;         // recWipe
      case 20:  Save ('r');  break;         // recSave
      case 21:  EdLrn  (3);  break;         // color
      case 22:  EdLrn  (5);  break;         // hearRec
      case 23:  EdLrn  (4);  break;         // hearLoop
   }
   else if (! StrCm (c, CC("init")))      Init ();
   else if (! StrCm (c, CC("quit")))     {Quit ();   return;}
   else if (! StrCm (c, CC("wipe")))      Wipe ();
   else if (! MemCm (c, CC("load "), 5))  Load (& c [5]);
   else if (! StrCm (c, CC("mute")))      EdLrn (6);
   else if (! StrCm (c, CC("prac")))      EdLrn (7);
   else if (! StrCm (c, CC("dump")))      Dump (true);
   else if (! StrCm (c, CC("quan")))      {SetDn ('q');   ReDo ();}
   else if (! StrCm (c, CC("showAll")))   ShowAll ();
   else if (! MemCm (c, CC("tran "), 5))
      {NotesOff ();   _f.tran  = (sbyte) Str2Int (& c [5]);   DscSave ();}
   else if (! MemCm (c, CC("htype "), 6))  HType  (  c [6]);
   else if (! MemCm (c, CC("trk "),   4))
                            {StrCp (_f.trk [Up.eTrk].name, & c [4]);   ReDo ();}
   else if (! MemCm (c, CC("grp "),   4))  NewGrp (& c [4]);
   else if (! MemCm (c, CC("snd "),   4))  NewSnd (& c [4]);
   else if (! MemCm (c, CC("dev "),   4))  NewDev (& c [4]);
   else if (! MemCm (c, CC("drmap "), 6))  DrMap  (& c [6]);
   else if (! MemCm (c, CC("trkEd "), 6))  TrkEd  (& c [6]);
   else if (! MemCm (c, CC("preTDr"), 6))  PreTDr ();
   else if (! MemCm (c, CC("tDr "),   4))     TDr (& c [4]);
   else if (! MemCm (c, CC("ctl"),    3))  Ctl    ();
   else if (! MemCm (c, CC("cue "),   4))  Cue    (& c [4]);
   else if (! MemCm (c, CC("chd "),   4))  Chd    (& c [4]);
   else if (! MemCm (c, CC("qua "),   4))  Qua    (& c [4]);
   else if (! MemCm (c, CC("setCtl "),7))  SetCtl (& c [7]);
   else if (! StrCm (c, CC("mov")))        Mov    ();
   Put ();
}


//______________________________________________________________________________
ubyte Song::ChkETrk ()                 // be sure eTrk is still ok
{  if (Up.eTrk >= _f.trk.Ln)  Up.eTrk = (ubyte)(_f.trk.Ln-1);
   return Up.eTrk;
}

void Song::RecWipe ()
{  NotesOff ();
   _recM.Ln = _recD.Ln = 0;
   if (! RCRD)  for (ubyte t = 0;  t < _f.trk.Ln;  t++)
      if (TLrn (t))  EvDel (t, 0, _f.trk [t].ne);
   ReDo ();
}


//______________________________________________________________________________
void Song::EdTime (char ofs)           // edit song time
{ ubyt2 bar, bt;
  TStr  str;
  char *s;
  bool  tofs = true;
// init str, bar n bt to "now"
   TmStr (str, _timer->Get () + (M_WHOLE/64));   // round time to next bar
   bar = (ubyt2)Str2Int (str, & s);   bt = (ubyt2)Str2Int (s+1);

   switch (ofs) {
   case 0:  NotesOff ();               // toggle timer's pause state
            Up.uPoz = Up.uPoz ? false : true;   Poz (Up.uPoz);
            emit sgUpd ("tbPoz");
            return;

   case 1:  if (PRAC || _lrn.pLrn)  {SetLp ('i');   return;}    // prac instead!

            Poz (false);               // timeBar1  ooUHN MO TAHM!
            TmHop ((PRAC || (_lrn.pLrn==LPRAC)) ? _lrn.lpBgn : 0);
            if (Up.uPoz)  Poz (true);   else Put ();       // restart schedulin
            return;

   case 2:  if ((bar > 1) && (bt <= 2))  bar--;     break;      // time<
   case 3:                               bar++;     break;      // time>

   case 4:  if (PRAC || _lrn.pLrn)  {SetLp ('<');   return;}    // time<<
            if (_pg) {
               bt =        _pag [_pg-1].col [0].blk [0].bar;
               if ((_pg == 1) || (bar > bt))  bar = bt;
               else  bar = _pag [_pg-2].col [0].blk [0].bar;
               tofs = false;
               break;
            }
            if (bar <= 8) bar = 1;  else bar -= 8;   break;
   case 5:  if (PRAC || _lrn.pLrn)  {SetLp ('>');   return;}    // time>>
            if (_pg) {
               bar = (_pg >= _pag.Ln) ? _bEnd :
                     _pag [_pg].col [0].blk [0].bar;
               tofs = false;
               break;
            }
                                         bar += 8;  break;
   case 6:  if (! PRAC) {              // timeBug
              ubyte t;                 // if no lrn, show message
               for (t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t))  break;
               if (t >= _f.trk.Ln)  {Cmd ("learn");   return;}

               Up.lrn = LPRAC;
            }
            SetLp ('b');   Cmd ("recWipe");   Cmd ("timeBar1");
            return;
   }
// hop to new bar (minus a teeny bit)
  ubyt4 t = Bar2Tm (bar);
   if (tofs)  {if (t >= (M_WHOLE/64))  t -= (M_WHOLE/64);   else t = 0;}
   TmHop (t);   if (! Up.uPoz)  Put ();
}


//______________________________________________________________________________
void Song::EdTmpo (char ofs)           // tempo
// bump curr tempo and ins/upd event in bar#1 of a drum trak
{ ubyt4 tp;
  ubyt2 tt;
  TStr  ts;
   if      (ofs <= 2) {
      tt = TmpoAt (_timer->Get ());    // straight track tempo (tmpoSto)
      tp = _timer->Tempo ();           // actual current tempo
      switch (ofs) {
      case 0:                          // 60=>80=>100%
         if      (_f.tmpo >= 9*FIX1/10)     // 100=>60
            tp = (60*tt / 100) + (((60*tt % 100) >= 50)?1:0);
         else if (_f.tmpo <= 7*FIX1/10)     // 60=>80
            tp = (80*tt / 100) + (((80*tt % 100) >= 50)?1:0);
         else                               // 80=>100
            tp = tt;
         break;
      case 1:  if (tp >= 2)    tp--;   break;    // bump to wanted actual
      case 2:  if (tp <= 959)  tp++;   break;
      }                                          // calc frac to make sto=>act
      _f.tmpo = (FIX1*tp / tt) + ((FIX1*tp % tt >= (tt/(ubyt4)2)) ? 1 : 0);
DBG("EdTmpo `d  _f.tmpo=`d/`d tmpoSto=`d tmpoAct=`d",
ofs, _f.tmpo, FIX1, tt, tp);
      DscSave ();   PutTp (tt);   ReTrk ();
   }
   else if (ofs <= 4) {                // transpose, actually :/
      NotesOff ();                     // SHUSH !
      if (ofs == 3) {if (_f.tran > (sbyte)-12) _f.tran--;}
      else          {if (_f.tran < (sbyte) 12) _f.tran++;}
      DscSave ();   StrFmt (ts, "transpose=`d", _f.tran);   Hey (ts);
   }
}


//______________________________________________________________________________
void Song::EdLrn (char ofs)            // this has gotten pretty hairy :(
{ ubyte e = ChkETrk (), t;
  char  c;
   if      (ofs == 0) {                // learn - toggle _f.lrn
      for (t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t))  break;
      if (t >= _f.trk.Ln) {
         Up.lrn = LHEAR;
         Hey (CC("First, pick track(s) to practice - "
                 "click track's Lrn column into a green arrow "
                 "(practice track)"));
      }
      else {
         if      (PLAY)  {Up.lrn = LPRAC;   SetLp ('i');}
         else if (PRAC)   Up.lrn = LHEAR;
         else             Up.lrn = LPLAY;
      }
      if (RCRD) {
        ubyt4 i;
         for (i = 0;  i < _f.cue.Ln;  i++)  // need ta init loops for 1st time ?
            if (_f.cue [i].tend && (_f.cue [i].s [0] == '['))  break;
         if (i >= _f.cue.Ln)  LoopInit ();
      }
      ReDo ();
      if (PRAC)  {Cmd ("recWipe");   Cmd ("timeBar1");}
      return;
   }
   else if (ofs == 3) {                // color
      if (++Cfg.ntCo == 3)  Cfg.ntCo = 0;   Cfg.Save ();
     TStr s;
      StrCp (s, CC("color of "));
      switch (Cfg.ntCo) {
         case 0:  StrAp (s, CC("scale"));      break;
         case 1:  StrAp (s, CC("velocity"));   break;
         default: StrAp (s, CC("track"));
      }
      Info (s);
   }
   else if (ofs == 4) {                // hearLoop (lrn not rec)
      t = Up.lrn;   Cmd ("recWipe");   Cmd ("timeBar1");
      Up.lrn = LHEAR;   _lrn.pLrn = t;   emit sgUpd ("tbLrn");
      if (_lrn.POZ)  {_lrn.POZ = false;   Poz (false);}
      return;                          // unpoz cuz we might be after timeBar1
   }
   else if (ofs == 5) {                // hearRec  (rec not lrn)
      t = Up.lrn;   Cmd ("timeBar1");   TmpoPik ('r');
      Up.lrn = LHREC;   _lrn.pLrn = t;   emit sgUpd ("tbLrn");
      if (_lrn.POZ)  {_lrn.POZ = false;   Poz (false);}
      return;
   }
   else if (ofs == 6) {                // flip shh (reset lrn)
      _f.trk [e].lrn = false;
      _f.trk [e].shh = ! _f.trk [e].shh;
   }
   else if (ofs == 7) {                // flip lrn (force ht/oct if lrn on)
      _f.trk [e].shh = false;
      if ((_f.trk [e].lrn = ! _f.trk [e].lrn)) {
         if (TDrm (e))  _f.trk [e].ht = '\0';
         else {
            c = _f.trk [e].ht;
            if ((c < '1') || (c > '7')) {
               for (t = 0;  t < _f.trk.Ln;  t++)
                  if ((t != e) && (_f.trk [t].ht == '4'))  break;
               _f.trk [e].ht = (t < _f.trk.Ln) ? '3' : '4';
            }
         }
      }
   }
   ReDo ();
}


//______________________________________________________________________________
void Song::HType (char c)              // \0=bg,1..7=lrn,x=flip Show
{ ubyte e = ChkETrk ();
TRC("HType `c t=`d", c, e);
   if      (c == 'x')                  // flip show
      c = (_f.trk [e].ht == 'S') ? '\0' : 'S';
   else if ((c < '1') || (c > '7'))    // non 1..7 => \0
      c = '\0';
   _f.trk [e].ht = c;
   ReDo ();
}


void Song::ShowAll ()
{ bool  dr, all = true;             // drums or melo, all shown now?
  ubyte t, nt = _f.trk.Ln;
   dr = TDrm (Up.eTrk);
   for (t = 0;  t < nt;  t++)  if (! _f.trk [t].lrn) {
      if (dr)  {if (  TDrm (t))  if (_f.trk [t].ht != 'S')
                                    {all = false;   break;}}
      else      if (! TDrm (t))  if (_f.trk [t].ht != 'S')
                                    {all = false;   break;}
   }
   for (t = 0;  t < nt;  t++)  if (! _f.trk [t].lrn) {
      if (dr)  {if (  TDrm (t))  _f.trk [t].ht = all ? '\0' : 'S';}
      else      if (! TDrm (t))  _f.trk [t].ht = all ? '\0' : 'S';
   }
   ReDo ();
}
