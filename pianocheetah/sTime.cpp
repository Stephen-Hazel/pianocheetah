// sTime.cpp - Time funcs of Song

#include "song.h"

TSgRow DSig = {0, 4, 4, 4, 1};       // default time sig
KSgRow CSig = {0, 0, 0, 1};          // default key  sig

TSgRow *Song::TSig (ubyt4 tm)
{ ubyt2 p = 0;
   for (;  (p+1 < (ubyt2)_f.tSg.Ln) && (_f.tSg [p+1].time <= tm);  p++)  ;
   return ((p   < (ubyt2)_f.tSg.Ln) && (_f.tSg [p  ].time <= tm)) ? & _f.tSg [p]
                                                                  : & DSig;
}

KSgRow *Song::KSig (ubyt4 tm)          // not really about time, but eh, whatev
{ ubyt2 p = 0;
   for (;  (p+1 < (ubyt2)_f.kSg.Ln) && (_f.kSg [p+1].time <= tm);  p++)  ;
   return ((p   < (ubyt2)_f.kSg.Ln) && (_f.kSg [p  ].time <= tm)) ? & _f.kSg [p]
                                                                  : & CSig;
}

char *Song::TmStr (char *str, ubyt4 tm, ubyt4 *tL8r, ubyte *subt)
// put song time into a string w bar.beat;  maybe return time of next bt & subbt
{ TSgRow *ts;
  ubyt4  dBr, dBt, dSb, l8r;
  ubyt2   br,  bt,  sb;
  ubyte  sub;
   ts = TSig (tm);   dBt = M_WHOLE    / ts->den;
                     dBr = dBt        * ts->num;
                     dSb = dBt / (sub = ts->sub);
   br  = (ubyt2)(ts->bar + (tm - ts->time) / dBr);
   bt  = (ubyt2)(1 +      ((tm - ts->time) % dBr) / dBt);
//DBG("tm=`d  num=`d den=`d sub=`d  br=`d bt=`d  dBr=`d dBt=`d dSb=`d",
//tm,  ts->num, ts->den, sub,  br, bt,  dBr, dBt, dSb);
   l8r = ts->time + (br - ts->bar) * dBr + dBt * bt;
   if (ts->num >= 10)  StrFmt (str, "`d.`02d", br, bt);
   else                StrFmt (str, "`d.`d",   br, bt);
   if (tL8r) *tL8r = l8r;
   if (subt) *subt = sub;
   return str;
}

char *Song::TmSt (char *str, ubyt4 tm, char fr)
// include extra .bx (ticks) if there is some
{ TSgRow *ts;
  ubyt4   dBr, dBt, bx;
  ubyt2    br,  bt;
   ts = TSig (tm);   dBt = M_WHOLE / ts->den;
                     dBr = dBt     * ts->num;
   br  = (ubyt2)(ts->bar + (tm - ts->time) / dBr);
   bt  = (ubyt2)(1 +      ((tm - ts->time) % dBr) / dBt);
   bx  =                  ((tm - ts->time) % dBr) % dBt;
   if      (br > 9999)  StrCp  (str, CC("9999"));
   else if ((bt == 1) && (bx == 0))  StrFmt (str, "`d",       br);
   else if (bx == 0)                 StrFmt (str, "`d.`d",    br, bt);
   else if (! fr)                    StrFmt (str, "`d.`d.`d", br, bt, bx);
   else                              StrFmt (str, "`d.`d.`d/`d",
                                                              br, bt, bx, dBt);
   return str;
}

ubyt2 Song::Tm2Bar (ubyt4 tm)
{ TStr s;
   return (ubyt2)Str2Int (TmStr (s, tm));
}


bool Song::Poz (bool tf, ubyt4 msx)
// return whether timer WAS paused;  slam it to tf;  redraw time w indicator
{ bool pOn;
  TStr s;
   pOn = _timer->Pause ();   _timer->SetPause (tf, msx);
   TmStr (s, _timer->Get ());
   if (tf)  StrFmt (Up.time, "X`s", s);   else StrCp (Up.time, s);
   emit sgUpd ("time");
TRC("Poz set `b was `b", tf, pOn);
   return pOn;
}

ubyt4 Song::Bar2Tm (ubyt2 b, ubyte bt)      // get song time of a bar + bt
{ ubyt2 s = 0;
  ubyt4 t;
   while (((ubyt4)s+1 < _f.tSg.Ln) && (_f.tSg [s+1].bar <= b))  s++;
   if ((s >= _f.tSg.Ln) || (_f.tSg [s].bar > b)) {
      t = (b-1) * M_WHOLE;             // none or none apply yet - use 4/4
      return t + (bt-1) * M_WHOLE / 4;
   }
   b -= _f.tSg [s].bar;                // got tsig so offset from it
   t = _f.tSg [s].time + (b * M_WHOLE * _f.tSg [s].num / _f.tSg [s].den);
   return    t + (bt-1) * M_WHOLE / _f.tSg [s].den;
}


ubyt4 Song::Str2Tm (char *s)           // turn rel str time to abs ubyt4
{ ubyt2 br = (ubyt2)Str2Int (s, & s);
   if (*s++ != '.')  return Bar2Tm (br, 1);
  ubyte bt = (ubyte)Str2Int (s, & s);
   if (*s++ != '.')  return Bar2Tm (br, bt);
  ubyte tk = (ubyte)Str2Int (s);
   return Bar2Tm (br, bt) + tk;
}


ubyte Song::CCValAt (ubyt4 tm, ubyte tr, char *cc)
// NOTE: STRICTLY < tm NOT <= tm ...:/
{ ubyt4  p, ne, maxt = 0;              // max time of (say, vol) cc in trks
  ubyte  c, cid, val, d, t, maxtr = 128;
  TrkEv *e;
   if (TDrm (tr))  return StrCm (cc, CC("Vol")) ? 64 : 127;

// get a cc valu into cid n val
   cid = CtlEv (cc, 'r');
   for (c = 0;  c < NMCC;  c++)  if (! StrCm (MCC [c].s, cc))
      {val = (ubyte) MCC [c].dflt;   break;}
   if (! (cid & 0x80)) {               // not in song?  must be default val then
//DBG("CCValAt tm=`d tr=`d cc=`s - melo new val=`d", tm, tr, cc, val);
      return val;
   }
   for (t = 0;  t < _f.trk.Ln;  t++) { // chase cid's actual valu in _f.trk[].e
      if ( ((d = _f.trk [t].dev) == _f.trk [tr].dev) &&
           ((c = _f.trk [t].chn) == _f.trk [tr].chn) )
         for (p = 0, ne = _f.trk [t].ne, e = _f.trk [t].e;
              (p < ne) && (e->time < tm);  p++, e++)
            if ((e->ctrl == cid) && (e->time >= maxt))
               {val = e->valu;   maxt = e->time;   maxtr = t;}
   }
//DBG("CCValAt tm=`d tr=`d cc=`s - melo got val=`d tr=`d tm=`d",
//tm, tr, cc, val, maxtr, maxt);
   return val;
}


ubyt2 Song::TmpoAct (ubyt2 val)
// turn stored tempo value mod'd by gui picked amt to actual tempo
{ ubyt2 rc = (ubyt2)(val * _f.tmpo / FIX1);
   if (((val * _f.tmpo) % FIX1) >= (FIX1    / 2))  rc++;   // round
   return rc;
}

ubyt2 Song::TmpoSto (ubyt2 val)
// turn actual tempo value back to a stored tempo per _f.tmpo adjust
{ ubyt2 rc = (ubyt2)(val * FIX1 / _f.tmpo);
   if (((val * FIX1) % _f.tmpo) >= (_f.tmpo / 2))  rc++;
   return rc;
}

ubyt2 Song::TmpoAt (ubyt4 tm, char act)
// like CCValAt, but tmpo weirdness.  return straight (non %'d) tempo unless act
{ ubyt2 val = 120;
  ubyt4 p, maxt = 0;
   for (p = 0;  (p < _f.tpo.Ln) && (_f.tpo [p].time <= tm);  p++)
      if (_f.tpo [p].time >= maxt)
         {val = _f.tpo [p].val;   maxt = _f.tpo [p].time;}
   return act ? TmpoAct (val) : val;
}


void Song::TmHop (ubyt4 tm)
{ ubyt4 p, ne, cc;
  ubyt2 cw;
  ubyte d, c, t, v, v2;
  char *cs;
  TStr  str;
  TrkEv *e;
TRC("TmHop `s `s",TmSt(str,tm), LrnS ());
   NotesOff ();   TmpoPik ('l');
   if (_lrn.pLrn)                      // reset lrn/pLrn
      {Up.lrn = _lrn.pLrn;   _lrn.pLrn = 0;   emit sgUpd ("tbLrn");}
TRC(" => `s", LrnS ());

   for (cc = 0;  cc < _cch.Ln;  cc++) {
   // control chasing - set each dev,chn,ctl in _cch to time 0, default value
      _cch [cc].time = 0;
      _cch [cc].valu = _cch [cc].val2 = _cch [cc].trk = 0;
      cs = CtlSt (_cch [cc].ctl);
//DBG(" cc=`d cs=`s", cc, cs);
      if (! StrCm (cs, CC("Prog"))) {  // prog is special - need it's trk
         for (t = 0;  t < _f.trk.Ln;  t++)
            if ((_f.trk [t].dev == _cch [cc].dev) &&
                (_f.trk [t].chn == _cch [cc].chn))  break;
         if (t < _f.trk.Ln) _cch [cc].trk = t;
      }
      else                             // else lookup default in dev/cc.txt
         for (c = 0;  c < NMCC;  c++)  if (! StrCm (MCC [c].s, cs))
            {_cch [cc].valu = MCC [c].dflt & 0x7F;
             _cch [cc].val2 = MCC [c].dflt >> 7;   break;}
//DBG(" trk=`d valu=`d val2=`d", _cch [cc].trk, _cch [cc].valu, _cch [cc].val2);
   }

   for (t = 0;  t < _f.trk.Ln;  t++) {
   // chase ctl actual valu at tm in _f.trk[].e, (and set trk's .p)
      d = _f.trk [t].dev;   c = _f.trk [t].chn;
      for (p = 0, ne = _f.trk [t].ne, e = _f.trk [t].e;
           (p < ne) && (e->time < tm);  p++, e++)     // not <= (for next Put)
         if (e->ctrl & 0x80) {
            for (cc = 0;  cc < _cch.Ln;  cc++)
               if ((_cch [cc].dev == d) && (_cch [cc].chn == c) &&
                   (_cch [cc].ctl == e->ctrl))   break;
            if ((cc < _cch.Ln) && (_cch [cc].time <= e->time)) {
               _cch [cc].trk  = t;
               _cch [cc].time = e->time;
               _cch [cc].valu = e->valu;
               _cch [cc].val2 = e->val2;
//DBG(" t=`d d=`d c=`d cc=`d p=`d time=`d valu=`d",t,d,c,cc,p,e->time,e->valu);
            }
         }
      _f.trk [t].p = p;                // chase _f.trk[].p
   }

   for (cc = 0;  cc < _cch.Ln;  cc++) {
   // ok it's either just init'd or latest at tm is set - dump em out midi
      cs = CtlSt (_cch [cc].ctl);
      v  =        _cch [cc].valu;
      v2 =        _cch [cc].val2;
//DBG(" cc=`s trk=`d dv=`d ch=`d valu=`d val2=`d",
//cs, _cch [cc].trk, _cch [cc].dev, _cch [cc].chn, v, v2);
      if      (! StrCm (cs, CC("Prog")))  SetChn (_cch [cc].trk);
      else if (! StrCm (cs, CC("Tmpo")))  PutTp ((v2 << 8) | v);
      else if (! StrCm (cs, CC("TSig")))  PutTs (v, 1 << (v2 & 0x0F), v2 >> 4);
      else {
         d  = _cch [cc].dev;
         if ((cw = Up.dvt [Up.dev [d].dvt].CCMap [_cch [cc].ctl & 0x7F]))
            Up.dev [d].mo->Put (_cch [cc].chn, cw, v, v2);
      }
   }

// chase all our other pos es
   for (p = 0, ne = _f.chd.Ln;  (p < ne) && (_f.chd [p].time < tm);  p++)  ;
   _pChd = p;
   for (p = 0, ne = _f.lyr.Ln;  (p < ne) && (_f.lyr [p].time < tm);  p++)  ;
   _pLyr = p;          PutLy ();
   for (p = 0, ne = _dn.Ln;     (p < ne) && (_dn    [p].time < tm);  p++)  ;
   _pDn = p;
   for (p = 0;  p < ne;  p++)  for (c = 0;  c < _dn [p].nNt;  c++)
      _dn [p].nt [c].nt &= 0x7F;       // clear all hi bits to unhit
   MemSet (_lrn.rec, 0, sizeof (_lrn.rec));

TRC(" timerset");                      // unpoz unless uPoz
   _timer->Set (_pNow = 1 + (_rNow = _now = tm));
   _lrn.POZ = false;   Poz (Up.uPoz);
   Draw ('a');
TRC(" timerset2");
   _timer->Set (_now);                 // Draw takes a while :/
TStr d1,d2,d3;
TRC("TmHop end: timer=`s timerSig=`s _now=`s",
TmSt(d1,_timer->Get ()), TmSt(d2,_timer->Sig ()), TmSt(d3,_now));
   _lrn.nt1 = true;
}
