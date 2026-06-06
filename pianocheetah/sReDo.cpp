// sReDo.cpp - init alll the data structs  ...and junk

#include "song.h"


bool Song::TDrm (ubyte t)  {return (_f.trk [t].chn == 9) ? true:false;}
bool Song::TLrn (ubyte t)  {return  _f.trk [t].lrn;}
bool Song::TRec (ubyte t)  {return  _f.trk [t].rec;}
bool Song::TSho (ubyte t)  {return (TLrn (t) || ((! SHRCRD) &&
                                                 (_f.trk [t].ht == 'S')))
                                   ? true:false;}
void Song::ReTrk ()
// give gui what it needs in Up.trk
{ ubyte r, nn, nc;
  ubyt2 ct;
  ubyt4 i;
  TStr  s, g, s2;
  BStr  tp;
  char *sl, *c;
  TrkEv *e;
   Up.trk.Ln = 0;
   for (r = 0;  r < _f.trk.Ln;  r++) {
      Up.trk.Ins (r);
      Up.trk [r].dvt = Up.dev [_f.trk [r].dev].dvt;
      Up.trk [r].drm = TDrm (r);
      *s = '\0';
      if      (_f.trk [r].shh)  StrCp (s, "mute");
      else if (_f.trk [r].lrn)  StrCp (s, "lrn");
      else if (_f.trk [r].rec)  StrCp (s, "rec");
      StrCp (Up.trk [r].lrn, s);
      *s = _f.trk [r].ht;   s [1] = '\0';
      if (_f.trk [r].lrn)
         {if (! ((*s >= '1') && (*s <= '7')))  *s = '\0';}
      else
         if (*s != 'S')  *s = '\0';
      StrCp (Up.trk [r].ht, s);
//DBG("tr=`d lrn='`s' ht='`s'", r, Up.trk [r].lrn, Up.trk [r].ht);
      StrCp (Up.trk [r].name, _f.trk [r].name);
      StrCp (s, SndName (r));
      *g = '\0';
      if (*s) {
         StrCp (g, s);
         sl = StrCh (g, '_');          // _ usta be a slash :/
         if (sl != nullptr) {
            *sl = '\0';
            StrCp (s, & s [StrLn (g)+1]);
         }
         else  *g = *s = '\0';         // non syn drums
      }
      StrCp (Up.trk [r].grp, g);       // SndGrp
      StrCp (Up.trk [r].snd, s);       // SndName
      if (_f.trk [r].grp)  StrCp  (s, CC("+"));
      else                 StrFmt (s, "`s.`d", DevName (r), _f.trk [r].chn+1);
      StrCp (Up.trk [r].dev, s);       // + / dev.chn

      StrFmt (tp, "#`d  ", r+1);

      if (_f.trk [r].nb)
           StrFmt (s, "notes=(`d)`d  ", _f.trk [r].nb, _f.trk [r].nn);
      else StrFmt (s, "notes=`d  ",                    _f.trk [r].nn);
      StrAp (tp, s);

      StrFmt (s, "ctrls=`d\n", _f.trk [r].ne - _f.trk [r].nb -
                              (_f.trk [r].nn - _f.trk [r].nb)*2);
      StrAp (tp, s);

      nn = nc = 0;
      for (i = 0, e = _f.trk [r].e;  ((nc<3)||(nn<3)) &&
                                     (i < _f.trk [r].ne);  i++) {
         StrFmt (s, "`<9s ", TmSt (s2, e [i].time));
         ct = e [i].ctrl;
         if (ct & 0x0080) {            // ctrl
            if (nc == 3)  continue;

            nc++;   StrAp (tp, s);
            StrCp (s2, CtlSt (ct));
            if      (! StrCm (s2,  CC("Tmpo")))   // tmpo,tsig,ksig are special
               StrFmt (s, CC("Tmpo=`d"),     e [i].valu | (e [i].val2<<8));
            else if (! StrCm (s2,  CC("TSig"))) {
               StrFmt (s, CC("TSig=`d/`d"),  e [i].valu,
                                       1 << (e [i].val2 & 0x0F));
               if (e [i].val2 >> 4)  StrFmt (s, "/`d",
                                         1 + (e [i].val2 >> 4));
            }
            else if (! StrCm (s2, CC("KSig"))) {
               StrAp (s, CC("KSig="));
               if   (! (e [i].val2 & 0x80)) StrAp (s, MKeyStr  [e [i].valu]);
               else if (e [i].valu != 11)   StrAp (s, MKeyStrB [e [i].valu]);
               else                         StrAp (s, CC("Cb"));     // weird :/
               if (e [i].val2 & 0x01)  StrAp (s, CC("m"));
               *s = CHUP (*s);
            }
            else {
               StrFmt (s, "`s=`d",            s2, e [i].valu);
               if (e [i].val2)  StrFmt (s, " `d", e [i].val2);
            }
            StrAp (s, CC("\n"));
            StrAp (tp, s);
         }
         else {                        // note
            if (nn == 3)  continue;

            nn++;   StrAp (tp, s);
            StrFmt (s, "`s`c`d\n",
               TDrm (r) ? MDrm2Str (s2, ct) : MKey2Str (s2, ct),
               EUP (& e [i]) ? '^' : (EDN (& e [i]) ? '_' : '~'),
               e [i].valu & 0x7F);
            StrAp (tp, s);
         }
      }
      tp [StrLn (tp)-1] = '\0';        // chop last \n
      StrCp (Up.trk [r].tip, tp);
   }
TRC("ReTrk eTrk=`d ln=`d", Up.eTrk, r);
   emit sgUpd ("trk");
}


//______________________________________________________________________________
static int SigCmp (void *p1, void *p2)  // ..._sig sortin (by .time)
{ ubyt4 t1 = *((ubyt4 *)p1), t2 = *((ubyt4 *)p2);
   return t1 - t2;
}

ubyt4 Song::ReEv (bool tpo)
// redo _f.ctl, _f.tpo,_f.tSg,_f.kSg, _cch given mod'd _f.trk[].e[]
{ ubyte t, d, c, tc;
  ubyt4 e, p, mint = 9999*M_WHOLE, maxt = 0, bard, tm;
  TStr  s;
  TrkEv *ev;
TRC("ReEv  rebuild _f.ctl, _f.tpo,_f.tSg,_f.kSg, _cch");
   CtlClean ();
   _cch.Ln = _f.tSg.Ln = _f.kSg.Ln = 0;
   if (tpo)  _f.tpo.Ln = 0;
   for (t = 0;  t < _f.trk.Ln;  t++) {
      for (d = _f.trk [t].dev, c = _f.trk [t].chn, ev = _f.trk [t].e,
           e = 0;  e < _f.trk [t].ne;  e++) {
         if ((tc = ev [e].ctrl) & 0x80) {
            StrCp (s, CtlSt (tc));
            if (tpo && (! StrCm (s, CC("Tmpo"))))  if (! _f.tpo.Full ()) {
               p = _f.tpo.Ins ();
               _f.tpo [p].time = ev [e].time;
               _f.tpo [p].val  = ev [e].valu | (ev [e].val2 << 8);
            }
            if (! StrCm (s, CC("TSig")))  if (! _f.tSg.Full ()) {
               p = _f.tSg.Ins ();
               _f.tSg [p].time = ev [e].time;
               _f.tSg [p].num  = ev [e].valu;
               _f.tSg [p].den  = 1 << (ev [e].val2 & 0x0F);
               _f.tSg [p].sub  = 1 +  (ev [e].val2 >> 4);
            }
            if (! StrCm (s, CC("KSig")))  if (! _f.kSg.Full ()) {
               p = _f.kSg.Ins ();
               _f.kSg [p].time = ev [e].time;
               _f.kSg [p].key  = ev [e].valu;
               _f.kSg [p].flt  = ev [e].val2 & 0x80;
               _f.kSg [p].min  = ev [e].val2 & 0x01;
            }
         // _cch just tracks unique dev,chn,ctl we have in song
         // TmHop uses it to chase control values to a time we're at
            for (p = 0;  p < _cch.Ln;  p++)
               if ((_cch [p].dev == d) && (_cch [p].chn == c) &&
                                          (_cch [p].ctl == tc))  break;
            if ((p >= _cch.Ln) && (! _cch.Full ())) {
               p = _cch.Ins ();
               _cch [p].dev = d;   _cch [p].chn = c;   _cch [p].ctl = tc;
            }
         }
         else {
            if (ev [e].time <  mint)   mint = ev [e].time;
            if (ev [e].time > _tEnd)  _tEnd = ev [e].time + 1;
         }
      }                                // mint, _tEnd are time range of NOTEs
   // calc maxt
      if (_f.trk [t].ne && (_f.trk [t].e [_f.trk [t].ne-1].time > maxt))
         maxt =             _f.trk [t].e [_f.trk [t].ne-1].time;
   }                                   // maxt of ALL evs (last bar to SHOW)
TRC(" sort tpo,tSig,kSig by time");
   if (tpo)  Sort (_f.tpo.Ptr (), _f.tpo.Ln, _f.tpo.Siz (), SigCmp);
   Sort (_f.tSg.Ptr (), _f.tSg.Ln, _f.tSg.Siz (), SigCmp);
   Sort (_f.kSg.Ptr (), _f.kSg.Ln, _f.kSg.Siz (), SigCmp);
TRC(" tpo.Ln=`d", _f.tpo.Ln);
   if (tpo && (! _f.tpo.Ln))           // every song needs tempo to keep edits
      for (t = 0;  t < _f.trk.Ln;  t++)  if (TDrm (t)) {
TRC("    empty so ins a 120");
         _f.tpo.Ins (0);
         _f.tpo [0].time = 0;
         _f.tpo [0].val  = 120;
         EvIns (t, 0);
         _f.trk [t].e [0].time = 0;
         _f.trk [t].e [0].ctrl = CtlEv (CC("tmpo"));
         _f.trk [t].e [0].valu = 120;
         _f.trk [t].e [0].val2 = 0;
         _f.trk [t].e [0].x    = 0;
         break;
      }
// make sure TSig times are on bar boundaries (for safety)
   tm = 0;   bard = M_WHOLE;
   for (p = 0;  p < _f.tSg.Ln;  p++) { // trunc to bar dur
      tm = _f.tSg [p].time = ((_f.tSg [p].time - tm) / bard * bard) + tm;
      bard = M_WHOLE * _f.tSg [p].num / _f.tSg [p].den;
   }

// set _f.tSg[].bar,
   if (_f.tSg.Ln)
      _f.tSg [0].bar = (ubyt2)(1 + _f.tSg [0].time / M_WHOLE);     // 4/4
   for (p = 1;  p < _f.tSg.Ln;  p++)  _f.tSg [p].bar =
      (ubyt2)(_f.tSg [p-1].bar +
      (_f.tSg [p].time - _f.tSg [p-1].time) /
                               (M_WHOLE / _f.tSg [p-1].den * _f.tSg [p-1].num));
// bump maxt w _f.chd,etc;  align maxt up to next bar boundary
   if (_f.lyr.Ln && ((tm = _f.lyr [_f.lyr.Ln-1].time) > maxt))  maxt = tm;
   if (_f.chd.Ln && ((tm = _f.chd [_f.chd.Ln-1].time) > maxt))  maxt = tm;
   if (_f.cue.Ln && ((tm = _f.cue [_f.cue.Ln-1].time) > maxt))  maxt = tm;
   if (_f.cue.Ln && ((tm = _f.cue [_f.cue.Ln-1].tend) > maxt))  maxt = tm;
   _bEnd = Tm2Bar (maxt);
   StrFmt (Up.bars, "`d", _bEnd);   emit sgUpd ("bars");

// now make sure KSig times are on bar boundaries (for safety)
   for (p = 0;  p < _f.kSg.Ln;  p++)
      _f.kSg [p].time = Bar2Tm (Tm2Bar (_f.kSg [p].time));
TRC("ReEv end mint=`d", mint);
   return mint;
}


//______________________________________________________________________________
static int DrCmp (void *p1, void *p2)
{ ubyte n1 = *((ubyte *)p1), n2 = *((ubyte *)p2);
  int   c1, c2;
   for (c1 = 0;  c1 < NMDrum;  c1++)  if (n1 == MKey (MDrum [c1].key))  break;
   for (c2 = 0;  c2 < NMDrum;  c2++)  if (n2 == MKey (MDrum [c2].key))  break;
   if ((c1 < NMDrum) && (c2 < NMDrum))  return c1 - c2;
   return n1 - n2;
}

void Song::SetDn (char qu)             // qu from DlgCfg quantize button ONLY
// calc notesets by time - all ntDns across lrn tracks => _dn[]
{ ubyte t, c, d, nn, x, pf, nt, nmin, nmax, pnt, f;
  ubyt2                         nsum, ntrm, pntrm;
  ubyt4 p, q, tpos [128], tm, ptm, dp, w;
  bool  got, qd [MAX_TRK], didq, fst;
  char  oc, ht, ch;
  TrkEv *e;
  ubyt4 ne;
  TStr  k, st;
  struct {ubyte pos;   char dir, key;} xx [64*1024];  // more _dn info
  struct {ubyte t;   ubyt4 p;} on [2][128];
TRC("SetDn qu=`c", qu);
   _pDn = 0;   _dn.Ln = 0;   didq = false;
   MemSet (tpos, 0, sizeof (tpos));
   MemSet (qd,   0, sizeof (qd));
   for (got = true;  got;) {
      got = false;
   // get tm - min time of a NtDn across all ? trks
      for (t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t))
         for (p = tpos [t], e = _f.trk [t].e, ne = _f.trk [t].ne;  p < ne;  p++)
            if (ENTDN (& e [p])) {
               if      (! got)           {tm = e [p].time;   got = true;}
               else if (e [p].time < tm)  tm = e [p].time;
               break;
            }
      if (got) {
      // build on[] noteset from noteDn evs @ 1st time
         MemSet (on, 0, sizeof (on));  // start fresh - notes all off
         for (t = 0;  t < _f.trk.Ln;  t++)  if (TLrn (t)) {
            if (qu == 'q') {           // ARE mini quantizing to 64th note
               for (p = tpos [t], e = _f.trk [t].e, ne = _f.trk [t].ne;
                    (p < ne) && (e [p].time < tm + 7);  p++)    // bouta 128th
                  if (ENTDN (& e [p])) {         // got a ntdn w time within
                     ptm = tm;                   // 12 ticks of this dn time?
                     if (e [p].time != tm) {     // if not matchin we DO qu
                        didq = true;   qd [t] = true;
                        ptm = e [p].time;        // signal we're CHANGIN it
TStr s1, s2, s3;
DBG(" SetDn quant ntDn trk=`d p=`d/`d nt=`s tmFr=`s tmTo=`s",
t, p, ne, MKey2Str (s3, e [p].ctrl), TmSt(s1,e [p].time), TmSt(s2,tm));
                     }
                     e [p].time = tm;   d = TDrm (t) ? 1 : 0;
                     on [d][e [p].ctrl].t = t+1;
                     on [d][e [p].ctrl].p = p;

                  // need to scoot a possible NtUp earlier cuzu me?
                     if (ptm != tm)  for (q = p;  q;  q--)
                        if ((e [q-1].ctrl == e [p].ctrl) && ENTUP (& e [q-1]) &&
                            (e [q-1].time >= tm) && (e [q-1].time <= ptm)) {
TStr s1, s2, s3;
DBG(" SetDn quant ntUp trk=`d p=`d/`d nt=`s tmFr=`s tmTo=`s",
t, q-1, ne, MKey2Str (s3, e [q-1].ctrl), TmSt(s1,e [q-1].time),
                                         TmSt(s2,tm?(tm-1):0));
                           e [q-1].time = tm ? tm-1 : 0;
                           break;
                        }
                  }
            }
            else {                     // NO quantizing
               for (p = tpos [t], e = _f.trk [t].e, ne = _f.trk [t].ne;
                    (p < ne) && (e [p].time <= tm);  p++)
                  if (ENTDN (& e [p])) {
                     d = TDrm (t) ? 1 : 0;
                     on [d][e [p].ctrl].t = t+1;
                     on [d][e [p].ctrl].p = p;
                  }
            }
            tpos [t] = p;
         }

      // stamp a Dn w all on melo,drum ntDns (drum ctrl => trk's din)
         if (_dn.Full ())  DBG("SetDn  outa room in Dn");
         dp = _dn.Ln;
         _dn [dp].time = tm;   _dn [dp].msec = 0;   _dn [dp].tmpo = 0;
                                                    _dn [dp].clip = '\0';
         MemSet (_dn [dp].velo, 0, sizeof (_dn[0].velo));
         for (nn = d = 0;  d < 2;  d++)  for (c = 0;  c < 128;  c++)
            if (on [d][c].t && (nn < BITS (_dn [0].nt))) {
               _dn [dp].nt [nn].nt = c;
               _dn [dp].nt [nn].t  = on [d][c].t - 1;
               _dn [dp].nt [nn].p  = on [d][c].p;
               nn++;
            }
         _dn [dp].nNt = nn;   _dn.Ln++;
      }
   }
// OK!  we have all real notes by dnTime/trk/nt
// now loop thru each oct/ht we got and make ONE ez note per dn per oct/ht
// drum oct/ht is 0, oct1=>1 .. 7=>7

// drum dns first.  melo nts later
   for (dp = 0;  dp < _dn.Ln;  dp++) {
      c = d = 128;   nn = 0;
      for (x = 0;  x < _dn [dp].nNt;  x++)
         if (TDrm (_dn [dp].nt [x].t)) {
            nt =   _dn [dp].nt [x].nt;
            if (c == 128)  d = c = x;  // our first keeper n dest pos
            else if (DrCmp (& nt, & _dn [dp].nt [c].nt) < 0)
                               c = x;  // the one to keep
            nn++;
         }
      if (nn > 1) {                    // some ta kill?
         _dn [dp].nt [d].p  = 0;       // NO P FO EZ !
         _dn [dp].nt [d].t  = _dn [dp].nt [c].t;
         _dn [dp].nt [d].nt = _dn [dp].nt [c].nt;

      // toss any other drum notes in the dn
         for (x = d+1;  x < _dn [dp].nNt;  x++)  if (TDrm (_dn [dp].nt [x].t)) {
            MemCp (& _dn [dp].nt [x], & _dn [dp].nt [x+1],
                   sizeof (_dn [0].nt [0]) * (_dn [dp].nNt - x - 1));
            _dn [dp].nNt--;
            x--;                       // so it stays the same across loop inc
         }
      }
   }

// melodic notes - max o one note o five per dn time for oc's tr's nts
   for (oc = '1';  oc <= '7';  oc++) {      // gather tracks w my ht
      k [0] = oc;   k [2] = '\0';      // oct of key
//DBG("oc=`c", k[0]);
      ht = (*k < '4') ? 'L' : 'R';

   // first we calc all the directions usin' nmin/nmax in dn n prev dn
      fst = true;   pnt = 128;
      for (dp = 0;  dp < _dn.Ln;  dp++) {
         c = 128;   nsum = nn = nmin = nmax = 0;      // stats for this dn
         for (x = 0;  x < _dn [dp].nNt;  x++)
            if (_f.trk [_dn [dp].nt [x].t].ht == oc) {
               nt =     _dn [dp].nt [x].nt;
               if (nn == 0)  nsum = nmin = nmax = nt;
               else         {nsum += nt;   if (nt < nmin) nmin = nt;
                                           if (nt > nmax) nmax = nt;}
               nn++;   if (c == 128)  c = x;
            }
         if (! nn)  xx [dp].pos = 99;  // if no notes in dn for my trks
         else {
            xx [dp].pos = c;           // first nt pos for my trks
            nt = nsum / nn;   ntrm = (ubyt2)(32768 * (nsum % nn) / nn);
            if      (nt   > pnt)    ch = '>';
            else if (nt   < pnt)    ch = '<';
            else if (ntrm > pntrm)  ch = '>';
            else if (ntrm < pntrm)  ch = '<';
            else                    ch = '=';
            xx [dp].dir = fst ? '=' : ch;
            fst = false;
//TStr xd;
//DBG(" `s dp=`d nn=`d nsum=`d nt=`d ntrm=`d pnt=`d pntrm=`d ch=`c",
//TmS(xd,_dn [dp].time), dp, nn, nsum, nt, ntrm, pnt, pntrm, xx [dp].dir);
            _dn [dp].nt [c].p  = 0;    // NO P FO EZ !
            _dn [dp].nt [c].nt = ((ht == 'L') ? nmin : nmax);
            pnt = nt;   pntrm = ntrm;

         // toss any notes of my trks beyond c
            for (x = c+1;  x < _dn [dp].nNt;  x++)
               if (_f.trk [_dn [dp].nt [x].t].ht == oc) {
                  MemCp (& _dn [dp].nt [x], & _dn [dp].nt [x+1],
                         sizeof (_dn [0].nt [0]) * (_dn [dp].nNt - x - 1));
                  _dn [dp].nNt--;
                  x--;                 // undo loop inc
               }
         }
      }

   // now collect manually picked notes in _f.cue[].s for my oct's tracks
      StrFmt (st, ".ezpos `c", k [0]);
      for (p = 0;  p < _f.cue.Ln;  p++)
                                     if (! MemCm (_f.cue [p].s, st, StrLn (st)))
         for (dp = 0;  dp < _dn.Ln;  dp++)
                                         if (_dn [dp].time == _f.cue [p].time) {
            xx [dp].dir = '!';
            xx [dp].key = _f.cue [p].s [8];
//DBG(" xx[`d].key=`c s=`s (.pos=`d)",
//dp, xx [dp].key, _f.cue [p].s, xx [dp].pos);
         }
      pf = 0;
      for (dp = 0;  dp < _dn.Ln;  dp++)  if (xx [dp].pos != 99) {
//DBG(" dp=`d", dp);

      // check for rolled chord/trill (fast stuff - time diff of < 15 ticks)
      // find max nt within trill notes and align to pinky=4 for rh/0 for lh
      // then follow <>= back to start of trill, then forward to end of it
        ulong pmax, pend;              // max dn pos, end of trill pos
         tm = _dn [dp].time;           // max is actually min for LH
         pmax = dp;   nmax = _dn [dp].nt [xx [dp].pos].nt;
         c = 0;                        // count #downs in trill
         for (p = dp+1;  p < _dn.Ln;  p++)  if (xx [p].pos != 99) {
            if (_dn [p].time >= tm+15)  break;

            c++;   pend = p;   tm = _dn [p].time;
            if (ht == 'L') {if (      _dn [       p].nt [xx [p].pos].nt < nmax)
                               nmax = _dn [pmax = p].nt [xx [p].pos].nt;}
            else            if (      _dn [       p].nt [xx [p].pos].nt > nmax)
                               nmax = _dn [pmax = p].nt [xx [p].pos].nt;
         }
         if (c) {                      // doin rolled chord/trill
           char pdir;                  // usin prev dir n goin in reverse
            pf = (ht == 'L') ? 0 : 4;
            k [1] = 'c' + pf;
            _dn [pmax].nt [xx [pmax].pos].nt = MKey (k);
            pdir =         xx [pmax].dir;
         // goin down if any
            for (p = pmax;    c && (p > dp);)          if (xx [p].pos != 99) {
               c--;  p--;              // decr loops are annoyin :/
               if      (pdir == '>')  {if (pf-- == 0)  pf = 4;}
               else if (pdir == '<')  {if (pf++ == 4)  pf = 0;}
               k [1] = 'c' + pf;
               _dn [p].nt [xx [p].pos].nt = MKey (k);
               pdir =      xx [p].dir;
            }
         // and now regular goin up
            pf = (ht == 'L') ? 0 : 4;
            for (p = pmax+1;  c && (p <= pend);  p++)  if (xx [p].pos != 99) {
               c--;
               if      (xx [p].dir == '<')  {if (pf-- == 0)  pf = 4;}
               else if (xx [p].dir == '>')  {if (pf++ == 4)  pf = 0;}
               k [1] = 'c' + pf;
               _dn [p].nt [xx [p].pos].nt = MKey (k);
            }
            dp = pend;   pf = 0;
         }
         else {                        // doin regular next dn
            if      (xx [dp].dir == '!')  pf = xx [dp].key - 'c';
            else if (xx [dp].dir == '<')  {if (pf-- == 0)  pf = 4;}
            else if (xx [dp].dir == '>')  {if (pf++ == 4)  pf = 0;}
            k [1] = 'c' + pf;
            _dn [dp].nt [xx [dp].pos].nt = MKey (k);
         }
      }
   }
// always need dn[pdn].time >= _now so add a dummy at time=0 if none yet
   if ( (! _dn.Ln) || _dn [0].time )
      {_dn.Ins (0);   _dn [0].time = 0;   _dn [0].nNt = 0;}

// re-sort (ta be safe) if quant happened
   if (didq) {
      for (t = 0;  t < _f.trk.Ln;  t++)  if (qd [t])
         Sort (_f.trk [t].e, _f.trk [t].ne, sizeof (TrkEv), EvCmp);
//    for (ubyte tx = 0;  tx < _f.trk.Ln;  tx++)  if (TLrn (tx))  DumpTrEv (tx);
   }
TRC("SetDn end");
}


//______________________________________________________________________________
static int LpCmp (void *p1, void *p2)
// by #times desc,time
{ sbyt4 *i1 = (sbyt4 *)p1, *i2 = (sbyt4 *)p2, d;
// if ((d = i2 [2] - i1 [2]))  return d;    // by .t desc (#times of bugs)
   return   i1 [0] - i2 [0];                //    .tm asc (time)
}

void Song::SetLp (char dir)
// i=init  .=refresh(but no TmHop)  <,>=hop  b=hop to most bugs (f=focus oldish)
// [cues, _dn, _bug=> calcd loops then set curr lpBgn,lpEnd n adj tempo %
// make sure _dn is SET !
{ ubyt4 nl, p, x, xt, l, tMn, tMx, in;
  bool  usex = true;                   // set _lrn.lp* to lp[x] ?
  TStr  ts, t2;
  struct {ubyt4 tm, te, d, nd;} lp [4096];
// tm-te is time range;  d = #distinct bug times;  nd = total times (from _dn)

TRC("SetLp dir=`c _now=`s   loops:", dir, TmSt (ts, _now));
// gather cue [s to remake lp[];  pick start time of loop we're in based on _now
   nl = xt = 0;
   for (p = 0;  p < _f.cue.Ln;  p++)
      if (_f.cue [p].tend && (_f.cue [p].s [0] == '[')) {
      // set lp[nl].tm - .te;  .nd = #_dn[]s;  .d set later
         lp [nl].tm = _f.cue [p].time;   lp [nl].te = _f.cue [p].tend;
                                         lp [nl].d = lp [nl].nd = 0;
         for (x = 0;  x < _dn.Ln;  x++)  if ((_dn [x].time >= lp [nl].tm) &&
                                             (_dn [x].time <  lp [nl].te))
            lp [nl].nd++;
//DBG("   `d `s-`s mx=`d",
//nl, TmSt(ts, lp [nl].tm), TmSt(t2, lp [nl].te), lp [nl].nd);
      // usually want 2nd loop of an overlap cuz we're usually sittin at lpBgn
      // so 2nd time thru will set xt again
         if ((_now >= lp [nl].tm) && (_now <= lp [nl].te))  xt = lp [nl].tm;
         nl++;
      }

// cleanup any junky bugs outside CURRENT loops
   for (p = 0;  p < _f.bug.Ln;) {
      for (in = l = 0;  l < nl;  l++)  if ((_f.bug [p].time >= lp [l].tm) &&
                                           (_f.bug [p].time <= lp [l].te))
         {in = 1;   break;}
      if (in) p++;   else _f.bug.Del (p);
   }
TRC(" _now's lp.tm=`s nLp=`d   bugs:", xt?TmSt(ts, xt):"outside", nl);

// count up distinct times n bug totals
   for (p = 0;  p < _f.bug.Ln;  p++) {
//DBG("   `d tm=`s n=`s", p, TmSt (ts, _f.bug [p].time), _f.bug [p].s);
      for (l = 0;  l < nl;  l++)  if ((_f.bug [p].time >= lp [l].tm) &&
                                      (_f.bug [p].time <= lp [l].te)) {
      // lp[].d = #distinct times of bugs
         lp [l].d++;
//DBG("   lp=`d now #times=`d #bugs=`d", l, lp [l].d);
      }
   }
   Sort (lp, nl, sizeof (lp[0]), LpCmp);

// nlB = nLp with any bugs
// nlB = nl;   while (nlB && lp [nlB-1].d == 0)  nlB--;
TRC(" lp[]:");
for (l = 0;  l < nl;  l++)  TRC("   `d tb=`s te=`s #times=`d/`d",
l, TmSt(ts,lp [l].tm), TmSt(t2,lp [l].te), lp [l].d, lp [l].nd);

   if      (dir == 'i')  x = 0;        // init loop to 1st one
   else if (dir == 'b')                // set x to loop w most bug times
        {for (x = l = 0;  l < nl;  l++)  if (lp [l].d > lp [x].d)  x = l;}
   else {                              // refind loop havin xt for lpBgn
      for (x = 0;  x < nl;  x++)  if (lp [x].tm == xt)  break;
      if (x >= nl)                     // outside all lps?  use worst one
         {x = 0;   if (dir == 'f')  dir = '.';}  // aaand focus/un => .
TRC(" cur lp#=`d/`d", x, nl);
      if      (dir == '<')  {if (x)         x--;}
      else if (dir == '>')  {if (x+1 < nl)  x++;}
TRC(" post<> lp#=`d", x);

   // focus/un?  plow thru bug times again repickin lpBgn,lpEnd
      else if (dir == 'f') {
TRC("focus");
         tMn = M_WHOLE*9999;   tMx = 0;
         for (p = 0;  p < _f.bug.Ln;  p++) {
            xt = _f.bug [p].time;
            if ((xt >= lp [x].tm) && (xt <= lp [x].te))
               {if (xt < tMn)  tMn = xt;   if (xt > tMx)  tMx = xt;}
         }
         tMn = NtDnNxt ((tMn < M_WHOLE/4) ? 0 : (tMn - M_WHOLE/4));
         tMx = NtDnPrv (                         tMx + M_WHOLE/4 );
         if ((_lrn.lpBgn == lp [x].tm) && (_lrn.lpEnd == lp [x].te)) {
            _lrn.lpBgn = tMn;   _lrn.lpEnd = tMx;     // FOCUS !!
            usex = false;                             // already did
         }                                            // else restore
      }
   }
   if (usex) {                         // set em unless we already did
      if (nl)  {_lrn.lpBgn = lp [x].tm;   _lrn.lpEnd = lp [x].te;}
      else     {_lrn.lpBgn = 0;           _lrn.lpEnd = _tEnd;}
   }
   if (dir != '.')  TmHop (_lrn.lpBgn);
// if (x < nlB) {                      // in a loop w bugs?
//    Hey (StrFmt (ts, "in loop #`d of `d  (bugs=`d)", x+1, nlB, lp [x].d));
//    if (PRAC) {
//       if (lp [x].nd == 0)
//DBG("SetLp() BUG!  _dn not set so div by 0 :(");
//       x = (lp [x].d * 100) / lp [x].nd;
//       if (x < 25) p = 100;   else if (x < 50) p = 80;   else p = 60;
//       _f.tmpo = FIX1 * p / 100;
//       p = TmpoAt (_timer->Get ());
//TRC(" ok%=`d so _f.tmpo=`d TmpoAct=`d", 100-x, _f.tmpo, p);
//       _lrn.lpRvw = (x < 50);
//       PutTp ((ubyt2)p);
//       DscSave ();
//       ReTrk ();
//    }
// }
   _f.ctl [CtlEv (CC("tmpo")) & 0x7F].sho = PRAC?'m':'n';
TRC("SetLp end - bgn=`s end=`s", TmSt(ts,_lrn.lpBgn), TmSt(t2,_lrn.lpEnd));
}


void Song::SetNt ()                    // turn trk[].e into trk[].n
{ bool  DN;                            // n build .nb, .nn too
  ubyte t, nt;
  ubyt4 ne, nn, n1, p, q, r, n, ov [128][2];
  TStr  s1, s2;
  TrkEv *e;
TRC("SetNt");
   if (_nt)  delete [] _nt;
   ne = 0;

// will never use a full ne, but whatev
   for (t = 0;  t < _f.trk.Ln;  t++)
      {_f.trk [t].nn = _f.trk [t].nb = 0;   ne += _f.trk [t].ne;}
   _nt = new TrkNt [ne];   MemSet (_nt, 0, sizeof (TrkNt) * ne);

   for (nn = 0, t = 0;  t < _f.trk.Ln;  t++) {
      _f.trk [t].n = & _nt [n1 = nn];
//DBG("trk=`d nn=`d", t, nn);
      for (nt = 0;  nt < 128;  nt++)  ov [nt][0] = ov [nt][1] = NONE;

   // find evs for each note;  one layer overlap allowed
      for (e = _f.trk [t].e, ne = _f.trk [t].ne, p = 0;  p < ne;  p++) {
         nt = e [p].ctrl;
         if (ENOTE (& e [p])) {        // note?
            if      (EUP (& e [p])) {  // ntUp - is it still unpaired?
               for (n = nn;  n > n1;  n--)  if (_nt [n-1].up == p)  break;
               if (n <= n1) {                    // yep, save it as broke
DBG(" no ntDn for ntUp at `s `s trk=`d",
TmSt (s1, e [p].time), MKey2Str (s2, e [p].ctrl), t);
                  _nt [nn].dn = NONE;   _nt [nn].up = p;
                  _nt [nn].tm = (e [p].time > (M_WHOLE/32-1)) ?
                                (e [p].time - (M_WHOLE/32-1)) : 0;
                  _nt [nn].te =  e [p].time;
                  _nt [nn].nt =  e [p].ctrl;
                  if ((_nt [nn].dn == NONE) || (_nt [nn].up == NONE))
                          _f.trk [t].nb++;
                  nn++;   _f.trk [t].nn++;
               }
            }
            else if (EDN (& e [p])) {  // ntDn - hunt for ntUp
               for (q = p+1;  q < ne;  q++)  if (e [q].ctrl == nt) {
                  if      (EUP (& e [q])) { // regular ntUp
                     _nt [nn].dn = p;   _nt [nn].up = q;   // n see if overlap
                     if ((ov [nt][0] != NONE) &&
                         (e [p].time >= e [ov [nt][0]].time) &&
                         (e [q].time <= e [ov [nt][1]].time))
                        _nt [nn].ov = true;
                  }                    // ntDn aGAIN - scan fer overlap or broke
                  else if (EDN (& e [q])) {
                     for (DN = true, r = q+1;  r < ne;  r++)
                        if (e [r].ctrl == nt) {
                           if      (EUP (& e [r]))    // up
                              {if (  DN) DN = false;   else            break; }
                                                           // yay  :)  2nd up
                           else if (EDN (& e [r]))    // dn
                              {if (! DN) DN = true;    else {r = ne;   break;}}
                        }                                  // rats :(  3rd down
                     if (r < ne) {_nt [nn].dn = ov [nt][0] = p;
                                  _nt [nn].up = ov [nt][1] = r;}
                     else        {_nt [nn].dn = p;   _nt [nn].up = NONE;
DBG(" no ntUp for ntDn at `s `s trk=`d",
TmSt (s1, e [p].time), MKey2Str (s2, e [p].ctrl), t);
                                 }
                  }
                  break;     // done on findin' very next ev on this nt
               }
               if (q >= ne)  {_nt [nn].dn = p;   _nt [nn].up = NONE;
DBG(" no ntUp for ntDn at `s `s trk=`d",
TmSt (s1, e [p].time), MKey2Str (s2, e [p].ctrl), t);
                             }
               _nt [nn].tm = e [p].time;   _nt [nn].nt = e [p].ctrl;
               _nt [nn].te = (_nt [nn].up != NONE) ? e [_nt [nn].up].time :
                                       (M_WHOLE/32-1+e [_nt [nn].dn].time);
               if ((_nt [nn].dn == NONE) || (_nt [nn].up == NONE))
                       _f.trk [t].nb++;
               nn++;   _f.trk [t].nn++;
            }
         }
      }
   }
TRC("SetNt end");
}


void Song::BarH (ubyt2 *h, ubyte *sb, ubyt2 bar)
// return h n subbeat of bar - by goin thru all shown tr's .n[]
// ntdn times at small divs force those.  else less divs (subbeats)
// can use .n[] even for ez mode cuz same ntdn TIMES
{ bool  mt, got [9];
  ubyt4 md, tb, te, nn, p, btdur, sbdur, t1, st [9];
  ubyte sub, t, s;
  TSgRow *ts;
  TrkNt  *n;
   tb = Bar2Tm (bar);   te = Bar2Tm (bar+1);
   ts = TSig (tb);
//DBG("BarH `d  `d/`d.`d", bar, ts->num, ts->den, ts->sub);
   btdur = (M_WHOLE * ts->num / ts->den) / ts->num;
   sbdur = btdur / ts->sub;
// if tsig HAS subbt, try to reduce from .4=>.2=>.1 etc  (else can't even try)
   if (ts->sub > 1) {
      MemSet (got, 0, sizeof (got));   // default to no notes on subbts
   // subbt time - st - time within BEAT dur of subbt boundaries (halfway thru)
      for (t = 0;  t < ts->sub;  t++)  st [t] = (t * sbdur) + sbdur/2;
//DBG("   btdur=`d sbdur=`d got[]=F, st:", btdur, sbdur);
//for(t=0;t<ts->sub;t++)DBG("      `d `d", t, st[t]);
   }

// empty (or notes only on bar line)?
// mindur of fully contained notes (of shown tracks)
// if doin subbt n NtDn in bar, set got[subbt]
   mt = true;   md = btdur;
   for (t = 0;  t < _f.trk.Ln;  t++)  if (TSho (t))
      for (n = _f.trk [t].n, nn = _f.trk [t].nn, p = 0;  p < nn;  p++)
         if ((n [p].tm >= tb) && (n [p].tm < te)) {
            if ((n [p].tm - tb) >= sbdur/2)  mt = false;   // NOPE !

            if ( (n [p].te <= te) && ((n [p].te - n [p].tm + 1) < md) ) {
               md = n [p].te - n [p].tm + 1;
//TStr d1,d2;
//DBG("      now mindur=`d cuz tr=`d `s `s",
//md, t, TmSt(d2, n [p].tm),  MKey2Str(d1,n [p].nt));
            }

         // mark subbt if tsig has - offset from beat in ticks
            if (ts->sub > 1) {
               t1 = n [p].tm - tb;   while (t1 >= btdur)  t1 -= btdur;
               for (s = 0;  s < ts->sub;  s++)  if (t1 < st [s])  break;
            // don't care bout "on beat" subbeats
               if (s && (s < ts->sub) && (! got [s])) {
//TStr d1,d2;
//DBG("      now got[`d] TRUE cuz tr=`d `s `s",
//s,  t, TmSt(d2, n [p].tm),  MKey2Str(d1,n [p].nt));
                  got [s] = true;
               }
            }
         }
//DBG("   empty=`b mindur=`d, got:", mt, md);
//if (ts->sub > 1) for(t=0;t<ts->sub;t++)DBG("      `d `d", t, got[t]);

   sub = 1;
   if (ts->sub > 1) {                  // see if subbt can be lessened
      for (s = 1;  s < ts->sub;  s++)  if (got [s])  {sub = ts->sub;   break;}
//DBG("   init sub=`d", sub);
      switch (sub) {                   // see bout a lower div if got empty stuf
         case 4:                       // 1 . x .  makes sb=2 work
            if ((! got [1]) && (! got [3]))  sub = 2;
            break;
         case 9:                       // b . . x . . x . . => 3
            if ((! got [1]) && (! got [2]) && (! got [4]) && (! got [5]) &&
                (! got [7]) && (! got [8]))  sub = 3;
            break;
         case 8:                       // b . x . x . x . => 4
                                       // b . . . x . . . => 2
            if ((! got [1]) && (! got [3]) && (! got [5]) && (! got [7])) {
               if (got [2] || got [6])  sub = 4;
               else                     sub = 2;
            }
            break;
         case 6:                       // b . . x . . => 2
                                       // b . x . x . => 3
            if ((! got [1]) && (! got [5])) {
               if  (! got [3])                  sub = 3;
               if ((! got [2]) && (! got [4]))  sub = 2;
            }
            break;
      }
   }
   if (mt)  sub = 0;                   // only need bar line
//DBG("   sub=`d", sub);
   *sb = sub;                          // store subbeat (1=beat,2+ subbt)

// default to MAX barh  (768 => 240)
   *h = (ubyt2)((te - tb) * 5 / 16);
//DBG("   default h(max)=`d", *h);

// usually, noteh = ntDur * barh / barDur
// calc less barh if md's h >14  (make sure short notes can be seen)
   if ((md*5/16) > 20) {               // shrink so h of mindur is 14
      *h = (ubyt2)((te - tb) * 20 / md);
//DBG("   shrink so h of mindur is 14=`d", *h);
   }

// got subbt>1 - expand h if needed
   if ((sub > 1) && (*h < (ubyt2)((te - tb) * 14 / (btdur / sub)))) {
      *h =                (ubyt2)((te - tb) * 14 / (btdur / sub));
//DBG("   expand h cuz subbt>1=`d", *h);
   }
// absolute min bar h  (768 => 40)
   if (mt) {
      *h = (ubyt2)((te - tb) * 5 / 96);
//DBG("   mt so min h=`d", *h);
   }

//DBG("BarH end - h=`d sub=`d", *h, *sb);
}


void Song::SetSym ()
// ok, now paginate it all up given our CtlNt's w,h and all them thar notes
// Draw calls us, not ReDo;  depends on SetDn(_dn[]) n SetNt(trk[].n) tho
{ ubyte nw, ww, nMn, nMx, nd, dmap [128], dpos, td, t, st, bl, sb, i, j, nt,
             o, oMn [7], oMx [7];
  ubyt2 W, H, x, xo, w, th, ch, b, dx, cw, y1, y2, h, oX [7];
  ubyt4 np, nc, nb, ns, p, q, nn, pc1, cb1, cs1, tb, te, ntb, nte, sn, d, e;
  bool  drm, mt, got;
  TStr  smn, smx;
  TrkNt   *n;
  DownRow *dr;
TStr ts1, ts2;
   W = Up.w;   H = Up.h;
TRC("SetSym w=`d h=`d", W, H);
   nw = W_NT;   ww = W_NTW;   th = Up.txH;  // dem consts
   for (cw = 0, p = 0;  p < _f.ctl.Ln;  p++) {
      if      (_f.ctl [p].sho == 'm')  cw += th;
      else if (_f.ctl [p].sho == 'y')  cw += 32+2;
   }
   _pag.Ln = _col.Ln = _blk.Ln = _sym.Ln = 0;
   for (b = 1;  b <= _bEnd;) {
      if (_pag.Full ()) {
TRC("SetSym end - w,h too small");
         return;                       // prob screen too small - give up
      }
      np = _pag.Ins ();                // init pag[np]
      _pag [np].col = & _col [pc1 = _col.Ln];
//DBG("b=`d/`d np=`d pc1=`d W=`d H=`d", b, _bEnd, np, pc1, W, H);
      _pag [np].nCol = 0;
      _pag [np].w = _pag [np].h = 0;
      do {
         if (_col.Full ()) {
TRC("_col full prob cuz w,h");
            break;
         }
         nc = _col.Ins ();             // init col[nc]
         _col [nc].blk = & _blk [cb1 = _blk.Ln];   _col [nc].nBlk = 0;
         _col [nc].sym = & _sym [cs1 = _sym.Ln];   _col [nc].nSym = 0;
//DBG(" nc=`d cb1=`d cs1=`d", nc, cb1, cs1);
         _col [nc].x   = _pag [np].w;  // remem 1st blk,sym of col
         ch = H_KB;

      // build blk[]s for col from b while blks h (per b's tm,tSc) fit
         BarH (& h, & sb, b);
         nb = _blk.Ins ();     _col [nc].nBlk++;
         _blk [nb].bar = b;    _blk [nb].tMn = Bar2Tm (b);
                               _blk [nb].tMx = Bar2Tm (b+1);
         _blk [nb].y   = ch;   _blk [nb].h   = h;   ch += h;
         _blk [nb].sb  = sb;
//DBG("  nb(1)=`d bar=`d ch=`d tMn=`d tMx=`d y=`d h=`d",
//nb,b,ch,_blk[nb].tMn, _blk[nb].tMx, _blk[nb].y, _blk[nb].h);
         while (b+1 <= _bEnd) {        // there has to BE another bar
            BarH (& h, & sb, b+1);
            if ((ch + h) > H)  break;  // won't fit, col complete, outa herez

            nb = _blk.Ins ();      _col [nc].nBlk++;  // add block, extend col
            _blk [nb].bar = ++b;   _blk [nb].tMn = Bar2Tm (b);
                                   _blk [nb].tMx = Bar2Tm (b+1);
            _blk [nb].y   = ch;    _blk [nb].h   = h;   ch += h;
            _blk [nb].sb  = sb;
//DBG("  nb=`d bar=`d ch=`d tMn=`d tMx=`d y=`d h=`d",
//nb,b,ch,_blk[nb].tMn, _blk[nb].tMx, _blk[nb].y, _blk[nb].h);
         }
         b++;                          // on to the next bar

      // init width parts of col based on sym[]s we'll have, n ntPoss
      // end up w dmap[nd] of drum tracks in col
      // and nMn,nMx for real notes;  oMn[], oMx, oX for ez notes
         nMn = 127;   nd = nMx = 0;
         for (o = 0;  o < 7;  o++)  {oMn [o] = 127;   oMx [o] = 0;}
         tb = Bar2Tm (_blk [cb1].bar);   te = Bar2Tm (_blk [nb].bar+1);

         if (SHRCRD) {                 // build limits based on _dn[].nt[].nt
            for (dr = & _dn [0], d = 0;  d < _dn.Ln;  d++, dr++) {
               ntb = dr->time;
               if (ntb <  tb)  continue;
               if (ntb >= te)  break;
               for (i = 0;  i < dr->nNt;  i++)
                  if (TDrm (t = dr->nt [i].t)) {
                     for (x = 0;  x < nd;  x++)  if (dmap [x] == t)  break;
                     if (x >= nd)  dmap [nd++] = t;
//TODO prob need better order in dmap?
                  }
                  else {
                     nt = dr->nt [i].nt;   o = nt/12 - 2;
                     if (nt < oMn [o])  oMn [o] = nt;
                     if (nt > oMx [o])  oMx [o] = nt;
                     if (nt < nMn)      nMn     = nt;
                     if (nt > nMx)      nMx     = nt;
                  }
               }
            }
         else                          // build limits base on trk [].n[].nt
            for (t = 0;  t < _f.trk.Ln;  t++)  if (TSho (t))
               for (n = _f.trk [t].n, nn = _f.trk [t].nn,
                    p = 0;  p < nn;  p++) {
                  if (n [p].te <  tb)  continue;
                  if (n [p].tm >= te)  break;
                  if (TDrm (t))  {dmap [nd++] = t;   break;}    // one'll do it
                  if (n [p].nt < nMn)  nMn = n [p].nt;
                  if (n [p].nt > nMx)  nMx = n [p].nt;
               }
         Sort (dmap, nd, sizeof (dmap [0]), DrCmp);

      // always want SOME notes in col (if none, use 4c-d;  if 1, use nMn+2)
         if      (! nMx)       {nMn = MKey (CC("4c"));   nMx = MKey (CC("4d"));}
         else if (nMx-nMn < 2)  nMx = nMn + 2;

         xo = dx = 0;                  // x offset (left white bump), drum x
         if (SHRCRD) {
            for (o = 0;  o < 7;  o++) {
               oX [o] = dx;
               if (oMx [o]) {
                  for (nt = oMn [o];  nt <= oMx [o];  nt++)
                     if (KeyCol [nt%12] == 'w')  dx += ww;
                  dx += 2;             // border
               }
            }
//for(o=0;o<7;o++)DBG("   oc=`d nMn=`s nMx=`s x=`d",
//o+1,MKey2Str(ts1,oMn[o]),MKey2Str(ts2,oMx[o]),oX[o]);
         }
         else {
         // if nMn or nMx are white, gotta give extra w (white bump)
            dx = (nMx-nMn+1) * nw;     // melo nt w - used by drum sym .x later
            if (KeyCol [nMn%12] == 'w')  dx += (xo = WXOfs [nMn%12] * nw/12);
            if (KeyCol [nMx%12] == 'w')  dx += (ww - nw -
                                                     WXOfs [nMx%12] * nw/12);
         }
         _col [nc].nMn  = nMn;   MemCp (_col [nc].oMn, oMn, sizeof (oMn));
         _col [nc].nMx  = nMx;   MemCp (_col [nc].oMx, oMx, sizeof (oMx));
                                 MemCp (_col [nc].oX,  oX,  sizeof (oX));
         _col [nc].nDrm = nd;    MemCp (_col [nc].dMap, dmap, 128);
      // left border, chd, cue (NOT left whitebump) (xo within syms' x)
         _col [nc].nx   =  _col [nc].x + 4 + (_lrn.chd?th:0) + W_Q;
      // plus note w (incl left and right white bumps)
         _col [nc].dx   =  _col [nc].nx + dx;
         _col [nc].cx   =  _col [nc].dx + nd * nw;
      // plus drum w + ctrl w + right border
         _col [nc].w    = (_col [nc].cx + cw + 4) - _col [nc].x;
         _col [nc].h    = ch;
//DBG("  nc=`d nMn=`s nMx=`s nDrm=`d w=`d h=`d nx=`d dx=`d (dx itself=`d)",
//nc,MKey2Str(ts1,nMn),MKey2Str(ts2,nMx),nd,
//_col[nc].w,_col[nc].h,_col[nc].nx,_col[nc].dx, dx);

         if (SHRCRD) {
            for (dr = & _dn [0], d = 0;  d < _dn.Ln;  d++, dr++) {
               ntb = dr->time;
               if (ntb <  tb)  continue;
               if (ntb >= te)  break;

               for (i = 0;  i < dr->nNt;  i++) {
                  t   = dr->nt [i].t;
                  nt  = dr->nt [i].nt;
                  nte = dr->time + (M_WHOLE/16);      // even trills always 16th
               // true nte is next dn in my ht
                  for (got = false, e = d+1;  e < _dn.Ln;  e++) {
                     for (j = 0;  j < _dn [e].nNt;  j++)
                        if (_f.trk [_dn [e].nt [j].t].ht == _f.trk [t].ht) {
                           if (    (_dn [e].time - 4) > nte) {
                              nte = _dn [e].time - M_WHOLE/48;
                              if (nte >= te)  nte = te-1;  // limit to col
                           }
                           got = true;   break;
                        }
                     if (got)  break;
                  }                 // LONG one or no next so go qnote
                  if ((! got) || ((nte - ntb) > M_WHOLE))
                     nte = dr->time + (M_WHOLE/4);    // FINALLY got nte
                  if (TDrm (t) && (nte > ntb+M_WHOLE/16))
                                   nte = ntb+M_WHOLE/16;   // limit drums
                  ns = _sym.Ins ();
                  _sym [ns].tr = t;   _sym [ns].nt = nt;   // not p !
                  _sym [ns].tm = ntb;
                  _sym [ns].top = _sym [ns].bot = true;
                  if      (ntb < ((tb < M_WHOLE/32) ? 0 : (tb-M_WHOLE/32))) {
                     _sym [ns].top = false;   _sym [ns].y = H_KB;
                  }
                  else if (ntb < tb) {
                  // top stickin into piano area
                     q = cb1;
                     _sym [ns].y = (ubyt2)(_blk [q].y - _blk [q].h *
                                           (_blk [q].tMn - ntb) /
                                           (_blk [q].tMx - _blk [q].tMn));
                  }
                  else {
                     for (q = cb1;  q < nb;  q++)
                        if ((ntb >= _blk [q].tMn) &&
                            (ntb <  _blk [q].tMx))  break;
                     _sym [ns].y = (ubyt2)(_blk [q].y + _blk [q].h *
                                           (ntb          - _blk [q].tMn) /
                                           (_blk [q].tMx - _blk [q].tMn));
                  }
                  if (nte >= te) {
                     _sym [ns].bot = false;   _sym [ns].h = ch - _sym [ns].y;
                  }
                  else {
                     for (q = cb1;  q < nb;  q++)
                        if ((nte >= _blk [q].tMn) &&
                            (nte <  _blk [q].tMx))  break;
                     _sym [ns].h = (ubyt2)(_blk [q].y + _blk [q].h *
                                           (nte          - _blk [q].tMn) /
                                           (_blk [q].tMx - _blk [q].tMn) -
                                           _sym [ns].y + 1);
                  }
                  if (_sym [ns].h < 4)  _sym [ns].h = 4;     // min we can do

                  if (TDrm (t)) {
                     for (j = 0;  j < nd;  j++)  if (dmap [j] == t)  break;
                     x = dx + j * nw;   w = nw;
                  }
                  else {
                     o = nt/12 - 2;    // always white within a (short) oct
                     MKey2Str (smn, oMn [o]);
                     MKey2Str (smx, nt);
                     x = oX [o] + (smx[1] - smn[1]) * ww;   w = ww;
//DBG("      oct=`d oX=`d smn=`s smx=`s",o+1,oX[o],smn,smx);
                  }
                  _sym [ns].x = x;   _sym [ns].w = w;
//DBG("   DN ns=`d tr=`d nt=`s x=`d y=`d w=`d h=`d top=`b bot=`b tm=`d=`s",
//ns,_sym[ns].tr, MKey2Str(ts1,nt),
//_sym[ns].x,_sym[ns].y, _sym[ns].w,_sym[ns].h, _sym[ns].top,_sym[ns].bot,
//ntb,TmSt(ts2,ntb));
                  _col [nc].nSym++;
               }
            }
         }
         else {
         // build REAL sym[]s for col calcin w=b,w,clip n nt,trk overlaps
         // sym.x is an offset from col's nx
         // order by reversed track so lead overdraws bass,
         // white then black so black overdraws,etc
            dpos = nd;
            for (t = (ubyte)_f.trk.Ln;  t--;)  if (TSho (t)) {
               if (drm = TDrm (t)) {
                  for (mt = true, td = 0;  td < nd;  td++)  if (dmap [td] == t)
                                                           {mt = false;  break;}
                  if (mt)  continue;      // empty of notes - neeext
                  dpos--;                 // bump dpos
               }
               for (bl = 0;  bl < (drm ? 1 : 2);  bl++)
                  for (n = _f.trk [t].n, nn = _f.trk [t].nn,
                       p = 0;  p < nn;  p++) {
                     ntb = n [p].tm;   nte = n [p].te;
                     if ((nte < tb) || (ntb >= te))      continue;

                     if ((! drm) && (KeyCol [n [p].nt % 12] ==
                                     (bl ? 'w' : 'b')))  continue;

                     ns = _sym.Ins ();
                     _sym [ns].tr = t;   _sym [ns].nt = p;
                     _sym [ns].top = (n [p].dn != NONE) ? true : false;
                     _sym [ns].bot = (n [p].up != NONE) ? true : false;
                     if      (ntb < ((tb < M_WHOLE/32) ? 0 : (tb-M_WHOLE/32))) {
                        _sym [ns].top = false;   _sym [ns].y = H_KB;
                     }
                     else if (ntb < tb) {
                     // top stickin into piano area
                        q = cb1;
                        _sym [ns].y = (ubyt2)(_blk [q].y - _blk [q].h *
                                              (_blk [q].tMn - ntb) /
                                              (_blk [q].tMx - _blk [q].tMn));
                     }
                     else {
                        for (q = cb1;  q < nb;  q++)
                           if ((ntb >= _blk [q].tMn) &&
                               (ntb <  _blk [q].tMx))  break;
                        _sym [ns].y = (ubyt2)(_blk [q].y + _blk [q].h *
                                              (ntb          - _blk [q].tMn) /
                                              (_blk [q].tMx - _blk [q].tMn));
                     }
                     if (nte >= te) {
                        _sym [ns].bot = false;   _sym [ns].h = ch - _sym [ns].y;
                     }
                     else {
                        for (q = cb1;  q < nb;  q++)
                           if ((nte >= _blk [q].tMn) &&
                               (nte <  _blk [q].tMx))  break;
                        _sym [ns].h = (ubyt2)(_blk [q].y + _blk [q].h *
                                              (nte          - _blk [q].tMn) /
                                              (_blk [q].tMx - _blk [q].tMn) -
                                              _sym [ns].y + 1);
                     }
                     if (_sym [ns].h < 4)  _sym [ns].h = 4;     // min we can do
                     _sym [ns].x = xo + (n [p].nt - nMn) * nw;  // default x,w
                     _sym [ns].w = nw;
                     if (drm)  _sym [ns].x = dx + dpos*nw; // weird x for drums
                     else {
                        if (! bl) {    // white keys - ofs left white bump
                           _sym [ns].x -= (WXOfs [n [p].nt % 12] * nw / 12);
                           _sym [ns].w  = ww;
                        }

                     // check for sym on dif melo trk, same nt, time overlap
                        y1 = _sym [ns].y;   y2 = y1 + _sym [ns].h - 1;
                        for (q = cs1;  q < ns;  q++) {
                           st = _sym [q].tr;   sn = _sym [q].nt;
                           if ( (t != st) && (! TDrm (st)) &&
                                (_f.trk [st].n [sn].nt == n [p].nt) &&
                                (_sym [q].y+_sym [q].h-1 > y1)      && // NOT =
                                (_sym [q].y              < y2) ) {
                              _sym [ns].x = _sym [q].x;
                              _sym [ns].w = _sym [q].w;
                              if (_sym [ns].w >= 9)
                                 {_sym [ns].x += 3;   _sym [ns].w -= 3;}
                           }
                        }
                        if (n [p].ov && (_sym [ns].w >= 9))
                           {_sym [ns].x += 3;   _sym [ns].w -= 3;}
                     }
//DBG("   NT ns=`d tr=`d nt=`s x=`d y=`d w=`d h=`d top=`b bot=`b",
//ns,_sym[ns].tr, MKey2Str(ts1,n [_sym[ns].nt].nt),
//_sym[ns].x,_sym[ns].y, _sym[ns].w,_sym[ns].h, _sym[ns].top,_sym[ns].bot);
                     _col [nc].nSym++;
                  }
            }
         }

      // complete out col, bumping pag - take last barline into account :/
         if (_col [nc].h + 2 > _pag [np].h)  _pag [np].h = _col [nc].h + 2;
         _pag [np].w += _col [nc].w;   _pag [np].nCol++;   nc++;

      } while ((b <= _bEnd) && (_pag [np].w < W));
//DBG(" lpEnd b=`d nc=`d", b, nc);

   // last col will usually go over, but always keep one of em
      if ((_pag [np].nCol > 1) && (_pag [np].w > W)) {     // reset to lop off
         _pag [np].nCol--;   nc--;                         // last col
         _pag [np].w = _pag [np].h = 0;
         for (p = pc1;  p < nc;  p++) {
            _pag [np].w += _col [p].w;
            if (_col [p].h + 2 > _pag [np].h)  _pag [np].h = _col [p].h + 2;
         }
         b        = _col [nc].blk [0].bar;
         _blk.Ln -= _col [nc].nBlk;
         _sym.Ln -= _col [nc].nSym;
         _col.Ln--;
//DBG("b=`d/`d bLn=`d sLn=`d cLn=`d", b, _bEnd, _blk.Ln, _sym.Ln, _col.Ln);
      }
   }
TRC("SetSym end");
}


//------------------------------------------------------------------------------
void Song::ReDo ()                     // somethin fundamental changed sooo
{ ubyte t;                             // rebuild eeeverythin
  char  ch;
TRC("ReDo");
   if (_lrn.POZ)                       Shush (false);
   NotesOff ();
   if ((! Up.uPoz) && _timer->Pause ())  Poz (false);
TRC(" clear stuph");
   MemSet (_lrn.rec, 0, sizeof (_lrn.rec));
   _f.ctl [CtlEv (CC("tmpo")) & 0x7F].sho = PRAC?'m':'n';
TRC(" ReEv SetDn SetNt SetLp TmHop SetSym Draw ReTrk DscSave :/");
   emit sgUpd ("tbPoz");   emit sgUpd ("tbLrn");
   ReEv ();   SetDn ();   SetNt ();   SetLp ('.');   TmHop (_now);
   _pg = _tr = 0;
   SetSym ();   Draw ('a');   ReTrk ();   DscSave ();
TRC("ReDo end");
}
