// txt2song.cpp - convert my "text song" formatte a.txt to a.song

#include "stv/os.h"                    // sigh - midi needs threads needs qt
#include "stv/midi.h"

#define VEL(v)    ((v)*126/9+1)        // turn v of 0-9 into midi velo of 1-127

TStr  FN, SFN,                         // fn of .txt, .song
      FNRats,                          // errors go herez
      SPath, Path,                     // path of pc, .txt dir
      ErrFN;                           // FN of curr .txt file loading
ubyt4 ErrLine;                         // line# of it

ubyte NTrk;
ubyt4  TrkNE [MAX_TRK];
TStr   TMd [MAX_TRK],
       TNm [MAX_TRK],
       TSn [MAX_TRK];
TrkEv  TEv [MAX_TRK][MAX_EVT];

ubyt4 NE;                              // easier syntax :/
TrkEv *E;

ubyte NCtrl;
WStr   Ctrl [128];

ubyte                      NMrk;
struct {TStr s;   ubyt4 t;} Mrk [128];

ubyt2                      NCue;
struct {TStr s;   ubyt4 t;} Cue [1024];

ubyte NSct;
TStr   Sct [128];

ubyte NTSg;
struct {ubyt4 time;  ubyte num, den, sub;  ubyt2 bar;} TSg [128]; // timesig

ubyt4 Time, Dur, DurArtc, TimeSave, DurMax;
bool  Grace, Roll, Chrd;
ubyte Octv, Velo;
char  Scale [12], Artc,           // defaults
      DurSym  [] = "WHQEST612",   // whole, half, ... 64th, 128th, 256th
      ArtcSym [] = "<=>-",        // staccato, portato, leggerio, legato
      NoteSym [] = "C_D_EF_G_A_B";


void Die (const char *msg)  {Die (CC(msg));}
void Die (      char *msg)
{ TStr s, p;
  File f;
   StrCp (p, ErrFN);   Fn2Path (p);   Fn2Path (p);
   StrFmt (s, "`s\n`s line `d\n", msg, & ErrFN [StrLn (p)+1], ErrLine+1);
   f.Save (FNRats, s, StrLn (s));
   exit (99);
}


//------------------------------------------------------------------------------
bool DoDur (char *b, ubyte l)
// are we in grace note mode?
// rolled chord mode?
// look fer optional artic at end else use default artic (Artc)
// build Dur checking for . and 3
// set Dur fer Time, and DurArtc for the notes
{ char *p, artc;
   Dur = 0;  Grace = Roll = false;

// special g grace note "duration"?  set Grace n return
   if ((l == 1) && (*b == 'G'))
      {DurArtc = Dur = 0;  Grace = true;  return true;}

// rolled chord?  set Roll, get following dur
   if (l && (*b == '~'))  {Roll = true;   b++;  l--;}

// check for artic symbol suffix
   artc = b [l-1];  if (StrCh (ArtcSym, artc))  l--;  else artc = Artc;
   if (l == 0)                         return false;

// add up our dur
   while (l) {
      if (! (p = StrCh (DurSym, *b)))  return false;
      DurArtc = M_WHOLE / (1 << (p - DurSym));        b++;  l--;
      if (l && (*b == '.'))  {DurArtc = DurArtc*3/2;  b++;  l--;}    // dotted
      if (l && (*b == '3'))  {DurArtc = DurArtc*2/3;  b++;  l--;}    // triplet
      Dur += DurArtc;
   }
   DurArtc = ((StrCh (ArtcSym, artc) - ArtcSym) + 1) * Dur / 4;
   return true;
}


char *DoNote (char *b, ubyte l)
// eat spaces.  find the space to end Duration.  get duration.  eat spaces.
// handle each note in the cluster:
//   check for velocity,  check fer DrumSym,  else do [Octave]Step[ShFlNat].
//   save if non velocity.  eat spaces.
{ char *p, shrp;
  bool  got, nt;
  ubyte len, step, f, chd [4], nChd, i, rollOfs = 0;
   while (l && (*b == ' '))  {b++; l--;}
   if (l == 0)                         Die ("Missin duration");
   for (len = 0;  len < l;  len++)  if (b [len] == ' ') break;
   if (! DoDur (b, len))               Die ("Bad duration");
   b += len;  l -= len;  while (l && (*b == ' '))  {b++; l--;}
   while (l) {
      got = nt = false;
   // velocity change?
      if ((l >= 2) && (*b == 'V')) {
         step = b [1];
         if ((step < '0') || (step > '9'))  Die ("Bad velocity");
                  got = true;  Velo = VEL (step-'0');  b += 2;  l -= 2;
      }
   // drum?
      if (l >= 4)  for (step = 0;  step < NMDrum;  step++)
         if (! MemCm (MDrum [step].sym, b, 4)) {
            step = MKey (MDrum [step].key);
            nt = got = true;   b += 4;   l -= 4;
            StrCp (TNm [NTrk], "DrumTrack");     // might as well
            StrCp (TSn [NTrk], "Drum/*");
            break;
         }
   // regular octNoteNote...
      if (! got) {
         if ((*b >= '0') && (*b <= '9'))  {Octv = *b - '0';  b++;  l--;}
         if (l == 0)                        Die ("Missin Nt after Oct");
         if (! (p = StrCh (NoteSym, *b)))   Die ("Bad note");
         step = p - NoteSym;                                 b++;  l--;
         if (l && StrCh (CC("%#@"), *b))  {shrp = *b;        b++;  l--;}
         else                              shrp = ' ';
         if (shrp != '%')
            {if (Scale [step] == '#') step++;  if (Scale [step] == '@') step--;}
         if (shrp == '#') step++;    if (shrp == '@') step--;
         step = (Octv+1)*12 + step;  nt = true;
      }
      if (nt) {
      // ok, got dur and step - create the note
         if (NE+2 >= MAX_EVT)          Die ("Hit max events");
         if (Grace) {
            if (Time < (M_WHOLE/32))   Die ("Gracenote can't start bar 1");
            E [NE+0].time = Time-(M_WHOLE/32);
            E [NE+1].time = Time-1;
         }
         else {
            E [NE+0].time = Time;
            E [NE+1].time = Time + DurArtc-1;
            if (Roll) {
               E [NE+0].time += rollOfs;
               rollOfs += (M_WHOLE/64);
            }
         }
         E [NE+0].ctrl = step;  E [NE+0].valu = 0x80|Velo;  E [NE+0].val2 = 0;
         E [NE+1].ctrl = step;  E [NE+1].valu =      Velo;  E [NE+1].val2 = 0;
         if (l && (*b == '!'))  {b++; l--;  E [NE+0].valu = 0xFF;
                                            E [NE+1].valu = 0x7F;}
         NE += 2;
         if (l && (*b == '+')) {       // CHORD !
            b++;   l--;                // default to major, n setup for dom7
            nChd = 2;   chd[0] = 4;   chd[1] = 3;   chd[2] = 3;
            if (l) {
               l--;
               switch (*b++) {
                  case '-': chd[0] = 3;  chd[1] = 4;  break;  // min
                  case '+':              chd[1] = 4;  break;  // aug
                  case '*': chd[0] = 3;               break;  // dim
                  case '6': chd[2] = 2;     // ...n fall thru
                  case '7': nChd = 3;
                     if      (l && (*b == '-'))
                        {chd[0] = 3;  chd[1] = 4;  b++;  l--;}
                     else if (l && (*b == '+'))
                                     {chd[2] = 4;  b++;  l--;}
               }
            }
            if ((NE+2*nChd) >= MAX_EVT)  Die ("Hit max events +");
            for (i = 0;  i < nChd;  i++) {
               step += chd [i];
               E [NE+0].time =                        E [NE-2].time;
               E [NE+0].ctrl = step;  E [NE+0].valu = E [NE-2].valu;
                                      E [NE+0].val2 = 0;
               E [NE+1].time =                        E [NE-1].time;
               E [NE+1].ctrl = step;  E [NE+1].valu = E [NE-1].valu;
                                      E [NE+1].val2 = 0;
               NE += 2;
            }
         }
      }
      while (l && (*b == ' '))  {b++; l--;}
   }
   return nullptr;
}


char *DoCC (char *b)
// try to find Ctrl=Valu...  n pop inna control change event (or special ones)
{ ubyte c, i;
  ubyt2 v, n, d;
  char *p, *p2, *p3;
   if (! (p = StrCh (b, '=')))  Die ("missing =");
   *p++ = '\0';

// special-ish ones...
   if (! StrCm (b, "sound" )) {
      StrCp (TSn [NTrk], p);
      if (! MemCm (p, "drum", 4))
         {StrCp (TNm [NTrk], "DrumTrack");
          StrCp (TSn [NTrk], "Drum/*");}
      return nullptr;
   }
   if (! StrCm (b, "mode"  )) {
      StrCp (TMd [NTrk], p);
      return nullptr;
   }
   if (! StrCm (b, "name"  )) {
      StrCp (TNm [NTrk], p);
      if (! MemCm (p, "drum", 4))
           {StrCp (TNm [NTrk], "DrumTrack");
            StrCp (TSn [NTrk], "Drum/*");}
      else if ( (! StrCm (p, "LH")) ||
                (! StrCm (p, "RH")) )
           {StrCp (& TMd [NTrk][1], p);   TMd [NTrk][0] = '?';}
      return nullptr;
   }
   if (! StrCm (b, "marker"))
      Die ("use section.  not marker.  sorry.");
   if (! StrCm (b, "section")) {
      if (NMrk >= BITS (Mrk))  Die ("tooo many sections");
      if (NMrk && (Time <= Mrk [NMrk-1].t))
         Die ("section's time is BEFORE previously found section - "
               "keep all sections in the same track");
      StrCp (Mrk [NMrk].s, p);   Mrk [NMrk].t = Time;
TStr db;
TRC("mark[`d].s=`s .t=`d=`s",
NMrk, Mrk[NMrk].s, Mrk[NMrk].t, TmS (db,Mrk[NMrk].t));
      NMrk++;
      return nullptr;
   }
   if (! StrCm (b, "cue")) {
      if (NCue >= BITS (Cue))  Die ("tooo many cue lines");
      if (NCue && (Time <= Cue [NCue-1].t))
         Die ("this cue time is BEFORE a prev cue - "
               "keep all !cue=... stuff in the SAME track");
      StrCp (Cue [NCue].s, p);   Cue [NCue].t = Time;
      NCue++;
      return nullptr;
   }

// get control id;  add it to list if new
   for (c = 0;  c < NCtrl;  c++)  if (! StrCm (Ctrl [c], b))  break;
   if (c >= NCtrl) {                   // new dude
      if (StrLn (b) > (sizeof (WStr)-1))
                                 Die ("control name too long");
      if (NCtrl >= BITS (Ctrl))  Die ("too many controls");
      StrCp (Ctrl [NCtrl++], b);
   }

// ok, parse the wierd ones first (tsig, ksig)
   if      (! StrCm (b, "TSig")) {
      if (! (p2 = StrCh (p, '/')))     Die ("TSig Missing /");
      p3 = StrCh (++p2, '/');
      n = (ubyt2) Str2Int (p);
      d = (ubyt2) Str2Int (p2);
      for (i = 0;  i < 8;  i++) if ((1 << i) == d) break;
      if (i >= 8)                      Die ("bad TSig denom");
      v = (i << 8) | n;
      if (p3) {                  // got subbeat?
         i = (ubyte) Str2Int (p3+1);
         if ((i >= 1) && (i <= 8))  v |= ((i-1) << 12);
         else                          Die ("bad TSig subbeat");
      }
      else if (v == 0x0204)      // default 4/4 to 4/4/4
         v = 0x3204;
   }
   else if (! StrCm (b, "KSig")) {
     char *map = CC("b2#b#b2#b#b2");
      v = i = MNt (p);
      if (p [StrLn (p)-1] == 'm')                v |= 0x0100;   // minor
      v |= i;
      if (map [i] != '2')  {if (map [i] == 'b')  v |= 0x8000;}  // b not #
      else                 {if (p [1]   == 'b')  v |= 0x8000;}
   }
   else
      v = (ubyt2) Str2Int (p);         // a regular one

   if (NE >= MAX_EVT)  Die ("tooo many events");
   E [NE].time = Time;   E [NE].ctrl = 0x80 | c;
                         E [NE].valu = (ubyte)(v & 0x00FF);
                         E [NE].val2 = (ubyte)(v >> 8);
   NE++;
   return nullptr;
}


char *DoChord (char *b, ubyte l)
// split line on spaces.  put each word into Cue[] w leading * n bump time by q
{ ubyte len;
  TStr  chd;
   for (;;) {
      while (l && (*b == ' '))  {b++;   l--;}
      if (l   == 0)  break;            // weed thru the spaces

      for (len = 0;  len < l;  len++)  if (b [len] == ' ') break;
      if (len == 0)  break;            // len to next space

      MemCp (chd, b, len);   chd [len] = '\0';
      if (StrCm (chd, "|")) {          // | really does nothin, just legibility
         if (StrCm (chd, "/")) {       // / only bumps Time
            if (NCue >= BITS (Cue))  Die ("tooo many chords");
            Cue [NCue].t     = Time;
            Cue [NCue].s [0] = '*';   StrCp (& Cue [NCue].s [1], chd);
            NCue++;
         }
         Time += M_WHOLE/4;
      }
      b += len;   l -= len;
   }
   return nullptr;
}


//------------------------------------------------------------------------------
void EndTrack ()
// set Trk[] ne n dur;  bump NTrk;  reset E,NE,Time
{  TrkNE [NTrk++] = NE;
   if (Time > DurMax)  DurMax = Time;
   E = & TEv [NTrk][0];   NE = Time = 0;
}


static int SigCmp (void *p1, void *p2)      // just sort by .time
{ ubyt4 t1 = *((ubyt4 *)p1), t2 = *((ubyt4 *)p2);
   return t1 - t2;
}

void TSig ()
// suck tsig ccs outa Ev so we can turn abs ubyt4 time to rel str time
{ ubyte t;
  ubyt4 p;
   NTSg = 0;
   for (t = 0;  t < NTrk;  t++)  for (p = 0;  p < TrkNE [t];  p++)
      if ( (TEv [t][p].ctrl & 0x80) &&
           (! StrCm (Ctrl [TEv [t][p].ctrl & 0x7F], "TSig")) ) {
         TSg [NTSg].time = TEv [t][p].time;
         TSg [NTSg].num  = TEv [t][p].valu;
         TSg [NTSg].den  = 1 << (TEv [t][p].val2 & 0x0F);
         TSg [NTSg].sub  = 1 +  (TEv [t][p].val2 >> 4);
         NTSg++;
      }
   Sort (TSg, NTSg, sizeof (TSg [0]), SigCmp);

// set TSg[].bar
   if (NTSg)  TSg [0].bar = (ubyt2)(1 + TSg [0].time / M_WHOLE);     // 4/4
   for (p = 1;  p < NTSg;  p++)  TSg [p].bar =
      (ubyt2)(TSg [p-1].bar + (TSg [p].time - TSg [p-1].time) /
                              (M_WHOLE / TSg [p-1].den * TSg [p-1].num));
}

char *TmSt (char *str, ubyt4 tm)
// time str relative to tsigs
{ ubyt4 dBr, dBt, bx;
  ubyt2 s = 0, br, bt;
   while (((ubyt4)s+1 < NTSg) && (TSg [s+1].time <= tm))  s++;
   if ((s >= NTSg) || (TSg [s].time > tm)) {
      dBt = M_WHOLE / 4;               // none apply yet - use 4/4/1
      dBr = dBt     * 4;
      if ((dBr == 0) || (dBt == 0))  DBG("TmSt a dBr=0 or dBt=0");
      br  = (ubyt2)(1 + (tm / dBr));
      bt  = (ubyt2)(1 + (tm % dBr) / dBt);
      bx  =             (tm % dBr) % dBt;
   }
   else {
      dBt = M_WHOLE / TSg [s].den;
      dBr = dBt     * TSg [s].num;
      if ((dBr == 0) || (dBt == 0))  DBG("TmSt b dBr=0 or dBt=0");
      br  = (ubyt2)(TSg [s].bar + (tm - TSg [s].time) / dBr);
      bt  = (ubyt2)(1 +          ((tm - TSg [s].time) % dBr) / dBt);
      bx  =                      ((tm - TSg [s].time) % dBr) % dBt;
   }
   if      (br > 9999)  StrCp  (str, "9999      ");
   else if ((bt == 1) && (bx == 0))
                        StrFmt (str, "`04d      ",   br);
   else if (bx == 0)    StrFmt (str, "`04d.`d    ",  br, bt);
   else                 StrFmt (str, "`04d.`d.`03d", br, bt, bx);
   return str;
}


TrkEv Ev [MAX_EVT];                    // ...whatev

void Put ()
// write .song, unrollin' sections if we got em
{ File  f;
  ubyte t, i, j, c;
  ubyt4 t1, t2, p, p1, ne;
  TStr  s, s1, s2, SB, to, lFN;
  ubyt4 st [128];                     // section's start time
  TrkEv *e;
  StrArr ly ("lyric.txt", 16000, 6000*sizeof(TStr));
   TSig ();

// get lyric.txt if any
   StrCp (lFN, FN);   Fn2Path (lFN);   StrAp (lFN, "/lyric.txt");
   ly.Load (lFN);

   if (! f.Open (SFN, "w"))  {DBG("couldn't write .song", SFN);   exit (99);}
   f.Put ("Track:\n");
   for (t = 0;  t < NTrk;  t++) {
      f.Put (StrFmt (s, ".  `s  .`s  `s\n",
         TSn [t][0] ? TSn [t] : "Piano_AcousticGrand",
         TMd [t], TNm [t]));
TRC("t=`d nEv=`d", t+1, TrkNE [t]);
   }
TRC("NSct=`d", NSct);
   if (NSct) {
      f.Put ("Lyric:\n");
      for (i = 0;  i < ly.NRow ();  i++) {
         StrCp (s2, ly.Get (i));   if (StrLn (s2) < 6)  continue;
         if (StrCh (CC("!?*"), s2 [5]))                    // escape em
            {StrCp (& s2 [6], & s2 [5]);  s2 [5] = '_';}
         f.Put (StrFmt (s, "`s/\n", s2));
      }
      Time = 0;
      for (i = 0;  i < NSct;  i++) {   // set st[] to time o Sct[]
         StrCp (s1, Sct [i]);
         for (j = 0;  j < NMrk;  j++)  if (! StrCm (Mrk [j].s, s1))  break;
         t1 = Mrk [j].t;   t2 = (j+1 < NMrk) ? Mrk [j+1].t : DurMax;
         st [i] = Time;   Time += (t2-t1);
         f.Put (StrFmt (s, "`s ?(`s\n", TmSt (s2, st [i]), s1));
      }
      for (i = 0;  i < NCue;  i++)
         f.Put (StrFmt (s, "`s ?`s\n", TmSt (s2, Cue [i].t), Cue [i].s));

      f.Put ("Event:\n");
      for (t = 0;  t < NTrk;  t++) {
         for (Time = 0, i = 0;  i < NSct;  i++) {
            for (j = 0;  j < NMrk;  j++)
               if (! StrCm (Mrk [j].s, Sct [i]))  break;
            t1 = Mrk [j].t;   t2 = (j+1 < NMrk) ? Mrk [j+1].t : DurMax;
            p1 = MAX_EVT+9;
            for (ne = p = 0;  p < TrkNE [t];  p++)
               if ((TEv [t][p].time >= t1) && (TEv [t][p].time < t2))
                  {ne++;   if (p < p1) p1 = p;}
            MemCp (Ev, & TEv [t][p1], ne * sizeof (Ev [0]));
            for (p = 0;  p < ne;  p++)
               {Ev [p].time -= t1;  Ev [p].time += Time;}

            for (e = Ev;  ne--;  e++) {
               f.Put (StrFmt (SB, "`s ", TmSt (s, e->time)));
               c = e->ctrl;
               if (c & 0x0080) {
                  StrCp (s, Ctrl [c & 0x7F]);
               // tmpo,tsig,ksig,prog get str values
                  if      (! StrCm (s, "prog"))
                     f.Put ("!Prog=*");
                  else if (! StrCm (s, "tmpo"))
                     f.Put (StrFmt (SB,
                             "!Tmpo=`d", e->valu | (e->val2<<8)));
                  else if (! StrCm (s, "tsig")) {
                     f.Put (StrFmt (SB,
                             "!TSig=`d/`d", e->valu, 1 << (e->val2 & 0x0F)));
                     if (e->val2 >> 4)
                        f.Put (StrFmt (SB, "/`d", 1 + (e->val2 >> 4)));
                  }
                  else if (! StrCm (s, "ksig")) {
                     f.Put ("!KSig=");
                     if   (! (e->val2 & 0x80))
                           StrCp (SB, MKeyStr  [e->valu]);
                     else if (e->valu != 11)
                           StrCp (SB, MKeyStrB [e->valu]);
                     else  StrCp (SB, "Cb");     // cuz B / Cb are WEIRD
                     if (e->val2 & 0x01)  StrAp (SB, "m");
                     *SB = CHUP (*SB);
                     f.Put (SB);
                  }
                  else {
                     f.Put (StrFmt (SB, "!`s=`d", s, e->valu));
                     if (e->val2)  f.Put (StrFmt (SB, " `d", e->val2));
                  }
               }
               else                          // note
                  f.Put (StrFmt (SB, "`s`c`d",
                     (! StrCm (TSn [t], "Drum/*")) ? MDrm2Str (s, c)
                                                   : MKey2Str (s, c),
                     (e->valu & 0x0080) ? ((e->val2 & 0x80) ? '~' : '_')
                                        : '^',  e->valu & 0x007F));
               f.Put ("\n");
            }
            Time += (t2 - t1);
         }
         f.Put (StrFmt (SB, "EndTrack `d #ev=`d\n", t+1, TrkNE [t]));
      }
   }
   else {
      if (NCue || NMrk || ly.NRow ())  f.Put ("Lyric:\n");
      for (i = 0;  i < ly.NRow ();  i++) {
         StrCp (s2, ly.Get (i));   if (StrLn (s2) < 6)  continue;
         if (StrCh (CC("!?*"), s2 [5]))                    // escape em
            {StrCp (& s2 [6], & s2 [5]);  s2 [5] = '_';}
         f.Put (StrFmt (s, "`s/\n", s2));
      }
      for (ubyt2 x = 0;  x < NCue;  x++)
         f.Put (StrFmt (s, "`s ?`s\n", TmSt (s2, Cue [x].t), Cue [x].s));
      for (ubyt2 x = 0;  x < NMrk;  x++) {
         StrCp (s1, Mrk [x].s);
         f.Put (StrFmt (s, "`s ?(`s\n", TmSt (s2, Mrk [x].t), s1));
      }
      f.Put ("Event:\n");
      for (t = 0;  t < NTrk;  t++) {
         for (e = & TEv [t][0], ne = TrkNE [t];  ne--;  e++) {
            f.Put (StrFmt (SB, "`s ", TmSt (s, e->time)));
            c = e->ctrl;
            if (c & 0x0080) {             // ctrl
               StrCp (s, Ctrl [c & 0x7F]);

            // tmpo,tsig,ksig,prog get str values
               if      (! StrCm (s, "prog"))
                  f.Put ("!Prog=*");
               else if (! StrCm (s, "tmpo"))
                  f.Put (StrFmt (SB,
                         "!Tmpo=`d", e->valu | (e->val2<<8)));
               else if (! StrCm (s, "tsig")) {
                  f.Put (StrFmt (SB,
                         "!TSig=`d/`d", e->valu, 1 << (e->val2 & 0x0F)));
                  if (e->val2 >> 4)
                     f.Put (StrFmt (SB, "/`d", 1 + (e->val2 >> 4)));
               }
               else if (! StrCm (s, "ksig")) {
                  f.Put ("!KSig=");
                  if   (! (e->val2 & 0x80))  StrCp (SB, MKeyStr  [e->valu]);
                  else if (e->valu != 11)    StrCp (SB, MKeyStrB [e->valu]);
                  else // Cb is weird :/ */  StrCp (SB, "Cb");
                  if (e->val2 & 0x01)  StrAp (SB, "m");
                  *SB = CHUP (*SB);
                  f.Put (SB);
               }
               else {
                  f.Put (StrFmt (SB, "!`s=`d", s, e->valu));
                  if (e->val2)  f.Put (StrFmt (SB, " `d", e->val2));
               }
            }
            else {                        // note
               f.Put (StrFmt (SB, "`s`c`d",
                  (! StrCm (TSn [t], "Drum/*")) ? MDrm2Str (s, c)
                                                : MKey2Str (s, c),
                  (e->valu & 0x0080) ? ((e->val2 & 0x80) ? '~' : '_')
                                     : '^',  e->valu & 0x007F));
            }
            f.Put ("\n");
         }
         f.Put (StrFmt (SB, "EndTrack `d #ev=`d\n", t+1, TrkNE [t]));
      }
   }
   f.Shut ();
}


//------------------------------------------------------------------------------
char *DoLine (char *b, ubyt2 l, ubyt4 line, void *ptr);  // forward decl

void DoFile (char *ifn)
// do file in current or Clip dir
{ File  f;
  char *msg;
  TStr  fn, pFn;
  ubyt4     pLn;
   StrCp (pFn, ErrFN);   pLn = ErrLine;
   StrFmt (fn, "`s/`s.txt", Path, ifn);
   StrCp (ErrFN, fn);    ErrLine = 0;
   if ((f.DoText (ErrFN, nullptr, DoLine))) {
      StrFmt (fn, "`s/clip/`s.txt", SPath, ifn);
      StrCp (ErrFN, fn);   ErrLine = 0;
      if ((msg = f.DoText (ErrFN, nullptr, DoLine)))  Die (msg);
   }
   StrCp (ErrFN, pFn);   ErrLine = pLn;
}


//------------------------------------------------------------------------------
char *DoLine (char *b, ubyt2 l, ubyt4 line, void *ptr)
// else do note clusters (sep'd by commas) n bump Time
{ ubyte i;
  bool  time1 = true, comma;
  ubyt4 dur1, n;
  char *m, *p;
  static ubyte unr = 0;
   (void)ptr;
   ErrLine = line;
//DBG("FN=`s line=`d NE=`d Time=`d", ErrFN, ErrLine, NE, Time);
   if (! StrCm (b, "unroll"))  {unr = 1;  return nullptr;}
   if (unr) {
      if (NSct >= BITS (Sct))
                      Die ("too many sections in unroll");
      for (i = 0;  i < NMrk;  i++)  if (! StrCm (Mrk [i].s, b))  break;
      if (i >= NMrk)  Die ("unknown section in unroll");
      StrCp (Sct [NSct++], b);
      return nullptr;
   }

// comment / empty line? - scram
   if ((*b == '-') || (l == 0))  return nullptr;

// chord on/off/line?
   if (! StrCm (b, "chord"   ))  {Chrd = true;   TimeSave = Time;
                                      return nullptr;}
   if (! StrCm (b, "chordend"))  {Chrd = false;  Time = TimeSave;
                                      return nullptr;}
   if (Chrd)  return DoChord (b, (ubyte)l);

// EndTrack?
   if (StrCm (b, "NextTrack") == 0)  {EndTrack ();  return nullptr;}

// include?
   if (*b == '#') {
      n = 1;                           // default to one rep
      if ((p = StrCh (b, ' ')))  {n = Str2Int (p+1);  *p = '\0';}
      while (n--)  DoFile (b+1);
      return nullptr;
   }

// CC?
   if (*b == '!')  return DoCC (b+1);

   for (i = 0;  i < l;  i++)  b [i] = CHUP (b [i]);

// default scale/artic?
   if (*b == '$')  {
      if (l < 8)             Die ("Bad scale");
      if (l > 8) {
         if (! StrCh (ArtcSym, b [8]))
                             Die ("Bad default Artic");
         Artc = b [8];
      }
      Scale[0] = b[1];  Scale[2] = b[2];  Scale[4] = b[3];
      Scale[5] = b[4];  Scale[7] = b[5];  Scale[9] = b[6];  Scale[11] = b[7];
      return nullptr;
   }

// do note clusters sep'd by commas n bump Time
   do {
      for (comma = false,
           i = 0;  i < l;  i++)  if (b [i] == ',')  {comma = true;  break;}
      b [i] = '\0';
      if ((m = DoNote (b, i++)))  return m;
      if (time1)  {dur1 = Dur;  time1 = false;}
      b += i;  l -= i;
   } while (comma);
   Time += dur1;
   return nullptr;
}


//------------------------------------------------------------------------------
int main (int argc, char *argv [])
// load .txt file n save in .song format
{ char *msg;
  File  f;
DBGTH("Txt2Song");
   App.Init ();
TRC("arg=`s", argv [1]);
   if (argc < 2)  return 99;

   StrCp (FN, argv [1]);
   if (f.Size (FN) == 0)  {DBG(".txt file is empty? `s", FN);   exit (99);}

// get .song filename into SFN
   StrCp (SFN, FN);   Fn2Path (SFN);   StrAp (SFN, "/a.song");

// init globals
   NE = Time = 0;   E = & TEv [0][0];
   Octv = 4;   Velo = VEL(7);   Artc = '>';
   MemCp (Scale, "c_d_ef_g_a_b", 12);
   StrCp (Path,  FN);   Fn2Path (Path);     // Path of current dir
   StrFmt (FNRats, "`s/RATS.txt", Path);   f.Kill (FNRats);

   StrCp (ErrFN, FN);   ErrLine = 0;        // FN we're loadin n line#
   App.Path (SPath, 'd');                   // path to pianocheetah dir

// read top .txt file line by line n do each track
   if ((msg = f.DoText (FN, nullptr, DoLine)))  Die (msg);

   Put ();                             // write it alll out into a.song
TRC("end");
   return 0;
}
