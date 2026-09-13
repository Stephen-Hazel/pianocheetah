// sRecord.cpp - all the junk for Song::EvRcrd()

#include "song.h"


void Song::Shush (bool tf)
// on bg chans (non lrn, non REC) set vol per tf
// to 0  else "whatever it was before"
{ MidiO *mo;
  bool   got;
  ubyte  t, dt;
  ubyt2  craw;
TRC("Shush `b", tf);
   for (t = 0;  t < _f.trk.Ln;  t++)  if (! TLrn (t)) {
      mo = Up.dev [_f.trk [t].dev].mo;
      got = false;                     // already did w my dev,chn?
      for (dt = 0;  dt < t;  dt++)  if ((_f.trk [t].dev == _f.trk [dt].dev) &&
                                        (_f.trk [t].chn == _f.trk [dt].chn))
         {got = true;  break;}
      if (! got) {
         dt = Up.dev [_f.trk [t].dev].dvt;
         if ((craw = Up.dvt [dt].CCID (CC("Vol"))))
            mo->Put (_f.trk [t].chn,
                     craw, tf ? 0 : CCValAt (_now, t, CC("Vol")), 0);
//DBG("   t=`d vol=`d", t, tf ? 0 : CCValAt (_now, t, CC("Vol")));
      }
   }
   if (! tf)  _lrn.POZ = false;
}


// step entry:  if poz'd, only 1 rec trk, build chord at now of certain durs
void Song::SetStDur (char c)           // dur key hit - flip (only) c of dur str
{ char *p, m, s2 [2];
  bool  b [6];
  ubyte i;
  TStr  dur;
  const char *s = "whqest";
// get dur mod (. 3 or nothin)
   StrCp (dur, _st.dur);
   m = '\0';
   if      (StrCh (dur, '.') != nullptr)  m = '.';
   else if (StrCh (dur, '3') != nullptr)  m = '3';

// get bitmap of durs
   for (i = 0;  i < 6;  i++)  b [i] = StrCh (dur, s [i]) != nullptr;

   if (c == '.') {
      if      (m == '\0')  m = '.';
      else if (m == '.')   m = '3';
      else                 m = '\0';
   }
   else                                // c == one of w h q e s t
      for (i = 0;  i < 6;  i++)  if (c == s [i])  b [i] = ! b [i];

// rebuild dur str
   *_st.dur = '\0';   s2 [1] = '\0';
   for (i = 0;  i < 6;  i++)  if (b [i])  {*s2 = s [i];   StrAp (_st.dur, s2);}
   if (*_st.dur && m)                     {*s2 = m;       StrAp (_st.dur, s2);}
}


ubyt4 Song::StDur ()                   // dur str into ticks
{ static char *sym = CC("whqest");
  char *s = _st.dur;
  ubyt4 o = 0;
   while (*s) {
      if      (*s == '.')  o  = o * 3 / 2;
      else if (*s == '3')  o  = o * 2 / 3;
      else                 o += M_WHOLE / (1 << (StrCh (sym, *s) - sym));
      s++;
   }
   return o;
}

/* M /m             M /m               ...another reason why i hate sheet music
 0 C /A    -
 1 G /E  # f        F /D  b b
 2 D /B  # cf       Bb/G  b eb
 3 A /F# # cfg      Eb/C  b eab
 4 E /C# # cdfg     Ab/F  b deab
 5 B /G# # cdfga    Db/Bb b degab
 6 F#/D# # cdefga   Gb/Eb b cdegab
 7 C#/A# # cdefgab  Cb/Ab b cdefgab
*/
static const char *Scale [7][2][3] = {
   {{"G", "E",  "f"},        {"F", "D",  "b"}},
   {{"D", "B",  "cf"},       {"Bb","G",  "eb"}},
   {{"A", "F#", "cfg"},      {"Eb","C",  "eab"}},
   {{"E", "C#", "cdfg"},     {"Ab","F",  "deab"}},
   {{"B", "G#", "cdfga"},    {"Db","Bb", "degab"}},
   {{"F#","D#", "cdefga"},   {"Gb","Eb", "cdegab"}},
   {{"C#","A#", "cdefgab"},  {"Cb","Ab", "cdefgab"}}
};

bool Song::Step (MidiEv *ev)
// step entry - poz'd n 1 rec trk: note events build a chord at now
{ ubyte n, t, tr, i, k, step, nt;
  TStr  s, s2;
  char *sc, shfl;
  KSgRow *ks;
  static char *noteSym = CC("c d ef g a b");

   if (! _timer->Pause ())  return false;
   for (n = t = tr = 0;  t < _f.trk.Ln;  t++)
      if (TRec (t) && (! TDrm (t)))  {n++;   tr = t;}
   if (n != 1)  return false;

   if (! MNTDN (ev))  return true;     // only need ntDn
   n = (ubyte) ev->ctrl;

// dur entry
   if  (n == MKey ("3c#"))  *_st.dur = '\0';      // reset
   if  (n == MKey ("3d#"))  SetStDur ('.');
   if  (n == MKey ("3f#"))  SetStDur ('w');
   if  (n == MKey ("3g#"))  SetStDur ('h');
   if  (n == MKey ("3a#"))  SetStDur ('q');
   if  (n == MKey ("4c#"))  SetStDur ('e');
   if  (n == MKey ("4d#"))  SetStDur ('s');
   if  (n == MKey ("4f#"))  SetStDur ('t');
   if ((n == MKey ("4g#")) &&  _st.artc     )  _st.artc--;
   if ((n == MKey ("4a#")) && (_st.artc < 3))  _st.artc++;

// note entry
   if ((n == MKey ("3c")) &&  _st.oct      )  _st.oct--;
   if ((n == MKey ("3d")) && (_st.oct  < 8))  _st.oct++;
   if ((n == MKey ("3e")) &&  _st.sh       )  _st.sh--;
   if ((n == MKey ("3f")) && (_st.sh   < 3))  _st.sh++;
   if  (n == MKey ("4c"))  {_st.nt = 'c';   _st.sh = 0;}
   if  (n == MKey ("4d"))  {_st.nt = 'd';   _st.sh = 0;}
   if  (n == MKey ("4e"))  {_st.nt = 'e';   _st.sh = 0;}
   if  (n == MKey ("4f"))  {_st.nt = 'f';   _st.sh = 0;}
   if  (n == MKey ("4g"))  {_st.nt = 'g';   _st.sh = 0;}
   if  (n == MKey ("4a"))  {_st.nt = 'a';   _st.sh = 0;}
   if  (n == MKey ("4b"))  {_st.nt = 'b';   _st.sh = 0;}

// do note
   step = StrCh (noteSym, _st.nt) - noteSym;
   if (_st.sh != 3) {               // non-natural means follow scale
      ks = KSig (_now);
      shfl = '#';   sc = CC("");    // in case of C(maj), Am
      for (i = 0;  i < BITS (Scale);  i++) {
         if (ks->key ==    MNt (CC(Scale [i][0][ks->min])))
            {shfl = '#';   sc = CC(Scale [i][0][2]);}
         if (ks->key ==    MNt (CC(Scale [i][1][ks->min])))
            {shfl = 'b';   sc = CC(Scale [i][1][2]);}
    }
DBG("nt=`c kskey=`d ksmin=`d shfl=`c sc=`s",
_st.nt, ks->key, ks->min, shfl, sc);
      if (StrCh (sc, _st.nt))  if (shfl == '#')  step++;   else step--;
   }
   if (_st.sh == 1)  step--;
   if (_st.sh == 2)  step++;
   nt = (_st.oct+1)*12 + step;
DBG("step=`d nt=`d", step, nt);

   if (n == MKey ("3g")) {             // turn _st.nt,ksig,oct,sh into nt
      NtIns (tr, _now, _now + StDur () * (1+_st.artc) / 4 - 1, nt);
      ReDo ();   DrawNow ();
   }

// advance
   if (n == MKey ("3a")) {
      if (*_st.dur)  _now += StDur ();
      else {
         TmStr (s, _timer->Get () + (M_WHOLE/64));  // round time to next bar
         TmHop (Bar2Tm ((ubyt2)Str2Int (s)));
      }
      ReDo ();   DrawNow ();
   }

// draw text info
   Info (StrFmt (s, "`s  `s`c  `d`c`c",
      TmSt (s2,_now,'f'),  _st.dur, "<=>-" [_st.artc],
      _st.oct, _st.nt, " b#%" [_st.sh]
   ));

// draw demo note
  ubyt4 p, c, tMn, tMx;
  PagDef *pg = & _pag [0];
  ColDef  co;
   if (! (p = _pg))                    // don't know pg at the moment :/
      {Up.pos.x1 = Up.pos.x2 = 0;   return true;}
   p--;
   for (c = 0;  c < pg [p].nCol;  c++) {    // find our col
      tMn = pg [p].col [c].blk [0].tMn;
      tMx = pg [p].col [c].blk [pg [p].col [c].nBlk-1].tMx;
      if ((_now >= tMn) &&
          (_now <  tMx-M_WHOLE/32))  break;
   }
   if (c >= pg [p].nCol)               // col not shown (don't think thisll go
      {Up.pos.x1 = Up.pos.x2 = 0;   return true;}
   MemCp (& co, & pg [p].col [c], sizeof (co));  // load column specs
DBG("Step p=`d c=`d nx=`d", p, c, co.nx);

   Up.pos.y1 = Tm2Y (_now, & co);
   Up.pos.y2 = Tm2Y (_now + StDur (), & co);
   Up.pos.x1 = Nt2X (nt,   & co);
   Up.pos.x2 = Up.pos.x1 + W_NT;
   DragRc ();
   return true;
}


//______________________________________________________________________________
bool Song::NtCmd (MidiEv *ev)
// command key?  set _ed n kick a cmd
{ ubyte n, i;
  char *cmd;
   n = ev->ctrl;
   if (! _f.trk.Ln) {                  // do DlgFL input ?
      if (MNTDN (ev)) {
         if      (n == MKey ("3b"))  emit sgUpd ("FLex");
         else if (n == MKey ("4c"))  emit sgUpd ("FLgo");
         else if (n == MKey ("4d"))  emit sgUpd ("FLdn");
         else if (n == MKey ("4e"))  emit sgUpd ("FLup");
         else  Info (CC("3b=exit  4c=go!  4d=dn  4e=up"));
      }
      return true;
   }
   if (_ed) {
      if (MNTDN (ev)) {
         for (i = 0;  i < NUCmd;  i++)  if (n == MKey (UCmd [i].nt))  break;
         if (i < NUCmd)  {Info (cmd = CC(UCmd [i].cmd));   Cmd (cmd);}
      }
      return true;
   }
   return false;
}


//______________________________________________________________________________
bool Song::DnOK (char nxt, ubyte *trk, MidiEv *ev)
// ONLy ok if ALL hit.   nxt=\0 for current(default) or n[ext]
{ ubyt4 tm;
  ubyte c, n, tr, oct;
  bool  d, ok = true;
  DownRow *dn;
   if (nxt != 'n') {dn = & _dn [_pDn  ];   tm = _pDn ? _dn [_pDn-1].time : 0;}
   else            {if (_pDn+1 >= _dn.Ln)  return false;
                    dn = & _dn [_pDn+1];   tm =        _dn [_pDn  ].time;}
   for (c = 0;  c < dn->nNt;  c++) {
      d = TDrm (tr = dn->nt [c].t);   n = dn->nt [c].nt;
      if (ev && (ev->ctrl == n) && (MDR (ev) == d))  *trk = tr; // where ev is
      if (! nxt)  _lrn.hld [_f.trk [tr].ht - '1'] = n;     // mark so no red dot
//TStr d1,d2,d3;
//DBG("   c=`d/`d dr=`b nt=`s recTm=`s dnTm=`s",
//c, dn->nNt, d, MKey2Str(d1,n), TmSt(d2,_lrn.rec [d][n].tm), TmSt(d3,tm));
      if (_lrn.rec [d?1:0][n].tm <= tm)  ok = false;
   }
TRC("DnOK(`s)=> `b", (nxt=='n')?"next":"curr", ok);
   return ok;
}


void Song::SetMSec (ubyt4 p, MidiEv *ev)
{ ubyt2 tpP, tpR, tpS;
  ubyt4 ms, msR, tk, nm, dn, ne, tm, i;
  ubyte n, t;
  sbyte h;
  char  cl;
  TStr  ts;
   ms = ev->msec;
TRC("SetMSec  p=`d _pDn=`d ms=`d", p, _pDn, ms);
   if (_dn [p].msec)  return;          // already got 1st note

   _dn [p].msec = ms;   _dn [p].tmpo = 0;   if (p)  _dn [p-1].tmpo = 0;
   if ( (p == 0) || (_dn [p-1].msec == 0) || (ms <= _dn [p-1].msec) )
      return;                          // ^ somethin not right msR wise

// msec = tick*625/(tmpo*2)   so tmpo=tick*625/(msec*2)
   msR = ms - _dn [p-1].msec;          // actual ms in recording
   tk  = _dn [p].time - _dn [p-1].time;
   nm  = tk * 625;                     // num,den of rec tempo
   dn  = msR * 2;
   tpR = nm / dn + ( ((nm % dn) > (dn/2)) ? 1 : 0 );  // round it

// clip if beyond +-1/4 of prescribed tempo (learn track tempo)
   tpP = TmpoAt (_dn [p-1].time, 'a');
   if      (tpR < (ubyt4)(tpP-tpP/4))  {tpS = tpP-tpP/4;   cl = 's';}
   else if (tpR > (ubyt4)(tpP+tpP/4))  {tpS = tpP+tpP/4;   cl = 'f';}
   else                                {tpS = tpR;         cl = '\0';}

// store it.  just usin new tpS from now on
   _dn [p-1].tmpo = TmpoSto ((ubyt2)tpS);
   _dn [p-1].clip = cl;
TRC(" msR=`d-`d=`d  tpRecd=`d tpPrescribed=`d clip=`c tpClipped=`d tpStored=`d",
_dn[p-1].msec, ms, msR,  tpR, tpP, cl?cl:' ', tpS, _dn [p-1].tmpo);

// update bug arr
   tm = _dn [p].time;   ne = _f.bug.Ln;
   for (i = 0;  (i < ne) && (_f.bug [i].time < tm-6);  i++)  ;
TStr s1,s2;
TRC(" bug pos=`d/`d  bugTm=`s dnTm=`s",
i, ne, (i<ne)?TmSt(s1,_f.bug [i].time):"-", TmSt(s2, tm));
   if ( (i < ne) && (_f.bug [i].time > tm-6) &&
                    (_f.bug [i].time < tm+6) ) { // got existing bug to upd/del
      h = (sbyte)Str2Int (_f.bug [i].s);
      if (cl)  {if (h < 9)  h++;}   else h--;    // bump hits per clip
TRC("    bug upd `s=>`d", _f.bug [i].s, h);
      if (h > 0)  StrCp (_f.bug [i].s, Int2Str (h, ts));
      else        _f.bug.Del (i);
   }
   else if (cl) {
      TxtIns (tm, CC("1"), & _f.bug);      // got new bug to ins
TRC("    bug ins");
   }
}


//______________________________________________________________________________
bool Song::Record (MidiEv *ev)
{ ubyte  t;
  ubyt4  p, x;
  TrkEv *e;
  Arr<TrkEv,MAX_RCRD> *re;
   if (_lrn.POZ)  return false;        // poz recording only happens when done

TRC("Record `s", MDR(ev)?"drum":"melo");
   if (! RCRD) {                       // doin live recording in hear mode?
      for (t = 0;  t < _f.trk.Ln;  t++) {
         if (MDR (ev))  {
            if (   TDrm (t)  && TRec (t) && (MCTRL (ev) ||
                                             (_f.trk [t].drm == ev->ctrl)))
                                          {EvInsT (t, ev);   break;}
         }
         else {                        // melo notes can go into parallel trks
            if ((! TDrm (t)) && TRec (t))  EvInsT (t, ev);
            if (MCTRL (ev))  break;    // but ctrls just in 1st
         }
      }
      Put ();
      return true;
   }

   re = MDR (ev) ? (& _recD) : (& _recM);   // got no true "track"
   if (re->Full ())  return false;
   for (p = 0;  p < re->Ln;  p++)  if ((*re) [p].time > ev->time)  break;
// filter lame ntUps due to EdCmd n looping leftovers
/* if (MNTUP (ev)) {
**    x = p;
**    while (x && ((*re) [x-1].time == ev->time)) {
**       --x;
**       if ((ENTDN (& ((*re) [x]))) && ((*re) [x].ctrl == ev->ctrl))
**          {p = x;   break;}
**    }
** }
*/
   re->Ins (p);   e = & ((*re) [p]);
   e->time = ev->time;   e->ctrl = (ubyte) ev->ctrl;
   e->valu = ev->valu;   e->val2 = ev->val2;   e->x = 0;
   return false;
}


//______________________________________________________________________________
bool Song::EvRcrd (ubyte dev, MidiEv *ev)
// deal with a midiin device's event
{ ubyte  c, dr, nt, tr, oct;
  ubyt4  pd, dnTm;
  MidiEv te;
  TStr   cSt, s1,s2,s3,s4,s5,s6,s7,s8,s9,sa;
  bool   re = false;
TRC("EvRcrd `s.`d `s `s\n"
" ms=`d _pNow=`s _rNow=`s _now=`s tmr=`s\n"
" _pDn=`d dn.time=`s dn+1.time=`s",
(dev<_mi.Ln)?_mi [dev].mi->Name ():"kbd", ev->chan+1, TmSt(s1,ev->time),
(ev->ctrl & 0xFF80)
? StrFmt  (s2, "c=`s v=`d v2=`d", MCtl2Str(s3,ev->ctrl), ev->valu, ev->val2)
: MNt2Str (s4, ev),
ev->msec, TmSt(s5,_pNow),TmSt(s6,_rNow),TmSt(s7,_now),TmSt(s8,_timer->Get ()),
_pDn,(_pDn<_dn.Ln)?TmSt(s9,_dn [_pDn  ].time):"x",
(1+   _pDn<_dn.Ln)?TmSt(sa,_dn [_pDn+1].time):"x"
);
// check ctrl first
   *cSt = '\0';
   if (MCTRL (ev)) {                   // in=>song first
      for (c = 0;  c < _ccMap.Ln;  c++)  if ((_ccMap [c].dev == dev) &&
                                             (_ccMap [c].cc  == ev->ctrl))
         {StrCp (cSt, _ccMap [c].str);   break;}
      if      (! *cSt) {               // not mapped?  show cc map dlg (once)
         if (Up.id == 99)  {Up.id = dev;   Up.icc = ev->ctrl;   PreCtl ();}
      }
      else if (! StrCm (cSt, CC("keyCmd"))) {    // edit on/off w help dlg (once
         if (ev->valu < 64)  {if (  _ed)  {_ed = 0;   emit sgUpd ("dHlpS");}}
         else                 if (! _ed)  {_ed = 1;   emit sgUpd ("dHlpO");}
      }
      else if (ev->ctrl = CtlEv (cSt)) {         // 0 if outa _f.ctl spots,etc
         _rNow = ev->time;
         if      (! StrCm (cSt, CC("pBnR"))) {
            ev->valu = CtlPBnR [c = CtlPBnRLn * ev->valu / 128].val;
            StrCp (s2, CtlPBnR [c].str);
         }
         else if (! StrCm (cSt, CC("pStp"))) {
            ev->valu = CtlPStp [c = CtlPStpLn * ev->valu / 128].val;
            StrCp (s2, CtlPStp [c].str);
         }
         else
            StrFmt (s2, "`d", ev->valu);
         re = Record (ev);
         Info (StrFmt (s1, "`s = `s", cSt, s2));
      }
TRC("EvRcrd end - ctrl");
      return re;
   }

// notes only now
   if (NtCmd (ev))  return false;      // filter note command evs
   if (Step  (ev))  return false;      // filter step entry evs

// map drum .din => .drm
   dr = MDR(ev) ? 1 : 0;   nt = (ubyte) ev->ctrl;
   if (dr)  for (tr = 0;  tr < _f.trk.Ln;  tr++)      // might hafta map .din
      if (TLrn (tr) && TDrm (tr) && (_f.trk [tr].din == nt))
         {ev->ctrl = nt = _f.trk [tr].drm;   break;}

// on NtUp, clear _lrn.rec, record n scram  ...or if HEAR
   if (! MNTDN (ev) || (! RCRD)) {
      _lrn.rec [dr][nt].tm = 0;
      _rNow = ev->time;   re = Record (ev);   DrawNow ();
TRC("EvRcrd end - ntup|hear");
      return re;
   }

// RCRD n NtDn from here on...  (no redo)
   if (_lrn.nt1)  {                    // kill rec on 1st ntdn of loop
      _lrn.nt1 = false;   _recM.Ln = _recD.Ln = 0;    // recWipe
      for (ulong x = 0;  x < _dn.Ln;  x++) {          // clear _dn's rec stuff
         _dn [x].msec = 0;   _dn [x].tmpo = 0;   _dn [x].clip = '\0';
         MemSet (_dn [x].velo, 0, sizeof (_dn [0].velo));
      }
      Draw ('a');
   }
   _lrn.rec [dr][nt].tm = ev->time + (ev->time ? 0 : 1);
   _lrn.rec [dr][nt].vl = ev->valu & 0x7F;

// check if got all nts;  find lrn trk - look in _pDn, then _pDn+1
   dnTm = _dn [pd = _pDn].time;
   tr = 0x80;                          // default to no matched lrn trk from _dn

   if (ev->time <= dnTm) {
      if (DnOK ('c', & tr, ev)) {                     // ding ding ding
         SetMSec (pd, ev);
         if (_lrn.POZ) {
TRC("   UNPOZ cuz done w down");
            ev->time = dnTm;
            _lrn.POZ = false;   _timer->Set (dnTm);   Shush (false);
            Poz (false);
            for (dr = 0;  dr < 2;  dr++)
                           for (nt = 0;  nt < 128;  nt++)  if (nt != ev->ctrl) {
            // ntup for prev'ly not, ntdn for prev'ly not
               if      (   _lrn.rec [dr][nt].tm && (! _lrn.prec [dr][nt].tm)) {
                  te.chan = dr?9:0;   te.time = dnTm;   te.ctrl = nt;
                  te.valu = 0x80 | _lrn.rec [dr][nt].vl;   te.val2 = 0;
                  Record (& te);       // NtDn
               }
               else if ((! _lrn.rec [dr][nt].tm) &&   _lrn.prec [dr][nt].tm ) {
                  te.chan = dr?9:0;   te.time = dnTm;   te.ctrl = nt;
                  te.valu = te.val2 = 0;
                  Record (& te);       // NtUp
               }
            }
         }
      }
   }
   else if (! _lrn.POZ) {
      pd++;
      if (DnOK ('n', & tr, ev)) {   // ding ding ding
         SetMSec (pd, ev);                             // BOING !!
TRC("   hard BOING forward");
         ev->time = dnTm = _dn [pd].time;
         _timer->SetSig (dnTm);   _timer->Set (dnTm);
      }
   }
TRC("   lrnTrk=`d pDn=`d", tr, pd);

   if (tr != 0x80) {                   // store our velo in _dn[].velo[]
      oct = TDrm (tr) ? 0 : (_f.trk [tr].ht - '0');
      _dn [pd].velo [oct] = ev->valu & 0x7F;     // .ht '1' => 1 (0 for drum)
TRC("   oct=`d", oct);
   }
   _rNow = ev->time;   Record (ev);   DrawNow ();
TRC("EvRcrd end");
   return false;
}


//______________________________________________________________________________
void Song::MIn ()
{ MidiEv e;
  bool re = false;
   for (ubyte d = 0;  d < _mi.Ln;  d++)
      while (_mi [d].mi->Get (& e))  if (EvRcrd (d, & e))  re = true;
   Put ();
   if (re) {                           // quick-ish ReDo
TRC("miReDo");
      SetNt ();   _pg = _tr = 0;   SetSym ();   Draw ('a');
TRC("miReDo end");
   }
}
