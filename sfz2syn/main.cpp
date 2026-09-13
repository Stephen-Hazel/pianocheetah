// sfz2syn.cpp - rip .wav files outa .sfz with key,velo ranges in .wav fn
//               and give it ta syn

#include "stv/os.h"
#include "stv/midi.h"
#include "stv/wav.h"

TStr  Top, To, SndN, Kit;              // top dir, dest dir,
                                       // soundDir under To, drumkit,
File  LF;                              // log file
ubyt2 NFn;
TStr   Fn [1024];                      // SFZ fn list
char  KGot [128];                      // keys in preset

enum SfzP {KY, TR, CT,   KL, KH,   VL, VH,   SB, SE, LO, LB, LE,
           PA, PT,   NO,   NSFZP};

const char *Lbl [NSFZP] = {
   "root", "tran", "tune",
   "key", "to",
   "vel", "to",
   "bgn", "end", "loop", "lpBgn", "lpEnd",
   "pan", "pTrk", "NOPE"
};

ubyte dfcc [128] = {              // default cc values for on_locc/on_hicc
   0, 0, 0, 0, 0, 0, 0, 100,      // 0 7=vol
   64, 0, 64, 127, 0, 0, 0, 0,    // 8 8=bal,10=pan,11=expr
   0, 0, 0, 0, 0, 0, 0, 0,        // 16
   0, 0, 0, 0, 0, 0, 0, 0,        // 24
   0, 0, 0, 0, 0, 0, 0, 0,        // 32
   0, 0, 0, 0, 0, 0, 0, 0,        // 40
   0, 0, 0, 0, 0, 0, 0, 0,        // 48
   0, 0, 0, 0, 0, 0, 0, 0,        // 56
   0, 0, 0, 0, 0, 0, 0, 0,        // 64
   0, 0, 0, 0, 0, 0, 0, 0,        // 72
   0, 0, 0, 0, 0, 0, 0, 0,        // 80
   0, 0, 0, 0, 0, 0, 0, 0,        // 88
   0, 0, 0, 0, 0, 0, 0, 0,        // 96
   0, 0, 0, 0, 0, 0, 0, 0,        // 104
   0, 0, 0, 0, 0, 0, 0, 0,        // 112
   0, 0, 0, 0, 0, 0, 0, 0         // 120
};


sbyt4 Key2Int (char *s)
// turn key string into int.  standard note notation (C#4), not mine
{ ubyte n;
  sbyt4 o;
  sbyte f = 0;
   for (n = 0;  n < 12;  n++)  if (CHDN (s [0]) == MKeyStr [n][0])  break;
   if (n >= 12)  o = Str2Int (s);              // c,c#,d,etc
   else {                                      // so c>0, d>2, e>4, f>5, etc
      s++;
      if (CHDN (*s) == 'b')  {f = -1;   s++;}
      if (      *s  == '#')  {f =  1;   s++;}
      o = ((*s < '0') || (*s > '9'))  ?  0 : ((*s-'0'+1)*12 + n + f);
   }
   if (o < 0)  o = 0;   if (o > 127)  o = 127;
   return o;
}

sbyt4 Vel2Int (char *s)
{ sbyt4 o = Str2Int (s);
   if (o < 1) o = 1;   if (o > 127) o = 127;
   return o;
}

//______________________________________________________________________________
void FnFixSl (char *s)  {  while (*s)  {if (*s == '\\') *s = '/';
                                        s++;}
                        }              // fix \ into / shoish


void DoWav (char *dp, char *sm, char got [3][NSFZP],
                               sbyt4 sfz [3][NSFZP])
{ sbyt4 i, sp, ia [15];
  ubyte j, k;
  TStr  fn, ts, s [15], wfn, xfn, fnx, sfn, pre, suf; // sfn is new syn fn
  BStr  cmd;
  bool  ok;
  BStr  ps;
  Wav   wv;
  File  f;
  Path  p;
DBG("DoWav dp=`s sm=`s", dp, sm);
// skip out if no sample or built in (starts w * like *sine, etc)
   if (*sm == '\0')  {DBG(" no sample=");            return;}
   if (*sm == '*')   {DBG(" `s no * samples", sm);   return;}

// resolve global,group levels down to region
   for (i = 0;  i < NSFZP;  i++) {
      for (j = 0;  j < 3;  j++)  if (got [j][i] == 'y')  break;
      if (j < 3)  {got [0][i] = '0' + j;   sfz [0][i] = sfz [j][i];}
   }

// default stuff ya ain't got (more later)
   if (got [0][KL] == 'n')  {got [0][KL] = 'd';   sfz [0][KL] = 0;}
   if (got [0][KH] == 'n')  {got [0][KH] = 'd';   sfz [0][KH] = 127;}
   if (got [0][VL] == 'n')  {got [0][VL] = 'd';   sfz [0][VL] = 1;}
   if (got [0][VH] == 'n')  {got [0][VH] = 'd';   sfz [0][VH] = 127;}

// skip out for regions we can't handle like rand>0, xf*, cc without set
   if (got [0][NO] != 'n') {DBG(" NOPE");
                            return;}
   if (got [0][KL] && got [0][KH])     // stamp KGot[]
      for (j = (ubyte)sfz [0][KL];  j <= (ubyte)sfz [0][KH];  j++)
         KGot [j] = '*';

// resolve actual sample fn given dp n sm
   StrFmt (fn, "`s/`s`s", Top, dp, sm);
   ok = (f.Size (fn) > 0) ? true : false;
   if (! ok) {
DBG(" sample not THERE: `s", fn);
      return;                          // sample not there so baaaail
   }
   *xfn = *fnx = '\0';   StrCp (wfn, fn);
   i = StrLn (fn);
// gotta convert some format to .wav?
   if ( (i > 4) && (! StrCm (& fn [i-4], ".mp3")) )
      StrCp (fnx,                        ".mp3");
   if ( (i > 4) && (! StrCm (& fn [i-4], ".ogg")) )
      StrCp (fnx,                        ".ogg");
   if ( (i > 5) && (! StrCm (& fn [i-5], ".flac")) )
      StrCp (fnx,                        ".flac");
   if (*fnx) {
      StrCp (xfn, fn);   StrAp (wfn, ".WAV", StrLn (fnx));
      App.Run (StrFmt (cmd, "ffmpeg -i `p `p", xfn, wfn));
   }
   else if ( (i > 4) && (  StrCm (& fn [i-4], ".wav")) ) {
DBG(" non .wav fn=`s ???", fn);
      return;                          // non .WAV so baaail
   }

   StrCp (ts, wfn);   Fn2Path (ts);
   StrCp (sfn, & wfn [StrLn (ts)+1]);  // just the .wav fn (no path)
   StrAp (sfn, "", 4);                 // toss .WAV too

   StrCp (fn, & fn [StrLn (Top)+1]);   // easier to read

// ok, dump params  (in file means that'll be default)
// always in: KY,                  SB,SE
// can be in:       CT,                  LO,LB,LE
// never in:     TR,   KL,KH,VL,VH,               PT
LF.Put (StrFmt (ps,
" `<3s-`<3s `>3d-`>3d `s`s\n",
MKey2Str(s[3],(ubyte)sfz[0][KL]), MKey2Str(s[4],(ubyte)sfz[0][KH]),
sfz[0][VL], sfz[0][VH],   fn, ok ? "" : " <=MISSING!!"));

   ia [0] = KY;   ia [1] = TR;   ia [2] = CT;
   ia [3] = SB;   ia [4] = SE;   ia [5] = LB;   ia [6] = LE;   ia [7] = LO;
   ia [8] = PA;   ia [9] = PT;
   ok = false;
   for (i = 0;  i < 10;  i++) {
      sp = ia [i];   s [i][0] = '\0';
      if (got [0][sp] != 'n') {
         ok = true;
         if (sp == KY)  StrFmt (ts, " `s=`s", Lbl [sp], MKey2Str (s[14],
                                                 (ubyte)sfz[0][sp]));
         else           StrFmt (ts, " `s=`d", Lbl [sp], sfz[0][sp]);
//       if (got [0][sp] != '0')  StrAp (ts, (got [0][sp]=='1')?"(gr)":"(gl)");
         StrCp (s [i], ts);
      }
   }
   if (ok)
LF.Put (StrFmt (ps, "  `s`s`s`s`s`s`s`s`s`s\n",
s[0],s[1],s[2],s[3],s[4], s[5],s[6],s[7],s[8],s[9]));

// load in the .wav file
   StrCp (ts, wv.Load (wfn));
   if (! MemCm ("ERROR", ts, 5)) {
LF.Put (StrFmt (ps, "`s for `s\n", ts, wfn));
      wv.Wipe ();
      if (*xfn)  f.Kill (wfn);         // clean up .flac's temp .wav
DBG(" can't read wfn=`s", wfn);
      return;                          // can't read .WAV so baaaail
   }
LF.Put (StrFmt (ps, "   `s\n", ts));

// PROCESSIN DOZE SFZ PARAMS !!
   if (*Kit) {                         // drum - kl,kh from ky else kl else kh
      if (sfz [0][KL] != sfz [0][KH]) {
         if      (got [0][KY] != 'n')  sfz [0][KL] = sfz [0][KH] = sfz [0][KY];
         else if (sfz [0][KL] > 0)     sfz [0][KH] = sfz [0][KL];
         else                          sfz [0][KL] = sfz [0][KH];
LF.Put (StrFmt (ps,
"        ERROR drum keyLo != keyHi.  pickin `s=`s\n",
MKey2Str (s[0], (ubyte)sfz [0][KL]), MDrm2Str (s[1], (ubyte)sfz [0][KL]) ));
      }
      MDrm2StG (pre, (ubyte)sfz[0][KL]);
      StrAp (pre, "_");   StrAp (pre, Kit);   StrAp (pre, "/");
   }
   else {                              // melo - prefix of _kX_ = kh else ky
      if (got [0][KH] == 'n')  sfz [0][KH] = sfz [0][KY];
      StrFmt (pre, "_k`s_", MKey2Str (s[0], (ubyte)(sfz [0][KH])));
   }
// sometimes _vX_ based on VL,VH
   if ((sfz[0][VL] != 1) || (sfz[0][VH] != 127))
      StrAp (pre, StrFmt (s[0], "`sv`03d_", *Kit ? "_" : "", sfz[0][VH]));

// suffix _L or _R based on wv bein mono n pan ya got
   *suf = '\0';
   if ((Str2Int (ts) == 1) && (got [0][PA] != 'n')) {
      if      (sfz [0][PA] < 0)  StrCp (suf, "_L");
      else if (sfz [0][PA] > 0)  StrCp (suf, "_R");
   }
// rip any existing _L ishness off orig fn
   j = (ubyte)StrLn (sfn);
   if ( (j > 1) && ((CHUP(sfn [j-1]) == 'L') || (CHUP(sfn [j-1]) == 'R')) &&
        ((sfn [j-2] == '_') || (sfn [j-2] == ' ') || (sfn [j-2] == '-')) )
      StrAp (sfn, "", 2);

// adjust wv key,cnt based off KY,TR,CT params
   if (got [0][KY] != 'n')  wv._key  = (ubyte)sfz [0][KY];
   if (got [0][TR] != 'n')  wv._key += (sbyte)sfz [0][TR];
   if (got [0][CT] != 'n') {
      i = sfz [0][CT];
      if (i < -99)  {k = (ubyte)(-i / 100);   wv._key -= k;   i += (k*100);}
      if (i >  99)  {k = (ubyte)( i / 100);   wv._key += k;   i -= (k*100);}
      if (i < 0)    {wv._key--;   i += 100;}
      wv._cnt = (ubyte)i;
   }

// adjust sample points base off SB,SE,LB,LE,LO
   if (got [0][SE] != 'n')
      {i = sfz [0][SE];   if ((ubyt4)i < wv._len)  wv._end  = (ubyt4)i;}
   if (got [0][SB] != 'n')
      {i = sfz [0][SB];   if ((ubyt4)i < wv._len)  wv._bgn  = (ubyt4)i;}
   if (got [0][LE] != 'n')
      {i = sfz [0][LE];   if ((ubyt4)i < wv._len)  wv._lEnd = (ubyt4)i;}
   if (got [0][LB] != 'n')
      {i = sfz [0][LB];   if ((ubyt4)i < wv._len)  wv._lBgn = (ubyt4)i;}
   if (got [0][LO] != 'n')
      {i = sfz [0][LO];   wv._loop = (i == 0) ? false : true;
       if (i && (wv._lBgn >= wv._end-1))  wv._lBgn = wv._bgn;}

// write dat babyyy OUT
   StrFmt (fn, "`s/`s`s`s`s.WAV", To, SndN, pre, sfn, suf);
   StrCp (ts, fn);   Fn2Path (ts);
   p.Make (ts);
   wv.Save (fn);
   wv.Wipe ();

   if (*xfn)  f.Kill (wfn);            // clean up .flac's temp .wav
}

char *LIn [3] = {CC("region"), CC("group"), CC("global")};

void DoSfz (char *fn)                  // Kit has been set, too
// parse a dang .sfz file and write out each .wav with a fn fer it's sfz params
{ StrArr a, b, c;
  BStr   r, r2;
  char  *rp, *p, *q, *x;
  bool   inC, no1;
  ubyt4  i, j, k, e;
  sbyt4  t, t2, t3, cc [1024], ncc, ccv [1024];
  ubyte  in, inp;                      // level we're in;  in previously
  TStr   pa, dp, sm, ts, fni;
  sbyt4  sfz [3][NSFZP];               // global, group, region levels of params
  char   got [3][NSFZP];               // whether defined within level
  Path   dr;
  File   f;
  ubyt4  nl;
  TStr   ls [800];
  BStr   ps;
  ubyte  pTrk;
LF.Put (StrFmt (ps, "`s`s\n", & fn [StrLn (Top)+1], *Kit ? " <== DRUM" : ""));
// pa is ABSolute path to preset (somewhere within Top)
   StrCp (pa, fn);   Fn2Path (pa);

// look for 999 format gm progch # on pathless fn prefix
   StrCp (ts, fn);   Fn2Path (ts);
   p   = & fn [StrLn (ts)+1];
   inp = (ubyte)Str2Int (p, & p);
DBG("   inp=`d pa=`s p=`s fn=`s Kit=`s", inp, pa, p, fn, Kit);

// drum GM Snd: Drum/Grp_Snd_Kit  Grp_Snd set in DoWav
   if      (*Kit)
      *SndN = '\0';

// melo GM Snd: Dir_Snd
   else if ((inp >= 1) && (inp <= 128) && (p == & fn [StrLn (ts)+4]))
      StrCp (SndN, MProg [inp-1]);               // ^^^ len of 3? hopefullyyy

// melo unGM Snd: x_PSet
   else {
      StrFmt (SndN, "x_");
      StrCp (ts, & fn [StrLn (Top)+1]);     // back to sfz dir/fn off Top
      StrAp (ts, "", 4);                    // chop off .sfz
      FnFix (ts, '-');                      // weird chars => -
      if (! MemCm (ts, "programs-", 9))  StrCp (ts, & ts [9]);
      StrAp (SndN, ts);
   }
   StrAp (SndN, "/");
DBG("   Snd='`s' Kit='`s'", SndN, Kit);     // Snd FINALLY DONE

   MemSet (KGot, '_', sizeof (KGot));  // try to see if it's drums
   pTrk = 0;

// do #includes - a holds output, b loads this sfz, c loads each #inc
   a.Init ("all", 16*1024);        // hopefully no 2+ layer #incs :(
   b.Init ("sfz", 16*1024);
   b.Load (fn);
   for (i = 0;  i < b.NRow ();  i++) {
      StrCp (r, b.str [i]);
      if (! MemCm (r, "#include ", 9)) {
         rp = & r [9];
         if ((p = StrCh (rp, '"')))
            {rp = p+1;   if ((p = StrCh (rp, '"')))  *p = '\0';}
         StrFmt (fni, "`s/`s", pa, rp);
         c.Init ("inc", 16*1024);
         c.Load (fni);
         for (j = 0;  j < c.NRow ();  j++)  a.Add (c.str [j]);
      }
      else  a.Add (r);
   }
   b.Init ("def", 1024);               // b gets reused for #define's :/

//DBG("   clear got,sfz");
   MemSet (got, 'n', sizeof (got));
   MemSet (sfz,   0, sizeof (sfz));

// default default_path n sample
   StrCp (dp, pa);   StrAp (dp, "/");   StrCp (dp, & dp [StrLn (Top)+1]);
//DBG("   default dp='`s'", dp);
   *sm = '\0';
   inC = false;   in = 2;              // 2=global, 1=group, 0=region
   ncc = 0;

// ok, parse dem dang rex !!
   for (i = 0;  i < a.NRow ();  i++) {
   // pull the rec into r and mess with r comment n #define-wise
      StrCp (r, a.str [i]);
//DBG("   i=`d r=`s", i, r);
      if ((p = StrSt (r, "//")))  *p = '\0';      // kill // comments (ez)
      if (inC) {                       // lookin for end of mult ln cmt
         if ((p = StrSt (r, "*/")))  {inC = false;   StrCp (r, p+2);}
         else                        continue;
      }
      while ((p = StrSt (r,   "/*"))) {     // for /* gotta check multiline
         if ((q = StrSt (p+2, "*/")))  StrCp (p, q+2);
         else                          {inC = true;   *p = '\0';}
      }
      if (! MemCm (r, "#define ", 8)) {
        ColSep s (r, 4);
         b.Add (StrFmt (r2, "`s `s", s.Col [1], s.Col [2]));
//DBG("   def `s", r2);
         continue;
      }
   // resolve #define'd $symbols
      for (p = r, j = 0;  j < StrLn (r);  j++, p++)  if (*p == '$') {
         for (k = 0;  k < b.NRow ();  k++) {
            StrCp (ts, b.str [k]);   if ((q = StrCh (ts, ' ')))  *q++ = '\0';
                                     else q = CC("");      // ts is repl valu
            e = StrLn (ts);                                // q  is new  valu
            if (! MemCm (p, ts, e)) {
               StrCp (ts, q);                    // new str into ts
               MemCp (p, p+e, StrLn (p+e)+1);    // del existing $symbol
               e = StrLn (ts);                   // new str's len
               MemCp (p+e, p, StrLn (p)+1);      // ins space for new str
               MemCp (p, ts, e);                 // copy new str in
               break;
            }
         }
      }
//DBG("   did #defines");

   // from here on we use rp
      rp = r;

   // level bump?
      if (*rp == '<') {                // get new level
         inp = in;
         if      (! MemCm (r, "<region>", 8))  {in = 0;   rp += 8;}
         else if (! MemCm (r, "<group>" , 7))  {in = 1;   rp += 7;}
         else {
            in = 2;
            if ((p = StrCh (rp, '>'))) rp = ++p;
         }
      // done w a region?  write stuff
         if (inp == 0)  DoWav (dp, sm, got, sfz);
         if (in  != 2)  {MemSet (got [in], 'n', sizeof (got [0]));
//DBG("   in=`s", LIn [in]);
         }
      }                                // unless global, reset all got

   // parse dem params in da rec:  first the strings w spaces at eol n chop
//DBG("   parsin params");
      if ((p = StrSt (rp, "default_path="))) {
//DBG("   default_path");
         StrCp (r2, & p [13]);   *p = '\0';
         if (*r2 == '$') {             // got a #define prob only in *bank.xml:(
            q = StrCh (r2, '/');       // part to append later after $etc/
            nl = dr.DLst (Top, ls, BITS (ls));
            for (j = 0;  j < nl;  j++)  if (StrSt (ls [j], "sample"))
               {StrFmt (dp, "`s/`s/", Top, ls [j]);   break;}
         // we're dead :(
            if (j >= nl)  {
DBG ("CAN'T FIND SAMPLE DIR :(");   return;}

            StrAp (dp, q+1);           // tack on suffix
         }
         else
            StrFmt (dp, "`s/`s", pa, r2);
         StrCp (dp, & dp [StrLn (Top)+1]);
LF.Put (StrFmt (ps, "default_path=`s\n", dp));
      }
      if ((p = StrSt (rp, "sample="))) {
//DBG("   sample");
         if ((q = StrCh (& p [7], ' ')) == NULL) {    // no spaces - noice :)
            StrCp (sm, & p [7]);   *p = '\0';
            if ((q = StrCh (sm, '$'))) {
               *q = '\0';
               if (sm [StrLn (sm)-1] == '.')  StrAp (sm, "", 1);
               StrAp (sm, ".wav");
               StrFmt (ts, "`s/`s`s", Top, dp, sm);
               if (! f.Size (ts))  StrAp (sm, ".flac", 4);
            }
            FnFixSl (sm);
         }
         else {
            if (p [7] == '*') {                       // ez-est at least
               MemCp (sm, & p [7], e = (ubyt4)(q-p-7));
               sm [e] = '\0';
               FnFixSl (sm);
               StrCp (p, q);
            }
            else {
            // got spaces so look for ending .wav / .flac
            // p points to sample=... in rec buf
            // r2 buf holds everything after sample=
            // q points to .ext in r2 buf
               StrCp (r2, & p [7]);
               if      ((q = StrSt (r2, ".wav"))) {
                  StrCp (p, (q [4] == ' ') ? (& q [4]) : "");
                  q [4] = '\0';   StrCp (sm, r2);
                  FnFixSl (sm);
DBG("   .wav: sm=`s", sm);
               }
               else if ((q = StrSt (r2, ".flac"))) {
                  StrCp (p, (q [5] == ' ') ? (& q [5]) : "");
                  q [5] = '\0';   StrCp (sm, r2);
                  FnFixSl (sm);
DBG("   .flac: sm=`s", sm);
               }
               else if ((q = StrCh (r2, '$'))) {
                  if ((x = StrCh (q, ' ')))  StrCp (p, x);
                  else                       *p = '\0';
                  *q = '\0';
                  if (r2 [StrLn (r2)-1] == '.')  StrAp (r2, "", 1);
                  StrCp (sm, r2);
                  StrAp (sm, ".wav");
                  FnFixSl (sm);
                  StrFmt (ts, "`s/`s`s", Top, dp, sm);
//DBG("   pa=`s dp=`s sm=`s ts='`s' size=`d", Top, dp, sm, ts, f.Size (ts));
                  if (! f.Size (ts))  StrAp (sm, ".flac", 4);
                  if (! f.Size (ts)) {
DBG("CAN'T GET SAMPLE :( `s", ts);   return;}
               }
               else {
DBG("CAN'T GET SAMPLE :( `s", r2);   return;}
            }
         }
      }
   // now the space sep'd params
     ColSep s (rp, 30);
//DBG("   rp='`s'", rp);
      for (j = 0;  j < 30;  j++)  if (s.Col [j][0]) {
         no1 = ((got [0][NO] == 'y') || (got [1][NO] == 'y') ||
                                        (got [2][NO] == 'y')) ? true : false;
         StrCp (ts, s.Col [j]);
         while (*ts == ' ')  StrCp (ts, & ts [1]);    // 1st col needs leadin
DBG("   ts=`s", ts);
         if      (! MemCm (ts, "set_cc", e = 6)) {    // spaces killed still
            t = Str2Int (& ts [e]);    // t is our cc#  t2 is it's value
            if ((p = StrCh (ts, '=')))  t2 = Str2Int (p+1);
            else                        t2 = -999999;
            for (e = 0;  e < (ubyt4)ncc;  e++)  if (cc [e] == t)  break;
            if (e < (ubyt4)ncc)  ccv [e] = t2;
            else if (ncc < BITS (cc))  {cc [ncc] = t;   ccv [ncc++] = t2;}
         }

         else if (! MemCm (ts, "pitch_keycenter=", e = 16))
            {got [in][KY] = 'y';   sfz [in][KY] = Key2Int (& ts [e]);}

         else if (! MemCm (ts, "transpose=",       e = 10))
            {got [in][TR] = 'y';   sfz [in][TR] = Str2Int (& ts [e]);}
         else if (! MemCm (ts, "tune=",            e = 5))
            {got [in][CT] = 'y';   sfz [in][CT] = Str2Int (& ts [e]);}

         else if (! MemCm (ts, "lokey=", e = 6))
            {got [in][KL] = 'y';   sfz [in][KL] = Key2Int (& ts [e]);}
         else if (! MemCm (ts, "hikey=", e = 6))
            {got [in][KH] = 'y';   sfz [in][KH] = Key2Int (& ts [e]);}

         else if (! MemCm (ts, "key=", e = 4))
            {got [in][KY] = got [in][KL] = got [in][KH] = 'y';
             sfz [in][KY] = sfz [in][KL] = sfz [in][KH] =
                                                  Key2Int (& ts [e]);}
         else if (! MemCm (ts, "lovel=", e = 6))
            {got [in][VL] = 'y';   sfz [in][VL] = Vel2Int (& ts [e]);}
         else if (! MemCm (ts, "hivel=", e = 6))
            {got [in][VH] = 'y';   sfz [in][VH] = Vel2Int (& ts [e]);}

         else if (! MemCm (ts, "offset=", e = 7))
            {got [in][SB] = 'y';   sfz [in][SB] = Str2Int (& ts [e]);}
         else if (! MemCm (ts, "end=",    e = 4))
            {got [in][SE] = 'y';   sfz [in][SE] = Str2Int (& ts [e]);}

         else if (! MemCm (ts, "loop_start=", e = 11))
            {got [in][LB] = 'y';   sfz [in][LB] = Str2Int (& ts [e]);}
         else if (! MemCm (ts, "loop_end=",   e =  9))
            {got [in][LE] = 'y';   sfz [in][LE] = Str2Int (& ts [e]);}

         else if (! MemCm (ts, "loop_mode=",  e = 10)) {
            if      (! StrCm (& ts [e], "no_loop"))          t = 0;
            else if (! StrCm (& ts [e], "one_shot"))         t = 0;
            else if (! StrCm (& ts [e], "loop_sustain"))     t = 1;
            else if (! StrCm (& ts [e], "loop_continuous"))  t = 1;
            else                                             t = 1;  // that ok?
            got [in][LO] = 'y';   sfz [in][LO] = t;
         }

         else if (! MemCm (ts, "pan=",    e = 4))
            {got [in][PA] = 'y';   sfz [in][PA] = Str2Int (& ts [e]);}

         else if (! MemCm (ts, "pitch_keytrack=", e = 15))
            {got [in][PT] = 'y';   sfz [in][PT] = Str2Int (& ts [e]);
             pTrk = 1;}

      // look for dumb layer stuff to ignore w NO
         else if ((! MemCm (ts, "locc", 4)) ||
                  (! MemCm (ts, "hicc", 4))) {
            t = Str2Int (& ts [4]);         // cc # we gots
            for (e = 0;  e < (ubyt4)ncc;  e++)  if (cc [e] == t)  break;
            t3 = (e < (ubyt4)ncc) ? ccv [e] : dfcc [t];
            if ((p = StrCh (ts, '='))) {
               t2 = Str2Int (p+1);          // value we compare to
               if (! MemCm (ts, "lo", 2))   // skip if ccv beyond limit
                     {if (t3 < t2)  got [in][NO] = 'y';}
               else  {if (t3 > t2)  got [in][NO] = 'y';}
            }
            else  got [in][NO] = 'y';       // no = ?  NRRRR !!
            if ((! no1) && (got [in][NO] == 'y'))
               DBG("   NOPE on locc/hicc: `s valu=`d in=`s", ts, t3, LIn [in]);
         }
         else if ((! MemCm (ts, "xfin_", 5)) ||
                  (! MemCm (ts, "xfout_", 6))) {
            got [in][NO] = 'y';             // toss if xfade
            if (! no1) DBG("   NOPE on xfin_/xfout_: `s in=`s", ts, LIn [in]);
         }
         else if ((! MemCm (ts, "on_locc", 7)) ||
                  (! MemCm (ts, "on_hicc", 7))){
            got [in][NO] = 'y';             // toss if CC trigger
            if (! no1) DBG("   NOPE on on_locc/on_hicc: `s in=`s", ts, LIn[in]);
         }
         else if (! MemCm (ts, "trigger=", e = 8)) {
            if (StrCm (& ts [e], "attack"))
               got[in][NO] = 'y';           // toss if weird trigger=
            if ((! no1) && (got [in][NO] == 'y'))
               DBG("   NOPE on trigger=(non attack): `s in=`s", ts, LIn [in]);
         }
         else if (! MemCm (ts, "lorand=", e = 7)) {
            for (t = e;  ts [t];  t++)      // ignore unless lorand=0.0
               if ((ts [t] != '0') && (ts [t] != '.'))  break;
            if (ts [t])  got [in][NO] = 'y';
            if ((! no1) && (got [in][NO] == 'y'))
               DBG("   NOPE on lorand=(non0): `s in=`s", ts, LIn [in]);
         }
         else if (! MemCm (ts, "seq_position=", e = 13)) {
            if (Str2Int(&ts[e]) > 1)  got[in][NO] = 'y';
            if ((! no1) && (got [in][NO] == 'y'))
               DBG("   NOPE on seq_position=(non1): `s in=`s", ts, LIn [in]);
         }
//       else                               // ignore unless seq_position=1
//          DBG ("???  `s", ts );
      }
   }
   if (in == 0)  DoWav (dp, sm, got, sfz);
   KGot [MKey ("8c")+1] = '\0';
   i = 0;   p = & KGot [MKey ("0a")];
   while ((q = StrSt (p, "_*")))  {i++;   p = q+2;}
LF.Put (StrFmt (ps, "`s `s\n",
((i > 2)||pTrk) ? "DRUM ?? " : "MELO ?? ", & KGot [MKey ("0a")]));
}


//______________________________________________________________________________
bool DoDir (void *ptr, char dfx, char *fn)
// put any .sfz into Fn[NFn]
{ ubyt4 ln = StrLn (fn);
   if ( (dfx == 'f') && (ln > 4) && (! StrCm (& fn [ln-4], ".sfz")) &&
        (NFn < BITS (Fn)) )
      StrCp (Fn [NFn++], fn);
   return false;
}


int Cmp (void *p1, void *p2)
{ char *s1 = (char *)p1, *s2 = (char *)p2;   return StrCm (s1, s2);  }


//______________________________________________________________________________
int main (int arc, char *argv [])
{ TStr  fn, ts, pa, ls, dr;
  BStr  r;
  char *pc;
  Path  p;
  File  f;
  ubyt4 i, j, k;
  char *rp;
  StrArr a;
DBGTH("sfz2syn");
DBG("bgn");
   App.Init ();
   StrCp (Top, argv [1]);
DBG("Top=`s", Top);
// Top minus path (no ext) => To - our sampleset in syn dir
   FnName (fn, Top);
   while ((pc = StrCh (fn, ' ')))  StrCp (pc, pc+1);
   while ((pc = StrCh (fn, '_')))  StrCp (pc, pc+1);
   StrFmt (To, "`s/device/syn/`s", App.Path (ts, 'd'), fn);
DBG("To=`s", To);

// ok, (re)make To dir n open log.txt there
   p.Kill (To);   p.Make (To);
   if (! LF.Open (StrFmt (fn, "`s/log.txt", To), "w")) {
DBG("couldn't write file=`s", fn);
      return 99;
   }

// FN [NFn] = dir's .SFZ files sorted by fn
   NFn = 0;   f.DoDir (Top, NULL, (FDoDirFunc)(& DoDir));
   Sort (Fn, NFn, sizeof (Fn[0]), & Cmp);

// 1st, kill the #included fns :/
   for (i = 0;  i < NFn;  i++) {
      a.Init ("sfz", 16*1024);
DBG("loadin `s", Fn [i]);
      a.Load (Fn [i]);
      StrCp (pa, Fn [i]);   Fn2Path (pa);
      for (j = 0;  j < a.NRow ();  j++) {
         StrCp (r, a.str [j]);
         if (! MemCm (r, "#include ", 9)) {
            rp = & r [9];
            if ((pc = StrCh (rp, '"')))
               {rp = pc+1;   if ((pc = StrCh (rp, '"')))  *pc = '\0';}
            StrFmt (fn, "`s/`s", pa, rp);
//DBG("#include `s", fn);
            for (k = 0;  k < NFn;  k++)  if (! StrCm (Fn [k], fn)) {
//DBG(" got it");
               RecDel (Fn, NFn--, sizeof (Fn [0]), k);
               if (k < i)  i--;
               break;
            }
         }
      }
   }

// load drum.txt which sez which presets are drum
   StrCp (ts, Top);   StrAp (ts, "/drum.txt");
   a.Init ("drum", 128);
DBG("loadin drum.txt");
   a.Load (ts);

// ok, plow em
   for (i = 0;  i < NFn;  i++) {
      FnName (ts, Fn [i]);   StrAp (ts, "", 4);
      *Kit = '\0';
      for (j = 0;  j < a.NRow ();  j++) {
         StrCp (dr, a.str [j]);        // cuz dr is wreckable
        ColSep s (dr, 1);
         if (StrSt (ts, s.Col [1]))  {StrCp (Kit, s.Col [0]);   break;}
      }
      StrFmt (ls,"Converting preset `d of `d fn='`s' Kit='`s'",
              i+1, NFn, & Fn [i][StrLn (Top)+1], Kit);
LF.Put(ls);   LF.Put ("\n");   DBG(ls);
      DoSfz (Fn [i]);
   }
   LF.Shut ();
   App.Run ("synsnd");
DBG("end");
   return 0;
}
