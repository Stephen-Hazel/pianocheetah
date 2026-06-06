// song.h - thread dealin w song data, midi, timer - da guts

#pragma once
#include "glo.h"

// lrn modes:  hear, hearRec, practice, play
#define LHEAR  'a'
#define LHREC  'b'
#define LPRAC  'c'
#define LPLAY  'd'
#define  HEAR  (Up.lrn == LHEAR)       // hear lrn
#define  HREC  (Up.lrn == LHREC)       // hear rec
#define  PRAC  (Up.lrn == LPRAC)
#define  PLAY  (Up.lrn == LPLAY)

#define   RCRD (PRAC || PLAY)
#define SHRCRD (RCRD || (_lrn.pLrn == LPRAC) || (_lrn.pLrn == LPLAY))

extern void   CInit ();
extern QColor CMap (ubyte n);

extern char  KeyCol [13];              // in sEdit.cpp
extern ubyte WXOfs  [12];

struct CCMDef {ubyte dev;   ubyt2 cc;   WStr str;};

//______________________________________________________________________________
extern int   EvCmp   (void *p1, void *p2);       // ..._TrkEv[] sortin
extern char *SongRec (char *buf, ubyt2 len, ubyt4 pos, void *ptr);
//extern KSgRow CSig;

const ubyte TB_DSC = 0;                // my .song table format stuph
const ubyte TB_TRK = 1;                // Track:    dev snd trkName
const ubyte TB_DRM = 2;                // DrumMap:  note snd
const ubyte TB_LYR = 3;                // Lyric:    time name
const ubyte TB_EVT = 4;                // Event:    placeholder fake STable...:/
const ubyte TB_MAX = 5;                // number of em

class SongFile {                       // all the data from a .song file
public:
   bool  got;                          // got somethin loaded?
   TStr  fn;                           // filename

   char                  dsc [65536];
   Arr<TrkRow, MAX_TRK>  trk;
   Arr<MapDRow,MAX_DRM>  mapD;
   Arr<TxtRow, MAX_LYR>  lyr, sct, chd, cue, bug;
   Arr<CtlRow, 128    >  ctl;
   TrkEv *ev;
   ubyt4 nEv, maxEv;

// song settings from _dsc section.  DscSave()/DscLoad() preserve/restore these
   ubyt4 tmpo;                         // tempo per-thousand-age
   sbyte tran;                         // transpose halfsteps

// special ctls
   Arr<TpoRow,MAX_SIG>  tpo;
   Arr<TSgRow,MAX_SIG>  tSg;
   Arr<KSgRow,MAX_SIG>  kSg;
};


//______________________________________________________________________________
struct RecDef {ubyt4 tm;  ubyte vl;};
struct LrnDef {
   char   pLrn;                        // prev lrn mode
   ubyt4  lpBgn, lpEnd;                // current loop's bgn,end times
   bool   POZ,                         // paused?
          nt1,                         // 1st note after loop HopTo?
          chd;                         // w of cue area
   ubyte  hld [7];                     // held note per track
   RecDef rec [2][128],                // time of rec notes in _dn via NtGet()
         prec [2][128];
};


//______________________________________________________________________________
// notation stuph
struct BlkDef  {ubyt2 bar, y, h;   ubyt4 tMn, tMx;   ubyte sb;};
                                                 // bar's tm block, y scale
struct SymDef  {ubyte tr;   ubyt4 nt, tm;        // tm only for ez
                bool top, bot;   ubyt2 x, y, w, h;};  // note's symbols
struct ColDef  {BlkDef *blk;   SymDef *sym;   ubyt4 nBlk,  nSym;
                ubyte nMn, nMx,    oMn [7], oMx [7],   nDrm, dMap [128];
                ubyt2 x, nx, dx, cx, w, h,  oX  [7];};
struct PagDef  {ColDef *col;   ubyt4 nCol;   ubyt2 w, h;};


//______________________________________________________________________________
class Song: public QObject {
   Q_OBJECT
public:
   Song ()                             // prep for 1st Wipe()
   {  _ed = 0;   *_f.fn = '\0';   _f.ev = nullptr;   _nt = nullptr;
      Up.pos.at = Up.pos.drg = '\0';
   }

private:
// sTime.cpp
   TSgRow *TSig  (ubyt4 tm);
   KSgRow *KSig  (ubyt4 tm);
   char *TmStr   (char *str, ubyt4 tm, ubyt4 *tL8r = nullptr,
                                       ubyte *subt = nullptr);
   char *TmSt    (char *str, ubyt4 tm);
   ubyt2 Tm2Bar  (ubyt4 tm);
   bool  Poz     (bool tf, ubyt4 msx = 0);
   ubyt4 Bar2Tm  (ubyt2 b, ubyte bt = 1);
   ubyt4 Str2Tm  (char *ts);
   void  TmHop   (ubyt4 tm);
   ubyte CCValAt (ubyt4 tm, ubyte trk, char *cc);
   ubyt2 TmpoAct (ubyt2 val);
   ubyt2 TmpoSto (ubyt2 act);
   ubyt2 TmpoAt  (ubyt4 tm, char act = '\0');    // return stored unless 'a'

// sDevice.cpp  (n sDevTyp.cpp)
   void  OpenMIn (), ShutMIn ();
   void  CCMapLoad ();

   char *DevName (ubyte t)
   {  if (!  Up.dev [_f.trk [t].dev].mo)  return CC("");
      return Up.dev [_f.trk [t].dev].mo->Name ();
   }
   char *SndName (ubyte t)
   {  if (_f.trk [t].snd == SND_NONE)  return CC("");
      return Up.dvt [Up.dev [_f.trk [t].dev].dvt].Snd (_f.trk [t].snd)->name;
   }
   void  NotesOff ();                  // notes+hold OFF
   void  PickDev (ubyte tr, char *sndName, char *devName = nullptr);
   ubyte OpenDev (char *nm);
   void  ShutDev (ubyte d);
   ubyte PickChn (char *dv);
   void  SetBnk ();
   void  SetChn (), SetChn (ubyte t);

// sFile.cpp
   bool  DscGet  (char *key, char *val);
   void  DscPut  (char *repl);
   void  DscInit (), DscLoad (), DscSave ();
   ubyte DrumCon ();                   // outputs _mapD
   void  DrumExp (bool setBnk = true);    // usin _mapD
   void  CCClean ();
   void  TmpoPik (char l_r);           // stamp cur tmpo w lrn or rec'd vals
   void  Load    (char *fn);
   void  Save    (char rec = 'a');     // 'a' for a.song  'r' for rec/.song

// sDump.cpp
   char *LrnS     ();
   void  DumpEv   (TrkEv *e, ubyte t, ubyt4 p = 1000000, char *pre = nullptr);
   void  DumpTrEv (ubyte t);
   void  DumpDn   ();
   void  DumpRec  ();
   void  Dump     (bool ev2 = false);

// sRecord.cpp
   void  Shush   (bool tf);            // flip by volume cc (only) on/off
   bool  DnOK    (char n = '\0', ubyte *tr = nullptr, MidiEv *ev = nullptr);
   bool  NtCmd   (MidiEv *ev);
   void  NtGet   (MidiEv *ev);
   void  SetMSec (ubyt4 p, MidiEv *ev);
   bool  Record  (MidiEv *ev);
   bool  EvRcrd  (ubyte dev, MidiEv *ev);

// sChrd.cpp
   void  PreChd ();
   void  Chd    (char *arg);
   void  PopChd (ubyt4 tm);
   void  ChdArr (ubyt4 tm);

// sUtil.cpp
   ubyt4 GetDn   (TrkEv *e, ubyt4 p, ubyte nt);
   ubyt4 GetUp   (TrkEv *e, ubyt4 p, ubyt4 ne, ubyte nt);
   ubyt2 Nt2X    (ubyte n, ColDef *co, char gr = '\0');
   ubyt2 Tm2Y    (ubyt4 t, ColDef *co, BlkDef **bl = nullptr);
   ubyt4 Y2Tm    (ubyt2 y, ColDef *co);
   ubyt4 SilPrv  (ubyt4 tm), SilNxt  (ubyt4 tm);
   ubyt4 NtDnPrv (ubyt4 tm), NtDnNxt (ubyt4 tm);
   void  ReCtlO  ();                   // map all DevTyps w song's _ctl[]s
   ubyte CtlEv   (char *cSt, char ro = '\0');
                                       // get _ctl pos; upd _cch,_ctl,dvt.CCMap
   char *CtlSt   (ubyte ctrl);
   void  CtlClean();                   // redo _ctl[] to just used ones sorted
   void  EvDel   (ubyte t, ubyt4 p, ubyt4 ne = 1);
   bool  EvIns   (ubyte t, ubyt4 p, ubyt4 ne = 1);
   void  EvInsT  (ubyte t, TrkEv  *ev);
   void  EvInsT  (ubyte t, MidiEv *ev);
   void  NtIns   (ubyte t, ubyt4 tm, ubyt4 te, ubyte c, ubyte v = 100);
   ubyt4 NtHit   (ubyte t, ubyt4 tmn, ubyt4 tmx, ubyte nmn, ubyte nmx);
   void  Split   ();
   bool  TxtIns  (ubyt4 tm, char *s, Arr<TxtRow,MAX_LYR> *at, char cue = '\0');
   void  TrkSnd  (ubyt4 snew);
   void  NewGrp  (char *grp),  NewSnd (char *snd),  NewDev (char *dev);
   void  TrkSplt ();
   void  TrkDel  (ubyte t);
   ubyte TrkIns  (ubyte t = MAX_TRK, char *name = nullptr,
                                     char *snd  = nullptr);
   void  LoopInit ();
   void  DrMap   (char *d);
   void  TrkEd   (char *op);
   ubyte GetSct  (TxtRow *sct);        // pull sections outa _f.cue[] > sct[64]

// sEdit.cpp
   void  PreTDr  (bool kick = true);
   void     TDr  (char *arg);
   void  PreCtl  ();
   void     Ctl  ();
   void  SetCtl  (char *arg);
   void  Cue     (char *s);
   void  PreQua  ();
   void     Qua  (char *tnf);
   void  NtDur   ();
   void  NtHop   ();
   void  Mov     ();

// sEdMs.cpp
   void  DragRc  ();
   char  MsPos   (sbyt2 x, sbyt2 y);
   void  DbgPos  (char x = '\0');      // MsDn,Mv,Up are slots below

// sCmd.cpp
   ubyte ChkETrk ();
   void  RecWipe (), Msg (char *s),
         EdSong (char ofs), EdTime (char ofs), EdTmpo (char ofs),
                                               EdLrn  (char ofs),
         ShowAll (), HType  (char c);

// sReDo.cpp
   bool  TSho (ubyte t), TLrn (ubyte t), TRec (ubyte t), TDrm (ubyte t);
   void  ReTrk  ();                    // give gui _trk info ta draw
   ubyt4 ReEv   (bool tpo = false);    // redo _tpo,_tSg,_kSg,_tEnd,etccccc
   void  BarH   (ubyt2 *h, ubyte *sb, ubyt2 b);
   void  SetDn  (char q = '\0'),       // default to no quantize
         SetLp  (char dir),
         SetNt  (),
         SetSym ();
   void  ReDo   ();

// sNote.cpp
   ubyte DrawRec (bool all, ubyt4 pp);
   void  DrawSym (SymDef *s, ColDef *co);
   void  DrawPg  (ubyt4 pp);
   void  DrawNow ();
   void  Draw    (char all = '\0');    // da top o notation drawin

// song.cpp
   void  PutTp (ubyt2 tp);
   void  PutTs (ubyte n, ubyte d, ubyte sb);
   void  PutLy ();
   void  PutCC (ubyte t, TrkEv *e);
   void  PutNt (ubyte t, TrkEv *e, bool bg = false);

   void  Init (), Quit ();
   void  Wipe ();
   void  Info (char *msg);             // into lyr area
   void  Hey  (char *msg);             // pop gui.hey in gui thr
   void  Die  (char *msg);             // pop gui.hey n croak

   Timer               *_timer;
   Arr<MInDef,MAX_DEVI> _mi;
   SongFile             _f;
   Arr<CCMDef,128>      _ccMap;
   Arr<TrkEv,MAX_RCRD>  _recM;         // rec events - melodic
   Arr<TrkEv,MAX_RCRD>  _recD;         // rec events - drum
   Arr<ubyt4,128>       _sySn;         // syn's sound bank: just melodic no drum

   ubyt4  _tEnd;                       // last time
   ubyt2  _bEnd;                       // last bar
// _now is curr time if poz'd, else next time Put happens (usually in future)
// _pNow to _rNow is timeframe for DrawNowP to update
//    Put, EvRcrd update _rNow;  _pNow follows after  (tSoon/Late window pDn tm)
   ubyt4  _now, _pNow, _rNow;
   LrnDef _lrn;

   ubyte _ed;                          // piano key editin'
   ubyt4 _pLyr, _hLyr, _pChd;

   Arr<CChRow,MAX_CCH>  _cch;          // ctl chasing
   TrkNt                *_nt;          // size is always same as _f.nEv
   Arr<DownRow,128*1024> _dn;          // notes ta learn
   ubyt4                _pDn;

   ubyt4  _pg;   ubyte _tr;   QRect _rc;    // ...notation junk
   Arr<PagDef,200   >  _pag;                // page and transitioning?
   Arr<ColDef,500   >  _col;
   Arr<BlkDef,9999  >  _blk;
   Arr<SymDef,131584>  _sym;

public slots:
   void Cmd (QString s);               // gui listenin
   void ReSz ()                        // note widget resize/repixmap
   {  TRC("ReSz");   _pg = _tr = 0;   SetSym ();   Draw ();  }

   void Put  ();                       // pcheetah's heartbeat
   void MIn  ();                       // top o da recordin' biz
   void MsDn (Qt::MouseButton  b, sbyt2 x, sbyt2 y);  // edit bizzz
   void MsMv (Qt::MouseButtons b, sbyt2 x, sbyt2 y);
   void MsUp (Qt::MouseButton  b, sbyt2 x, sbyt2 y);

signals:
   void sgCmd (QString cmd);
   void sgUpd (QString upd);           // gui talkin
};
