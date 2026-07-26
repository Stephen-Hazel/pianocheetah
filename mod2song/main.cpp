// mod2song.c - suck in a .MOD, write out .song aaand .WAVs for syn

#include "stv/os.h"
#include "stv/wav.h"
#include "stv/midi.h"


// our parsin' limits
const ubyt4 MAX_EVNT = 256*1024;
const ubyte NTRK = 31;                 // instrument 1..31 => track 0..30
const ubyte EVSZ = 4;
const ubyte NCHN = 4;
const ubyte NLIN = 64;                 // 4 bars of 4/4 1e&a2e&a3e&a4e&a


// mod format stuffs...
const ubyt4 STD_TEMPO = 6;             // 6 clocks per 16th note

const ubyte MC_ARPEG = 0x00;           // arpeggio if param!=0
const ubyte MC_PORTU = 0x01;           // portamento up
const ubyte MC_PORTD = 0x02;           // portamento down
const ubyte MC_PORTT = 0x03;           // tone portamento
const ubyte MC_VIBRA = 0x04;           // vibrato
const ubyte MC_PTVOL = 0x05;           // tone portamento volume
const ubyte MC_VBVOL = 0x06;           // vibrato volume
const ubyte MC_TREME = 0x07;           // tremelo
const ubyte MC_PAN   = 0x08;           // panning8
const ubyte MC_OFFST = 0x09;           // offset
const ubyte MC_VOLSL = 0x0A;           // volume slide
const ubyte MC_POSJP = 0x0B;           // position jump
const ubyte MC_VOLUM = 0x0C;           // volume
const ubyte MC_ENDJP = 0x0D;           // pattern break
const ubyte MC_EX    = 0x0E;           // mod command ex
const ubyte MC_TEMPO = 0x0F;           // speed / tempo

const ubyt2 Period [] = {              // 5 x 12 notes 2c..6b
  1712,1616,1524,1440,1356,1280,1208,1140,1076,1016, 960, 906,
   856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
   428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
   214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
   107, 101,  95,  90,  85,  80,  76,  71,  67,  64,  60,  57
};

struct STMOD {
   char  songname [20];
   struct {
      char  name [22];
      ubyt2 length;                    // ...these 4 gotta get swabbed :/
      ubyt2 volume;
      ubyt2 repeat;                    // loop start
      ubyt2 replen;                    // loop length
   }     sample [31];
   ubyte songlen;                      // len of playseq
   ubyte pad;
   ubyte playseq [128];                // order to play blocks in
   char  magic [4];                    // "M.K." (version id) for plain mod
};


// globs -----------------------------------------------------------------------
TStr   FN, To;                         // arg path/filename.mod and new out dir
TStr   SS;                             // sampleset
File   LF;                             // To/log.txt fer loggin

ubyte  Mod [16*1024*1024];             // mem to hold the whole .mod file
ubyt4  ModLen, ModPos;                 // len of buf and pos we're parsin' at

STMOD *Hdr;                            // pointers to spots in Mod[] to parse
ubyte *Blk [128];
ubyt2 NBlk;
ubyt4  TempoDur = STD_TEMPO;

TStr   TSn [NTRK];                     // sound name per track
TrkEv  TEv [NTRK][MAX_EVNT];           // events - results of parsin'
ubyt4  NEv [NTRK];                     // how many per track (instrument)

ubyte PanChn [4]  = {0, 255, 255, 0};
ubyte PanTrk [31] = {128, 128, 128, 128, 128, 128, 128, 128,
                     128, 128, 128, 128, 128, 128, 128, 128,
                     128, 128, 128, 128, 128, 128, 128, 128,
                     128, 128, 128, 128, 128, 128, 128};

//------------------------------------------------------------------------------
#define EVEN(n)     ((n)&0xFFFFFFFE)
#define EVEN_UP(n)  EVEN((n)+1)

void PutWav (ubyte t, ubyt4 len, ubyt4 lpBgn, ubyt4 lpEnd)
// convert soundtracker sample to .wav file
{ TStr  fn, pa;
  BStr  ls;
  Path  p;
  File  f;
  ubyt4 ln1, ln2, ln3, ln4, i;
  WAVEFORMATEX wf;
  struct {ubyt4 manuf;  ubyt4 prod;  ubyt4 per;  ubyt4 note;  ubyt4 frac;
          ubyt4 sfmt;   ubyt4 sofs;  ubyt4 num;  ubyt4 dat;   ubyt4 cue;
          ubyt4 loop;   ubyt4 bgn;   ubyt4 end;  ubyt4 frc;   ubyt4 times;
  } smpl;
  ubyte *ptr = & Mod [ModPos];
  ubyt4  frq = 8363; //16726;                  // <=ntsc,  (not 16574=pal??)
   len *= 2;                           // that's teh way MODs do it...:/
   for (i = 0;  i < len;  i++)  ptr [i] = (ubyte)(((sbyte *)ptr) [i] + 128);
   ln4 = 60;
   ln3 = EVEN_UP (len);
   ln2 = 16;
   ln1 = 12 + ln2 + 8 + ln3 + 8 + ln4;

   wf.wFormatTag      = WAVE_FORMAT_PCM;
   wf.nChannels       = 1;
   wf.wBitsPerSample  = 8;
   wf.nSamplesPerSec  = frq;
   wf.nBlockAlign     = wf.nChannels   * wf.wBitsPerSample / 8;
   wf.nAvgBytesPerSec = wf.nBlockAlign * wf.nSamplesPerSec;

   MemSet (& smpl, 0, sizeof (smpl));
   smpl.per  = 1000000000 / frq;
   smpl.note = 60;                     // middle c
   smpl.frac = 0;                      // cents
   smpl.num  = 1;
   if ((lpEnd-lpBgn) > 4) {
      smpl.bgn = lpBgn*2;              // those dang MODs...:/
      smpl.end = lpEnd*2;
   }
   else  {smpl.bgn = smpl.end = len;}

   StrFmt (fn, "`s/`s/a.wav", To, TSn [t]);
   StrCp (pa, fn);   Fn2Path (pa);   p.Make (pa);

   if (! f.Open (fn, "w")) {LF.Put (StrFmt (ls, "can't write file `s\n", fn));
                            return;}

   f.Put (CC("RIFF")    );   f.Put (& ln1, 4);
   f.Put (CC("WAVEfmt "));   f.Put (& ln2, 4);   f.Put (& wf,   ln2);
   f.Put (CC("data"    ));   f.Put (& len, 4);   f.Put (ptr,    len);
   if (len % 2)  f.Put (CC(""), 1);
   f.Put (CC("smpl"));       f.Put (& ln4, 4);   f.Put (& smpl, ln4);

   f.Shut ();
}


//------------------------------------------------------------------------------
void Swab (ubyt2 *uw)
// swap ubyt2 around cuz Amiga was MSB first, we're LSB first
{ ubyte *ub = (ubyte *)uw;
  ubyte  t = ub [0];   ub [0] = ub [1];   ub [1] = t;
}


ubyt4 DUR (ubyt4 t)
{ ubyt4 out, res = M_WHOLE/64;
   out = ((4*t) / TempoDur) * res;
   if ((((4*t) % TempoDur) * 2) > TempoDur) out += res;
   return (out);
}


void CvtTrk (ubyte t)
{ ubyt4 n = 0, time = 0, dur, maxdur;
  ubyt2 per, vol, note;
  ubyt4 b, bl, l, c, i, j;
  ubyte in, tempo = 6, ins [4], cmd [4], byt [4],
        ttempo, tcmd, tbyt, *p;
  bool  got, rep;
  TStr  ts, ts2;
  BStr  ps;
   in = t + 1;                         // trk 0..30 <=> ins 1..31
LF.Put (StrFmt (ps, "inst=`d ----------\n", in));
LF.Put (CC("tr time        dur key vel   vol cmd\n"));
   rep    = (Hdr->sample [t].replen > 1) ? true : false;
   maxdur =  Hdr->sample [t].length * (M_WHOLE/64) / 110;
   for (b = 0;  b < Hdr->songlen;  b++) {   // each block of song
      bl = Hdr->playseq [b];
      for (l = 0;  l < NLIN;  l++) {        // each line of block
         for (c = 0; c < NCHN; c++) {       // each chan of line
         // blk's lin's trk's ev (4 bytes of)
            p = & Blk [bl][(l*EVSZ*NCHN)+(c*EVSZ)];
            ins [c] = (p [0] & 0x10) | (p [2] >> 4);
            cmd [c] =  p [2] & 0x0F;
            byt [c] =  p [3];
            if ((cmd [c] == MC_TEMPO) && (byt [c] >= 1) && (byt [c] <= 32))
               tempo = byt [c];
         }
         for (c = 0; c < 4; c++) {
            p = & Blk [bl][(l*EVSZ*NCHN)+(c*EVSZ)];
            if (ins [c] != in)  continue;   // ev has to be for THIS ins

            vol = Hdr->sample [t].volume;   // usually 64
            if ((cmd [c] == MC_VOLUM) && (byt [c] <= 64))  vol = byt [c];

         // hi nibble reused :/
            per = *((ubyt2 *)p);   Swab (& per);   per &= 0x0FFF;
            if (! per)  continue;           // no note frq, no nothin

            got = false;
            for (note = 0;  note < BITS (Period);  note++)
               if (per == Period [note])  {got = true;  break;}
            if (! got) {
LF.Put (StrFmt (ps,
" Unknown note period `d($`03x) at block=`d line=`d channel=`d\n",
per, per, bl, l, c));
               continue;
            }

            dur = DUR (tempo);
            ttempo = tempo;   got = true;
            for (i = l+1;  got && (i < NLIN);  i++) {
               for (j = 0;  j < NCHN;  j++) {
                  tcmd = Blk [bl][(i*16)+(j*4)+2] & 0x0F;
                  tbyt = Blk [bl][(i*16)+(j*4)+3];
                  if ((tcmd == MC_TEMPO) && (tbyt >= 1) && (tbyt <= 32))
                     ttempo = tbyt;
                  if (tcmd == MC_ENDJP) {got = false;  break;}
                  if (tcmd == MC_POSJP)  got = false;
               }
               if (tcmd == MC_ENDJP) break;      // i know,  ...YUK

               p = & Blk [bl][(i*NCHN*EVSZ)+(c*EVSZ)];
               if ( (((ubyt2 *) p)[0]         )            ||
                   ((((ubyt2 *) p)[1] & 0x0FFF) == ((ubyt2)MC_VOLUM<<8)) )
                     got = false;
               else  dur += (DUR (ttempo));
            }
            if ((! rep) && (dur > maxdur))  dur = maxdur;
            if (cmd [c] == MC_PAN)  PanChn [c] = byt [c];
            if (PanTrk [t] != PanChn [c]) {
                PanTrk [t]  = PanChn [c];
               if (n >= MAX_EVNT)  Die ("hit max events");
               TEv [t][n].time = time;
               TEv [t][n].ctrl = 0x80;      // pan
               TEv [t][n].valu = PanChn [c] >> 1;
               n++;
            }
            TEv [t][n].time = time;
            TEv [t][n].ctrl = MKey (CC("2c")) + note;
            TEv [t][n].valu = 0x80 | (ubyte)((127 * vol) / 64);
            if (TEv [t][n].valu == 0x80)  TEv [t][n].valu = 0x81;
LF.Put (StrFmt (ps,
"`>2d `s `>3d `<3s `>3d    `>2d `02x\n",
in, TmS (ts2, time), dur, MKey2Str (ts, MKey (CC("2c")) + note),
TEv [t][n].valu & 0x7F, vol, cmd [c]));
            n++;
            if (n >= MAX_EVNT)  Die ("hit max events");
            TEv [t][n].time = time + dur - 1;
            TEv [t][n].ctrl = MKey (CC("2c")) + note;
            TEv [t][n].valu = 0;
            n++;
         }
         if ((cmd [0] == MC_ENDJP) || (cmd [1] == MC_ENDJP) ||
             (cmd [2] == MC_ENDJP) || (cmd [3] == MC_ENDJP))  break;
         time += (DUR (tempo));
         if ((cmd [0] == MC_POSJP) || (cmd [1] == MC_POSJP) ||
             (cmd [2] == MC_POSJP) || (cmd [3] == MC_POSJP))  break;
      }
   }
   NEv [t] = n;
}


//------------------------------------------------------------------------------
void CvtMod ()
{ ubyte  trk, i, t, c;
  ubyt4  b, j;
  File   f;
  TStr   s, s2, sfn, tfn;
  char   buf [8192];
  BStr   ps;
  TrkEv *e;

// parse header
   Hdr = (STMOD *)Mod;
   if (ModLen < sizeof (STMOD))
      {LF.Put (CC("Header too small\n"));   return;}
//DBG("magic=`c`c`c`c",Hdr->magic[0],Hdr->magic[1],Hdr->magic[2],Hdr->magic[3]);
// if (MemCm (Hdr->magic, CC("M.K."), 4))
//    {LF.Put (CC("not a regular (M.K.) MOD file\n"));   return;}

   ModPos += sizeof (STMOD);
   LF.Put (CC("id length volume repeat repLen name\n"));
   for (i = 0;  i < BITS (Hdr->sample);  i++) {
      Hdr->sample [i].name [21] = '\0';
      FnFix (Hdr->sample [i].name, '-');
      Swab (& Hdr->sample [i].length);   Swab (& Hdr->sample [i].volume);
      Swab (& Hdr->sample [i].repeat);   Swab (& Hdr->sample [i].replen);
      LF.Put (StrFmt (ps,
"`>2d `>6d `>6d `>6d `>6d `s\n",
i+1, Hdr->sample [i].length, Hdr->sample [i].volume,
     Hdr->sample [i].repeat, Hdr->sample [i].replen, Hdr->sample [i].name));
   }

// setup block pointers (to 1K blocks of bytes in file)
   for (i = 0;  i < Hdr->songlen;  i++)
      if (Hdr->playseq [i] >= NBlk)  NBlk = Hdr->playseq [i] + 1;
   if (NBlk ==  0)  {LF.Put (CC("hmmm, no blocks?\n"));     return;}
   if (NBlk > 128)  {LF.Put (CC("hmmm, >128 blocks?\n"));   return;}

   for (b = 0;  b < NBlk;  b++)  {
      LF.Put (StrFmt (ps,
"block `d => `d=$`08x\n", b, ModPos, ModPos));
      Blk [b] = & Mod [ModPos];   ModPos += (EVSZ * NCHN * NLIN);  // 1024
   }

// makin' tracks fer each instrument (trk 0..30 => ins 1..31)
   for (trk = 0;  trk < NTRK;  trk++)  CvtTrk (trk);

// save out the samples as To/x_99InstName~hold (/a.wav)
   for (t = 0;  t < NTRK;  t++) {
      StrFmt (TSn [t], "x_`02d`s`s",
              t+1, Hdr->sample [t].name,
                  (Hdr->sample [t].replen > 4) ? "" : "~hold");
DBG("sample for trk `d => `d=$`08x  (len=`d)  `s",
t, ModPos, ModPos, ModLen, TSn [t]);
      if (Hdr->sample [t].length)
         PutWav (t, Hdr->sample [t].length,  Hdr->sample [t].repeat,
                    Hdr->sample [t].repeat + Hdr->sample [t].replen);
      ModPos += (2 * Hdr->sample [t].length);
   }

// write .song
   Fn2Name (FN);   StrCp (sfn, FN);   StrAp (sfn, CC(".song"));
   if (! f.Open (sfn, "w"))
      {LF.Put (StrFmt (ps, "couldn't write `s\n", sfn));   return;}

   f.Put (CC("id length volume repeat repLen name\n"));
   for (t = 0;  t < NTRK;  t++) {
      StrFmt (buf, "`>2d `>6d `>6d `>6d `>6d `s\n",
         t+1, Hdr->sample [t].length, Hdr->sample [t].volume,
         Hdr->sample [t].repeat, Hdr->sample [t].replen, Hdr->sample [t].name);
      f.Put (buf);
   }
   f.Put (CC("\nTrack:\n"));
   for (t = 0;  t < NTRK;  t++)  if (NEv [t])
      f.Put (StrFmt (ps, "syn  `s_`s  .SH\n",  TSn [t], SS));

// write events
   f.Put (CC("Event:\n"));
   for (t = 0;  t < NTRK;  t++)  if (NEv [t]) {
      e = & TEv [t][0];
      for (j = 0;  j < NEv [t];  j++) {
         f.Put (StrFmt (s, "`<9s ", TmS (s2, e [j].time)));
         c = e [j].ctrl;
         if (c & 0x0080) {          // ctrl
            StrCp (s, CC("pan"));   // we only do pan :/
/*
            if      (! StrCm (s,  CC("Tmpo")))  // tmpo,tsig,ksig are special
               f.Put (StrFmt (s, CC("!Tmpo=`d"),
                                              e [j].valu | (e [j].val2<<8)));
            else if (! StrCm (s,  CC("TSig"))) {
               f.Put (StrFmt (s, CC("!TSig=`d/`d"),  e [j].valu,
                                           1 << (e [j].val2 & 0x0F)));
               if (e [j].val2 >> 4)  f.Put (StrFmt (s, "/`d",
                                            1 + (e [j].val2 >> 4)));
            }
            else if (! StrCm (s, CC("KSig"))) {
               f.Put (CC("!KSig="));
               if   (! (e [j].val2 & 0x80)) StrCp (s, MKeyStr  [e [j].valu]);
               else if (e [j].valu != 11)   StrCp (s, MKeyStrB [e [j].valu]);
               else                         StrCp (s, CC("Cb"));  // weird :/
               if (e [j].val2 & 0x01)  StrAp (s, CC("m"));
               *s = CHUP (*s);
               f.Put (s);
            }
            else {
*/
               f.Put (StrFmt (s2, "!`s=`d",           s, e [j].valu));
//             if (e [j].val2)  f.Put (StrFmt (s, " `d", e [j].val2));
//          }
         }
         else {                     // note
            StrFmt (s, "`s`c`d",
               MKey2Str (s2, c),
               EUP (& e [j]) ? '^' : (EDN (& e [j]) ? '_' : '~'),
               e [j].valu & 0x7F);
            f.Put (s);
         }
         f.Put (CC("\n"));
      }
      f.Put (StrFmt (s, "EndTrack `d #ev=`d\n", t+1, NEv [t]));
   }
   f.Shut ();
}


int main (int argc, char *argv [])
{ TStr fn;
  Path p;
  FDir d;
  File f;
DBGTH("Mod2Song");
   App.Init ();
TRC("arg=`s", argv [1]);               // we EXPECT ta be called by midimp
                                       // so .../3_queue/dir/song/a.mod
// MODs have loads of weird .exts so can't check
   if (argc < 2)  Die ("need .../song/a.mod");
   StrCp (FN, argv [1]);

// sampleset is MOD-fn of arg w spaces => -
   StrCp (fn, FN);   Fn2Path (fn);     // toss /a.mod leaving path/song
   StrCp (SS, CC("MOD-"));                 // MOD- prefix
   FnName (& SS [4], fn);              // toss path leaving just fn
   FnFix (SS, '-');                    // spaces => - etc
TRC("bank=`s", SS);

// make syn's new bank
   App.Path (To, 'd');   StrAp (To, CC("/device/syn/"));   StrAp (To, SS);
   if (d.Got (To))  p.Kill (To);

// ok, make To dir n open log.txt there
   p.Make (To);   StrCp (fn, To);   StrAp (fn, CC("/log.txt"));
   if (! LF.Open (fn, "w"))   Die ("Mod2Song  couldn't write log file");
LF.Put (StrFmt (fn, "`s\n", FN));

// load mod file into memory
   if ((ModLen = f.Load (FN, Mod, sizeof (Mod))) == 0)
                              Die ("MOD2Song  .mod not found");
   if (ModLen == BITS (Mod))  Die ("MOD2Song  .mod too big");

   CvtMod ();                          // do eeeet

   LF.Shut ();
   App.Run (CC("synsnd"));
TRC("end");
   return 0;
}
