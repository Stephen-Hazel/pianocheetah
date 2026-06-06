// sUtil.cpp - gory util funcs o Song

#include "song.h"

char  KeyCol [13] = "wbwbwwbwbwbw";
ubyte WXOfs  [12] = {0, 99, 4, 99, 8, 0, 99, 3, 99, 6, 99, 9};

ubyt2 Song::Nt2X (ubyte n, ColDef *co, char gr)
// RCRD is ez n special
// nx is past border, chord, cue
// bump by leftmost whitebump + W_NT*(this note-colMin note)
// then normally offset left by whiteBump of n unless gr[id] = white tail x
{ ubyte m = co->nMn, oc, ky;
  ubyt2 nx, xo;
  TStr  sn, sx;
   nx = co->nx;
   if (SHRCRD) {
      oc = n/12 - 2;   ky = n%12;
      xo = W_NTW / 2;                  // default to not right
      if ( (n >= co->oMn [oc]) &&
           (n <= co->oMx [oc]) && (KeyCol [ky] == 'w') ) {
         MKey2Str (sn, co->oMn [oc]);   MKey2Str (sx, n);
         xo = (sx[1] - sn[1]) * W_NTW;
      }
      return nx + co->oX [oc] + xo;
   }
   nx += ((KeyCol [m % 12] == 'w') ? (WXOfs [m % 12] * W_NT / 12) : 0);
   nx += (W_NT * (n - m));
   if (! gr)  if (KeyCol [n % 12] == 'w')  nx -= (WXOfs [n % 12] * W_NT / 12);
   return nx;
}


ubyt2 Song::Tm2Y (ubyt4 t, ColDef *co, BlkDef **bl)
{ ubyt4 b;
//DBG("Tm2Y t=`d co=`08x bl=`08x", t, co, bl);
   for (b = 0;  (b+1) < co->nBlk;  b++)  if ((t >= co->blk [b].tMn) &&
                                             (t <  co->blk [b].tMx))  break;
   if (bl)  *bl = & co->blk [b];
//DBG("Tm2Y b=`d", b);
   return (ubyt2)(co->blk [b].y + (co->blk [b].h-1) * (t - co->blk [b].tMn) /
                                  (co->blk [b].tMx - 1  -  co->blk [b].tMn));
}


ubyt4 Song::Y2Tm (ubyt2 y, ColDef *co)
// y can be above/below valid Ys for col if dragging cues,chords
{ ubyt4 t;
   if (y < co->blk [0].y) {            // reuse t - dur above blk
      t =                   (co->blk [0].tMx - 1 - co->blk [0].tMn) *
                            (co->blk [0].y - y) / (co->blk [0].h - 1);
      if (t <= co->blk [0].tMn)  return co->blk [0].tMn - t;
      return 0;              // return y timescaled by blk#0;  limited to 0
   }
   for (t = 0;  (t+1) < co->nBlk;  t++)     // end with USEable blk in t
      if ((y >= co->blk [t].y) && (y < co->blk [t].y+co->blk [t].h))  break;
   return co->blk [t].tMn + (co->blk [t].tMx - 1 - co->blk [t].tMn) *
                            (y - co->blk [t].y) / (co->blk [t].h - 1);
}


ubyt4 Song::GetDn (TrkEv *e, ubyt4 p, ubyte nt)
// edcmds and looping can "leak" ntUps
// 8cDn,4f#Dn,8cUp,4f#Up=>recorded 4f#;  looping with some nts Dn;  => :(
// try to prevent that by looking for prev ntDn w/ no prev ntUp (on same nt)
{ ubyt4 end = p;
//TStr d1;
//DBG("GetDn p=`d nt=`s", p, MKey2Str (d1, nt));
   while (p--)  if (e [p].ctrl == nt) {               // got prev ev same nt?
      if      (! (e [p].valu & 0x80))  break;         // got nother (diff) up :(
      else if (! (e [p].val2 & 0x80)) {
//DBG("rc=`d", p);
       return p;      // got dn :)
}
   }
//DBG("rc=end p=`d", p);
   return end;
}


ubyt4 Song::GetUp (TrkEv *e, ubyt4 p, ubyt4 ne, ubyte nt)
// look for next ntUp w/ no prev ntDn (on same nt)
{ ubyt4 end = p++;
   for (;  p < ne;  p++)  if (e [p].ctrl == nt) {     // got latr ev same nt?
      if      (! (e [p].valu & 0x80))  return p;      // got up :)
      else if (! (e [p].val2 & 0x80))  return end;    // got nother (diff) dn :(
   }
   return end;
}


ubyt4 Song::SilPrv (ubyt4 tm)
// scoot q note earlier n look back for silence (no notes coverin time)
// if no silence for 2 bars back, just use tm as is
{ ubyte t;
  bool  got = true;
  ubyt4 tIn, t1, p, bp;
  TrkEv *e;
   if (tm <= (M_WHOLE/4))  return 0;
   tIn = tm;
   tm -= (M_WHOLE/4);   t1 = tm;

// keep checkin for ? notes coverin tm n get new min;  check THAT2
   do
      for (got = false, t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t))
         for (e = _f.trk [t].e, p = _f.trk [t].ne;  p;) {
            --p;                    // skip evs till we get a ntUp in p-1
            if (ENTUP (& e [p])) {
               if (e [p].time < tm)  break; // end of the line for this trk
               if ((bp = GetDn (e, p, e [p].ctrl)) < p)
                  if (e [bp].time < tm)  {got = true;   tm = e [bp].time;}
            }
         }
   while (got);

   if (tm == t1) {                     // was in silence already - get 1st NtDn
      for (t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t))
         for (e = _f.trk [t].e, p = 0;  p < _f.trk [t].ne;  p++)
            if (ENTDN (& e [p]) && (e [p].time >= tm))     // t1 has min so far
               {if (e [p].time <= t1) t1 = e [p].time;   break;}
      if (t1 < tm)  tm = t1;
   }
   if (tm)  tm--;
   if (tm+M_WHOLE*2 < tIn)  tm = tIn;  // don't go further back than 2 bars
   return tm;
}


ubyt4 Song::SilNxt (ubyt4 tm)
// scoot q note later n look forward for silence (no notes coverin time)
// if no silence for 2 bars foreward, just use tm as is
{ ubyte t;
  bool  got = true;
  ubyt4 tIn, t1, tme, ne, p, ep;
  TrkEv *e;
   tme = Bar2Tm (_bEnd);
   tIn = tm;
   tm += (M_WHOLE/4);   if (tm >= tme)  return tme;
                        t1 = tm;
// keep checkin for ? notes coverin tm n get new max;  check THAT2
   do
      for (got = false, t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t))
         for (e = _f.trk [t].e, ne = _f.trk [t].ne, p = 0;  p < ne;  p++)
            if (ENTDN (& e [p])) {  // skip evs till we get a ntDn
               if (e [p].time > tm)  break;   // end of the line for this trk
               if ((ep = GetUp (e, p, ne, e [p].ctrl)) > p)
                  if (e [ep].time > tm)  {got = true;   tm = e [ep].time;}
            }
   while (got);

   if (tm == t1) {                     // was in silence already - get end NtUp
      for (t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t))
         for (e = _f.trk [t].e, p = _f.trk [t].ne;  p;) {
            p--;
            if (ENTUP (& e [p]) && (e [p].time <= tm))     // t1 has max so far
               {if (e [p].time >= t1) t1 = e [p].time;   break;}
         }
      if (t1 > tm)  tm = t1;
//    tm += ((M_WHOLE/4) - tm % (M_WHOLE/4));
   }
   if (tm > tme)  tm = tme;
   if (tm > tIn+M_WHOLE*2)  tm = tIn;  // don't go further forward than 2 bars
   return tm;
}


//______________________________________________________________________________
// funcs around _f.ctl[]

void Song::ReCtlO ()                   // rebuild used _f.dvts' CCMaps
{ ubyte d, i;
  TStr  s;
   for (d = 0;  d < Up.dvt.Ln;  d++)  if (Up.dvt [d].Name () [0]) {  // used?
      MemSet (Up.dvt [d].CCMap, 0, sizeof (Up.dvt [d].CCMap));
      for (i = 3;  i < _f.ctl.Ln;  i++)     // skip tmpo,tsig,ksig
         if (! (Up.dvt [d].CCMap [i] = Up.dvt [d].CCID (_f.ctl [i].s))) {
Info(StrFmt (s, "DvT=`s can't do cc=`s", Up.dvt [d].Name(), _f.ctl [i].s));
DBG (           "DvT=`s can't do cc=`s", Up.dvt [d].Name(), _f.ctl [i].s);
         }
   }
}


ubyte Song::CtlEv (char *cSt, char ro)
// turn cSt into _f.ctl pos|0x80 (and ins if new and room)
// return 0 if outa room
{ ubyte c;
  TStr  s;
   for (c = 0;  c < _f.ctl.Ln;  c++)  if (! StrCm (_f.ctl [c].s, cSt))  break;
   if (c >= _f.ctl.Ln) {               // new one to insert
      if (ro != '\0')  return 0;       // just checkin - don't wanna ins

Info(StrFmt (s, "CtlEv  new! `s => `d", cSt, c));
      if (_f.ctl.Full ()) {
Info(StrFmt (s, "CtlEv  out of room for `s :(", cSt));
DBG (           "CtlEv  out of room for `s :(", cSt);
         return 0;
      }
      _f.ctl.Ln++;   StrCp (_f.ctl [c].s, cSt);   _f.ctl [c].sho = 'm';
      ReCtlO ();
   }
   return 0x80 | c;
}


char *Song::CtlSt (ubyte ctrl)  {return _f.ctl [ctrl & 0x7F].s;}


static int CtlCmp (void *p1, void *p2)  // ..._f.ctl str sortin (by MCC[] pos)
{ char *s1 = (char *)p1, *s2 = (char *)p2;
  ubyte i1, i2;
   s1++;   s2++;                       // skip past sho char
   for (i1 = 0;  i1 < NMCC;  i1++)  if (! StrCm (s1, MCC [i1].s))  break;
   for (i2 = 0;  i2 < NMCC;  i2++)  if (! StrCm (s2, MCC [i2].s))  break;
   if ((i1 < NMCC) && (i2 < NMCC))  return i1 - i2;
   if (i1 < NMCC)  return -1;
   if (i2 < NMCC)  return  1;
   return StrCm (s1, s2);
}


void Song::CtlClean ()
// compact _f.ctl[] to just used n shown ones.  sorted per device/cc.txt
{ ubyte ncc, c, d, t, tc, cm [128];
  TStr   cc [128];
  ubyt4  e;
  TrkEv *ev;
TRC("CtlClean");
   MemSet (cc, 0, sizeof (cc));        // default to 'don't got it'
   for (t = 0;  t < _f.trk.Ln;  t++)   // find used ctls we DO got
      for (ev = _f.trk [t].e, e = 0;  e < _f.trk [t].ne;  e++)
         if ((tc = ev [e].ctrl) & 0x80)  cc [tc & 0x7F][0] = 'x';
// rebuild cc[] w only used ones 0 str of   sho + s
   for (ncc = c = 0;  c < _f.ctl.Ln;  c++)  if (cc [c][0])
      StrFmt (cc [ncc++], "`c`s", _f.ctl [c].sho, _f.ctl [c].s);
   Sort (cc, ncc, sizeof (cc [0]), CtlCmp);
// build map of old=>new (_f.ctl[]=>cc[])
   for (c = 0;  c < _f.ctl.Ln;  c++)  for (d = 0;  d < ncc;  d++)
      if (! StrCm (_f.ctl [c].s, & cc [d][1]))
         {cm [c] = d;   break;}
   for (t = 0;  t < _f.trk.Ln;  t++)   // restamp ctl evs
      for (ev = _f.trk [t].e, e = 0;  e < _f.trk [t].ne;  e++)
         if ((tc = ev [e].ctrl) & 0x80)  ev [e].ctrl = 0x80 | cm [tc & 0x7F];
   _f.ctl.Ln = ncc;                    // rebuild _f.ctl[]
   for (c = 0;  c < ncc;  c++)  {_f.ctl [c].sho = cc [c][0];
                          StrCp (_f.ctl [c].s,  & cc [c][1]);}
   ReCtlO ();
TRC("CtlClean end");
}


//______________________________________________________________________________
void Song::EvDel (ubyte t, ubyt4 p, ubyt4 ne)
// kill off some events in a track and adj other trk[].e's
// caller NEEDS to verify p..p+ne are w/in trk's e[]
{ ubyt4 pAbs;
   if (! ne)                return;
   pAbs = & _f.trk [t].e [p] - _f.ev;
   RecDel (_f.ev, _f.nEv, sizeof (TrkEv), pAbs, ne);
   for (ubyte i = 0; i < _f.trk.Ln; i++)
      if ((i != t) && (_f.trk [i].e >  _f.trk [t].e))  _f.trk [i].e -= ne;
   _f.trk [t].ne -= ne;   _f.nEv -= ne;
}


bool Song::EvIns (ubyte t, ubyt4 p, ubyt4 ne)
// if room, ins new spots;  bump following _f.trk[].e's
//TODO tracks' .e must stay in order :/
// if t0..2 all have same .e and ins into t1, t2.e SHOULD be bumped, t0.e NOT
{ ubyt4 pAbs;
   if (! ne)                    return true;
   if (_f.nEv + ne > _f.maxEv)  return false;   // no rooom :(
   pAbs = & _f.trk [t].e [p] - _f.ev;
   RecIns (_f.ev, _f.nEv += ne, sizeof (TrkEv), pAbs, ne);
   for (ubyte i = t+1; i < _f.trk.Ln; i++)
      if (_f.trk [i].e >= _f.trk [t].e)  _f.trk [i].e += ne;
   _f.trk [t].ne += ne;
   return true;
}


void Song::EvInsT (ubyte t, TrkEv *ev)
// insert ev based on it's time
// find time pos.  for ntUp, put it in front of any ntDn w same time&key
// bump nn,nb if note in rec trk
{ ubyt4 ne, p, x;
  TrkEv *e;
   e = _f.trk [t].e;   ne = _f.trk [t].ne;
   for (p = 0;  p < ne;  p++)  if (e [p].time > ev->time)  break;
   if (ENTUP (ev)) {
      x = p;
      while (x && (e [x-1].time == ev->time)) {
         --x;
         if ((ENTDN (& e [x])) && (e [x].ctrl == ev->ctrl)) {p = x;   break;}
      }
   }
   if (EvIns (t, p)) {
      MemCp (& e [p], ev, sizeof (TrkEv));
      if ((t >= _f.trk.Ln) && (! (ev->ctrl & 0x0080))) {
         if (ev->valu & 0x80)  {_f.trk [t].nn++;   _f.trk [t].nb++;}
         else                                      _f.trk [t].nb--;
//DBG("EvInsT1 t=`d nn=`d nb=`d", t, _f.trk [t].nn, _f.trk [t].nb);
      }
   }
}


void Song::EvInsT (ubyte t, MidiEv *ev)
// same as above EvInsT but w MidiEv
{ ubyt4 ne, p, x;
  TrkEv *e;
//DBG("EvInsT t=`d", t);
   e = _f.trk [t].e;   ne = _f.trk [t].ne;
   for (p = 0;  p < ne;  p++)  if (e [p].time > ev->time)  break;
   if (MNTUP (ev)) {
      x = p;
      while (x && (e [x-1].time == ev->time)) {
         --x;
         if ((ENTDN (& e [x])) && (e [x].ctrl == ev->ctrl)) {p = x;   break;}
      }
   }
   if (EvIns (t, p)) {
//TStr s, s2;
//if (MNOTE (ev))  TRC("EvInsT tr=`d p=`d/`d e=`08x tm=`s `s",
//t, p, ne, e, TmSt (s, ev->time), MNt2Str (s2, ev));
//else             TRC("EvInsT tr=`d p=`d/`d e=`08x tm=`s `s $`02x",
//t, p, ne, e, TmSt (s, ev->time), CtlSt (ev->ctrl), ev->valu);
      e [p].time = ev->time;   e [p].ctrl = (ubyte) ev->ctrl;
      e [p].valu = ev->valu;   e [p].val2 = ev->val2;   e [p].x = 0;
      if ((t >= _f.trk.Ln) && (! (ev->ctrl & 0x0080))) {
         if (ev->valu & 0x80)  {_f.trk [t].nn++;   _f.trk [t].nb++;}
         else                                      _f.trk [t].nb--;
//DBG("EvInsT2 t=`d nn=`d nb=`d", t, _f.trk [t].nn, _f.trk [t].nb);
      }
   }
}


void Song::NtIns (ubyte t, ubyt4 tm, ubyt4 te, ubyte c, ubyte v)
{ TrkEv ed, eu;
   ed.time = tm;   eu.time = te;   ed.ctrl = eu.ctrl = c;
   ed.valu = 0x80 | v;   eu.valu = 64;   ed.val2 = eu.val2 = 0;
   EvInsT (t, & ed);   EvInsT (t, & eu);
}


bool Song::TxtIns (ubyt4 t, char *s, Arr<TxtRow,MAX_LYR> *ta, char cue)
{ ubyt4 p, d;
  char *c;
   if (ta->Full ())  return false;     // no room
   for (p = 0;  p < ta->Ln;  p++)  if ((*ta) [p].time >= t) break;
   ta->Ins (p);
   (*ta) [p].time = t;
   if ((*s == '/') && cue)             // cues have the /dur thingy
         {d = Str2Int (& s [1], & c);
          (*ta) [p].tend = t + d;   StrCp ((*ta) [p].s, c);}
   else  {(*ta) [p].tend = 0;       StrCp ((*ta) [p].s, s);}
   return true;
}


ubyt4 Song::NtHit (ubyte t, ubyt4 tmn, ubyt4 tmx, ubyte nmn, ubyte nmx)
// see if any notes in trk t are w/in tm,nt box, if so return 1st hit's p+1 / 0
{ ubyt4  p, q;
  TrkEv *e;
  ubyt4 ne, tm;
  ubyte nt;
   e = _f.trk [t].e;   ne = _f.trk [t].ne;
   for (p = 0;  p < ne;  p++)  if (ENTDN (& e [p])) {      // all ntDns in t
      nt = e [p].ctrl;   if ((nt < nmn) || (nt > nmx))  continue;
   // find assoc ntUp
      for (q = p+1;  q < ne;  q++)  if (ENTUP (& e [q]) &&
                                                (e [q].ctrl == nt))  break;
      if (q < ne) {
         tm = e [p].time;   if ((tm >= tmn) && (tm <= tmx))  return p+1;
         tm = e [q].time;   if ((tm >= tmn) && (tm <= tmx))  return p+1;
      }
   }
   return 0;
}


//______________________________________________________________________________
void Song::TrkSnd (ubyt4 snew)
{ ubyte d;
   ChkETrk ();
   d = _f.trk [Up.eTrk].dev;
   if (TDrm (Up.eTrk))                 // change drum sound?
      if (! Up.dev [d].mo->Syn ())
         {Hey (CC("Can't edit GM DrumMap"));   return;}
   _f.trk [Up.eTrk].snd = snew;
   SetBnk ();   ReTrk ();
}


void Song::NewGrp (char *gr)           // slam to a new sound picked by gui
{ TStr t;
  bool dr;
TRC("NewGrp `s r=`d", gr, Up.eTrk);
   ChkETrk ();
   StrCp (t, gr);   StrAp (t, CC("_"));
   TrkSnd (Up.dvt [Up.dev [_f.trk [Up.eTrk].dev].dvt].SndID (t));
}


void Song::NewSnd (char *sn)           // slam to a new sound picked by gui
{ TStr  g;
  char *sl;
TRC("NewSnd `s r=`d", sn, Up.eTrk);
   ChkETrk ();
   StrCp (g, SndName (Up.eTrk));
   sl = StrCh (g, '_');   if (sl)  sl [1] = '\0';
   StrAp (g, sn);
   TrkSnd (Up.dvt [Up.dev [_f.trk [Up.eTrk].dev].dvt].SndID (g));
}

/*
void Song::NewSnd (char ofs)
// step to new snd via devtyp snd list and offset type
{ ubyte t;
  ubyt4 snew;
   ChkETrk ();
   t = Up.dev [_f.trk [Up.eTrk].dev].dvt;
   if (TDrm (Up.eTrk))
      {Hey (CC("Can't edit GM DrumMap"));   return;}
   if (! Up.dvt [t].SndNew (& snew, _f.trk [Up.eTrk].snd, ofs))
       Hey (CC("can't change sound"));
}
*/


//______________________________________________________________________________
void Song::NewDev (char *dNm)          // slam trk to new dev picked by gui
{ ubyte d, oDv, nCh, tr, t, t1, t2;
  char  cmap [17];
  TStr  sNm;
  bool  got;
  TrkRow ro;
   ChkETrk ();   tr = Up.eTrk;
DBG("NewDev `s   tr=`d", dNm, tr);     // get trk n old dev,chn
   d = oDv = _f.trk [tr].dev;
DBG(" old dv=`d", oDv);
   if ( (   _f.trk [tr].grp  && (! StrCm (dNm, CC("+")))) ||    // no change?
        ((! _f.trk [tr].grp) && (! StrCm (dNm, DevName (tr)))) )
                                                            {ReDo ();   return;}
   if      (! StrCm (dNm, CC("+"))) {  // to grouped?
      if      (tr == 0)
         {Hey (CC("can't + the 1st track"));                 ReDo ();   return;}
      else if ( (   TDrm (tr)  && (! TDrm (tr-1))) ||
                ((! TDrm (tr)) &&    TDrm (tr-1) ) )
         {Hey (CC("can't share channel with prev track"));   ReDo ();   return;}
      d = _f.trk [tr-1].dev;   nCh = _f.trk [tr-1].chn;
   }
   else if (TDrm (tr))         nCh = 9;
   else if ((nCh = PickChn (dNm)) == 255)   // all melo chans of new dev in use?
      {Hey (CC("device's channels are all in use"));         ReDo ();   return;}
DBG(" new ch=`d", nCh+1);

// get prev snd name as str
   if (nCh != 9) {
      if (_f.trk [tr].snd != SND_NONE)
            StrCp (sNm, Up.dvt [Up.dev [d].dvt].Snd (_f.trk [tr].snd)->name);
      else  StrCp (sNm, CC("Piano_AcousticGrand"));
DBG(" sNm=`s", sNm);
   }

// t1-t2 covers my group range...
   t1 = t2 = tr;
   while (                       _f.trk [t1  ].grp)   t1--;
   while ((t2+1 < _f.trk.Ln) && (_f.trk [t2+1].grp))  t2++;
DBG(" tr rng=`d - `d", t1, t2);

// goin to ungrouped?  gotta scoot below old grp
   if (_f.trk [tr].grp && StrCm (dNm, CC("+"))) {
DBG(" goin to ungrouped");
      if (tr != t2) {                       // scoot if not already at bot
         MemCp (& ro,          & _f.trk [tr], sizeof (ro));
         MemCp (& _f.trk [tr], & _f.trk [t2], sizeof (ro));
         MemCp (& _f.trk [t2], & ro,          sizeof (ro));
         Up.eTrk = tr = t2;
      }
      _f.trk [tr].grp = false;
      _f.trk [tr].snd = _f.trk [t1].snd;    // ungroup,set snd to orig top trk
      t1 = t2 = tr;
   }

// will old dev be gonzo?
   NotesOff ();
   for (got = false, t = 0;  t < _f.trk.Ln;  t++)  if ((t < t1) || (t > t2))
      if (oDv == _f.trk [t1].dev)  got = true;
   if (! got) {
DBG(" shut old dev");
      ShutDev (_f.trk [t1].dev);
   }

   if (StrCm (dNm, CC("+"))) {
      d = OpenDev (dNm);      // unless +, get new one
DBG(" open new dev=`d", d);
   }

   _f.trk [tr].dev = d;
   _f.trk [tr].chn = nCh;
   _f.trk [tr].grp = (StrCm (dNm, CC("+")) ? false : true);
DBG(" new dv=`d ch=`d grp=`b", d, nCh+1, _f.trk [tr].grp);

// new sndid since new dev :/
   got = false;                        // drum syn check
   if (nCh != 9)  _f.trk [t1].snd = Up.dvt [Up.dev [d].dvt].SndID (sNm);
   else           got = true;          // hello drum syn
DBG(" tr=`d sndid=`d", t1, _f.trk [t1].snd);

// setup all trks of grp
   for (t = t1+1;  t <= t2;  t++) {
DBG(" tr=`d too", t);
      _f.trk [t].dev = _f.trk [t1].dev;
      _f.trk [t].chn = _f.trk [t1].chn;
      if (! got)  _f.trk [t].snd = _f.trk [t1].snd;
   }
// redo drum sounds fer syn (toss the useless _f.mapD n DrumExp recalcs
   if (got)  {DrumCon ();   _f.mapD.Ln = 0;   DrumExp ();}      // and SetBnk()s
   else      SetBnk ();
   ReDo ();
DBG("NewDev end");
}


//______________________________________________________________________________
void Song::TrkSplt ()
// just split the one ? track into 1st stab at 3LH,4RH split on 3e
{ ubyte t, t2;
  ubyt4 p, p2;
  TrkEv *e, ev;
   for (t = 0;  t < _f.trk.Ln;  t++)  if ((! TDrm (t)) && TLrn (t))  break;
   if (t >= _f.trk.Ln)  {Hey (CC("pick a learn track to split"));   return;}

// insert new 3LH in t+1 and setup all the track junk
   if (TrkIns (t2 = t+1, CC(""), NULL) == MAX_TRK)
                      {Hey (CC("couldn't insert LH track :("));   return;}
   _f.trk [t ].ht  = '4';
   _f.trk [t2].grp = true;
   _f.trk [t2].shh = false;
   _f.trk [t2].lrn = true;
   _f.trk [t2].rec = false;
   _f.trk [t2].ht  = '3';
   _f.trk [t2].dev = _f.trk [t].dev;   _f.trk [t2].chn = _f.trk [t].chn;
   _f.trk [t2].snd = _f.trk [t].snd;   _f.trk [t2].drm = _f.trk [t].drm;
   _f.trk [t2].vol = _f.trk [t].vol;   _f.trk [t2].pan = _f.trk [t].pan;
   for (e = _f.trk [t].e, p = p2 = 0;  p < _f.trk [t].ne;) {
      if (e [p].ctrl <= MKey (CC("3e"))) {
         MemCp (& ev, & e [p], sizeof (ev));
         EvDel (t, p);   EvIns (t2, p2);   MemCp (& _f.trk [t2].e [p2++],
                                                  & ev, sizeof (ev));
      }
      else  p++;
   }
   ReDo ();
}


//______________________________________________________________________________
void Song::TrkDel (ubyte t)
// kill any evs;  maybe close dev;  adjust trk refs
{ ubyte e;
  ubyt4 i;
  bool  got;
   if (t >= _f.trk.Ln)  return;        // can't :/
TRC("TrkDel t=`d", t);
   NotesOff ();
   if (((ubyte)(t+1) < _f.trk.Ln) && _f.trk [t+1].grp && (! _f.trk [t].grp)) {
      _f.trk [t+1].grp = false;        // re-start group on next track down
      _f.trk [t+1].dev = _f.trk [t].dev;
      _f.trk [t+1].chn = _f.trk [t].chn;
   }
   EvDel (t, 0, _f.trk [t].ne);
   for (got = false, e = 0;  e < _f.trk.Ln;  e++)
      if ((e != t) && (_f.trk [e].dev == _f.trk [t].dev))
         {got = true;   break;}
   if (! got)  ShutDev (_f.trk [t].dev);
   _f.trk.Del (t);
   if (t < _f.trk.Ln)         _f.trk.Ln--;  // adj all .trk refs
   if (Up.eTrk >  t)          Up.eTrk--;
   if (Up.eTrk >= _f.trk.Ln)  Up.eTrk--;
}


ubyte Song::TrkIns (ubyte t, char *name, char *snd)
// Chrd uses t,name,snd
// +    uses t,(NULL,NULL)
{  if (_f.trk.Full ())  {DBG("too many tracks");   return MAX_TRK;}
TRC("TrkIns t=`d/`d nm=`s sn=`s", t, _f.trk.Ln, name?name:"", snd?snd:"");
   if (t >= _f.trk.Ln)  t = (ubyte)(_f.trk.Ln);

// init most _f.trk fields to 0;  set .name, .e
   _f.trk.Ins (t);                     // MemSets _f.trk [t] to 0
   if (name)  StrCp (_f.trk [t].name, name);
   _f.trk [t].e   = t ? (_f.trk [t-1].e + _f.trk [t-1].ne) : _f.ev;
   if (snd != nullptr)
      PickDev (t, *snd ? snd : CC("Piano/AcousticGrand"));
   else if (t+1 < _f.trk.Ln) {
      _f.trk [t].dev = _f.trk [t+1].dev;   _f.trk [t].chn = _f.trk [t+1].chn;
      _f.trk [t].snd = _f.trk [t+1].snd;   _f.trk [t].drm = _f.trk [t+1].drm;
      _f.trk [t].vol = _f.trk [t+1].vol;   _f.trk [t].pan = _f.trk [t+1].pan;
      _f.trk [t].grp = _f.trk [t+1].grp;   _f.trk [t+1].grp = true;
      _f.trk [t].shh = _f.trk [t+1].shh;   _f.trk [t].lrn = _f.trk [t+1].lrn;
                                           _f.trk [t].rec = _f.trk [t+1].rec;
      _f.trk [t].ht  = _f.trk [t+1].ht;
   }
   ReTrk ();
TRC("TrkIns: t=`d", t);
   return t;
}


void Song::TrkEd (char *arg)
// it's kind of a pain to keep groups ok in up/dn
{ ubyte t = ChkETrk (), tb, te, dtr, ntr;
  ubyt4 p, cb, ce, cd, tt, maxt;
  char  op = *arg;
  TrkRow tr [MAX_TRK];
   if ((op == '*') || (op == '-')) {
      cb = ce = cd = maxt = 9999*M_WHOLE;
      for (p = 0;  p < _f.cue.Ln;  p++) {
         if (! StrCm (_f.cue [p].s, CC("{")))
            {if (cb < maxt)  {Hey (CC("multiple { cues"));   return;}
             cb = _f.cue [p].time;}
         if (! StrCm (_f.cue [p].s, CC("}")))
            {if (ce < maxt)  {Hey (CC("multiple } cues"));   return;}
             ce = _f.cue [p].time;}
         if (! StrCm (_f.cue [p].s, CC("^")))
            {if (cd < maxt)  {Hey (CC("multiple ^ cues"));   return;}
             cd = _f.cue [p].time;}
      }
      if (cb == maxt)  {Hey (CC("there's no { cue"));   return;}
      if (ce == maxt)  {Hey (CC("there's no } cue"));   return;}
      if (cd == maxt)  {Hey (CC("there's no ^ cue"));   return;}
TStr d1,d2,d3;
DBG("cb=`s ce=`s cd=`s", TmSt(d1,cb), TmSt(d2,ce), TmSt(d3,cd));
      if (op == '-') {                 // TODO check overlap if cd<cb
      // evs
         for (t = 0;  t < _f.trk.Ln;  t++)
            for (p = 0;  p < _f.trk [t].ne;  p++)
               if (_f.trk [t].e [p].time >= cb)
                  {_f.trk [t].e [p].time += cd;   _f.trk [t].e [p].time -= cb;}
      // markers
         for (p = 0;  p < _f.lyr.Ln;  p++)  if (_f.lyr [p].time >= cb)
            {_f.lyr [p].time += cd;   _f.lyr [p].time -= cb;}
         for (p = 0;  p < _f.chd.Ln;  p++)  if (_f.chd [p].time >= cb)
            {_f.chd [p].time += cd;   _f.chd [p].time -= cb;}
         for (p = 0;  p < _f.cue.Ln;  p++) {
            if (_f.cue [p].time >= cb)
               {_f.cue [p].time += cd;   _f.cue [p].time -= cb;}
            if (_f.cue [p].tend >= cb)
               {_f.cue [p].tend += cd;   _f.cue [p].tend -= cb;}
         }
      }
      else { // '*'                    ...TODO check for overlap if cd>ce
      // evs
         for (t = 0;  t < _f.trk.Ln;  t++)
            for (p = 0;  p < _f.trk [t].ne;  p++)
               if ((tt = _f.trk [t].e [p].time) >= cb)
                  _f.trk [t].e [p].time = (tt < ce) ?
                           (cb + (tt-cb) * (cd-cb) / (ce-cb)) : (tt + cd - ce);
      // markers
         for (p = 0;  p < _f.lyr.Ln;  p++)  if ((tt = _f.lyr [p].time) >= cb)
            _f.lyr [p].time = (tt < ce) ? (cb + (tt-cb) * (cd-cb) / (ce-cb))
                                        : (tt + cd - ce);
         for (p = 0;  p < _f.chd.Ln;  p++)  if ((tt = _f.chd [p].time) >= cb)
            _f.chd [p].time = (tt < ce) ? (cb + (tt-cb) * (cd-cb) / (ce-cb))
                                        : (tt + cd - ce);
         for (p = 0;  p < _f.cue.Ln;  p++) {
            if ((tt = _f.cue [p].time) >= cb)
               _f.cue [p].time = (tt < ce) ? (cb + (tt-cb) * (cd-cb) / (ce-cb))
                                           : (tt + cd - ce);
            if ((tt = _f.cue [p].tend) >= cb)
               _f.cue [p].tend = (tt < ce) ? (cb + (tt-cb) * (cd-cb) / (ce-cb))
                                           : (tt + cd - ce);
         }
      }
      ReDo ();                         // rebuild it aaaall
      return;
   }
   tb = te = t;                        // find group's full range
   while ( tb                         && _f.trk [tb  ].grp)  tb--;
   while (((ubyte)(te+1) < _f.trk.Ln) && _f.trk [te+1].grp)  te++;
   if      (op == 'x')  TrkDel  (t);
   else if (op == '+')  TrkIns  (t);
   else if (op == 's')  TrkSplt ();
   else if (op == 'u') {
      if (t == tb) {                   // no group or top trk of grp
         if (tb == 0)  {Hey (CC("already at the top"));   return;}
         dtr = tb-1;   ntr = 1;   while (_f.trk [dtr].grp) {dtr--;  ntr++;}
//DBG("Ln=`d tb=`d te=`d dtr=`d ntr=`d", _f.trk.Ln, tb, te, dtr, ntr);
         MemCp (tr,                & _f.trk [dtr], sizeof (TrkRow) * ntr);
         MemCp (& _f.trk [dtr],    & _f.trk [tb],  sizeof (TrkRow) * (te-tb+1));
         MemCp (& _f.trk [te-ntr+1], tr,           sizeof (TrkRow) * ntr);
         Up.eTrk -= ntr;
      }
      else {                           // w/in a group - scoot 1:  t<=>t-1
         MemCp (tr,             & _f.trk [t-1], sizeof (TrkRow));
         MemCp (& _f.trk [t-1], & _f.trk [t],   sizeof (TrkRow));
         MemCp (& _f.trk [t],   tr,             sizeof (TrkRow));
         if (t-1 == tb) {              // bump'd it into top of group's trk
            _f.trk [t-1].grp = false;   _f.trk [t].grp = true;
            _f.trk [t  ].snd = _f.trk [t-1].snd; // restore orig top trk snd
            _f.trk [t-1].snd = tr     [0  ].snd;
         }
         Up.eTrk--;
      }
   }
   else if (op == 'd') {
      if (t == tb) {                   // no group or top trk of grp
         if (te == _f.trk.Ln-1)  {Hey (CC("already at the bottom"));   return;}
         dtr = te+1;   ntr = 1;
         while (((ubyt4)(dtr+ntr) < _f.trk.Ln) && _f.trk [dtr+ntr].grp)  ntr++;
         MemCp (tr,                & _f.trk [dtr], sizeof (TrkRow) * ntr);
         MemCp (& _f.trk [tb+ntr], & _f.trk [tb],  sizeof (TrkRow) * (te-tb+1));
         MemCp (& _f.trk [tb],     tr,             sizeof (TrkRow) * ntr);
         Up.eTrk += ntr;
      }
      else {                           // w/in a group - scoot 1: t<=>t+1
         if (t  == _f.trk.Ln-1)  {Hey (CC("already at the bottom"));   return;}
         MemCp (tr,             & _f.trk [t+1], sizeof (TrkRow));
         MemCp (& _f.trk [t+1], & _f.trk [t],   sizeof (TrkRow));
         MemCp (& _f.trk [t],   tr,             sizeof (TrkRow));
         Up.eTrk++;
      }
   }
   ReDo ();
}


ubyte Song::GetSct (TxtRow *sct)       // put sections of _f.cue[] into sct[64]
{ ubyt4 i;
  ubyte ns = 0;
   MemSet (sct, 0, sizeof (TxtRow)*64);
   for (i = 0;  (ns < 64) && (i < _f.cue.Ln);  i++)
      if (_f.cue [i].s [0] == '(')
         MemCp (& sct [ns++], & _f.cue [i], sizeof (TxtRow));
TStr x;
for(i=0;i<ns;i++) TRC("sct[`d].s=`s .time=`d/`s",
i,sct[i].s,sct[i].time,TmSt(x,sct[i].time));
   return ns;
}


void Song::DrMap (char *dr)
// set .din of a drum
{ ubyte t = ChkETrk();
  TStr  s, s2;
TRC("DrMap trk=`d drm=`s", t, dr);
   MemCp (s, dr, 4);   s [4] = '\0';
   _f.trk [t].din = MDrm (dr);
   if (_f.trk [t].drm != _f.trk [t].din)
      StrFmt (_f.trk [t].name, "`s => `s",
         MDrm2Str (s,  _f.trk [t].din),
         MDrm2Str (s2, _f.trk [t].drm));
   else  MDrm2Str (_f.trk [t].name,
                       _f.trk [t].drm);
   ReDo ();
}


//______________________________________________________________________________
ubyt4 Song::NtDnNxt (ubyt4 tm)         // find next trk=? noteDn >= tm
{ ubyte t;
  ubyt4 p, ne, tIn = tm;
  bool  got = false;
  TrkEv *e;
   for (t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t)) {
      for (e = _f.trk [t].e, ne = _f.trk [t].ne, p = 0;  p < ne;  p++)
         if (ENTDN (& e [p]) && (e [p].time >= tIn))  break;
      if (p < ne)  {if ((! got) || (e [p].time < tm))  tm = e [p].time;
                    got = true;}
   }
   return got ? (tm ? (tm-1) : tm) : tIn;
}

ubyt4 Song::NtDnPrv (ubyt4 tm)         // find prev trk=? noteDn+beat <= tm
{ ubyte t;
  ubyt4 p, pEnd, ne, tIn = tm;
  bool  got = false;
  TrkEv *e;
   for (t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t)) {
      for (p = 0, pEnd = 0xFFFFFFFF, e = _f.trk [t].e, ne = _f.trk [t].ne;
           p < ne;  p++) {
         if (ENTDN (& e [p]) && (e [p].time <  tIn))  pEnd = p;
         if (ENTDN (& e [p]) && (e [p].time >= tIn))  break;
      }
      if (pEnd < ne)  {if ((! got) || (e [pEnd].time > tm))  tm = e [pEnd].time;
                       got = true;}
   }
   if (! got)  return tIn;
  TSgRow *ts = TSig (tm);
   return tm + M_WHOLE / ts->den;
}


#define MAXBAR  (800)

void Song::LoopInit ()
// find repeat ranges, init loop points, clear bug counts
{ ubyte t, nTr, tr [MAX_TRK];
  ubyt2 b, b2, bi;
  ubyt4 tMin, tMax, tX, p, l, ne, nEv,
        nPat, patBgn [MAXBAR], patEnd [MAXBAR], bar [MAXBAR], barf [MAXBAR];
  TrkEv  *e, *ev;
  TrkRow *trk;
  TStr    ts;
// get trk set from ui
   nTr = 0;   ne = 0;
   for (t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t))
      {tr [nTr++] = t;   ne += _f.trk [t].ne;}
   if (nTr == 0)        {Hey (CC("pick some tracks to practice"));   return;}
   if (_bEnd > MAXBAR)  {Hey (CC("too many bars (max=800)"));        return;}

// make the "empty" pattern;  init ev,nEv
   patBgn [0] = patEnd [0] = 0;   nPat = 1;
   nEv = 0;   ev = new TrkEv [ne];   MemSet (ev, 0, ne * sizeof (TrkEv));
                                     MemSet (barf, 0, sizeof (barf));
// pull out each bar's ntDn ev ctrl/inbar-times to compare (a pat per bar)
   for (b = 0;  b < _bEnd;  b++) {
      tMin = Bar2Tm (b+1);   tMax = Bar2Tm (b+2);
      patBgn [nPat] = nEv;             // start ev, patEnd is end ev+1
      for (t = 0;  t < nTr;  t++) {
         trk = & _f.trk [tr [t]];
         for (p = 0, l = trk->ne, e = trk->e;      // skip evs till >= tMin
                 (p < l) && (e [p].time < tMin);  p++)  ;
      // only ntOns within bar time - no CCs, relative tm, velo=0 for comparing
         for (;  (p < l) && (e [p].time < tMax);  p++)
            if (ENOTE (& e [p]) && EDOWN (& e [p]))
               {ev [nEv  ].ctrl = e [p].ctrl;
                ev [nEv++].time = e [p].time - tMin;}
      }
      patEnd [nPat] = nEv;

      if (patBgn [nPat] == patEnd [nPat])  bar [b] = 0;    // empty bar can't
      else {                                               // match anythin
         for (p = 1;  p < nPat;  p++)
         // got same bar pat?  (same ne, matching evs)
            if ( ((patEnd [p]    - patBgn [p]   ) ==
                  (patEnd [nPat] - patBgn [nPat])) &&
                 (! MemCm ((char *)& ev [patBgn [p]],
                           (char *)& ev [patBgn [nPat]],
                           sizeof (TrkEv) * (patEnd [p] - patBgn [p]), 'x')) )
               break;
         if (p < nPat) {
         // matched a pat!  junk this pat, bar[]=0, n set my barf
            bar [b] = 0;   nEv = patBgn [nPat];  // n find bar stated 1st
            for (b2 = 0;  b2 < b;  b2++)  if (bar [b2] == p)
                                             {barf [b] = b2+1;   break;}
         }
         else bar [b] = nPat++;        // else itsa new pat - ah well
      }
   }
   delete [] ev;
// OK!  pat[]s of ev[]s are all compared n can toss em
// bar  has pat#.  but 0 if empty or rep of a pat#
// barf has            0 if empty or bar+1 of pat#'s first bar

// kill existing [range or .repeat bar *
   for (p = 0;  p < _f.cue.Ln;) {
      if (! _f.cue [p].tend)  p++;
      else {
         StrCp (ts, _f.cue [p].s);
         if ( (*ts == '[') || (! MemCm (ts, CC(".repeat bar"), 11)) )
               _f.cue.Del (p);
         else  p++;
      }
   }

// put em in fresh - only skip if >= 3 empty/rep'd bars
   for (b = 0;  b+2 < _bEnd;) {        // first, skip ranges
      if (bar [b] || bar [b+1] || bar [b+2])  b++;
      else {                           // yay :)
         b2 = b+2;                     // can it be longer?
         while ((b2+1 < _bEnd) && (! bar [b2+1]))  b2++;
         for (bi = b;  bi <= b2;  bi++)
            if ( barf [bi] &&          // nonempty and 1st or non-in-seq-bar
                 ((bi == b) || (barf [bi] != barf [bi-1]+1)) ) {
               tMin = Bar2Tm (bi+1);   if (tMin) tMin--;
               StrFmt (ts, ".repeat bar `d", barf [bi]);
               TxtIns (tMin, ts, & _f.cue, 'c');
            }
         b = b2+1;
      }
   }
   for (b = 0;  b < _bEnd;) {          // then, loop ranges [ n /dur
      if (b == 0)  while ((bar [b] == 0) && (b < _bEnd))  b++;
      tMin = Bar2Tm (b+1);      if (b)             tMin -= (M_WHOLE/4);
      for (b2 = b;  b2 < b+3;  b2++)
         if ((b2+3 < _bEnd) && (! bar [b2+1]) &&
             (! bar [b2+2]) && (! bar [b2+3]))  break;
      tMax = Bar2Tm (b2+2)-1;   if (b2 < _bEnd-1)  tMax += (M_WHOLE/4);
      tMin = NtDnNxt (tMin);   tMax = NtDnPrv (tMax);
      TxtIns (tMin, StrFmt (ts, "/`d[", tMax-tMin), & _f.cue, 'c');
      b = b2+1;
      if ((b < _bEnd+2) && (! bar [b]) && (! bar [b+1]) && (! bar [b+2]))
         while ((b < _bEnd) && (! bar [b]))  b++;
   }

   _f.bug.Ln = 0;
   ReDo ();
}
