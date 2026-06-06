// sDump.cpp

#include "song.h"

char *Song::LrnS ()
{ static TStr s;
   StrCp (s, CC("x"));
   if (Up.lrn    == LHEAR)  StrCp (s, CC("hear"));
   if (Up.lrn    == LHREC)  StrCp (s, CC("hRec"));
   if (Up.lrn    == LPRAC)  StrCp (s, CC("prac"));
   if (Up.lrn    == LPLAY)  StrCp (s, CC("play"));
   if (_lrn.pLrn == LHEAR)  StrAp (s, CC(" p=hear"));
   if (_lrn.pLrn == LHREC)  StrAp (s, CC(" p=hRec"));
   if (_lrn.pLrn == LPRAC)  StrAp (s, CC(" p=prac"));
   if (_lrn.pLrn == LPLAY)  StrAp (s, CC(" p=play"));
   return s;
}


void Song::DumpEv (TrkEv *e, ubyte t, ubyt4 p, char *pre)
{ TStr  o, s, ts;
  ubyte v;
   StrFmt (o, "`s t=`d ", pre ? pre : "", t);
   if (p < 1000000)  StrAp (o, StrFmt (ts, "p=`d ", p));
   StrAp (o, TmSt (ts, e->time));   StrAp (o, CC(" "));
   if (ECTRL (e))
      StrFmt (&o[StrLn(o)], "`s(cc`d)=`02x,`02x",
              CtlSt (e->ctrl), e->ctrl & 0x7F, e->valu, e->val2);
   else {
      StrFmt (&o[StrLn(o)], "`s`c`d",
         TDrm (t) ? MDrm2Str (s, e->ctrl) : MKey2Str (s, e->ctrl),
         EUP (e) ? '^' : (EDN (e) ? '_' : '~'),  e->valu & 0x007F);
   }
   DBG(o);
}


void Song::DumpTrEv (ubyte t)
{  DBG("t=`d ne=`d nb=`d nn=`d",
       t, _f.trk [t].ne, _f.trk [t].nb, _f.trk [t].nn);
  TrkEv *ev = _f.trk [t].e;
   for (ubyt4 e = 0;  e < _f.trk [t].ne;  e++, ev++)  DumpEv (ev, t, e);
}


void Song::DumpDn ()
{ TStr d1;
  ulong p;
   DBG("_dn p tm msec tmpo tmpoAct  /  nt trk p  /  velo");
   for (p = 0;  p < _dn.Ln;  p++) {
      DBG("`d `s `d `d `d",
          p, TmSt (d1,_dn [p].time), _dn [p].msec, _dn [p].tmpo,
          TmpoAct (_dn [p].tmpo));
     TStr s;
      for (ubyte c = 0;  c < _dn [p].nNt;  c++)
         DBG("   `s `d `d", MKey2Str(s,_dn [p].nt [c].nt), _dn [p].nt [c].t,
                                                           _dn [p].nt [c].p);
      *s = '\0';
      for (ubyte c = 0;  c < 7;  c++)
         StrAp (s, StrFmt (d1, " `d", _dn [p].velo [c]));
      DBG(s);
   }
   DBG("_dn end");
}


void Song::DumpRec ()
{ TrkEv *ev;
  ubyt4  e;
   for (e = 0, ev = & _recM [0];  e < _recM.Ln;  e++, ev++)
      DumpEv (ev, (ubyte)98, e);
   for (e = 0, ev = & _recD [0];  e < _recD.Ln;  e++, ev++)
      DumpEv (ev, (ubyte)99, e);
}


void Song::Dump (bool e2)
{ ubyte  t;
  ubyt4  s;
  TStr   t1, t2, t3;
  TrkEv *ev;
//DumpX ();   return;
DBG("DUMP");
// DumpDn ();
   Sy.Dump ();
   DBG(
      "bEnd=`d tEnd=`s tr.Ln=`d eTrk=`d ed=`d pLyr=`d\n"
      "now=`s pDn=`d/`s\n"
      "dn.Ln=`d nEv=`d maxEv=`d\n"
      "ntCo=`d barCl=`b\n"
      "SnF_tmpo=`d tran=`d `s\n"
      "POZ=`b uPoz=`b\n"
      "lrn: lpBgn=`s lpEnd=`s pg=`d",
      _bEnd, TmSt(t1,_tEnd), _f.trk.Ln, Up.eTrk, _ed, _pLyr,
      TmSt(t2,_now), _pDn, TmSt(t3,_dn[_pDn].time),
      _dn.Ln, _f.nEv, _f.maxEv,
      Cfg.ntCo, Cfg.barCl,
      _f.tmpo, _f.tran, LrnS (),
      _lrn.POZ, Up.uPoz,
      TmSt(t1,_lrn.lpBgn), TmSt(t2,_lrn.lpEnd), _pg
   );

/* DBG("mi name.type");
   for (t = 0;  t < _mi.Ln;     t++)  DBG("`>3d `s.`s",
      t,  _mi [t].mi->Name (),  _mi [t].mi->Type ());
*/
   DBG("dev name.type dvt#");
   for (t = 0;  t < Up.dev.Ln;  t++)  DBG("`>3d `s.`s `d",
      t,  Up.dev [t].mo ? Up.dev [t].mo->Name () : "(empty)",
          Up.dev [t].mo ? Up.dev [t].mo->Type () : "",
          Up.dev [t].dvt);
   DBG("dvt name nCC nSn nDr");
   for (t = 0;  t < Up.dvt.Ln;  t++)  DBG("`>3d `s `d `d `d",
      t,  Up.dvt [t].Name (), Up.dvt [t].NCC (),
          Up.dvt [t].NSn  (), Up.dvt [t].NDr ());
   DBG("trk name             "
       "dev chn      snd        e       ne       nn       nb"
       "        p shh lrn ht drm");
   for (t = 0; t < _f.trk.Ln; t++)
      DBG("`>3d `s`<15s "
           "`>3d `>3d `>8d `08x `>8d `>8d `>8d `>8d `b   `b   `b   `c  `02x",
         t,
         _f.trk [t].grp?"+":".",
         _f.trk [t].name,
         _f.trk [t].dev,
         _f.trk [t].chn,
         _f.trk [t].snd,
         _f.trk [t].e,
         _f.trk [t].ne,
         _f.trk [t].nn,
         _f.trk [t].nb,
         _f.trk [t].p,
         _f.trk [t].shh,
         _f.trk [t].lrn,
         _f.trk [t].rec,
         _f.trk [t].ht ?_f.trk [t].ht :' ',
         _f.trk [t].drm
      );
   for (t = 0;  t < Up.dev.Ln;  t++)
      if (Up.dev [t].mo)  Up.dev [t].mo->DumpOns ();

/* DBG("ctl name sho");
   for (t = 0;  t < _f.ctl.Ln;  t++)
      DBG("`>3d `s `c", t, CtlSt (t), _f.ctl [t].sho);
   DBG("cch dev chn ctl");
   for (t = 0;  t < _cch.Ln;  t++)
      DBG("`>3d `>3d `>3d `<8s",
         t, _cch [t].dev, _cch [t].chn,  CtlSt (_cch [t].ctl)
      );
   DBG("tSg     time  bar num den sub");
   for (s = 0;  s < _f.tSg.Ln;  s++)
      DBG("`>3d `>8d `>4d `>3d `>3d",
         s,
         _f.tSg [s].time,
         _f.tSg [s].bar,
         _f.tSg [s].num,
         _f.tSg [s].den,
         _f.tSg [s].sub
      );
   DBG("kSg     time key min flt");
   for (s = 0;  s < _f.kSg.Ln;  s++)
      DBG("`>3d `>8d `<3s `<3b `<3b",
         s,
         _f.kSg [s].time,
         MKeyStr [_f.kSg [s].key],
         _f.kSg [s].min,
         _f.kSg [s].flt
      );
   DBG("mapD shh lrn rec ht inp ctl vol pan snd");
   for (t = 0;  t < _mapD.Ln;  t++)
      DBG("`>3d `b `c `s `s `>3d `>3d `d",
         t, _mapD [t].shh, _mapD [t].lrn, _mapD [t].rec,
         _mapD [t].ht ? _mapD [t].ht : ' ',
         MDrm2Str(t2,_mapD [t].inp), MDrm2Str(t1,_mapD [t].ctl),
         _mapD [t].vol, _mapD [t].pan, _mapD [t].snd);
   for (t = 0;  t < _dvt.Ln;  t++)  {DBG("dvt=`02d...", t);   _dvt [t].Dump ();}
   DBG("lyr     time  str");
   for (s = 0;  s < _f.lyr.Ln;  s++)
      DBG("`>3d `s `s", s, TmSt (t1, _f.lyr [s].time), _f.lyr [s].s);
   DBG("chd     time  str");
   for (s = 0;  s < _f.chd.Ln;  s++)
      DBG("`>3d `s `s", s, TmSt (t1, _f.chd [s].time), _f.chd [s].s);
   DBG("cue time tend str");
   for (s = 0;  s < _f.cue.Ln;  s++)
      DBG("`d `s `s `s", s,  TmSt(t1,_f.cue [s].time),
           _f.cue [s].tend ? TmSt(t2,_f.cue [s].tend) : "", _f.cue [s].s);
   DBG("bug time hits");
   for (s = 0;  s < _f.bug.Ln;  s++)
      DBG("`d `s `s", s, TmSt(t1,_f.bug [s].time), _f.bug [s].s);
   DBG("_f.tmpo=`d/`d", _f.tmpo, FIX1);
   DBG("{ _f.tpo time tmpo tmpoAct");
   for (p = 0;  p < _f.tpo.Ln;  p++)
      DBG("`d `s `d `d",  p, TmSt (d1,_f.tpo [p].time), _f.tpo [p].val,
                                               TmpoAct (_f.tpo [p].val));
   DBG("_f.tpo end");

  ubyt4 ct [2][12];
  ubyte r;
   MemSet (ct, 0, sizeof (ct));
   for (t = 0; t < _f.trk.Ln; t++)  if (! TDrm (t)) {
      r = _f.trk [t].lrn ? 1 : 0;
      ev = _f.trk [t].e;
      for (ubyt4 e = 0;  e < _f.trk [t].ne;  e++, ev++)
         if (ENTDN (ev))  ct [r][ev->ctrl % 12]++;
   }
   for (t = 0; t < 12; t++) ct [2][t] = ct [0][t] + ct [1][t];
   DBG("lrn,non,tot:");
   DBG("`>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d",
      ct[0][0], ct[0][1], ct[0][2], ct[0][3], ct[0][4], ct[0][5],
      ct[0][6], ct[0][7], ct[0][8], ct[0][9], ct[0][10], ct[0][11]);
   DBG("`>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d",
      ct[1][0], ct[1][1], ct[1][2], ct[1][3], ct[1][4], ct[1][5],
      ct[1][6], ct[1][7], ct[1][8], ct[1][9], ct[1][10], ct[1][11]);
   DBG("`>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d `>4d",
      ct[2][0], ct[2][1], ct[2][2], ct[2][3], ct[2][4], ct[2][5],
      ct[2][6], ct[2][7], ct[2][8], ct[2][9], ct[2][10], ct[2][11]);
   DBG("} DUMP");

*/
   if (e2) {
      for (t = 0; t < _f.trk.Ln; t++) {
         DBG("t=`d ne=`d nb=`d nn=`d",
             t, _f.trk [t].ne, _f.trk [t].nb, _f.trk [t].nn);
         ev = _f.trk [t].e;
         for (ubyt4 e = 0;  e < _f.trk [t].ne;  e++, ev++)  DumpEv (ev, t, e);
      }

   }
//if (Sy != nullptr)  Sy->Dump ();
   DumpRec ();
DBG("DUMP end");
}
