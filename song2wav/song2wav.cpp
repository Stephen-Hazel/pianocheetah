// song2wav.cpp  .song => .wav via Syn but directly...

#include "song2wav.h"
#include "sTime.cpp"
#include "sLoad.cpp"

Song  Sg;
sbyt2 SmpBuf [2*1024];                 // 1024 stereo sbyt2 samples


void Song::DumpEv (TrkEv *e, ubyte t, ubyt4 p)
{ TStr  o, s, ts;
  ubyte v;
   StrFmt (o, "t=`d ", t);
   if (p < 1000000)  StrAp (o, StrFmt (ts, "p=`d ", p));
   StrAp (o, TmS (ts, e->time));   StrAp (o, " ");
  bool dr = (_trk [t].chn == 9) ? true : false;
   if (ECTRL (e))
      StrFmt (&o[StrLn(o)], "`s(cc`d)=`02x,`02x",
              _ctl [e->ctrl & 0x7F], e->ctrl & 0x7F, e->valu, e->val2);
   else {
      StrFmt (&o[StrLn(o)], "`s`c`d",
         dr ? MDrm2Str (s, e->ctrl) : MKey2Str (s, e->ctrl),
         EUP (e) ? '^' : (EDN (e) ? '_' : '~'),  e->valu & 0x007F);
   }
   DBG(o);
}

void Song::Dump ()
{ ubyte t;
  ubyt2 s;
   DBG("{ DUMP");
   DBG("now=`d end=`d nEv=`d", _now, _tEnd, _nEv);
   DBG("trk name chn snd e ne p shh");
   for (t = 0;  t < _trk.Ln;  t++)
      DBG("`d `s `d `s `08x `d `d `b",
         t,
         (char *)_trk [t].name,
         _trk [t].chn+1,
         _trk [t].snd,
         _trk [t].e,
         _trk [t].ne,
         _trk [t].p,
         _trk [t].shh
      );

   DBG("ctl name");
   for (t = 0;  t < _ctl.Ln;  t++)  DBG("`d `s", t, (char *)_ctl [t]);
   DBG("sig time bar num den");
   for (s = 0;  s < _tSg.Ln;  s++)
      DBG("`d `d `d `d `d",
         s,
         _tSg [s].time,
         _tSg [s].bar,
         _tSg [s].num,
         _tSg [s].den
      );
/*
  TrkEv *ev;
   for (t = 0; t < _trk.Ln; t++) {
      DBG("t=`d ne=`d", t, _trk [t].ne);
      ev = _trk [t].e;
      for (ubyt4 e = 0;  e < _trk [t].ne;  e++, ev++)  DumpEv (ev, t, e);
   }
*/
   DBG("} DUMP");
}


void Song::Wipe ()  // wipe all data and "empty" display
{ ubyt2 i;
DBG("Wipe");
   for (i = 0;  i < 16;  i++)  Sy.Put ((ubyte)i, MC_CC|M_ASOFF, 0, 0);
   _now = 0;
   for (i = 0;  i < _trk.Ln;  i++)  _trk [i].name [0] = '\0';  _trk.Ln = 0;
   for (i = 0;  i < _ctl.Ln;  i++)  _ctl [i][0]       = '\0';  _ctl.Ln = 0;
   _nEv = 0;
   _tSg.Ln = 0;
   _timer.SetTempo (120);
}


//______________________________________________________________________________
void Song::SetChn (ubyte t)            // send ProgCh on trk's chn
{ ubyte c = _trk [t].chn;
   if (c == 9)  return;

DBG("SetChn tr=`d ch=`d", t, c+1);
  char *s = _trk [t].snd;
  ubyte p;
   if (*s == '\0')  return;

   for (p = 0;  _bnk [p][0];  p++)  if (! StrCm (_bnk [p], s))  break;
   if (_bnk [p][0])  {
      Sy.Put (c, MC_PROG, p);
DBG("   prog ch=`d prog=`d", c+1, p);
   }
}


void Song::PutCC (ubyte t, TrkEv *e)
{ ubyte c = e->ctrl & 0x7F;
   if      (! StrCm (_ctl [c], "Tmpo"))
   { ubyt4 tp = e->valu + (e->val2 << 8);   _timer.SetTempo (tp);}
   else if (! StrCm (_ctl [c], "TSig"))  ;
   else if (! StrCm (_ctl [c], "Prog"))  SetChn (t);
   else {
     ubyt2 craw = _dvt.CCMap [c];
      if (craw) Sy.Put ((ubyte)_trk [t].chn, craw, e->valu, e->val2);
   }
}


void Song::PutNt (ubyte t, TrkEv *e)
{  Sy.Put ((ubyte)_trk [t].chn, e->ctrl,  e->valu, e->val2);
//DBG("  PutNt chan=`d ctrl=`s valu=`02x val2=`02x",
//_trk [t].chn+1, MKey2Str (n, e->ctrl),  e->valu, e->val2);
}


//------------------------------------------------------------------------------
ubyt4 Song::Put (File *f)
// the main dude - writes whole song to .wav
//    slices up song time between midi events, converts to sample ofs,
//    sends midi, writes buffers of samples
{ ubyte  t, ctl;
  ubyt4  tL8r, p, ne, tm, s, b, totSmp = 0;
  TStr   bar, end;
  TrkEv *e;
DBG("Song::Put bgn");
   TmStr (bar, _tEnd);   StrFmt (end, "`04d", Str2Int (bar));
   for (_now = 0;  _now < _tEnd;) {
      TmStr (bar, _now, & tL8r);   StrAp (bar, " / ");   StrAp (bar, end);
DBG(" bar=`s _now=`d tl8r=`d tEnd=`d", bar, _now, tL8r, _tEnd);
   // plow thru tracks from .p to tNow and write events
      for (t = 0;  t < _trk.Ln;  t++) {
         for (e = _trk [t].e,  ne = _trk [t].ne,  p = _trk [t].p;
              (p < ne) && (e [p].time <= _now);   p++) {
            if      ((ctl = e [p].ctrl) & 0x80)  PutCC (t, & e [p]);
            else if (! _trk [t].shh)             PutNt (t, & e [p]);
         }
         _trk [t].p = p;
         if (p < ne)  if ((tm = e [p].time) < tL8r)  tL8r = tm;
      }

   // when do we write events next?  render samples till then
      _now = tL8r;
      totSmp += (s = _timer.Set (_now));
DBG(" _now=`d s=`d totSmp=`d", _now, s, totSmp);
      while (s) {
         b = (s > 1024) ? 1024 : s;   s -= b;
DBG(" b=`d s=`d", b, s);
         Sy.PutWav (SmpBuf, b);   f->Put (SmpBuf, b*4);
      }
   }
DBG("Song::Put end totSmp=`d", totSmp);
   return totSmp;
}


void WavHdr (File *f, ubyt4 totLen = 0)
// rewind and update header with lengths
{ ubyt4 ln1, ln2, ln3;
  WAVEFORMATEX wf;
   wf.wFormatTag      = WAVE_FORMAT_PCM;
   wf.nChannels       = 2;
   wf.wBitsPerSample  = 16;
   wf.nSamplesPerSec  = 44100;
   wf.nBlockAlign     = wf.nChannels   * wf.wBitsPerSample / 8;
   wf.nAvgBytesPerSec = wf.nBlockAlign * wf.nSamplesPerSec;
   ln3 = totLen*4;                     // stereo * sizeof (sbyt2)
   ln2 = 16;
   ln1 = 12 + ln2 + 8 + ln3;
   f->Seek (0, '<');
   f->Put ("RIFF"    );  f->Put (& ln1, 4);
   f->Put ("WAVEfmt ");  f->Put (& ln2, 4);  f->Put (& wf, ln2);
   f->Put ("data"    );  f->Put (& ln3, 4);
}


int main (int argc, char *argv [])
{ TStr fn;
  File f;
DBGTH("S2W"); DBG("bgn");
   App.Init ();
   StrCp (fn, argv [1]);
   Sy.Init ('w');   Sg.Init ();   Sg.Load (fn);

   StrAp (fn, ".wav", 5);
   if (! f.Open (fn, "w"))  Die ("can't write .wav file");

DBG("write .wav");
   WavHdr (& f);   WavHdr (& f, Sg.Put (& f));   f.Shut ();

DBG("wipe/quit");
   Sg.Wipe ();   Sy.Quit ();   return 0;
DBG("end");
}
