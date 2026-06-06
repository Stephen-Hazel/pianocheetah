// glo.h - global vars sigh

#pragma once
#include "stv/ui.h"
#include "stv/timer.h"
#include "stv/syn.h"
#include "stv/midi.h"


const ubyt4 LRN_BT = M_WHOLE/64;       // wakeup slice for Put
const ubyt4 NONE   = 0x7FFFFFFF;       // time of this means "no time"
                                       // .p of this means "no event/note"
const ubyt2 W_Q   = 24;                // w of cue area (not incl chord w)
const ubyt2 W_NT  = 14;                // w of black/drum note  th(text h)=20
const ubyt2 W_NTW = 24;                // w of white note
const ubyt2 H_NW  = 24;                // h of now line
const ubyt2 H_T   = H_NW+8;            // h of tmp cnv for bg behind now
                                       // plus half dots below it
const ubyt2 H_KB = 47;                 // h for keys (oct.bmp)

const ubyte MAX_DEVI = 8;              // max midiin devices to listen to
const ubyt4 MAX_RCRD = 32000;          // max events for recording in btw saves

// MAX_DEV used as max midiout devices for song
const ubyt2 MAX_CCH  = 512;            //     controllers to chase
const ubyt2 MAX_SIG  = 4096;           //     time,keysigs

const ubyte PRG_NONE = 255;            // prg value for "no program"

const ubyt2 SND_MAX  = 63*1024;        // max sounds per DevTyp
const ubyt2 SND_MAXD = 1024;           // max sound dirs per DevTyp
const ubyt4 SND_NONE = 0xFFFFFFFF;     // no sound


struct UCmdDef {const char *cmd, *grp, *desc;   WStr nt, ky;};
extern UCmdDef UCmd [];                // user's key map
extern ubyte  NUCmd;


//______________________________________________________________________________
// track, melo/drum note, ev pos of a notedown
struct NtDef   {ubyte t, nt;   ubyt4 p;};

// msec of rec ev down calcs (micro) tmpo btw ev downs and clip sez if way off
// nt[] has notes to hit (lrn) and velo has ez trk velos
struct DownRow {ubyt4 time, msec;  ubyt2 tmpo;  char clip;
                ubyte nNt;  NtDef nt [14];  ubyte velo [8];};

// combined ntDn,ntUp and if there's overlap btw tracks
struct TrkNt   {ubyt4 dn, up, tm, te;   ubyte nt;   bool ov;};

// drum map cache - use: into drumCon, outa drumExp (Load,Save) n temp in SetBnk
struct MapDRow {ubyte ctl, inp, vol, pan;   ubyt4 snd;   bool shh, rec, lrn;
                                                         char ht;};

//______________________________________________________________________________
struct TrkRow  {ubyte  dev, chn,   din, drm,   vol, pan;
                bool   grp, lrn, shh, rec;  // these 2^ JUST for syn drum chans
                char   ht;
                TStr   name, etc;
                ubyt4  snd;
                TrkEv *e;
                TrkNt *n;
                ubyt4 ne, nn, nb, p;};           // #e, #n, #broke, pos
struct CtlRow  {WStr s;   char sho;};            // song's ctl map sho=y,n,m(ini
struct CChRow  {ubyte dev, chn, ctl, trk, valu, val2;  ubyt4 time;};
struct TxtRow  {ubyt4 time, tend;  TStr s;};     // lyr,sct,chd,cue,bug
struct TpoRow  {ubyt4 time;  ubyt2 val;};                       // tempo (orig)
struct TSgRow  {ubyt4 time;  ubyte num, den, sub;  ubyt2 bar;}; // timesig
struct KSgRow  {ubyt4 time;  ubyte key, min, flt;};             // keysig


//______________________________________________________________________________
// midi devtype base stuph
struct CcRow  {ubyt2 raw;   WStr map;};
struct SnRow  {TStr name;   ubyte prog, bank, bnkL;};


// midi IN stuph
struct MInDef {                        // midi in dev stuff
   MidiI          *mi;
   Arr<CcRow, 128> cc;
   static char *CcRec (char *buf, ubyt2 len, ubyt4 pos, void *ptr);
};


// midi OUT stuph
struct DevRow  {MidiO *mo;   ubyte dvt;};

class DevTyp {     // sounds and controls used per midiout device type
public:
         DevTyp ();
   void  Open (char *devTyp);
   void  Shut ();
   char *Name ()  {return _name;}
   ubyt4 NCC  ()  {return _cc.Ln;}
   ubyt4 NSn  ()  {return _sn.Ln;}
   ubyt4 NDr  ()  {return _nDr;}

   ubyt2 CCID (char *name)             // str to dvt's raw out ubyt2
   {  for (ubyte c = 0; c < _cc.Ln; c++)  if (! StrCm (name, _cc [c].map))
                                                      return _cc [c].raw;
      return 0;
   }

   ubyt4  SndID  (char *name, bool xmatch = false);
   SnRow *Snd    (ubyt4 id)  {if (id >= _sn.Ln) id = 0;   return & _sn [id];}
   bool   SndNew (ubyt4 *pnew, ubyt4 pos, char ofs);
   void   SGrp   (char *t, bool dr);   // ...gui hooks
   void   SNam   (char *t, char *grp, bool dr);
   void   Dump   ();

   ubyt2 CCMap [128];                  // map song ctl id => raw midi ubyt2
   TStr               _name;
   Arr<SnRow,SND_MAX> _sn;   ubyt4 _nDr;
   Arr<CcRow,128    > _cc;
private:
   static char *SnRec (char *buf, ubyt2 len, ubyt4 pos, void *ptr);
   static char *CcRec (char *buf, ubyt2 len, ubyt4 pos, void *ptr);
};


//______________________________________________________________________________
// edit pos junkss
struct PosDef {
   char  at, got, drg;                 // sEdit.cpp docs these
   ubyt4 pg, co, tm, sy, p;            // page, column, time, symbol, trk ev pos
   ubyt4 tmBr, tmBt, hBt;              // trunc'd bar time, nearest beat,
   sbyt2 x1, y1, x2, y2, xp, yp, xo, yo;               // n half beat dur
   ubyte ct, cp, tr;                   // control, ctl pos, track
   TStr  str, etc, stp, stn;
   KSgRow kSg;
   bool  pPoz;                         // pause prev on
};


//______________________________________________________________________________
struct UTrkRow {
   ubyte dvt;   bool drm;   TStr lrn, ht, name, grp, snd, dev;   BStr tip;
};
struct UpdLst {                        // dlg val passin, etc, etc
   ubyt2 txH;
   QPixmap *oct, *bg [2], *bg2 [2], *now, *dot, *cue, *bug, *fade, *tr;
// stuff Song sets for gui
   bool  uPoz;                         // user said poz, not just learn mode
   char  lrn;
   TStr  hey, ttl, song, time, bars, tmpo, tsig, lyr;
   Arr<DevTyp,MAX_DEV>  dvt;           // for showin gui
   Arr<DevRow,MAX_DEV>  dev;
   Arr<UTrkRow,MAX_TRK> trk;
   ubyte    eTrk;                      // edit trk
// CtlNt's stuff
   QPointF  gp;                        // globalPos() x,y for movin dlgs
   char     ntCur;
   ubyt2    w, h;                      // ctlNt's size
   QRect    tpos, drag;                // drawnow's area to update, ms drag rc
   QPixmap *pm, *tpm;                  // main(all) and now(update) note pixmaps
   Canvas   cnv, tcnv;
   PosDef   pos, posx;                 // edit pos junkss and a copy when needed

   ubyte   nR, id, rHop;
   ubyt2       icc;
   TStr     d [256][6];
};
extern UpdLst Up;                      // what gui needs from song

extern QColor CScl [2][12];


//______________________________________________________________________________
struct CfgDef {
// global settings from etc/cfg.txt       (global prefs for all songs)
   ubyte ntCo;                         // note color
   bool  barCl;                        // send bar# to clipboard? (for lyr edit)
   void  Init (), Load (), Save ();    // for global settings (not song)
   sbyte tran;
};
extern CfgDef Cfg;

struct FLstDef {
   const ubyt4      MAX = 100000;      // sigh
   const ubyte      X = sizeof (TStr)-1;
   Arr<TStr,100000> lst;               // last byte flags rand already picked
   ubyt4            pos,  xLen, xPos;
   TStr                   xFn;
   bool             ext;
   void Load (), Save ();
   bool DoFN  (char *fn);
   bool DoDir (char *dir);
};
extern FLstDef FL;


extern ubyte ChdBtw (TStr **out, char *i1, char *i2);      // in sChd.cpp
