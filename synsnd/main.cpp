// SynSnd.cpp  build device/syn/sound.txt from all the dirs of wav files

#include "stv/os.h"
#include "stv/midi.h"


static TStr DirSyn, Snd [63*1024];
static ubyt4       NSnd;
static char        *MGrp [] = {        // group order for melodic(non drum) snds
   CC("Piano"),     CC("Organ"), CC("SynLead"),
   CC("Bass"),
   CC("Guitar"),    CC("SoloStr"), CC("Ensemble"),
   CC("Brass"),     CC("Reed"),    CC("Pipe"),  CC("Ethnic"),
   CC("ChromPerc"), CC("Perc"),
   CC("SynPad"),    CC("SynFX"),   CC("SndFX"),
   CC("x")
};


bool DoDir (void *ptr, char dfx, char *fn)
// list all dirs under .../device/syn/
// other than bank/, Drum/, every dir is a Snd
// Bank/Drum/Grp_Snd_Kit => Drum/Grp_Snd_Kit_Bank
// Bank/Grp_Snd          =>      Grp_Snd_Bank
{ ubyte nsl = 0, ln = StrLn (fn);
  char *p;
  TStr  s, t, bnk, kit;
//DBG("got dfx=`c fn=`s", dfx, fn);
   if ((dfx == 'd') && (ln > StrLn (DirSyn))) {
      p = & fn [StrLn (DirSyn)+1];     // past .../device/syn/
      while ((p = StrCh (p, '/')) != nullptr)  {nsl++;   p++;}
//DBG("nsl=`d", nsl);
      if ( nsl &&                                // not raw bank
           (ln > 5) &&                           // long enough fer anything
           StrCm (& fn [ln-5], "/Drum") &&       // not raw drum dir
           (NSnd < BITS (Snd)) ) {               // got room
         StrCp (s, & fn [StrLn (DirSyn)+1]);
         StrCp (t, s);
        ColSep c (t, 1, '/');
         StrCp (bnk, c.Col [0]);
         StrCp (s,   c.Col [1]);
         StrFmt (Snd [NSnd], "`s_`s", s, bnk);
//TRC("   snd=`s n=`d", Snd[NSnd], NSnd);
         NSnd++;
      }
   }
   return false;  // neva stop
}


int SndCmp (void *p1, void *p2)
// drum vs melo, grp, snd, bank, (kit if drum)
{ char *s1, *s2;
  int   d1, d2, g1, g2, o, n1, n2;
  TStr  b1, b2;
   StrCp (b1, (char *)p1);   s1 = & b1 [0];
   StrCp (b2, (char *)p2);   s2 = & b2 [0];
//DBG("s1=`s s2=`s", s1, s2);
   d1 = MemCm (s1, "Drum/", 5) ? 1 : 0;     // 0=drum vs 1=melo
   d2 = MemCm (s2, "Drum/", 5) ? 1 : 0;
   if (d1 - d2)  return d1-d2;

//DBG("drum same");
   if (d1) {                           // melodic sounds - pretty ez
//DBG("melo");
      for (g1 = 0;  g1 < BITS (MGrp);  g1++)
         if (! MemCm (s1, MGrp [g1], StrLn (MGrp [g1])))  break;
      for (g2 = 0;  g2 < BITS (MGrp);  g2++)
         if (! MemCm (s2, MGrp [g2], StrLn (MGrp [g2])))  break;
      if (g1 - g2)  return g1-g2;                // grp order
//DBG("melo grp same");
      return StrCm (s1, s2);                     // then strcm snd,bnk
//DBG("melo cmp");
   }
                                       // left w Drum/*
   g1 = MemCm (s1, "Drum/x_", 6) ? 0 : 1;        // 1=x_999_b_k, 0=grp_snd_b_k
   g2 = MemCm (s2, "Drum/x_", 6) ? 0 : 1;
   if (g1 - g2)  return g1-g2;
//DBG("Drum/x_ same");

   if (g1 == 1)  return StrCm (s1, s2);          // ez

//DBG(" not x_ w just cmp");
// cmp MDrum pos by drnt pos in MDrum (sorts by dgrp,drnt)
   for (n1 = 0;  n1 < NMDrum;  n1++)
      if (! MemCm (& s1 [10], MDrum [n1].sym, 4))  break;
   for (n2 = 0;  n2 < NMDrum;  n2++)
      if (! MemCm (& s2 [10], MDrum [n2].sym, 4))  break;
   if (n1 - n2)  return n1-n2;         // diff Drum/DGrp_DSnd

//DBG("drm order same so cmp");
   return StrCm (s1, s2);              // else by kit,bnk
}


int main (int arc, char *argv [])
// load;  remap samplesets,drumsets;  sort all sound dirs
{ TStr ts;
  File f;
DBGTH("SynSnd");
TRC("bgn");
   App.Init ();
   StrFmt (DirSyn, "`s/device/syn", App.Path (ts, 'd'));
TRC("DirSyn=`s", DirSyn);

   NSnd = 0;  f.DoDir (DirSyn, NULL, (FDoDirFunc)(& DoDir));
   Sort (Snd, NSnd, sizeof (Snd[0]), SndCmp);

   StrAp (DirSyn, "/sound.txt");
   if (f.Open (DirSyn, "w")) {
      for (ubyt4 i = 0;  i < NSnd;  i++)
         f.Put (StrFmt (ts, "`<40s . . .\n", Snd [i]));
      f.Shut ();
   }
TRC("end");
   return 0;
}
