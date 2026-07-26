// sEdit.cpp - gory edit funcs o Song tied to mouse,dialogs,etc

#include "song.h"

void Song::PreTDr (bool kick)
// load Up.nR/.d[] w song's section cues n drum rhy picks
{ ubyte n, j;
  ubyt4 i, ln, pLn;
  BStr  b;
  TStr  s;
  char *p, *e;
   n = Up.nR = 0;
   for (i = 0;  i < _f.cue.Ln;  i++)  if (_f.cue [i].s [0] == '(') {
      for (j = 0;  j < n;  j++)
         if (! StrCm (& _f.cue [i].s [1], Up.d [j][0]))  break;
      if (j >= n) {
         StrCp (s, CC("(off)"));
         StrCp (Up.d [n][1], s);   StrCp (Up.d [n][2], s);
                                   StrCp (Up.d [n][3], s);
         StrCp (Up.d [n++][0], & _f.cue [i].s [1]);
      }                                // ^ new section
   }
   DscGet (CC("drumpat={"), b);
// search thru song's drumpat desc lines n set pata,patb,fill per section
   ln = 0;  pLn = 0;
   do {
      p = & b [pLn];
      if ((e = StrCh (p, '\n')))  for (i = 0;  i < n;  i++) {
         StrFmt (s, "`s=", Up.d [i][0]);
         if (! MemCm (p, s, StrLn (s))) {
            p += StrLn (s);
            MemCp (s, p, (e-p));   s [e-p] = '\0';
           ColSep ss (s, 3);
            for (j = 0;  j < 3;  j++)  StrCp (Up.d [i][j+1], ss.Col [j]);
         }
      }
   } while ((pLn = LinePos (b, ++ln)));
   Up.nR = n;
   if (kick)  emit sgUpd ("dTDr");
}


void Song::TDr (char *arg)
{ ubyte j, k, t, r, c;
  char *a;
  TStr  s;
  ubyt4 i;
  BStr  b;
  char *p, ud;
  TrkEv *e2;
   NotesOff ();
   a = arg;   r = Str2Int (a, & a);
              c = Str2Int (a, & a);   while (*a == ' ')  a++;
// set new dsc
   PreTDr (false);   StrCp (Up.d [r][c], a);
DBG("TDr r=`d c=`d s=`d", r, c, a);
   StrCp (b, CC("drumpat={\n"));
   for (j = 0;  j < Up.nR;  j++)
      StrAp (b, StrFmt (s, "`s=`s `s `s\n",
             Up.d [j][0], Up.d [j][1], Up.d [j][2], Up.d [j][3]));
   StrAp (b, CC("}"));
   DscPut (b);

// unique sections from _f.cue w time
  TxtRow m [64];
  ubyte nm;
   nm = GetSct (m);

// make a.txt song file with section patterns
  bool  co;
  ubyt2 b1 = 1, br, bb, be;            // bars start at 1 not 0 !
  TStr  fn, pt [3];
  Path  d;
  File  f;
   App.Path (fn, 'd');   StrAp (fn, "/3_queue/drumpat");   d.Make (fn);
   StrAp (fn, CC("/a.txt"));
   if (! f.Open (fn, "w"))  Die (StrFmt (s, "can't write song file `s", fn));
   f.Put (CC("-- drumpat.txt\n"
             "!name=drum\n"));
  TSgRow *ts = TSig (0);
   f.Put (StrFmt (s, "!TSig=`d/`d\n", ts->num, ts->den));

   if (m [0].time) {                   // oops gotta pad in start bars
      b1 = Tm2Bar (m [0].time);
      for (j = 1;  j < b1;  j++)  f.Put (CC("w\n"));
      f.Put (CC("--\n"));
   }                                   // sigh, watch out for weirdness
   if (! StrCm (Up.d [0][1], CC("(continue)")))
      StrCp (   Up.d [0][1], CC("(off)"));
   for (i = 0;  i < nm;) {
   // j = section's pos in tDr[]
      for (j = 0;  j < Up.nR;  j++)
         if (! StrCm (& m [i].s [1], Up.d [j][0])) {
            for (k = 0;  k < 3;  k++) {     // load pattern in pt[]
               if (StrCm (Up.d [j][k+1], CC("(off)")))
                            StrFmt (pt [k], "#drum/`s/`s\n",
                                    (k<2) ? "main" : "fill", Up.d [j][k+1]);
               else if (k)  StrCp  (pt [k], pt [k-1]);
               else         StrCp  (pt [k], CC("w\n"));
         }
         break;
      }
      bb =    Tm2Bar (m [i].time);
      do {
         ++i;
         if (i >= nm)  {be = _bEnd+1;   break;}

         be = Tm2Bar (m [i].time);
         for (j = 0;  j < Up.nR;  j++)
            if (! StrCm (& m [i].s [1], Up.d [j][0])) {
               co = StrCm (Up.d [j][1], CC("(continue)")) ? false : true;
            break;
         }
      }
      while (co);

      for (br = bb;  br < be;  br++)
         f.Put (pt ["abababac" [(br - bb) % 8] - 'a']);
   }
   f.Put (CC("NextTrack\n"));
   f.Shut ();

// Txt2Song it, load it, and replace drumtrack
   App.Run (StrFmt (s, "txt2song `p", fn));
   Fn2Path (fn);   StrAp (fn, CC("/a.song"));
TRC("a");
  STable st [TB_MAX];
   st [TB_DSC].Init (CC("Descrip:"), 1, MAX_DSC);
   st [TB_TRK].Init (CC("Track:")  , 4, MAX_TRK);
   st [TB_DRM].Init (CC("DrumMap:"), 7, MAX_DRM);
   st [TB_LYR].Init (CC("Lyric:")  , 2, MAX_LYR);
   st [TB_EVT].Init (CC("Event:")  , 2, MAX_EVT);
   if ((p = f.DoText (fn, & st, SongRec)))
      {TRC("  DoText err=`s", p);   return;}
TRC("b");

   DrumCon ();
TRC("c");
   for (t = 0;  t < _f.trk.Ln;  t++)  if (TDrm (t))  break;
   if (t >= _f.trk.Ln)  return;
TRC("d");

   for (i = 0;  i < _f.trk [t].ne;)    // keep ctrls / kill notes
      {if (_f.trk [t].e [i].ctrl & 0xFF80)  i++;   else EvDel (t, i);}
TRC("e");

// realloc _f.ev etc
  ubyt4 ne = st [TB_EVT].NRow () - 2;  // omit 1st !TSig=
TRC("f");
  TrkEv *olde = _f.ev;                 // old ev buf
TRC("g");
   _f.maxEv = ne + _f.nEv+MAX_RCRD;    // new size
TRC("h");
   _f.ev = new TrkEv [_f.maxEv];       // new ev buf
   MemCp (_f.ev, olde, _f.nEv * sizeof (TrkEv));
   delete olde;
TRC("i");

// rebuild _f.trk[].e's
   for (_f.trk [0].e = _f.ev, i = 1;  i < _f.trk.Ln;  i++)
      _f.trk [i].e = _f.trk [i-1].e + _f.trk [i-1].ne;
TRC("j");

// EvIns new notes into drumtrack
   e2 = & _f.trk [t].e [_f.trk [t].ne];
TRC("k");
   EvIns (t, _f.trk [t].ne, ne);
TRC("l");
   for (i = 0;  i < ne;  i++, e2++) {
TRC("m");
      e2->time = Str2Tm (st [TB_EVT].Get (1+i, 0));
      StrCp (s,          st [TB_EVT].Get (1+i, 1));
      e2->ctrl = MDrm (s);
TRC("n");
      ud = s [4];
      e2->valu = (ubyte)Str2Int (& s [5]) | ((ud == '_') ? 0x80 : 0);
      e2->val2 = (ud == '~') ? 0x80 : 0;
   }
TRC("o");

// cleanup n TmHop
TRC("p");
   Fn2Path (fn);   d.Kill (fn);
TRC("q");
   if (c == 2)  c = 1;   else if (c == 3)  c = 7;  else c = 0;
   DrumExp ();   ReDo ();   TmHop (m [r].time + M_WHOLE*c);
}


//______________________________________________________________________________
void Song::PreCtl ()
// dlg to pick mapping of input to song controls;  and show/hide/mini
{ ubyte n, d, c, i, j;
  TStr     ds, s, sh, b;
   Up.rHop = 0;
   n = 0;
   for (d = 0;  d < _mi.Ln;  d++) {
      StrCp (ds, _mi [d].mi->Name ());
      for (c = 0;  c < _mi [d].cc.Ln;  c++) {
         StrCp (Up.d [n][0], ds);   StrCp (Up.d [n][4], Int2Str (d, b));
         StrCp (Up.d [n][1], _mi [d].cc [c].map);
                                    StrCp (Up.d [n][5], Int2Str (c, b));
         if ((Up.id == d) && (Up.icc == _mi [d].cc [c].raw))  Up.rHop = n;
         StrCp (Up.d [n][2], CC("-"));
         StrCp (Up.d [n][3], CC("hide"));
         n++;
      }
   }
   Up.nR = n;
   Up.d [n][0][0] = _lrn.chd ? 'Y' : 'N';   // view chords flag :/
   for (i = 0;  i < n;  i++) {
      d = Str2Int (Up.d [i][4]);
      c = Str2Int (Up.d [i][5]);
      *s = '\0';
      for (j = 0;  j < _ccMap.Ln;  j++)
         if ((_ccMap [j].dev == d) &&
             (_ccMap [j].cc  == _mi [d].cc [c].raw))
            {StrCp (s, _ccMap [j].str);   StrCp (Up.d [i][2], s);   break;}
      if (*s)  for (j = 0;  j < _f.ctl.Ln;  j++)  if (! StrCm (s, CtlSt (j))) {
         if      (_f.ctl [j].sho == 'y')  StrCp (sh, CC("show"));
         else if (_f.ctl [j].sho == 'm')  StrCp (sh, CC("mini"));
         else                             StrCp (sh, CC("hide"));
         StrCp (Up.d [i][3], sh);
      }
   }
   emit sgUpd ("dCtl");
}


void Song::Ctl ()
// re set _ccMap and store in device/ccmap.txt;  set cc's show
{ TStr  fn, ds, cs, ms, sh;
  ubyte     d,  c, i, j, n;
  File  f;
  BStr  bs;
   App.Path (fn, 'd');   StrAp (fn, CC("/device/ccmap.txt"));
   if (! f.Open (fn, "w"))  return;

   _lrn.chd = (Up.d [Up.nR][0][0] == 'Y') ? true : false;
   n = _ccMap.Ln = 0;
   for (i = 0;  i < Up.nR;  i++) {
      StrCp (ds, Up.d [i][0]);   d = Str2Int (Up.d [i][4]);
      StrCp (cs, Up.d [i][1]);   c = Str2Int (Up.d [i][5]);
      StrCp (ms, Up.d [i][2]);
      StrCp (sh, Up.d [i][3]);
      if (StrCm (ms, CC("-"))) {
         f.Put (StrFmt (bs, "`s `s `s\n",  ds, cs, ms));
         _ccMap [n].dev = d;
         _ccMap [n].cc  = _mi [d].cc [c].raw;
         StrCp (_ccMap [n].str, ms);
         n++;   _ccMap.Ln++;

         for (j = 0;  j < _f.ctl.Ln;  j++)  if (! StrCm (ms, CtlSt (j))) {
            if      (*sh == 's')  _f.ctl [j].sho = 'y';
            else if (*sh == 'h')  _f.ctl [j].sho = 'n';
            else                  _f.ctl [j].sho = 'm';
         }
      }
   }
   f.Shut ();
   Up.id = 99;
   ReDo ();
}


void Song::SetCtl (char *arg)
// mouse ctl event editing
{ char *s, *c;
  ubyte tr, cc, mc;
  ubyt4 tm, p;
  TStr  ts;
  MidiEv e;
  TrkEv  ev;
// parse our dang string args: track, epos, time, ctl, val
   tr = (ubyte)Str2Int (arg, & s);   p = (ubyt4)Str2Int (s, & s);
   tm = (ubyt4)Str2Int (s, & s);
   while (*s == ' ')  s++;
   c = s;
   if ((s = StrCh (s, ' ')))  *s++ = '\0';  else s = c;
   while (*s == ' ')  s++;
TRC("setCtl tr=`d p=`d tm=`d ctl=`s val=`s", tr, p, tm, c, s);
   if (tr < 128) {
      EvDel (tr, p);
      if (! StrCm (c, CC("KILL")))  {ReDo ();   return;}
   }
   else {                              // need SOME track to put it in...
      if ( (! StrCm (c, CC("ksig"))) ||     // these HAVE to go in drum trak
           (! StrCm (c, CC("tsig"))) || (! StrCm (c, CC("tmpo"))) ) {
         for (tr = 0;  tr < _f.trk.Ln;  tr++)  if (TDrm (tr))  break;
         if (tr >= _f.trk.Ln)
            {DBG     ("no drum track to put control into");
             Info (CC("no drum track to put control into"));   return;}
      }
      else {                           // find a learn track
         for (tr = 0;  tr < _f.trk.Ln;  tr++)  if (TLrn (tr))  break;
         if (tr >= _f.trk.Ln)
            {DBG     ("no learn track to put control into");
             Info (CC("no learn track to put control into"));   return;}
      }
   }
// scoot tsig,ksig time to bar start
   if ( (! StrCm (c, CC("tsig"))) || (! StrCm (c, CC("ksig"))) )
      tm = Bar2Tm (Tm2Bar (tm));
   for (mc = 0;  mc < NMCC;  mc++)  if (! StrCm (c, MCC [mc].s))  break;
   if (MCC [mc].typ == 'x')  {CtlX2Val (& ev, c, s);
                              e.valu = ev.valu;              e.val2 = ev.val2;}
   else                      {e.valu = (ubyte)Str2Int (s);   e.val2 = 0;}
TRC("set time=`s tr=`d ctrl=`s valu=`d val2=`d",
TmSt (ts, tm), tr+1, c, e.valu, e.val2);
   e.time = tm;   e.ctrl = CtlEv (c);
   EvInsT (tr, & e);
   ReDo ();
}


//______________________________________________________________________________
void Song::Cue (char *s)
{ TStr s1;
DBG("Cue '`s'", s);DbgPos('x');
   if (! StrCm (s, CC("loopInit")))  return LoopInit ();
   if (*s == '[')  return;             // can't edit loops

  ubyt4 tm = Up.posx.tm, te = 0;
   if (Up.posx.got)
      {tm = _f.cue [Up.posx.p].time;
       te = _f.cue [Up.posx.p].tend;   _f.cue.Del (Up.posx.p);}
   if (*s) {
      StrCp (s1, s);
      if (StrCh (CC("<>"), *s)) {      // need dur?
         if (! Up.posx.got)  te = tm + M_WHOLE;
         StrFmt (s1, "/`d`s", te-tm, s);
      }
      if (*s == '(')                   // chop tm at bar for sections
         tm = Bar2Tm (Tm2Bar (tm));
TStr ts;
DBG("TxtIns tm=`s, s1=`s", TmSt(ts,tm), s1);
      TxtIns (tm, s1, & _f.cue, 'c');
   }
   ReDo ();
}


//______________________________________________________________________________
void Song::PreQua ()
{
DBG("PreQua");
  PagDef *pg = & _pag [Up.pos.pg];
  ColDef  co;
   MemCp (& co, & pg->col [Up.pos.co], sizeof (co));  // load column
  SymDef *it = & co.sym [Up.pos.sy];
  ubyte   t  = it->tr;
  TrkNt *nt = & _f.trk [t].n [it->nt];
  ubyt4  tm, dp, tp, tn;
  TStr   st, s1, s2;
   if (nt->dn == NONE)  StrCp (st, CC("ntDn: NONE\n"));
   else {
      TmSt (s1, _f.trk [t].e [nt->dn].time);
      StrFmt (st,                     "ntDn: `s velo=`d\n",
            s1, _f.trk [t].e [nt->dn].valu & 0x7F);
   }
   if (nt->up == NONE)  StrAp (st, CC("ntUp: NONE"));
   else {
      TmSt (s2, _f.trk [t].e [nt->up].time);
      StrFmt (& st [StrLn (st)],      "ntUp: `s velo=`d",
            s2, _f.trk [t].e [nt->up].valu & 0x7F);
   }
   tm = nt->tm;
   for (dp = 0;  dp < _dn.Ln;  dp++)  if (_dn [dp].time == tm)  break;

// pass st, t, it->nt;  dlg sends on t,nt,qua
   StrCp (Up.pos.str, st);   Up.pos.tr = t;   Up.pos.p = it->nt;
   StrCp (Up.pos.stp, TmSt (s1,  dp           ? _dn [dp-1].time :  0));
   StrCp (Up.pos.stn, TmSt (s1, (dp+1<_dn.Ln) ? _dn [dp+1].time : tm));
   emit sgUpd ("dQua");
}


void Song::Qua (char *tnq)             // set quantize
{ ubyte t;
  ubyt4 n, p, ne;
  char *s, q;
  TrkNt *nt;
  TrkEv  dn, up, *e;
DBG("Qua `s", tnq);
   t  = (ubyte) Str2Int (tnq, & s);
   n  =         Str2Int (s,   & s);
   q  = s [1];
DBG("Qua t=`d n=`d q=`c", t, n, q);
//Dump (true);
   nt = & _f.trk [t].n [n];
   if (nt->dn != NONE)  MemCp (& dn, & _f.trk [t].e [nt->dn], sizeof (dn));
   if (nt->up != NONE)  MemCp (& up, & _f.trk [t].e [nt->up], sizeof (up));
DBG("t=`d n=`d/`d dn=`d up=`d", t, n, _f.trk [t].nn, nt->dn, nt->up);
   if (nt->dn == NONE) {
      MemCp (& dn, & up, sizeof (dn));   dn.time = (up.time >(M_WHOLE/32-1)) ?
                                                   (up.time -(M_WHOLE/32-1)):0;
                                         dn.valu = 100;
TRC("made ntDn");
   }
   if (nt->up == NONE) {
      MemCp (& up, & dn, sizeof (dn));   up.time = dn.time + (M_WHOLE/32-1);
                                         up.valu = 64;
TRC("made ntUp");
   }
// quantize prev/next
   if ((p = nt->dn) == NONE) {
      e = _f.trk [t].e;   ne = _f.trk [t].ne;
      for (p = 0;  (p < ne) && (dn.time >= e [p].time);  p++)  ;
      EvIns (t, p);   MemCp (& e [p], & dn, sizeof (dn));
   }
   _f.trk [t].e [p].time = Str2Tm ((q == 'p') ? Up.pos.stp : Up.pos.stn);
//TOOD check dur :/
   ReDo ();
}


void Song::NtDur ()                    // set note duration (end time)
{ char *s;
  ubyt4 ap, ac, as, tm, t1, tMx, p, ne;
  sbyt2 y2;
  ubyte t, nt;
  SymDef *sy;
  TrkEv  *e, up;
   ap =  Up.pos.pg;   ac = Up.pos.co;   as = Up.pos.sy;   y2 = Up.pos.y2;
  PagDef *pg = & _pag [ap];
  ColDef  co;
   MemCp (& co, & pg->col [ac], sizeof (co));
   sy = & co.sym [as];
   t = sy->tr;
//DBG("pg=`d co=`d sy=`d y2=`d", ap, ac, as, y2);
   if (y2 <= sy->y)  return;           // just quit if dur is lame

   tm = Y2Tm (y2, & co);               // y=> time
   nt = _f.trk [t].n [sy->nt].nt;
   t1 = _f.trk [t].n [sy->nt].tm;
//TStr d1,d2,d3;
//DBG("tm=`s nt=`s t1=`s", TmSt(d1,tm), MKey2Str (d2,nt), TmSt(d3,t1));
   e = _f.trk [t].e;   ne = _f.trk [t].ne;
   for (p = 0;  p < ne;  p++)          // find next ntDn of nt to limit dur
      if ((e [p].ctrl == nt ) &&    (e [p].time > t1) &&
          (e [p].valu & 0x80) && (! (e [p].val2 & 0x80)))  break;
   tMx = (p < ne) ? e [p].time : (t1 + 4*M_WHOLE);
//DBG("tMx=`s", TmSt(d1,tMx));
   if (tm >= tMx)  tm = tMx-1;

// ok, let's do this thing...
   if ((p = _f.trk [t].n [sy->nt].up) != NONE) {
      MemCp (& up, & _f.trk [t].e [p], sizeof (up));
      EvDel (t, p);
   }
   else {
      MemCp (& up, & _f.trk [t].e [_f.trk [t].n [sy->nt].dn], sizeof (up));
      up.valu = 64;
   }
   up.time = tm;
   e = _f.trk [t].e;   ne = _f.trk [t].ne;
   for (p = 0;  (p < ne) && (e [p].time <= up.time);  p++)  ;
   EvIns (t, p);   MemCp (& e [p], & up, sizeof (up));
   ReDo ();
}


void Song::NtHop ()                    // move note time,dur to new key
{ ubyt4 ap, ac, as, p, ne;
  sbyt2 x2;
  ubyt2 nx;
  ubyte t, dnt;
  SymDef *sy;
  TrkNt  *n;
  TrkEv  *e, dn, up;
  TStr    s1, s2, s3;
   ap = Up.pos.pg;   ac = Up.pos.co;
   as = Up.pos.sy;   x2 = Up.pos.x1 + W_NT/2;
  PagDef *pg = & _pag [ap];
  ColDef  co;
   MemCp (& co, & pg->col [ac], sizeof (co));
   sy = & co.sym [as];   t = sy->tr;   n = & _f.trk [t].n [sy->nt];
   e = _f.trk [t].e;                   // n is only ok if NON ez (! RCRD)

DBG("NtHop pg=`d co=`d sy=`d x2=`d tr=`d", ap, ac, as, x2, t);
   nx  = co.nx;
   dnt = ((x2 < nx) || (x2 >= co.cx)) ? 0 :
            co.nMn + (x2 - nx) / W_NT; // note to move it to
DBG("dnt=`s", MKey2Str (s1, dnt));
   if (RCRD) {                         // restart eztrack on this note pos
   // kill any existing .ezpos oct* cues for my oct/track
      StrFmt (s1, ".ezpos `d", (sy->nt / 12)-1);
      for (p = 0;  p < _f.cue.Ln;) {   // kill it if ya got it
         if ( (_f.cue [p].time == sy->tm) &&
              (! MemCm (_f.cue [p].s, s1, StrLn (s1))) )
               _f.cue.Del (p);
         else  p++;
      }
   // and if resettin pos, we're done.  else set new pos .ezpos_nt
      if (dnt)  {StrFmt (s1, ".ezpos `s", MKey2Str (s2, dnt));
                 TxtIns (sy->tm, s1, & _f.cue, 'c');}
      ReDo ();
      return;
   }
   if (! dnt)  return;

// quit if overlap
   if ((p = NtHit (t, n->tm, n->te, dnt, dnt))) {
      StrFmt (s1, "can't move - overlap at note=`s time=`s",
         MKey2Str (s2, dnt), TmSt (s3, e [p-1].time));
      Hey (s1);
      return;
   }

// ok, let's do this thing...  set dn,up; kill old; ins new
   if ((p = n->up) != NONE)  MemCp (& up, & e [p],     sizeof (up));
   else                     {MemCp (& up, & e [n->dn], sizeof (up));
                             up.time += (M_WHOLE/32-1);   up.valu = 64;}
   if ((p = n->dn) != NONE)  MemCp (& dn, & e [p],     sizeof (dn));
   else                     {MemCp (& dn, & e [n->up], sizeof (dn));
                             dn.time -= (M_WHOLE/32-1);   dn.valu = 100;}
   if ((p = n->up) != NONE)  EvDel (t, p);
   if ((p = n->dn) != NONE)  EvDel (t, p);
   dn.ctrl = up.ctrl = dnt;

   e = _f.trk [t].e;   ne = _f.trk [t].ne;
   for (p = 0;  (p < ne) && (e [p].time <= dn.time);  p++)  ;
   EvIns (t, p);   MemCp (& e [p], & dn, sizeof (dn));

   e = _f.trk [t].e;   ne = _f.trk [t].ne;
   for (p = 0;  (p < ne) && (e [p].time <= up.time);  p++)  ;
   EvIns (t, p);   MemCp (& e [p], & up, sizeof (up));

   ReDo ();
}


//______________________________________________________________________________
struct TPDef {ubyte t;   ubyt4 p;};

int TPCmp (void *a1, void *a2)         // by t,p desc
{ TPDef *p1 = (TPDef *)a1, *p2 = (TPDef *)a2;
  int t;
   if ((t = p2->t - p1->t))   return t;
        t = p2->p - p1->p;    return t;
}

void Song::Mov ()
// move rect of notes to RH,LH,bg,kill
// if nondrag, insert a new note
{ char *s, *c, ht;
  ubyt4 ap, ac, p, p1, tm, t1, t2, tMx, dBt, dSb, ne, nn, i, nDel, nIns;
  sbyt2 x1, y1, x2, y2, tp;
  ubyt2 nx, br;
  ubyte tr, tr1, tD, tR, tL, vDn, vUp, nt, nSb, bt;
  TStr  s1;
  SymDef *it;
  TrkEv  *e, ev, *ins, up, dn;
  TPDef          *del;
  TrkNt  *n;
   ap = Up.pos.pg1;   ac = Up.pos.co1;   x1 = Up.pos.x1;   y1 = Up.pos.y1;
                                         x2 = Up.pos.x2;   y2 = Up.pos.y2;
   s = Up.pos.str;
DBG("Mov pg=`d co=`d x1=`d x2=`d y1=`d y2=`d s='`s'",
ap, ac, x1, x2, y1, y2, s);
  PagDef *pg = & _pag [ap];
  ColDef  co;
   MemCp (& co, & pg->col [ac], sizeof (co));
   nx = co.nx;
   if (*s == '\0') {                   // just gonna ins a note
      if (x1 >= Nt2X (co.nMx+1, & co))
         {Hey (CC("sorry, can't do drums yet"));   return;}

      tm = Y2Tm (y1, & co);            // y=> time=> round to subbeat
      TmStr (s1, tm, & t2, & nSb);   br = (ubyt2)Str2Int (s1, & c);
                                     bt = (ubyte)Str2Int (c+1);
      t1 = Bar2Tm (br, bt);   dBt = t2 - t1;   dSb = dBt / nSb;
      t1 += (((tm - t1) / dSb) * dSb);           // t1 now has trunc'd subbt
      if ((tm - t1) >= (dSb / 2))  t1 += dSb;    // now rounded subbt

      nt = co.nMn + (x1 - nx) / W_NT;  // note is easy

   // find nearest (unbroke) note on any ? trk (by time, then by frq)
   // use THAT dude's trk, dur, n velos for ins
      p1 = NONE;   tr1 = 0;
      for (tr = 0;  tr < _f.trk.Ln;  tr++)  if (TLrn (tr))
         for (n = _f.trk [tr].n, nn = _f.trk [tr].nn, p = 0;  p < nn;  p++)
                                 if ((n [p].dn != NONE) && (n [p].up != NONE)) {
//DBG("p=`d/`d  t1=`d nt=`d  tmDn=`d tmUp=`d nt=`d  p1=`d",
//p, nn,  t1, nt,  e [n [p].dn].time, e [n [p].up].time, n [p].nt,  p1);
            if      (p1 == NONE)
                  {p1 = p;   tr1 = tr;}
            else if (ABSL((sbyt4)n [p].tm - (sbyt4)t1) <
                                ABSL((sbyt4)_f.trk [tr1].n [p1].tm - (sbyt4)t1))
                  {p1 = p;   tr1 = tr;}
            else if (ABSL((sbyt4)n [p].tm - (sbyt4)t1) ==
                                ABSL((sbyt4)_f.trk [tr1].n [p1].tm - (sbyt4)t1))
               {if  (ABSL((sbyt4)n [p].nt - (sbyt4)nt) <=
                                ABSL((sbyt4)_f.trk [tr1].n [p1].nt - (sbyt4)nt))
                  {p1 = p;   tr1 = tr;}}
            else if (n [p].tm > t1)  break;      // past min
         }
      if (p1 != NONE) {
         vDn =     _f.trk [tr1].e [_f.trk [tr1].n [p1].dn].valu & 0x7F;
         vUp =     _f.trk [tr1].e [_f.trk [tr1].n [p1].up].valu & 0x7F;
         t2 = t1 + _f.trk [tr1].e [_f.trk [tr1].n [p1].up].time -
                   _f.trk [tr1].e [_f.trk [tr1].n [p1].dn].time;
//DBG("GOT p1=`d tmDn=`d tmUp=`d  t1=`d t2=`d vDn=`d vUp=`d",
//p1, e [n [p1].dn].time, e [n [p].up].time,  t1, t2, vDn, vUp);
      }
      else {                           // default trk, velos, n dur
         vDn = 100;   vUp = 64;   t2 = t1 + dBt - 1;
         for (tr = 0;  tr < _f.trk.Ln;  tr++)  if (TLrn (tr))  break;
         if (tr >= _f.trk.Ln)
            {Hey (CC("you need a practice track to add notes on"));   return;}
         tr1 = tr;
      }

   // limit t2 at next ntDn time on same nt
      e = _f.trk [tr1].e;   ne = _f.trk [tr1].ne;
      for (p = 0;  p < ne;  p++)         // find next ntDn of nt to limit dur
         if ((e [p].ctrl == nt ) &&    (e [p].time > t1) &&
             (e [p].valu & 0x80) && (! (e [p].val2 & 0x80)))  break;
      tMx = (p < ne) ? e [p].time : (t1 + 4*M_WHOLE);
      if (t2 >= tMx)  t2 = tMx-1;

   // pop in our dn,up evs
      ev.time = t1;   ev.ctrl = nt;   ev.valu = vDn | 0x80;   ev.val2 = 0;
      for (p = 0;  (p < ne) && (ev.time >= e [p].time);  p++)  ;
      EvIns (tr1, p);   MemCp (& e [p], & ev, sizeof (ev));

      ev.time = t2;                   ev.valu = vUp;
      e = _f.trk [tr1].e;   ne = _f.trk [tr1].ne;
      for (p = 0;  (p < ne) && (ev.time >= e [p].time);  p++)  ;
      EvIns (tr1, p);   MemCp (& e [p], & ev, sizeof (ev));

      ReDo ();
      return;
   }

// ok, hunt em down n kill/move each dude
   if (x2 < x1)  {tp = x1;   x1 = x2;   x2 = tp;}
   if (y2 < y1)  {tp = y1;   y1 = y2;   y2 = tp;}
  ubyt2 dx = co.dx;
DBG("flipfix co.nx=`d co.dx=`d x1=`d x2=`d y1=`d y2=`d",
nx, dx, x1, x2, y1, y2);
   if (x1 >= dx)  {Hey (CC("sorry, can't do drums yet :("));   return;}

   if (x2 >= dx)  x2 = dx-1;
   if (*s != 'x') {                    // find dest track if not doin del
     bool got [3];
      MemSet (got, 0, sizeof (got));
      for (i = 0;  i < co.nSym;  i++) {
DBG("sym `d x1=`d x2=`d y1=`d y2=`d",
i,nx+co.sym [i].x, nx+co.sym [i].x+co.sym [i].w-1,
     co.sym [i].y,    co.sym [i].y+co.sym [i].h-1);
         if ((((nx+co.sym [i].x                    >= x1) &&
               (nx+co.sym [i].x                    <= x2)) ||
              ((nx+co.sym [i].x + co.sym [i].w - 1 >= x1) &&
               (nx+co.sym [i].x + co.sym [i].w - 1 <= x2))) &&
             (((   co.sym [i].y                    >= y1) &&
               (   co.sym [i].y                    <= y2)) ||
              ((   co.sym [i].y + co.sym [i].h - 1 >= y1) &&
               (   co.sym [i].y + co.sym [i].h - 1 <= y2)))) {
            ht = _f.trk [co.sym [i].tr].ht;
DBG("   ht=`c", ht);
            if      (ht >  '3')  got [0] = true; // got RH
            else if (ht <  '4')  got [1] = true; //     LH
            else if (ht == 'S')  got [2] = true; //     Show
         }
      }
      tR = tL = 255;
      for (tr = 0;  tr < _f.trk.Ln;  tr++) {
         ht = _f.trk [tr].ht;   if ((ht < '1') || (ht > '7'))  continue;
         if ((tR == 255) && (ht > '3')) tR = tr;
         if ((tL == 255) && (ht < '4')) tL = tr;
      }
      if (*s != '#') {
         if ((tR == 255) || (tL == 255))
            {Hey (CC("you need to have RH and LH tracks"));   return;}
      }
      else {
         if ( got [0] && ((tR == 255) || ((ubyt4)(tR+1) >= _f.trk.Ln) ||
                          (! _f.trk [tR+1].grp) || TLrn (tR+1)) )
            {Hey (CC("add a RH backing track (+ track) to move notes to"));
             return;}
         if ( got [1] && ((tL == 255) || ((ubyt4)(tL+1) >= Up.eTrk) ||
                          (! _f.trk [tL+1].grp) || TLrn (tL+1)) )
            {Hey (CC("add a LH backing track (+ track) to move notes to"));
             return;}
         tR++;   tL++;
      }
DBG("got0=`b 1=`b 2=`b tR=`d tL=`d",
got[0],got[1],got[2],tR,tL);
   }
   ins = new TrkEv [co.nSym * 2];   del = new TPDef [co.nSym * 2];
   nIns = nDel = 0;                    // may not need ins, but whatever..
   for (i = 0;  i < co.nSym;  i++)
      if ((((nx+co.sym [i].x                    >= x1) &&
            (nx+co.sym [i].x                    <= x2)) ||
           ((nx+co.sym [i].x + co.sym [i].w - 1 >= x1) &&
            (nx+co.sym [i].x + co.sym [i].w - 1 <= x2))) &&
          (((   co.sym [i].y                    >= y1) &&
            (   co.sym [i].y                    <= y2)) ||
           ((   co.sym [i].y + co.sym [i].h - 1 >= y1) &&
            (   co.sym [i].y + co.sym [i].h - 1 <= y2)))) {
         it = & co.sym [i];
      // already ok?
         if ( ((*s == '#') && (_f.trk [it->tr].ht == 'S')) ||
              ((*s == '>') && (it->tr == tR)) ||
              ((*s == '<') && (it->tr == tL)) )  continue;
      // first, we kill
         if ((p = _f.trk [it->tr].n [it->nt].dn) != NONE)
            {del [nDel].t = it->tr;   del [nDel].p = p;   nDel++;}
         if ((p = _f.trk [it->tr].n [it->nt].up) != NONE)
            {del [nDel].t = it->tr;   del [nDel].p = p;   nDel++;}

      // next, gotta ins em incl making fake 2nd (unless doin del)
         if (*s != 'x') {
TStr db1, db2;
DBG("move `s `s from tr=`d to tR=`d tL=`d",
TmSt    (db1,_f.trk[it->tr].n [it->nt].tm),
MKey2Str(db2,_f.trk[it->tr].n [it->nt].nt), it->tr, tR, tL);
            if ((p = _f.trk [it->tr].n [it->nt].up) != NONE)
               MemCp (& up, & _f.trk [it->tr].e [p], sizeof (up));
            else {
               MemCp (& up, & dn, sizeof (dn));
               up.time =  dn.time + (M_WHOLE/32-1);
               up.valu = 64;
            }
            if ((p = _f.trk [it->tr].n [it->nt].dn) != NONE)
               MemCp (& dn, & _f.trk [it->tr].e [p], sizeof (dn));
            else {
               MemCp (& dn, & up, sizeof (up));
               dn.time = (up.time > (M_WHOLE/32-1)) ?
                         (up.time - (M_WHOLE/32-1)) : 0;
               dn.valu = 100;
            }
            if      (*s == '>')  dn.x = up.x = tR;
            else if (*s == '<')  dn.x = up.x = tL;
            else if (_f.trk [it->tr].ht > '3')  dn.x = up.x = tR;
            else                                dn.x = up.x = tL;
            MemCp (& ins [nIns++], & dn, sizeof (dn));
            MemCp (& ins [nIns++], & up, sizeof (up));
         }
      }
   Sort (del, nDel, sizeof (TPDef), TPCmp);      // by tr,p desc so this works:
   for (i = 0;  i < nDel;  i++)  EvDel (del [i].t, del [i].p);
   for (i = 0;  i < nIns;  i++) {
      tD = ins [i].x;   e = _f.trk [tD].e;   ne = _f.trk [tD].ne;
      ins [i].x = 0;
      for (p = 0;  (p < ne) && (e [p].time <= ins [i].time);  p++)  ;
      EvIns (tD, p);
      MemCp (& e [p], & ins [i], sizeof (TrkEv));
   }
   delete [] ins;   delete [] del;
   ReDo ();
}
