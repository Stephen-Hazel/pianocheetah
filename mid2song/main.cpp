// mid2song.cpp - convert a .mid file to .song file

#include "stv/os.h"
#include "stv/midi.h"

// need a "broader" TrkEv in here;  .prog not stamped till PutSong
struct EvRow {ubyt4 time;  ubyt2 ctrl;  ubyte valu, val2;       // .x fer tmpo
              ubyt2 chan, trak;  ubyt4 prog;};

struct TrRow {TStr dev, snd, name;  ubyt4 dur, eFr, eTo;};

// chan:  device(0..127)<<4|chan;  prog: prog<<16|bnkH<<8|bnkL;  trak in TrRow
struct PrRow {ubyt2 chan, trak;  ubyt4 prog, time;};

struct TxRow {ubyt4 time;  TStr s;};                  // text evs in time

struct StRow {ubyt2 trak, chan, hand, chn2, pr1;      // track stats
              ubyt4 prog, bgn, end, nT, nN, nC;};

struct CcRow {ubyt2 raw;  ubyte pos;  WStr str;};

struct TsRow {ubyt4 time;  ubyte num, den, sub;  ubyt2 bar;}; // timesig


//------------------------------------------------------------------------------
char  HCX;                             // y if marker HCXSetting* found else \0

ubyte Mid [1024*1024];                 // mem to hold the whole .mid file
ubyt4 MidLn, MidP;                     // len of buf and pos we're parsin' at

File  Fs;                              // song file we're writin
TStr  FN, SB, TMS;                     // .mid filename, buf, time buf

WStr
 Desc [] = {
   "","Text","Copyright","Track","Instrument",
      "Lyric","Marker","Cue","Sound","Device",
      "Text10","Text11","Text12","Text13","Text14","Text15"
 },
 KyStr [2][2][8] = {
   {{"x", "G", "D", "A", "E", "B", "F#","C#"},   // M#
    {"C", "F", "Bb","Eb","Ab","Db","Gb","Cb"}},  // Mb
   {{"x", "E", "B", "F#","C#","G#","D#","A#"},   // m#
    {"A", "D", "G", "C", "F", "Bb","Eb","Ab"}}   // mb
 },
 KyNum [2][2][8] = {
   {{"0C", "0G", "0D", "0A", "0E", "0B", "0F#","0C#"},   // M#
    {"0C", "0F", "0Bb","0Eb","0Ab","0Db","0Gb","0Cb"}},  // Mb
   {{"0A", "0E", "0B", "0Gb","0C#","0G#","0D#","0A#"},   // m#
    {"0A", "0D", "0G", "0C", "0F", "0Bb","0Eb","0Ab"}}   // mb
 };

class Song {
public:
   Song ()  {NDv = 0;}
   void CvtMid ();

   Arr<EvRow,MAX_EVT> Ev;              // buf w parsed midi events
   Arr<TrRow,MAX_TRK> Tr;              // tracks
   Arr<PrRow,16384>   Pr;              // program change info
   TStr               Dv [MAX_DEV];   ubyte NDv;
   Arr<TxRow,MAX_LYR> Ly;   char LyCmp;     // buf w lyric events
   Arr<StRow,1000>    St;              // "statistic" track/chan/prog/etc info
   Arr<TsRow,1000>    Ts;              // timesigs to turn abs time to TmSt
private:
   void  Trk2Ev  (ubyte tr, ubyt2 res, ubyt4 l);
   void  TSig    ();
   char *TmSt    (char *ts, ubyt4 tm);
   void  KSig    ();
   void  PutSong ();
   void  Dump    (bool ev = false);
};

Song *S;

void Ugh (char *s)
{ TStr fn, t;
  File f;
   if (Fs.IsOpen ())  Fs.Shut ();
   if (S)   delete S;
   StrCp (fn, FN);   Fn2Path (fn);   StrAp (fn, CC("/a.song"));   Fs.Kill (fn);
   StrAp (fn, CC("RATS.txt"), 6);
   f.Save (fn, StrFmt (t, "`s\nfn=`s\n", s, FN), StrLn (t));
DBG("mid2song error=`s", s);
   exit (99);
}


// track readin dudes ----------------------------------------------------------
void GetByt (ubyt4 *p, ubyt4 l, ubyte *it)
{ static TStr err;
   if (*p >= l)
      Ugh (StrFmt (err,
         "EOF in GetByt :(  pos=`d=$`08x end=`d for `s",
         MidP+*p, MidP+*p, MidP+l, FN));
   *it = Mid [MidP + *p];  (*p)++;
}

void GetVar (ubyt4 *p, ubyt4 l, ubyt4 *it)
// Read oneuh those wierd variable length numbers...
{ ubyt4 val = 0, len = 0;
  ubyte b;
  static TStr err;
   do {
      if (*p >= l)
         Ugh (StrFmt (err,
            "EOF in VarLen param :(  pos=`d=$`08x end=`d for `s",
            MidP+*p, MidP+*p, MidP+l, FN));
      val = (val << 7) | ((b = Mid [MidP + (*p)++]) & 0x7F);
      if (++len > 5)                   // len5 is ok cuz 7 bits*4+4 bits=> 32bit
         Ugh (StrFmt (err,
            "Bad VarLen param :(  pos=`d=$`08x end=`d vlen=`d for `s",
            MidP+*p, MidP+*p, MidP+l, len, FN));
   } while (b & 0x80);
   *it = val;
}

void Skip (ubyt4 *p, ubyt4 l, ubyt4 skp)
{ static TStr err;
   if ((*p += skp) >= l)
      Ugh (StrFmt (err,
         "EOF in Skip :(  pos=`d=$`08x end=`d skp=`d for `s",
         MidP+*p, MidP+*p, MidP+l, skp, FN));
}

void FixStrLy (ubyte *s, ubyte l)
{ ubyt4 c;                             // \n-ish         => /
   s [l] = '\0';                       // space/nonascii => _
   for (c = 0; c < l; c++) {
      if ((s [c] == '@') || (s [c] == '\r') || (s [c] == '\n') ||
          (s [c] == '/') || (s [c] == '\\'))  s [c] = '/';
      if ((s [c] <= ' ') || (s [c] > '~'))    s [c] = '_';
   }
}

void FixStr1 (ubyte *s, ubyte l)
{ ubyt4 c;
   s [l] = '\0';
   for (c = 0; c < l; c++)  if ((s [c] < ' ') || (s [c] > '~'))  s [c] = '_';
}

void FixStr2 (ubyte *s, ubyte l)
{ ubyt4 c;
   for (c = 0; c < l; c++)  if (s [c] == ' ')  s [c] = '_';
   if (l)  {l--;  while (l && (s [l] == '_')) l--;  s [l+1] = '\0';}
}


//------------------------------------------------------------------------------
void Song::Trk2Ev (ubyte tr, ubyt2 res, ubyt4 l)
// load a midi track into e and print text info, etc in song file
{ ubyt4 p;                      // parse pos (offset from MidP)
  ubyt4 delta, ftime, time;     // delta => filetime*res => time
  ubyte pStat;                  // midi running status byte
  TStr  s, trknm, chop;
  char  ptrknm, gotKar = 0;
  ubyt4 hmmlen, hmmgot, i, j, prog [256*16], ep, pp, lp;
  ubyte c, tc, minr, flat, regParm [256*16][5], chopln;
  sbyte num;
  ubyt2 chan;
  static ubyt4 pLyrTm = 0;
TRC("{ Trk2Ev tr=`d res=`d trPos=`d trLen=`d", tr, res, MidP, l);
// remember track name and which kind of name it was (to pick best one)
   trknm [0] = '\0';   ptrknm = 0;

// init filetime and running status
   ftime = 0;   pStat = 0;   chan = 0;   MemSet (prog, 0, sizeof (prog));
   MemSet (regParm, 0, sizeof (regParm));

   for (p = 0;  p < l;) {
      GetVar (& p, l, & delta);
      ftime += delta;  time = (ftime * 192) / res;
      GetByt (& p, l, & c);
//DBG("`02d `s <=delta=`d byte=$`02x - pos=`d=$`08x (p=`d l=`d)",
//tr+1, TmS(TMS,time), delta, c,  MidP+p, MidP+p, p, l);
      if (c == 0xFF) {
      // parse those damn midifile "meta" events
      // 0=SeqNo, 1=Text, 2=Copyright, 3=SeqName/TrackName, 4=Instrument,
      // 5=Lyric, 6=Marker, 7=CuePoint, 8=Program(Patch), 9=Device(Port),
      // $20=cakewalkChan, $21=cakewalkPort,
      // $2F=EndOfTrack, $51=Tempo, $54=SMPTEOffset, $58=TimeSig, $59=KeySig
         pStat = 0;
         GetByt (& p, l, & c);       // NOW c is the meta type
         GetVar (& p, l, & hmmlen);
         if ((p + hmmlen) > l)
            Ugh (CC("Trk2Ev  EOF readin a $FF??"));
         switch (c) {
            case 0:   //...sequence number
               if (hmmlen == 2)
{TRC("`02d `s: SeqNo=`d",
 tr+1, TmS (TMS, time), Mid [MidP+p]<<8 | Mid [MidP+p+1]);}
               else
{TRC("`02d `s: SeqNo=(here)", tr+1, TmS (TMS, time));}
               break;
            case 5:   //...LYRIC text event
               hmmgot = (hmmlen > MAXTSTR) ? MAXTSTR : hmmlen;
               MemCp (s, & Mid [MidP+p], hmmgot);
               FixStrLy ((ubyte *) s, (ubyte) hmmgot);
 TRC("`02d `s: ly=`s cmp=`b", tr+1, TmS (TMS, time), s, LyCmp);
               if (LyCmp)        break;     // already got track of lyrics
               if (*s == '\0')   break;     // skip cr,lf or hold
               if (Ly.Full ())   break;     // got room?
               lp = Ly.Ins ();         // starts w our flags?  esc w _
               if (StrCh (CC("!?*"), *s))  {StrCp (& s [1], s);  *s = '_';}
               if (time && (time == pLyrTm))  time++;      // SHOISH
               Ly [lp].time = time;   StrCp (Ly [lp].s, s);
               pLyrTm = time;
               break;
            case 6:   //...MARKER text event (put it as cue - ?marker)
               hmmgot = (hmmlen > MAXTSTR) ? MAXTSTR : hmmlen;
               MemCp (s, & Mid [MidP+p], hmmgot);
               FixStrLy ((ubyte *) s, (ubyte) hmmgot);
               if (*s == '\0')   StrCp (s, CC("-"));
               if (Ly.Full ())  break;           // got room?
               lp = Ly.Ins ();
               StrCp (& s [1], s);  *s = '?';
               Ly [lp].time = time;   StrCp (Ly [lp].s, s);
               if (StrSt (s, CC("HCXSetting")))  HCX = 'y';
               break;
            case 9:   //...DevName text event
               hmmgot = (hmmlen > MAXTSTR) ? MAXTSTR : hmmlen;
               MemCp (s, & Mid [MidP+p], hmmgot);
               s [hmmgot] = '\0';
               if (*s == '\0')   StrCp (s, CC("00"));
               for (i = 0;  i < NDv;  i++)
                  if (! StrCm (s, Dv [i]))  break;
               if (i >= NDv) {
                  if (NDv >= BITS (Dv))  Ugh (CC("Trk2Ev  too many devices"));
                  StrCp (Dv [NDv++], s);
               }
               chan = (ubyt2)((i << 4) | (chan & 0x0F));
TRC("`02d `s: Device=`s  (port=`d chan=`d)",
tr+1, TmS (TMS, time), s, i+1, (chan & 0x0F)+1);
               break;
            case 1:   case 2:   case 3:   case 4:   case 7:   case 8:
            case 10:  case 11:  case 12:  case 13:  case 14:
            case 15:  //...Text,Copyright,Seq/TrackName,Instrument,
                      //   CuePoint,Program(Patch),10-15 text events
               hmmgot = (hmmlen > MAXTSTR) ? MAXTSTR : hmmlen;
               MemCp (s, & Mid [MidP+p], hmmgot);
               FixStr1 ((ubyte *) s, (ubyte) hmmgot);
               if ((c == 3) && (StrCm (s, CC("Words"), 'x') == 0))  gotKar = 1;
               if ((c == 1) && gotKar) {
                  FixStrLy ((ubyte *) s, (ubyte) hmmgot);
                  if (LyCmp)       break;   // already got track of lyrics
                  if (*s == '\0')  break;   // skip cr,lf or hold
                  if (Ly.Full ())  break;
                  lp = Ly.Ins ();
                  if (StrCh (CC("!?*"), *s))  {StrCp (& s [1], s);  *s = '_';}
                  Ly [lp].time = time;   StrCp (Ly [lp].s, s);
                  break;
               }
               for (chopln = 0;
                    chopln < (int)(StrLn (s) / 120 +
                                  (StrLn (s) % 120 ? 1 : 0));
                    chopln++) {
                  MemCp (chop, & s [120*chopln], 120);  chop [120] = '\0';
                  if      (c == 1)
                     StrFmt (SB, "`02d `s: `s",
                        tr+1, TmS (TMS, time),            chop);
                  else if (c != 3)
                     StrFmt (SB, "`02d `s: `s=`s",
                        tr+1, TmS (TMS, time), Desc [c],  chop);
                  else *SB = '\0';     // no need to show track 2x
TRC(SB);
               }
               FixStr2 ((ubyte *) s, (ubyte) hmmgot);
            // prefer track, then instr, then text for track's name
               switch (c) {                                        // track
                  case 3:      StrCp (trknm, s);  ptrknm = c;   break;
                  case 4:  if  (ptrknm != 3)                       // instr
                              {StrCp (trknm, s);  ptrknm = c;}  break;
                  case 1:  if ((ptrknm != 3) && (ptrknm != 4))     // text
                              {StrCp (trknm, s);  ptrknm = c;}  break;
                  default: if ((ptrknm != 3) && (ptrknm != 4) &&
                               (ptrknm != 1) && (ptrknm == 0))
                              {StrCp (trknm, s);  ptrknm = c;}
               }
               break;
            case 0x20: // INVALID(yet used) midi channel prefix (chan LSB)
                       // (chan LSB: for sysx, meta events)
               if (hmmlen == 1) {
                  i = Mid [MidP+p];  chan = (ubyt2)((chan & 0xFFF0) | i);
TRC("`02d `s: Chan_Cakewalk=`d  (port=`d chan=`d)",
tr+1, TmS (TMS, time), i+1, (chan>>4)+1, (chan & 0x0F)+1);
               }
               else
TRC("`02d `s: Chan_Cakewalk=??", tr+1, TmS (TMS, time));
               break;
            case 0x21: // INVALID(yet used) midi port prefix    (chan MSB)
               if (hmmlen == 1) {
                  i = Mid [MidP+p];  chan = (ubyt2)((i << 4) | (chan & 0x0F));
TRC("`02d `s: Port_Cakewalk=`d  (port=`d chan=`d)",
tr+1, TmS (TMS, time), i+1, (chan>>4)+1, (chan & 0x0F)+1);
               }
               else
TRC("`02d `s: Port_Cakewalk=??", tr+1, TmS (TMS, time));
               break;
            case 0x2F: //...End track - end time is set (REQUIRED)
TRC("`02d `s: EndTrack", tr+1, TmS (TMS, time));
               Tr [tr].dur = time;
               break;
            case 0x51: //...Tempo
            {  i = Mid [MidP+p]<<16 | Mid [MidP+p+1]<<8 | Mid [MidP+p+2];
               j =      60000000 / i;  // bpm
              ubyt4 n = 60000000 % i;  // frac bpm is n / i
//            ubyte x = (ubyte)(256*n/i);   if ((256*n) >= (i/2))  x++;
               if (n >= (i/2))  j++;
TRC("`02d `s: Tempo=`d BPM (`d uSec/Quarter)",   // fracBPM=`d(`d/256)",
tr+1, TmS (TMS,time), j, i);                     //, x, (x>127)?(127-x):x);
               for (ep = 0;  ep < Ev.Ln;  ep++)
                  if ((Ev [ep].time == time) && (Ev [ep].ctrl == MC_TMPO))
                     break;            // no dup tmpo per time - use last
               if (ep >= Ev.Ln)  ep = Ev.Ins ();
               Ev [ep].time = time;
               Ev [ep].ctrl = MC_TMPO;
               Ev [ep].valu = (ubyte)( j       & 0x00FF);
               Ev [ep].val2 = (ubyte)((j >> 8) & 0x00FF);
//             Ev [ep].x    = x;
               Ev [ep].trak = tr;
               Ev [ep].chan = chan;
               break;
            }
            case 0x54: //...SMPTE offset
TRC("`02d `s: SMPTEOfs=`02d:`02d:`02d.`02d.`02d",
tr+1, TmS (TMS, time), Mid [MidP+p+0], Mid [MidP+p+1], Mid [MidP+p+2],
Mid [MidP+p+3], Mid [MidP+p+4]);
               break;
            case 0x58: //...TimeSignature
TRC("`02d `s: TimeSig=`d/`d (`d clocks/metronone, "
     "`d notated 32nd notes/24 clocks)",
tr+1, TmS (TMS, time), Mid [MidP+p+0], 1 << Mid [MidP+p+1],
                       Mid [MidP+p+2],      Mid [MidP+p+3]);
               for (ep = 0;  ep < Ev.Ln;  ep++)
                  if ((Ev [ep].time == time) && (Ev [ep].ctrl == MC_TSIG))
                     break;            // no dup tsig per time - use last
               if (ep >= Ev.Ln)  ep = Ev.Ins ();
               Ev [ep].time = time;
               Ev [ep].ctrl = MC_TSIG;
               Ev [ep].valu = Mid [MidP+p+0];
               Ev [ep].val2 = Mid [MidP+p+1];
               if ((Ev [ep].valu == 4) && (Ev [ep].val2 == 2))
                  Ev [ep].val2 |= (3 << 4);      // for 4/4, default subbeat=4
               Ev [ep].trak = tr;
               Ev [ep].chan = chan;
               break;
            case 0x59: //...KeySignature
               num = (sbyte) Mid [MidP+p];
               if (num <= 0)  {num = -num;  flat = 1;}  else flat = 0;
               minr = Mid [MidP+p+1] ? 1 : 0;
TRC("`02d `s: KeySig=`s `s (`d `s)",
tr+1, TmS (TMS, time), KyStr [minr][flat][(ubyte)num],
minr ? "Minor" : "Major", (int)num, flat ? "Flats" : "Sharps");
               for (ep = 0;  ep < Ev.Ln;  ep++)
                  if ((Ev [ep].time == time) && (Ev [ep].ctrl == MC_KSIG))
                     break;            // no dup ksig per time - use last
               if (ep >= Ev.Ln)  ep = Ev.Ins ();
               Ev [ep].time = time;
               Ev [ep].ctrl = MC_KSIG;
               Ev [ep].valu = MKey (KyNum [minr][flat][(ubyte)num]) % 12;
               Ev [ep].val2 =             (minr?0x01:0) | (flat?0x80:0);
               Ev [ep].trak = tr;
               Ev [ep].chan = chan;
               break;
            case 0x7F:
               break;
            default:
TRC("`02d `s: $FF`02x=(Len `d) ", tr+1, TmS (TMS, time), (int)c, hmmlen);
               for (i = 0;  (i < hmmlen) && (i < 16);  i++)
TRC("`02x ", (int) Mid [MidP+p+i]);
               for (i = 0;  (i < hmmlen) && (i < 16);  i++) {
                  tc = Mid [MidP+p+i];
TRC("`c", ((tc<' ') || (tc>'~')) ? '.' : tc);
               }
         }
         p += hmmlen;
      }
      else if ((c == M_SYSX) || (c == M_EOX)) {  // ...toss sysex
//TODO handle mastervolume,masterbalance?
      // F0,7F,7F,04,01,LSB,MSB,F7(bal=02)
         pStat = 0;
         GetVar (& p, l, & hmmlen);
         Skip   (& p, l,   hmmlen);
TRC("`02d `s: SysEx of `d bytes tossed", tr+1, TmS (TMS, time), hmmlen);
      }
      else if (c > M_SYSX) {                     // ...toss realtime msg
         pStat = 0;
TRC("`02d SysRealtime Cmd $`02x tossed", tr+1, (int)c);
         if (c == M_SONGPOS)  Skip (& p, l, 2);
         if (c == M_SONGSEL)  Skip (& p, l, 1);
      }
      else {
         if (c & 0x80)  {pStat = c;   GetByt (& p, l, & c);}
         chan = (chan & 0xFFF0) | (pStat & 0x0F);
         ep = Ev.Ins ();               // assume we're savin somethin;  gotta
         Ev [ep].time = time;          // Del it if not !!
         Ev [ep].ctrl = c;
         Ev [ep].trak = tr;
         Ev [ep].chan = chan;
//TRC(" MIDI time=`d ctrl=`02x trak=`d chan=`04x",time, c, tr, chan);
         switch (pStat & 0xF0) {
            case M_NOTE: GetByt (& p, l, & tc);
                         Ev [ep].valu = tc ? (0x80 | tc) : 64;
                                            // valu of 0 means noteoff w vel=64
                         break;
            case M_NPRS: GetByt (& p, l, & tc);
                         Ev [ep].valu =       0x80 | tc;
                         Ev [ep].val2 = 0x80;
                         break;
            case M_NOFF: GetByt (& p, l, & tc);
                         Ev [ep].valu =              tc;
                         break;
            case M_PROG:
TRC("`02d `s: ProgChange Chan=`d $`02x=`s bnk=$`04x",
tr+1, TmS (TMS, time), chan+1, c, MProg [c], prog [chan] & 0x0000FFFF);
                         if ((chan & 0x0F) == 9)      // kill drum ProgCh's
                            {Ev.Del (ep);   break;}
                         if (Pr.Full ())
                            Ugh (CC("Trk2Ev  hit max program changes"));
                         prog [chan] = (prog [chan] & 0x0000FFFF) | (c << 16);
                         pp = Pr.Ins ();
                         Pr [pp].chan = chan;
                         Pr [pp].prog = prog [chan];
                         Pr [pp].trak = tr;
                         Pr [pp].time = time;
                         Ev [ep].ctrl = MC_PROG;
                         break;
            case M_PRSS: Ev [ep].ctrl = MC_PRSS;
                         Ev [ep].valu = c;
                         break;
            case M_PBND: Ev [ep].ctrl = MC_PBND;
                         GetByt (& p, l, & tc);
                         Ev [ep].valu = tc;
                         Ev [ep].val2 = c;
                         break;
            case M_CTRL: GetByt (& p, l, & tc);
                         Ev [ep].valu = tc;
                      // bankMLB,LSB only stored in prog[], not Ev[]
                         if      (c == M_BANK) {             // Bank
TRC("`02d `s: BankHi Chan=`d $`02x", tr+1, TmS (TMS, time), chan+1, c);
                            if ((chan & 0x0F) != 9)          // no drum ProgCh's
                               prog [chan] = (prog [chan] & 0x00FF00FF) |
                                                                       (c << 8);
                            Ev.Del (ep);
                            break;
                         }
                         else if (c == M_BNKL) {             // BankLo
TRC("`02d `s: BankLo Chan=`d $`02x", tr+1, TmS (TMS, time), chan+1, c);
                            if ((chan & 0x0F) != 9)          // no drum ProgCh's
                               prog [chan] = (prog [chan] & 0x00FFFF00) | c;
                            Ev.Del (ep);
                            break;
                         }
                      // rpn,nrpn are (only) a mode to pick REAL ctrl#
                         else if ((c >= M_NRPNL) && (c <= M_RPNH)) {
                            regParm [chan][c-M_NRPNL] = tc;
                            regParm [chan][4] = (c >= M_RPNL) ? 1 : 0;
                            Ev.Del (ep);
                            break;
                         }
                         else if ((c == M_DATH) || (c == M_DATL)) {
                           ubyt2 cno;
                            if (regParm [chan][4]) {  // rp
                               cno = regParm [chan][2] + regParm [chan][3]*128;
                               if (cno >= 8192)  break;
                               Ev [ep].ctrl |= MC_RP;
                            }
                            else {                    // np
                               cno = regParm [chan][0] + regParm [chan][1]*128;
                               if (cno >= 16382)  break;
                               Ev [ep].ctrl |= MC_NP;
                            }
                            Ev [ep].ctrl |= (cno << 1) |
                                            ((c == M_DATL) ? 1 : 0);
                            break;
                         }
                         else {
//TRC("  cc `02x", c);
                            Ev [ep].ctrl = MC_CC | c;
                         }
                         break;
            default:     Ugh (StrFmt (SB,
"Trk2Ev tr=`02d tm=`s pos=`d badMIDIevent=$`02x`02x",
tr+1, TmS (TMS, time), MidP+p, (int) pStat, (int) c));
         }
         if (Ev.Full ()) Ugh (CC("Trk2Ev  Hit max output events :("));
      }
   }
   StrCp (Tr [tr].name, trknm);
   if (Ly.Ln && (! LyCmp))  LyCmp = 'y';         // only want 1 track of lyrs
TRC("} Trk2Ev");
}


//------------------------------------------------------------------------------
static int SigCmp (void *p1, void *p2)      // just sort by .time
{ ubyt4 t1 = *((ubyt4 *)p1), t2 = *((ubyt4 *)p2);
   return t1 - t2;
}

void Song::TSig ()
// suck tsig ccs outa ev so we can turn abs ubyt4 time to rel str time
{ ubyt4 e, p, tm, bard;
   Ts.Ln = 0;
   for (e = 0;  e < Ev.Ln;  e++)  if (Ev [e].ctrl == MC_TSIG) {
      p = Ts.Ins ();
      Ts [p].time = Ev [e].time;
      Ts [p].num  = Ev [e].valu;
      Ts [p].den  = 1 << (Ev [e].val2 & 0x0F);
      Ts [p].sub  = 1 +  (Ev [e].val2 >> 4);
   }
   Sort (Ts.Ptr (), Ts.Ln, Ts.Siz (), SigCmp);

// make sure TSig times are on bar boundaries (for safety)
   tm = 0;   bard = M_WHOLE;
   for (p = 0;  p < Ts.Ln;  p++) {     // trunc to bar dur
      tm = Ts [p].time = ((Ts [p].time - tm) / bard * bard) + tm;
      bard = M_WHOLE * Ts [p].num / Ts [p].den;
   }

// set Ts[].bar
   if (Ts.Ln)  Ts [0].bar = (ubyt2)(1 + Ts [0].time / M_WHOLE);      // 4/4
   for (p = 1;  p < Ts.Ln;  p++)  Ts [p].bar =
      (ubyt2)(Ts [p-1].bar +
      (Ts [p].time - Ts [p-1].time) / (M_WHOLE / Ts [p-1].den * Ts [p-1].num));

}

char *Song::TmSt (char *str, ubyt4 tm)
// time str relative to tsigs
{ ubyt4 dBr, dBt, bx;
  ubyt2 s = 0, br, bt;
   while (((ubyt4)s+1 < Ts.Ln) && (Ts [s+1].time <= tm))  s++;
   if ((s >= Ts.Ln) || (Ts [s].time > tm)) {
      dBt = M_WHOLE / 4;               // none apply yet - use 4/4/1
      dBr = dBt     * 4;
      br  = (ubyt2)(1 + (tm / dBr));
      bt  = (ubyt2)(1 + (tm % dBr) / dBt);
      bx  =             (tm % dBr) % dBt;
   }
   else {
      dBt = M_WHOLE / Ts [s].den;
      dBr = dBt     * Ts [s].num;
      br  = (ubyt2)(Ts [s].bar + (tm - Ts [s].time) / dBr);
      bt  = (ubyt2)(1 +         ((tm - Ts [s].time) % dBr) / dBt);
      bx  =                     ((tm - Ts [s].time) % dBr) % dBt;
   }
   if      (br > 9999)  StrCp  (str, CC("9999      "));
   else if ((bt == 1) && (bx == 0))
                        StrFmt (str, "`04d      ",   br);
   else if (bx == 0)    StrFmt (str, "`04d.`d    ",  br, bt);
   else                 StrFmt (str, "`04d.`d.`03d", br, bt, bx);
   return str;
}


//------------------------------------------------------------------------------
ubyte flSh [12][2] = {                 // #/b lookup by key,min
   {'b', 'b'},       // c maj min
   {'b', '#'},       // c# ...
   {'#', 'b'},
   {'b', 'b'},
   {'#', '#'},
   {'b', 'b'},
   {'b', '#'},
   {'#', 'b'},
   {'b', '#'},
   {'#', 'b'},
   {'b', 'b'},
   {'#', '#'}
};
/* *Ma = "C   (0b)\0"                  "Db  (5b  also C# below)\0"
         "D   (2#)\0"                  "Eb  (3b)\0"
         "E   (4#)\0"                  "F   (1b)\0"
         "Gb  (6b  also F# below)\0"   "G   (1#)\0"
         "Ab  (4b)\0"                  "A   (3#)\0"
         "Bb  (2b)\0"                  "B   (5#  also Cb below)\0"
   *Mi = "A   (0b)\0"                  "Bb  (5b  also A# below)\0"
         "B   (2#)\0"                  "C   (3b)\0"
         "C#  (4#)\0"                  "D   (1b)\0"
         "Eb  (6b  also D# below)\0"   "E   (1#)\0"
         "F   (4b)\0"                  "F#  (3#)\0"
         "G   (2b)\0"                  "G#  (5#  also Ab below)\0"
*/
void Song::KSig ()
// if we have none or just C, use notes to guess at REAL ksig
{ ubyt4 i, e, ksp = 0x7FFFFFFF, nnt [12], got, mx, maj;
  ubyte v, v2;
   MemSet (nnt, 0, sizeof (nnt));
   for (e = 0;  e < Ev.Ln;  e++) {     // find 1st ksig ev
      if ( (ksp == 0x7FFFFFFF) && (Ev [e].ctrl == MC_KSIG) )  ksp = e;
                                       // count nondrum note down pitches
      if ( ((Ev [e].chan & 0x0F) != 9) && (! (Ev [e].ctrl & 0xFF80)) &&
            (Ev [e].valu & 0x80)       && (! (Ev [e].val2 & 0x80)) )
         nnt [Ev [e].ctrl % 12]++;
   }
//DBG("nnt: `d `d `d `d `d `d `d `d `d `d `d `d",
//nnt[0],nnt[1],nnt[2],nnt[3],nnt[4],nnt[5],
//nnt[6],nnt[7],nnt[8],nnt[9],nnt[10],nnt[11]);
   for (got = 0, i = 0;  i < 7;  i++) {     // mark top 7 notes in got
      for (mx = 12, e = 0;  e < 12;  e++) {
         if ( ((mx == 12) || (nnt [e] > nnt [mx])) &&
              (! ((1 << e) & got)) )  mx = e;
      }
      if (mx < 12)  got |= (1 << mx);
   }
   maj = 0x0AB5;                       // ksig pattern for c maj / a min
   for (i = 0;  i < 12;  i++) {        // (backwards wwhwwwh)
      if (got == maj)  break;
      maj = ((maj << 1) & 0x0FFF) | ((maj & 0x0800) ? 1 : 0);   // next key
   }
   if (i >= 12)  return;               // rats, couldn't guess it :(

// ok, it's either i maj or (i+9) min depending on leading tone of i+11 / i+8
   v2 = (nnt [(i+11) % 12] > nnt [(i+8) % 12]) ? 0 : 1;    // set min/maj bit
   if (v2)  i = (i+9) % 12;            // minor so pick relative minor tonic
   v2 |= ((flSh [i][v2] == 'b') ? 0x80 : 0);
   v   = (ubyte)i;

// upd else ins it
   if (ksp != 0x7FFFFFFF) {            // upd guess if WAS c maj(0 80 = C b maj)
      if ((Ev [ksp].valu == 0) && (Ev [ksp].val2 == 0x0080)) {
         Ev [ksp].valu = v;
         Ev [ksp].val2 = v2;
TRC("upd ksig nt=`d flt=`s min=`s", v, (v2 & 0x80) ? "y":"n",
                                       (v2 &    1) ? "y":"n");
      }
   }
   else {                              // ins our ksig guess
      e = Ev.Ins ();
      Ev [e].time = 0;
      Ev [e].ctrl = MC_KSIG;
      Ev [e].valu = v;   Ev [e].val2 = v2;   Ev [e].chan = 9;
                         Ev [e].trak = 0;    Ev [e].prog = 0;
TRC("ins ksig nt=`d flt=`s min=`s", v, (v2 & 0x80) ? "y":"n",
                                       (v2 &    1) ? "y":"n");
   }
}


#ifdef DBG_ON

void Song::Dump (bool ev)
{ ubyt4 p;
  char  dbg [800];
  TStr  tmp;
DBG("Dump");
   if (ev) {
      DBG("Event: Tr Time $Chan $Prog Ev __________");
      for (p = 0; p < Ev.Ln; p++) {
         StrFmt (dbg, "`08d: `02d `s `02x `06x ",
                 p, Ev [p].trak, TmS (TMS, Ev [p].time), Ev [p].chan,
                                                         Ev [p].prog);
         if (Ev [p].ctrl & 0xFF80)                              // control
              DBG("`s`s(`d)=$`02x,$`02x",
                    dbg,  MCtl2Str (tmp, Ev [p].ctrl, 'r'), Ev [p].ctrl,
                                               Ev [p].valu, Ev [p].val2);
         else DBG("`s`<4s `s `d",      // note
                    dbg,  MKey2Str (tmp, (ubyte)Ev [p].ctrl),
                    (Ev [p].valu & 0x80) ? "Dn" : "Up",  Ev [p].valu & 0x007F);
      }
   }
   DBG("ProgChange: $Chan Time Trak $Prog __________");
   for (p = 0;  p < Pr.Ln;  p++)
      DBG("`02d: `02x `s `02d `06x",
           p, Pr [p].chan, TmS (TMS, Pr [p].time), Pr [p].trak, Pr [p].prog);
   DBG("Stat: trak $chan $prog  bgn end  hand $pr1 $chn2  __________");
   for (p = 0;  p < St.Ln;  p++)
      DBG("`02d: `02d `02x `06x  `>8d `>8d  `d `02x `d  nT=`d nN=`d nC=`d",
           p, St [p].trak, St [p].chan, St [p].prog,
           St [p].bgn, St [p].end,
           St [p].hand, St [p].pr1, St [p].chn2,
           St [p].nT, St [p].nN, St [p].nC);
   DBG("Track: len name dev snd __________");
   for (p = 0;  p < Tr.Ln;  p++)
      DBG("`02d: `s `s `s `s",
           p, TmS (TMS, Tr [p].dur), Tr [p].name, Tr [p].dev, Tr [p].snd);
   if (! ev)  {DBG("} Dump");   return;}
DBG("Dump end");
}
#endif

int PrCmp (void *p1, void *p2)  // ...Pr sortin: chan,time,trak
{ int t;
  PrRow *i1 = (PrRow *)p1, *i2 = (PrRow *)p2;
   if ((t = i1->chan - i2->chan))  return t;
   if ((t = i1->time - i2->time))  return t;
        t = i1->trak - i2->trak;   return t;
}

int EvCmp (void *p1, void *p2)  // ...Ev sortin: chan,prog,trak,time,
{ int t;                                     // ctrlHiBit-DESC,ctrlLo7,valu,val2
  EvRow *e1 = (EvRow *)p1, *e2 = (EvRow *)p2;
   if ((t = e1->chan - e2->chan))  return t;
   if ((t = e1->prog - e2->prog))  return t;
   if ((t = e1->trak - e2->trak))  return t;
   if ((t = e1->time - e2->time))  return t;
   if ((t = (e2->ctrl & 0x80) - (e1->ctrl & 0x80)))  return t;
   if ((t = e1->ctrl - e2->ctrl))  return t;
   if ((t = e1->valu - e2->valu))  return t;
        t = e1->val2 - e2->val2;   return t;
}

int StCmp (void *p1, void *p2)  // ...St: drum,chan,bgnTime,trak,endTime
{ int t, d1, d2;
  StRow *s1 = (StRow *)p1, *s2 = (StRow *)p2;
   d1 = (s1->chan == 9) ? 1 : 0;
   d2 = (s2->chan == 9) ? 1 : 0;
   if ((t = d1 - d2))              return t;
   if ((t = s1->chan - s2->chan))  return t;
   if ((t = S->Ev [s1->bgn].time - S->Ev [s2->bgn].time))  return t;
   if ((t = s1->trak - s2->trak))  return t;
   return S->Ev [s1->end].time - S->Ev [s2->end].time;
}

int Grp [16] = {                       // sound group order
    0, // Piano
    2, // Organ
    1, // ChromPerc
   10, // SynLead
   11, // SynPad
    3, // Guitar
    5, // SoloStr
    6, // Ensemble
    7, // Brass
    8, // Reed
    9, // Pipe
   13, // Ethnic
   12, // SynFX
    4, // Bass
   14, // Perc
   15  // SndFX
};

int StCmp2 (void *p1, void *p2)        // drum,hand,pr1 grp order,chan,chn2
{ int t, g1, g2;
  StRow *s1 = (StRow *)p1, *s2 = (StRow *)p2;
   g1 = ((s1->chan & 0x0F) == 9) ? 1 : 0;
   g2 = ((s2->chan & 0x0F) == 9) ? 1 : 0;
   if ((t = g1 - g2))  return t;
   if ((t = s1->hand - s2->hand))  return t;
   for (g1 = 0; g1 < (int)BITS (Grp); g1++)
      if ((s1->pr1 >> 3) == Grp [g1])  break;
   for (g2 = 0; g2 < (int)BITS (Grp); g2++)
      if ((s2->pr1 >> 3) == Grp [g2])  break;
   if ((t = g1  - g2))             return t;
   if ((t = s1->pr1  - s2->pr1 ))  return t;
   if ((t = s1->chan - s2->chan))  return t;
   return   s1->chn2 - s2->chn2;
}


ubyt4 UnGS (ubyt4 c)
// got GS drums but song files use only XG (cuz it's the superset) so convert
{ TStr s;
   for (ubyte i = 0;  i < NMDrum;  i++)
      if (! StrCm (MDrum [i].gs, MKey2Str (s, (ubyte)c, 'b')))
         return MKey (MDrum [i].key);
   return c;
}


//------------------------------------------------------------------------------
void Song::PutSong ()  // ...FROM HELLLLL !!  clean up the TERRIBLE midi format
{ ubyt4 e, s, s1, c;                   // into somethin we can DEAL with
  ubyte tr;
  ubyt2 t, dd,  dtrk [128];  // per dev, store drum trak (tr# will ALWAYS be >0)
  bool  mt, gs = false;
  char  dr;
  TStr  ts, tm, snnm;
  File  tf;
TRC("PutSong bgn");
   TSig ();                            // organize TimeSig cc events
   KSig ();                            //          KeySig
   Sort (Pr.Ptr (), Pr.Ln, Pr.Siz (), PrCmp);    // by chan,time,trak

#ifdef DBG_ON
DBG("dump#1"); Dump(false);//Dump(true);
#endif
// create DrumTrack trak for each dev that has drum notes
   dr = 'n';
   MemSet (dtrk, 0, sizeof (dtrk));
   for (e = 0;  e < Ev.Ln;  e++)
      if ( ((Ev [e].chan & 0x0F) == 9) && (dtrk [Ev [e].chan >> 4] == 0) ) {
      // got drum ev, on dev we haven't seen yet
         if (Tr.Full ())  Ugh (CC("PutSong  too many tracks to add drum trk"));
         tr = (ubyte)Tr.Ins ();        // init new end Tr[].name,dur
         StrCp (Tr [tr].name, CC("DrumTrack"));   Tr [tr].dur = 0;
         dtrk [Ev [e].chan >> 4] = tr;      // mark trk# for a given dev
         dr = 'y';                          // got =A= trk to jam tmpo,tsig into
      }
// no drums used?  still make a dev=0 drumtrack
   if (dr == 'n') {
      if (Tr.Full ())     Ugh (CC("PutSong  TOO many tracks to add tmpo trk"));
      tr = (ubyte)Tr.Ins ();
      StrCp (Tr [tr].name, CC("DrumTrack"));   Tr [tr].dur = 0;
      dtrk [0] = tr;
   }
// get 1st drum dev that has stuff
   for (dd = 0; dd < BITS (dtrk); dd++)  if (dtrk [dd])  break;

   for (e = 0;  e < Ev.Ln;  e++) {
   // move tmpo,tsig,ksig ctrls (nontrack) to DrumTrack for dev 0
      if      ((Ev [e].ctrl == MC_TMPO) || (Ev [e].ctrl == MC_TSIG) ||
                                           (Ev [e].ctrl == MC_KSIG))
         {Ev [e].trak = dtrk [dd];   Ev [e].chan = (dd << 4) | 9;}

   // move any drum events to dev's DrumTrack n tweak it's .dur (hoakily?)
      else if ((Ev [e].chan & 0x0F) == 9) {
      // these only exist in RolandGS, else use "drumgs.txt" check to pick
         if ((Ev [e].ctrl >= MKey (CC("6Db"))) &&
             (Ev [e].ctrl <= MKey (CC("6E"))))
            gs = true;
         Ev [e].trak = t = dtrk [Ev [e].chan >> 4];
         if (Tr [t].dur < Ev [e].time) {
             Tr [t].dur = Ev [e].time;
            if (Tr [t].dur % (M_WHOLE/4))
                Tr [t].dur += ((M_WHOLE/4) - Tr [t].dur % (M_WHOLE/4));
         }
      }
   // stamp prog into each nondrum Ev[]
      else {
         for (c = 0;  c < Pr.Ln;  c++)  if (Ev [e].chan == Pr [c].chan)  break;
         if (c < Pr.Ln) {              // else it's already init'd to 0
            while ( (c+1 < Pr.Ln) && (Ev [e].chan == Pr [c+1].chan) &&
                                     (Ev [e].time >= Pr [c+1].time) )  c++;
            Ev [e].prog = Pr [c].prog;
         }
      }
   }
   StrCp (ts, FN);   Fn2Path (ts);   StrAp (ts, CC("/drumgs.txt"));
   if (tf.Size (ts))  gs = true;       // gotta convert GS to standard XG sigh

// by chan,prog,trak,time,ctrlHiBit-DESC,ctrlLo7,valu,val2
   Sort (Ev.Ptr (), Ev.Ln, Ev.Siz (), EvCmp);
#ifdef DBG_ON
DBG("dump#2"); Dump(false); //Dump(true);
#endif

// make an index of Ev in St
   for (e = 0;  e < Ev.Ln;  e++) {
      for (s = 0;  s < St.Ln;  s++)
         if ((Ev [e].chan == St [s].chan) && (Ev [e].prog == St [s].prog) &&
             (Ev [e].trak == St [s].trak))
            break;
      if (s >= St.Ln) {                // new guy
         if (St.Full ())  Ugh (CC("PutSong  tooo many St entries :("));
         St.Ins ();
         St [s].chan = Ev [e].chan;
         St [s].prog = Ev [e].prog;
         St [s].trak = Ev [e].trak;
         St [s].bgn  = St [s].end = e;
         St [s].nN   = St [s].nC  = St [s].nT = 0;
      }
      if     ((Ev [e].ctrl == MC_TMPO) || (Ev [e].ctrl == MC_TSIG) ||
                                          (Ev [e].ctrl == MC_KSIG)) St [s].nT++;
      else if (Ev [e].ctrl & 0xFF80)                                St [s].nC++;
      else                                                          St [s].nN++;
      if (e > St [s].end)  St [s].end = e;
   }

// so St[] goes per chan,time  puttin 1st progch to kill off at top
   Sort (St.Ptr (), St.Ln, St.Siz (), StCmp);    // by chan,bgnTime,trak,endTime

// lose 1st MC_PROG of chan
   for (s = 0;  s < St.Ln;  s++) {
      for (e = St [s].bgn;  e <= St [s].end;  e++)
         if (Ev [e].ctrl == MC_PROG) {      // got it!  del it, bump end n break
            Ev.Del (e);
            for (s1 = 0;  s1 < St.Ln;  s1++)
               if ((s1 != s) && (St [s1].bgn >= e))
                  {St [s1].bgn--;   St [s1].end--;}
            if (St [s].end > e)  St [s].end--;
            St [s].nC--;
            break;
         }
   // if more tracks w same chan, skip em
      while ((s+1 < St.Ln) && (St [s+1].chan == St [s].chan))  s++;
   }

// toss any St[] entries that are completely empty or
// if a full chan has no notes or tmpo/tsig/ksig events, kill all it's St[] recs
   for (s = 0;  s < St.Ln;) {
      if (St [s].nN || St [s].nC || St [s].nT) {
         s1 = s;  c = St [s].chan;
         mt = (St [s].nN || St [s].nT) ? false : true;
         while ((s+1 < St.Ln) && (St [s+1].chan == c))
            {s++;  if (St [s].nN || St [s].nT)  mt = false;}
         if (mt)       {St.Del (s1, s+1-s1);   s = s1;}
         else           s++;
      }
      else
         {St.Del (s);   s++;}
   }
#ifdef DBG_ON
DBG("dump#3"); Dump(false);
#endif

// preserve existing order within chan via chn2
// mark hand and sort full chans on progch group order next
   for (s = 0;  s < St.Ln;  s++) {
      St [s].hand = 0;                 // default to no hand
      t = St [s].trak;   StrCp (ts, Tr [t].name);     // got LH/RH ?
      if ((St [s].chan & 0x0F) == 9)  ;               // drum
      else {
         if (HCX) {                    // from home concert xtreme?
            if      (St [s].chan == 3)  St [s].hand = 1;   // RH
            else if (St [s].chan == 2)  St [s].hand = 2;   // LH
         }
         else {
            if ( (StrSt (ts, CC("RH")) && (! StrSt (ts, CC("RHY")))) ||
                 StrSt (ts, CC("right")) )  St [s].hand = 1;
            if ( StrSt (ts, CC("LH")) ||
                 StrSt (ts, CC("left" )) )  St [s].hand = 2;
         }
      }
      if ((s == 0) || (St [s-1].chan != St [s].chan)) {
         St [s].chn2 = 1;
         St [s].pr1  = St [s].prog >> 16;
      }
      else {                           // w/in chan, keep order dup hand,pr1
         St [s].chn2 = St [s-1].chn2 + 1;
         St [s].pr1  = St [s-1].pr1;
         if ((! St [s].hand) && St [s-1].hand)  St [s].hand = St [s-1].hand;
      }
   }
   Sort (St.Ptr (), St.Ln, St.Siz (), StCmp2);
                                       // by drum,hand,pr1 grp,chan,chan2
#ifdef DBG_ON
DBG("dump#4"); Dump(false);
#endif

// WHEW!  cleanup DONE!  now...
// build .song file given St [], etc, etc
   Fs.Put (FN);   Fs.Put (CC("\n"));
/*
** Fs.Put ("...tracks in\n");
** for (t = 0;  t < Tr.Ln;  t++)
**    Fs.Put (StrFmt (ts, "`02d `s\n", t+1, Tr [t].name));
** Fs.Put ("...tracks out\n"
**         "##  hand  chan  sound  itrack  nT  nC  nN\n");
** for (s = 0;  s < St.Ln;  s++)
**    Fs.Put (StrFmt (ts,
**       "`02d  `d  `d  `s.`04d  `02d`s  `d  `d  `d\n",
**       s+1, St [s].hand, St [s].chan+1, MProg [St [s].prog >> 16],
**       St [s].prog & 0x0FFFF, St [s].trak+1, Tr [St [s].trak].name,
**       St [s].nT, St [s].nC, St [s].nN));
*/
   Fs.Put (CC("Track:\n"));
   for (s = 0;  s < St.Ln;  s++) {
      t = St [s].trak;
      if      (St [s].hand == 1) StrCp (tm, CC(".?RH"));
      else if (St [s].hand == 2) StrCp (tm, CC(".?LH"));
      else                       StrCp (tm, CC(".SH"));
      StrCp (ts, Tr [t].name);
      if (s && (St [s].chan == St [s-1].chan))  *tm = '+';
      ts [40] = '\0';                  // trim to 40 chars MAX
      while (StrLn (ts) && (ts [StrLn (ts)-1] == ' '))  StrAp (ts, CC(""), 1);
      if ((St [s].chan & 0x0F) == 9)  StrCp (snnm, CC("Drum/*"));
      else                            StrCp (snnm, MProg [St [s].prog >> 16]);
      if (St [s].prog & 0x00FFFF)
         StrFmt (& snnm [StrLn (snnm)], ".`03d`03d",
            (St [s].prog & 0x00FF00) >> 8, St [s].prog & 0x0000FF);
      Fs.Put (StrFmt (SB, ".  `s  `s  `s#`s\n", snnm, tm, ts, snnm));
TRC("SB=`s", SB);
   }
   if (Ly.Ln) {                        // tack any Lyrics onto .song file
      Fs.Put (CC("Lyric:\n"));
      for (e = 0;  e < Ly.Ln;  e++)
         Fs.Put (StrFmt (SB, "`s `s\n", TmSt (TMS, Ly [e].time), Ly [e].s));
   }
   { StrArr ly (CC("lyric.txt"), 16000, 6000*sizeof(TStr));
     TStr   lFN, s, s2;
   // get lyric.txt if any
      StrCp (lFN, FN);   Fn2Path (lFN);   StrAp (lFN, CC("/lyric.txt"));
      ly.Load (lFN);
      if (ly.NRow () && (Ly.Ln == 0))  Fs.Put (CC("Lyric:\n"));
      for (ubyt4 i = 0;  i < ly.NRow ();  i++) {
         StrCp (s2, ly.Get (i));   if (StrLn (s2) < 6)  continue;
         Fs.Put (StrFmt (s, "`s/\n", s2));
      }
   }
   Fs.Put (CC("Event:\n"));            // main event data
   for (s = 0;  s < St.Ln;  s++) {
      if ((St [s].chan & 0x0F) == 9)  StrCp (snnm, CC("Drum/*"));
      else                            StrCp (snnm, MProg [St [s].prog >> 16]);
      for (e = St [s].bgn;  e <= St [s].end;  e++) {
         Fs.Put (StrFmt (SB, "`s ", TmSt (TMS, Ev [e].time)));
         c = Ev [e].ctrl;
         if (c & 0xFF80) {             // ctrl
           ubyt4 i;
            for (i = 0;  i < NMCC;  i++)  if (MCC [i].raw == c)
                              {StrCp (ts, MCC [i].s);   break;}
            if (i >= NMCC)  MCtl2Str (ts, (ubyt2)c);
         // tmpo,tsig,ksig,prog get str values
            if      (! StrCm (ts, CC("prog")))
               Fs.Put (CC("!Prog=*"));
            else if (! StrCm (ts, CC("tmpo")))
               Fs.Put (StrFmt (SB,
                       "!Tmpo=`d", Ev [e].valu | (Ev [e].val2<<8)));
            else if (! StrCm (ts, CC("tsig"))) {
               Fs.Put (StrFmt (SB,
                       "!TSig=`d/`d", Ev [e].valu, 1 << (Ev [e].val2 & 0x0F)));
               if (Ev [e].val2 >> 4)
                  Fs.Put (StrFmt (SB, "/`d", 1 + (Ev [e].val2 >> 4)));
            }
            else if (! StrCm (ts, CC("ksig"))) {
               Fs.Put (CC("!KSig="));
               if   (! (Ev [e].val2 & 0x80))
                     StrCp (SB, MKeyStr  [Ev [e].valu]);
               else if (Ev [e].valu != 11)
                     StrCp (SB, MKeyStrB [Ev [e].valu]);
               else  StrCp (SB, CC("Cb"));      // cuz B / Cb are WEIRD
               if (Ev [e].val2 & 0x01)  StrAp (SB, CC("m"));
               *SB = CHUP (*SB);
               Fs.Put (SB);
            }
            else {
               Fs.Put (StrFmt (SB, "!`s=`d", ts, Ev [e].valu));
               if (Ev [e].val2)  Fs.Put (StrFmt (SB, " `d", Ev [e].val2));
            }
         }
         else {                        // note
            if ( gs && ((St [s].chan & 0x0F) == 9) )  c = UnGS (c);
            Fs.Put (StrFmt (SB, "`s`c`d",
               ((St [s].chan & 0x0F) == 9) ? MDrm2Str (ts, (ubyte)c)
                                           : MKey2Str (ts, (ubyte)c),
               (Ev [e].valu & 0x0080) ? ((Ev [e].val2 & 0x80) ? '~' : '_')
                                      : '^',  Ev [e].valu & 0x007F));
         }
         Fs.Put (CC("\n"));
      }
      Fs.Put (StrFmt (SB, "EndTrack `d #ev=`d\n",
                      s+1, St [s].end-St [s].bgn+1));
   }
TRC("PutSong end");
}


//------------------------------------------------------------------------------
void Song::CvtMid ()
// parse the whole midi file, then reparse and write out .song n .trak files
{ ubyt4 tlen;                // len isn't really PART of the header info
  ubyte tr;
  struct {ubyt4 len;  ubyt2 fmt, ntrk, res;} MThd;
   if (StrCm ((char *)Mid, CC("MThd"), 'x'))  Ugh (CC("CvtMid  Bad .mid hdr"));
   MThd.len  = Mid [4]<<24 | Mid [5]<<16 | Mid [6]<<8 | Mid [7];
   MThd.fmt  = Mid [8] <<8 | Mid [9];
   MThd.ntrk = Mid [10]<<8 | Mid [11];
   MThd.res  = Mid [12]<<8 | Mid [13];
TRC("CvtMid nTrk=`d", MThd.ntrk);
   if (MidLn < (MidP = 8 + MThd.len))     Ugh (CC("CvtMid  No .mid hdr"));
   for (;  Tr.Ln < MThd.ntrk;  MidP += tlen) {
      if (StrCm ((char *)(& Mid [MidP]), CC("MTrk"), 'x'))
                                          Ugh (CC("CvtMid  Bad trk hdr"));
      tlen = Mid [MidP+4]<<24 | Mid [MidP+5]<<16 | Mid [MidP+6]<<8 |
                                                   Mid [MidP+7];
      if (MidLn < ((MidP += 8) + tlen))   Ugh (CC("CvtMid  EOF in track"));
      if (Tr.Full ())                  Ugh (CC("CvtMid  too many .mid tracks"));
      tr = (ubyte)Tr.Ins ();         Tr [tr].eFr = Ev.Ln;
      Trk2Ev (tr, MThd.res, tlen);   Tr [tr].eTo = Ev.Ln;
   }
   PutSong ();                         // fix up/write Ev[] into .song file
TRC("CvtMid end");
}


int main (int argc, char *argv [])
{ TStr fn, to;
  File f;
DBGTH("Mid2Song");
   App.Init ();
   StrCp (FN, argv [1]);
   if (argc < 2)  {DBG ("usage is Mid2Song fn.mid");   return 99;}
TRC("FN=`s", FN);

// load the midi file into memory
   if ((MidLn = f.Load (FN, Mid, sizeof (Mid))) == 0)
                                                     Ugh (CC(".mid not found"));
   if (MidLn >= BITS (Mid))                          Ugh (CC(".mid too big"));
   if (MemCm ((char *)Mid, CC("RIFF"), 4, 'x') == 0)      // damn .RMI files...
      MemCp (Mid, & Mid [20], MidLn -= 20);

// parse n save mem to .song file
   StrCp (fn, FN);   Fn2Path (fn);   StrAp (fn, CC("/a.song"));
   if (! Fs.Open (fn, "w"))  Ugh (CC("Can't write .song file"));

   S = new Song ();   S->CvtMid ();   delete S;

   Fs.Shut ();
TRC("end");
   return 0;
}
