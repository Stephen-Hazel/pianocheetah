// sDevice.cpp - device/channel/sound funcs for Song

#include "song.h"

// MidiIn stuph
char *MInDef::CcRec (char *buf, ubyt2 len, ubyt4 pos, void *ptr)
// parse a record of the ccin.txt file
{ MInDef *m = (MInDef *)ptr;   (void)len;
// outa room?
   if (pos >= 128) {
      DBG("MInDef::CcRec ccin.txt for `s is > 128 lines", m->mi->Type ());
      return CC("x");
   }
  ColSep ss (buf, 3);
   if ((*buf == '#') || (! ss.Col [1][0]) ||
                          (ss.Col [1][0] == '.'))  return nullptr;
// got all the cols we need?
   if (StrLn (ss.Col [1]) > MAXWSTR) {
      DBG("MInDef::CcRec ccin.txt for `s line `d  map cc too long",
          m->mi->Type (), pos+1);
      return CC("x");
   }
  ubyt4 n = m->cc.Ins ();
          m->cc [n].raw = MCtl (ss.Col [0]);
   StrCp (m->cc [n].map,        ss.Col [1]);
   return nullptr;
}


void Song::OpenMIn ()
// open every non OFF midiin device we got in device.txt
//   set _mi[].mi to device
//   load ccin.txt into _mi[].cc[] ubyt2 .raw comes in and turns to str .map
//   send out dev SET LOCAL CONTROL OFF if there's one of same name (that's on)
{ MidiI *mi;
  TStr   iname, t, xs, fn;
  ubyte  i = 0, e [4];
  ubyt4  n;
  File   f;
  MidiO *mo;
   _mi.Ln = 0;
   while (Midi.GetPos ('i', i++, iname, t, xs, xs))  if (StrCm (t, CC("OFF"))) {
      if (_mi.Full ())  break;         // got room?

   // init our device
      n = _mi.Ln++;
      _mi [n].mi = mi = new MidiI (iname, _timer);

   // load ccin.txt of devtype (only devtype stuff input needs)
      _mi [n].cc.Ln = 0;
      StrFmt (fn, "`s/device/`s/ccin.txt",  App.Path (t, 'd'), mi->Type ());
      f.DoText (fn, & _mi [n], _mi [0].CcRec);

   // send local control off (cc 122 0) if MidiO of same name exists
      if (Midi.Get ('o', iname, xs, xs, xs)) {
TRC("   Local Control=OFF for `s", iname);
         mo = new MidiO (iname, 'n');
         if (! mo->Dead ())
            {e [0] = M_CTRL;   e [1] = M_LOCAL;   e [2] = 0;   mo->PutMEv (e);}
         delete mo;   mo = nullptr;
      }
   }
}


void Song::ShutMIn ()
// send LOCAL CONTROL back ON;   shut all the midiins down
{ ubyt4  n;
  ubyte  e [4];
  TStr   nm, xs;
  MidiO *mo;
   while (_mi.Ln) {
      n = _mi.Ln - 1;
      StrCp (nm, _mi [n].mi->Name ());

   // send local control back on (cc 122 7F) if MidiO of same name exists
      if (Midi.Get ('o', nm, xs, xs, xs)) {
TRC("   Local Control=ON for `s", nm);
         mo = new MidiO (nm, 'n');
         if (! mo->Dead ())
            {e [0] = M_CTRL;   e [1] = M_LOCAL;   e [2] = 0x7F;
                                                                mo->PutMEv (e);}
         delete mo;   mo = nullptr;
      }
      delete _mi [n].mi;     _mi [n].mi = nullptr;
             _mi [n].cc.Ln = 0;
      _mi.Ln--;
   }
}


//______________________________________________________________________________
void Song::CCMapLoad ()
{ TStr fn, s;
  ulong i, j;
  ubyte d, c;
  StrArr t (CC("ccmap"), 128, 128*sizeof(TStr));
   App.Path (fn, 'd');   StrAp (fn, CC("/device/ccmap.txt"));
   t.Load (fn);
   _ccMap.Ln = 0;
   for (i = j = 0;  i < t.num;  i++) {
      StrCp (s, t.str [i]);
     ColSep ss (s, 3);
      if ((*s == '#') || (ss.Col [2][0] == '\0')) continue;
      for (d = 0;  d < _mi.Ln;  d++)
         if (! StrCm (_mi [d].mi->Name (), ss.Col [0]))  break;
      if (d >= _mi.Ln) {
DBG("couldn't find device '`s' in ccmap line `d", ss.Col [0], i);
         continue;
      }
      for (c = 0;  c < _mi [d].cc.Ln;  c++)
         if (! StrCm (_mi [d].cc [c].map, ss.Col [1]))  break;
      if (c >= _mi [d].cc.Ln) {
DBG("couldn't find control name '`s' in ccin.txt of `s",
ss.Col [1], _mi [d].mi->Name ());
         continue;
      }
      _ccMap.Ln++;
      _ccMap [j].dev = d;
      _ccMap [j].cc  = _mi [d].cc [c].raw;
      StrCp (_ccMap [j].str, ss.Col [2]);   j++;
   }
}


//______________________________________________________________________________
// MidiOut stuph
ubyte Song::OpenDev (char *nm)
// return Up.dev pos if got;  else KICK it up n return new pos (n maybe devtype)
{ ubyte d, t;
TRC("OpenDev `s", nm);
   for (d = 0;  d < Up.dev.Ln;  d++)   // already open?
      if (Up.dev [d].mo && (! StrCm (nm, Up.dev [d].mo->Name ())))  break;
   if (d < Up.dev.Ln)  {TRC("  already got");   return d;}

// gotta getta new one
   for (d = 0;  d < Up.dev.Ln;  d++)  if (Up.dev [d].mo == nullptr)  break;
   if (d >= Up.dev.Ln) {
      if (Up.dev.Full ())  DBG("OpenDev  tooo many devices");
      Up.dev.Ln++;
   }
   Up.dev [d].mo = new MidiO (nm);
   if (Up.dev [d].mo->Dead ()) {
      delete Up.dev [d].mo;   Up.dev [d].mo = nullptr;
      return MAX_DEV;
   }

// got it's DevTyp yet?
   for (t = 0;  t < Up.dvt.Ln;  t++)
      if (! StrCm (Up.dvt [t].Name (), Up.dev [d].mo->Type ()))  break;
   if (t >= Up.dvt.Ln) {               // need newguy
      for (t = 0;  t < Up.dvt.Ln;  t++)  if (! Up.dvt [t].Name () [0])  break;
      if (t >= Up.dvt.Ln) {
         if (Up.dvt.Full ())  DBG("OpenDev  outa DevTyp slots");
         Up.dvt.Ln++;                  // ^ain't gonna happen as device slots
      }                                // will run out first
      Up.dvt [t].Open (Up.dev [d].mo->Type ());
   }
   Up.dev [d].dvt = t;
   return d;
}


void Song::ShutDev (ubyte d)
{  if (! Up.dev [d].mo)  return;
  ubyte t = Up.dev [d].dvt;
TRC("ShutDev d=`d=`s dvt=`d", d, Up.dev [d].mo->Name (), t);
   delete Up.dev [d].mo;   Up.dev [d].mo = nullptr;   Up.dev [d].dvt = MAX_DEV;
   while (Up.dev.Ln && (Up.dev [Up.dev.Ln-1].mo == nullptr))
      Up.dev.Del (Up.dev.Ln-1);

// can I shut down DevTyp?
   for (d = 0;  d < Up.dev.Ln;  d++)
      if (Up.dev [d].mo && (Up.dev [d].dvt == t))  break;
   if (d >= Up.dev.Ln) {
      Up.dvt [t].Shut ();
      while (Up.dvt.Ln && (! Up.dvt [Up.dvt.Ln-1].Name ()[0]))  Up.dvt.Ln--;
   }
}


void Song::NotesOff ()                 // playing notes n hold cc go off
{  for (ubyte d = 0;  d < Up.dev.Ln;  d++)  if (Up.dev [d].mo)
                                                Up.dev [d].mo->NotesOff ();
}


ubyte Song::PickChn (char *dv)
{ ubyte nc, t;
  TStr  cmap;
   nc = StrCm (dv, CC("syn")) ? 16 : 128;
   MemSet (cmap, '.', nc);   cmap [9] = 'x';   cmap [nc] = '\0';
   for (t = 0;  t < _f.trk.Ln;  t++)
      if (! StrCm (Up.dev [_f.trk [t].dev].mo->Name (), dv))
                     cmap [_f.trk [t].chn] = 'x';
   for (t = 0;  t < nc;  t++)  if (t != 'x')  break;
   if (t == nc)  t = 255;
   return t;
}


void Song::PickDev (ubyte tr, char *sndName, char *devName)
// map a dev,chn for track tr given it's devName,sndName and other trks' chans
{ ubyte i, d, c = 0, nD = 0, m, x, nc;
  TStr  dLst [MAX_TRK][2], buf, ty, xs, dv, cmap;
  bool  cOk;
  DevTyp dt;
TRC("PickDev tr=`d snd=`s dev=`s", tr, sndName, devName?devName:"");
   if ( _f.trk [tr].grp  &&  tr  &&    // sanity check grp w GOT prv & drum-ness
        ( (MemCm (sndName, CC("Drum"), 4) ? 0 : 1) ==
          (TDrm (tr-1) ? 1 : 0)) )
        {d = _f.trk [tr-1].dev;   c = _f.trk [tr-1].chn;}
   else {
   // list all output device,dtypes we got (skippin OFF,dead) in order we got em
      d = 0;
      while (Midi.GetPos ('o', d++, buf, ty, xs, dv))
         if (StrCm (ty, CC("OFF")) && (*dv != '?'))
            {StrCp (dLst [nD][0], buf);   StrCp (dLst [nD++][1], ty);}
      if (nD == 0)
Die (CC("Ain't no MIDI output devices?\nConnect one and run MidiCfg."));
   // if got arg dev, hop it to list's top
      if (devName)
         for (i = 0;  i < nD;  i++)  if (! StrCm (dLst [i][0], devName)) {
            StrCp  (ty, dLst [i][1]);  // swap to top
            RecDel (dLst, nD--, sizeof (dLst [0]), i);
            RecIns (dLst, ++nD, sizeof (dLst [0]), 0);
            StrCp  (dLst [0][0], devName);   StrCp (dLst [0][1], ty);
            break;
         }
   // look at each devtype for an exact sound match;  then again for mapped snd
      for (cOk = false, x = 0;  (! cOk) && (x <  2);  x++)
                   for (i = 0;  (! cOk) && (i < nD);  i++) {
TRC("  i=`d/`d x=`d/2 `s.`s", i, nD, x, dLst [i][0], dLst [i][1]);
         dt.Open (dLst [i][1]);
         if (! MemCm (sndName, CC("Drum/"), 5))
              {if (dt._nDr)  {cOk = true;   d = i;   c = 9;}}
         else {                        // melodic need a new chan + snd match x
            nc = StrCm (dv, CC("syn")) ? 16 : 128;
            MemSet (cmap, '.', nc);   cmap [9] = 'x';   cmap [nc] = '\0';
            for (m = 0;  m < _f.trk.Ln;  m++)
               if ( (m != tr) && (_f.trk [m].dev < Up.dev.Ln) &&
                    Up.dev [_f.trk [m].dev].mo &&
                    (! StrCm (Up.dev [_f.trk [m].dev].mo->Name (),
                              dLst [i][0])) )
                  if (_f.trk [m].chn < nc)  cmap [_f.trk [m].chn] = 'x';
            for (c = 0;  c < nc;  c++)  if (cmap [c] == '.')  break;
            if ( (c < nc) &&
                 (SND_NONE != dt.SndID (sndName, x ? false : true)) )
               {cOk = true;   d = i;}
         }
         dt.Shut ();
      }
      if (! cOk) {                     // NO drum devs?  just use 1st dev
         if (! MemCm (sndName, CC("Drum/"), 5))  {d = 0;   c = 9;}
         else                          // no dice - gotta blow...:(
Die (StrFmt (xs, "PickDev track `d sound `s - out of midi device/channels :(",
tr+1, sndName));
      }
      d = OpenDev (dLst [d][0]);       // HEY!  d changes to pos in Up.dev !!
      if (d == MAX_DEV) {
         return;
      }
   }
   _f.trk [tr].dev = d;   _f.trk [tr].drm = PRG_NONE;
   _f.trk [tr].chn = c;   _f.trk [tr].snd = SND_NONE;
   if (c != 9)  _f.trk [tr].snd =
                          Up.dvt [Up.dev [_f.trk [tr].dev].dvt].SndID (sndName);
TRC("PickDev end: dev=`s.`s.`d snd=`d=`s",
Up.dev [d].mo->Name (), Up.dev [d].mo->Type (), c+1,
_f.trk [tr].snd, (c==9)?"-":sndName);
}


//______________________________________________________________________________
void Song::SetChn (ubyte t)
// send BankLo/BankHi/ProgCh for trk's dev/chn
{ ubyte d = _f.trk [t].dev, c = _f.trk [t].chn;
  ubyt4 s = _f.trk [t].snd, p;
TRC("SetChn tr=`d dv=`d ch=`d", t, d, c+1);
   if (s == SND_NONE)  return;
   if (Up.dev [d].mo->Syn ()) {
      if (c == 9)  return;             // SetBnk already did drum sounds
      for (p = 0;  p < _sySn.Ln;  p++)  if (_sySn [p] == s)  break;
TRC(" syn prog p=`d", p);
      Up.dev [d].mo->Put (c, MC_PROG, p);
      return;
   }
  SnRow *sn = Up.dvt [Up.dev [d].dvt].Snd (s);
TRC(" GM prog=`d=`s bank=`d bnkL=`d",
sn->prog,MProg[sn->prog],sn->bank,sn->bnkL);
   if (sn->bnkL != PRG_NONE) Up.dev [d].mo->Put (c, MC_CC|M_BNKL, sn->bnkL);
   if (sn->bank != PRG_NONE) Up.dev [d].mo->Put (c, MC_CC|M_BANK, sn->bank);
   if (sn->prog != PRG_NONE) Up.dev [d].mo->Put (c, MC_PROG,      sn->prog);
}


void Song::SetChn ()                   // do progch's for ALL song melo chans
{  for (ubyte t = 0;  t < _f.trk.Ln;  t++)  if (! _f.trk [t].grp)  SetChn (t);
//TODO now scan to curr time sending prog evs
}


void Song::SetBnk ()
// tell syn it's (new) sounds
{ ubyte t, sy, mc = 0;                 // trk#, syn device, max chans fer syn
  ubyt4 s, ts;
  TStr  st, snd [256];
TRC("SetBnk");
   for (sy = 0;  sy < Up.dev.Ln;  sy++)
      if (Up.dev [sy].mo && Up.dev [sy].mo->Syn ())  break;
TRC("  got syn dev=`d", sy);
   if (sy >= Up.dev.Ln)  {SetChn ();   return;}      // got no syn so byeee
   for (s = 0;  s < 256;  s++) snd [s][0] = '\0';
   _sySn.Ln = 0;
   for (t = 0;  t < _f.trk.Ln;  t++) {
      if (! Up.dev [_f.trk [t].dev].mo->Syn ())  continue;
      if (_f.trk [t].chn != 9) {
         if (_f.trk [t].chn > mc)  mc = _f.trk [t].chn;
         if ((ts = _f.trk [t].snd) == SND_NONE)  continue;

         for (s = 0;  s < _sySn.Ln;  s++)  if (_sySn [s] == ts)  break;
         if (s >= _sySn.Ln) {
            if (_sySn.Full ())  {Hey (CC("SetBnk  too many sounds fer Syn"));
                                 SetChn ();   return;}
TRC("  new snd  tr=`d sndid=`d pos=`d `s",
t, ts, s, SndName (t));
            StrCp (snd [_sySn.Ln], SndName (t));
            _sySn      [_sySn.Ln++] = ts;
         }
      }
      else {
         if ((ts = _f.trk [t].snd) == SND_NONE)  continue;

         s = _f.trk [t].drm;
         StrCp (snd [128+s], SndName (t));
TRC("  new drm  tr=`d sndid=`d `s `s",
t, ts, MDrm2Str (st, s), SndName (t));
      }
   }
TRC("  nSnd=`d  maxch=`d", _sySn.Ln, mc);
   Sy.LoadSnd (snd, mc);
   SetChn ();                          // redo chan progch biz
}
