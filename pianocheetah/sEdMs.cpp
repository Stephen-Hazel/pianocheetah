// sEdMs.cpp - handle mouse Dn,Up,Mv n kickin dialogs

#include "song.h"


void Song::DragRc ()
{ ubyt2 x1, x2, y1, y2, t;
   Up.pos.pg1 = Up.pos.pg;   Up.pos.co1 = Up.pos.co;
   x1 = Up.pos.x1;   y1 = Up.pos.y1;   x2 = Up.pos.x2;   y2 = Up.pos.y2;
   if (x2 < x1)  {t = x1;   x1 = x2;   x2 = t;}
   if (y2 < y1)  {t = y1;   y1 = y2;   y2 = t;}
   Up.drag.setLeft  (x1);   Up.drag.setTop    (y1);
   Up.drag.setRight (x2);   Up.drag.setBottom (y2);
   emit sgUpd ("nt");
}

const ubyt4 DRAG = 2;                  // drag threshold (if <, then plain clik)

char Song::MsPos (sbyt2 x, sbyt2 y)
// find mouse n set Up.pos.pg co tm etc
// at:  \0 [k]eys [c]hord [q]cue [r]cue.tend [x]control [f]inger [d]ur [n]ewNote
// at q,r,x:  got=\0 or 'y'
// at x:      ct
// at f,d:    sy
{ ubyt4 c, p, ne, tm1, tm2;
  ubyt2 nx, dx, cx, th = Up.txH, w;
  PagDef *pg;
  ColDef  co;
   Up.pos.at = Up.pos.got = '\0';   if (! (p = _pg))  return Up.pos.at;

// if showing transition(got _rc) next page unless w/in rect
   p--;
   if (x < _rc.left () || y < _rc.top ())  p++;
   pg = & _pag [p];
   for (c = 0;  (c+1 < pg->nCol) && (x >= pg->col [c+1].x);  c++)  ;
   MemCp (& co, & pg->col [c], sizeof (co));     // load column
   if ( (y > co.h) || (x >= (co.x+co.w-4)) || (x < (co.x+4)) )
      return Up.pos.at;                // outa col or in border - we got nothin
   nx = co.nx;   dx = co.dx;   cx = co.cx;

   Up.pos.pg = p;   Up.pos.co = c;
   Up.pos.tm = Y2Tm (y, & co);   tm1 = Y2Tm (y-2, & co);
                                 tm2 = Y2Tm (y+2, & co);
// turn tm into tmBt,tmBr,hBt (dur)
   { TStr  s;
     char *c;
     ubyt4 tm = Up.pos.tm, t1, t2, hbt;
     ubyt2 br;
     ubyte bt;
      TmStr (s, Up.pos.tm, & t2);
      br = Str2Int (s, & c);   bt = Str2Int (c+1);
      t1 = Bar2Tm (br, bt);            // trunc'd bar.beat spot
      hbt = (t2 - t1) / 2;             // half a beat dur
      Up.pos.hBt = hbt;
      if ((t2 - tm) < (tm - t1))  bt++;     // find NEARest beat
      Up.pos.tmBr = Bar2Tm (br);
      Up.pos.tmBt = Bar2Tm (br, bt);
//DBG("MsPos t=`d t1=`d t2=`d hbt=`d br=`d bt=`d s=`s",
//tm, t1, t2, hbt, br, bt, s);
   }

   if (y < H_KB)  return (Up.pos.at = 'k');      // keys area?

   if      (x < nx) {                  // chord/cue area
      if (_lrn.chd && x < (nx-W_Q))
         return (Up.pos.at = 'c');     // chord area (no dragging)

      ne = _f.cue.Ln;   Up.pos.p = NONE;   *Up.pos.str = '\0';
      for (p = 0;  (p < ne) && (_f.cue [p].time <  tm1);  p++)  ;
      for (     ;  (p < ne) && (_f.cue [p].time <= tm2);  p++) {
         if (! Up.pos.got) {
            Up.pos.got = 'y';   Up.pos.p = p;
            StrCp (Up.pos.str, _f.cue [p].s);
         }
         else {
            StrAp (Up.pos.str, CC(" / "));
            StrAp (Up.pos.str, _f.cue [p].s);
         }
      }
      if (Up.pos.got && *Up.pos.str)
         return (Up.pos.at = 'q');
   // now look thru all (unsorted) .tends
      for (p = 0;  p < ne;  p++)
         if (_f.cue [p].tend && (_f.cue [p].tend >= tm1) &&
                                (_f.cue [p].tend <= tm2)) {
            Up.pos.got = 'y';   StrCp (Up.pos.str, _f.cue [Up.pos.p = p].s);
            return (Up.pos.at = 'r');
         }
      return (Up.pos.at = 'q');        // if new just do q
   }
   else if (x >= cx) {                 // control area
     sbyt2 tx = (sbyt2)cx;
      Up.pos.cp = 0;
      for (ubyte i = 0;  i < _f.ctl.Ln;  i++)  if (_f.ctl [i].sho != 'n') {
         w = th;   if (_f.ctl [i].sho == 'y')  w += 32+2;
         if (x < (tx += w))  {Up.pos.ct = i;   break;}
         else                 Up.pos.cp++;
      }
     TrkEv *e;
     ubyt4 ne, pr;
     TStr  cs;
     bool  cg = false;                 // global ctl? (tmpo,ksig,tsig)
     ubyte tr, td = 255;               // ...so look for it on drum trk
      StrCp (cs, CtlSt (Up.pos.ct));
      if ( (! StrCm (CC("tmpo"), cs)) || (! StrCm (CC("ksig"), cs)) ||
                                         (! StrCm (CC("tsig"), cs)) ) {
         cg = true;
         for (ubyte t = 0;  t < _f.trk.Ln;  t++)
            if (TDrm (t))  {td = t;   break;}
      }
      for (tr = 0;  tr < _f.trk.Ln;  tr++)  if ( (   cg  && (td == tr)) ||
                                                 ((! cg) &&  TSho (tr)) ) {
         for (e = _f.trk [tr].e, ne = _f.trk [tr].ne,
              Up.pos.p = 0;  Up.pos.p < ne;  Up.pos.p++)
            if (e [Up.pos.p].ctrl == (0x80|Up.pos.ct)) {
               if (e [Up.pos.p].time > tm2)  break;
               Up.pos.tr     = tr;   Up.pos.got = 'p';   pr = Up.pos.p;
               if (e [Up.pos.p].time >= tm1)
                  {Up.pos.tr = tr;   Up.pos.got = 'y';   break;}
            }
         if (Up.pos.got) {
            if (Up.pos.got == 'p')  Up.pos.p = pr;
            break;       // break out ALL the way :/
         }
      }
TStr x1;
DBG("MsPos at=x ct=`d cp=`d got=`b tr=`d p=`d tm=`s",
Up.pos.ct, Up.pos.cp, Up.pos.got, Up.pos.tr, Up.pos.p, TmSt(x1,Up.pos.tm));
      return (Up.pos.at = 'x');
   }
// ok, has ta be nt area so hunt down a symbol
// d[ur] for bot half, f[ing] for top, n[ew] for not over sym
  SymDef *it = NULL;
  ubyt4   s, sy1;
   for (s = 0;  s < co.nSym;  s++)
      if ((x >= nx+co.sym [s].x) && (x < nx+co.sym [s].x + co.sym [s].w) &&
          (y >=    co.sym [s].y) && (y <    co.sym [s].y + co.sym [s].h))
         it = & co.sym [sy1 = s];
   if (! it)  return (Up.pos.at = 'n');     // no sym?  [n]ew note area
   Up.pos.sy = sy1;
   if ((it->h >= 10) && (y >= it->y + it->h*2/3))
      return (Up.pos.at = 'd');
   return    (Up.pos.at = 'f');
}


void Song::DbgPos (char x)
{ TStr s, s2, s3;
   if (x)  MemCp (& Up.pos, & Up.posx, sizeof (Up.pos));
DBG("Up.pos {\n"
    "   at=`c got=`c drg=`c pg=`d co=`d\n"
    "   tm=`s sy=`d p=`d\n"
    "   tmBr=`s tmBt=`s hBt=`d\n"
    "   x1=`d y1=`d x2=`d y2=`d xp=`d yp=`d xo=`d yo=`d\n"
    "   ct=`d cp=`d tr=`d str=`s pPoz=`b\n"
    "}",
   Up.pos.at, Up.pos.got ? Up.pos.got : ' ', Up.pos.drg ? Up.pos.drg : ' ',
   Up.pos.pg, Up.pos.co, TmS(s, Up.pos.tm), Up.pos.sy, Up.pos.p,
   TmS(s2, Up.pos.tmBr), TmS(s3, Up.pos.tmBt), Up.pos.hBt,
   Up.pos.x1, Up.pos.y1, Up.pos.x2, Up.pos.y2, Up.pos.xp, Up.pos.yp,
                                               Up.pos.xo, Up.pos.yo,
   Up.pos.ct, Up.pos.cp, Up.pos.tr, Up.pos.str, Up.pos.pPoz
);
}


void Song::MsDn (Qt::MouseButton b, sbyt2 x, sbyt2 y)
// given Up.pos, see if we're dragging
// _drg:  [q]cue [x]ctlUpd rect[m]ov [d]ur [n]oteHop
{ PagDef *pg;
  ColDef  co;
  TStr    s;
  ubyt2   nx;
   MsPos (x, y);
DBG("MsDn x=`d y=`d b=`d", x, y, b);   DbgPos ();
   if (! Up.pos.at)  return;

   if (   b == Qt::RightButton) {           // just for ctl killin
      if ((Up.pos.at == 'x') && Up.pos.got)
         Cmd (StrFmt (s, "setCtl `d `d 0 KILL KILL", Up.pos.tr, Up.pos.p));
      return;
   }
   if (! (b == Qt::LeftButton))  return;    // need regular button

   pg = & _pag [Up.pos.pg];
   MemCp (& co, & pg->col [Up.pos.co], sizeof (co));  // load column
   nx = co.nx;
   Up.pos.pPoz = Poz (true);   NotesOff ();

   if (Up.pos.at == 'k') {             // keyboard area - ctl[].sho editin
      Up.id = 99;   PreCtl ();   _pg = 0;
      if (! Up.pos.pPoz) Poz (false);
      return;
   }
   if (Up.pos.at == 'c') {             // chord area
      PreChd ();
      _pg = 0;
      if (! Up.pos.pPoz) Poz (false);
      return;
   }
   if ((Up.pos.at == 'q') || (Up.pos.at == 'r')) {    // cue time or tend
      Up.pos.drg = Up.pos.at;
      Up.pos.x1 = co.x+4;   Up.pos.x2 = co.x + co.w - 4;
                            Up.pos.yp = Up.pos.y1 = y;   Up.pos.y2 = y + 1;
      DragRc ();   return;
   }
   if (Up.pos.at == 'x') {             // control area - drag time,valu
      Up.pos.drg = 'x';   Up.pos.xp = x;   Up.pos.x1 = nx;   Up.pos.x2 = x;
                          Up.pos.yp = y;   Up.pos.y1 = y;    Up.pos.y2 = y+1;
      DragRc ();   return;
      return;
   }

   if (Up.pos.at == 'n') {             // not on sym: drag rect n move nt group
      Up.pos.drg = 'm';   Up.pos.x1 = Up.pos.x2 = x;
                          Up.pos.y1 = Up.pos.y2 = y;
   // else ins note
      DragRc ();   return;
   }
  SymDef *it = & co.sym [Up.pos.sy];
   if (Up.pos.at == 'd') {             // drag a new dur
      Up.pos.drg = 'd';   Up.pos.x1 = nx + it->x;   Up.pos.x2 = nx + it->w - 1;
                          Up.pos.y1 = it->y;        Up.pos.y2 = y;
      DragRc ();   return;
   }
   if (Up.pos.at == 'f') {             // hop a note else quant dlg
      Up.pos.drg = 'n';                // (used to be fingering sigh)
      Up.pos.xp = x;   Up.pos.xo = x - (nx + it->x);
                       Up.pos.x1 = x - Up.pos.xo;
                       Up.pos.x2 = Up.pos.x1 + W_NT;
      Up.pos.yp = y;   Up.pos.yo = y -       it->y;
                       Up.pos.y1 = y - Up.pos.yo;
                       Up.pos.y2 = Up.pos.y1 + W_NT*2;
      DragRc ();   return;
   }
}


void Song::MsMv (Qt::MouseButtons b, sbyt2 x, sbyt2 y)
{ PagDef *pg;
  ColDef  co;
  ubyt4   t;
  ubyt2   cx, th = Up.txH;
  ubyte   v1;
  TStr    s, s2, s3, cs;
  char    c, ct;
  TrkEv  *e;
//DBG("MsMv x=`d y=`d b=`d", x, y, b);   DbgPos ();
   if (! b) {
      MsPos (x, y);   //DbgPos ();
      switch (Up.pos.at) {
         case 'q':
         case 'r': if (Up.pos.got)  {c = '|';   break;}
         case 'x': if (Up.pos.got)  {c = 'X';   break;}
         case 'n': c = 'X';   break;
         case 'f': c = 'H';   break;
         case 'd': c = '|';   break;
         default:  c = '\0';
      }
      if (c != Up.ntCur)  {Up.ntCur = c;   emit sgUpd ("ntCur");}
      *s = '\0';
      if ((Up.pos.at == 'f') || (Up.pos.at == 'd')) {
      // ?tEnd,track#,name,dev#,+,upVelo
         pg = & _pag [Up.pos.pg];
         MemCp (& co, & pg->col [Up.pos.co], sizeof (co));
        ubyte  nt,  tr = co.sym [Up.pos.sy].tr;
        bool        dr = TDrm (tr);
//DBG("pg=`d co=`d sy=`d", Up.pos.pg, Up.pos.co, Up.pos.sy);
        ubyt4  tm, te;
        TrkEv *ev = nullptr;
         if (RCRD) {                   // cuz ez has no actual note
            tm = te =   co.sym [Up.pos.sy].tm;   // SetSym set me
            nt = (ubyte)co.sym [Up.pos.sy].nt;   // not p !
            te += (M_WHOLE/8*3/4);
//DBG("ez      tr=`d nt=`d tm=`d te=`d", tr, nt, tm, te);
         }
         else {
           TrkNt *n = & _f.trk [tr].n [co.sym [Up.pos.sy].nt];
            tm = n->tm;   te = n->te;   nt = n->nt;
            if (n->dn != NONE)  ev = & _f.trk [tr].e [n->dn];
//DBG("dr/real tr=`d nt=`d tm=`d te=`d", tr, nt, tm, te);
         }
         TmSt           (s , tm);    StrAp (s, CC("-"));
         StrAp (s, TmSt (s2, te));   StrAp (s, CC(" "));
         StrAp (s, dr ? MDrm2Str (s2, nt) : MKey2Str (s2, nt));
         StrAp (s, ev ? StrFmt (s2, "_`d ", ev->valu & 0x007F) : CC(" "));
         if (! dr)  {StrAp (s, SndName (tr));   StrAp (s, CC(" "));}
         StrAp (s, DevName (tr));
         if (! dr)  StrAp (s, StrFmt (s2, ".`d #`d `s",
                           _f.trk [tr].chn+1, tr+1, _f.trk [tr].name));
         Info (s);
      }
//DBG(" Up.Pos at=`c got=`b str=`s", Up.pos.at, Up.pos.got, Up.pos.str);
      if (Up.pos.at == 'x') {
         if (Up.pos.got) {
            e = & _f.trk [Up.pos.tr].e [Up.pos.p];
            StrCp (cs, CtlSt (Up.pos.ct));
            TmSt (s, e->time);   StrAp (s, CC(" "));   StrAp (s, cs);
                                                       StrAp (s, CC("="));
            for (t = 0;  t < NMCC;  t++)  if (! StrCm (MCC [t].s, cs))  break;
            ct = (t >= NMCC) ? 'u' : MCC [t].typ;
            v1 = e->valu;
            if (Up.pos.got == 'y')     // versus 'p' (prev ev)
               {StrCp (& s [4], s);   MemCp (s, CC("[+] "), 4);}
            if (! StrCm (cs, CC("tmpo")))
                 StrAp (s, StrFmt (s2, "`d", TmpoAct (v1 | e->val2 << 8)));
            else if     (ct != 'x')
                 StrAp (s, StrFmt (s2, "`d", (ct=='s') ? ((sbyt4)v1-64) : v1));
            else StrAp (s, CtlX2Str (s2, cs, e));
         }
         else {                        // gotta find it
DBG("  nope");
            *s = '\0';
         }
         Info (s);
      }
      if ( (Up.pos.at == 'q') && Up.pos.got &&
           (((Up.pos.str [0] == '(') && (! StrCh (CC("vc"), Up.pos.str [1]))) ||
            (*Up.pos.str == '.')) )
         Info (& Up.pos.str [1]);
      return;
   }
   if (! (b & Qt::LeftButton))  return;

// draggin - erase old cursor rect n draw new one
   if ((Up.pos.drg == 'q') || (Up.pos.drg == 'r')) {
      DragRc ();   Up.pos.y1 = y;   Up.pos.y2 = y + 1;   DragRc ();
      pg = & _pag [Up.pos.pg];
      MemCp (& co, & pg->col [Up.pos.co], sizeof (co));
      t = Y2Tm (y, & co);   TmSt (s, t);   Info (s);
   }
   if (Up.pos.drg == 'x')  {
      DragRc ();   Up.pos.y1 = y;   Up.pos.y2 = y + 1;   DragRc ();
      pg = & _pag [Up.pos.pg];
      MemCp (& co, & pg->col [Up.pos.co], sizeof (co));
      t = Y2Tm (y, & co);   TmSt (s, t);
      StrCp (cs, CtlSt (Up.pos.ct));
      StrFmt (s2, "time=`s control=`s", s, cs);
      for (t = 0;  t < NMCC;  t++)  if (! StrCm (MCC [t].s, cs))  break;
      ct = (t >= NMCC) ? 'u' : MCC [t].typ;
      if (ct != 'x') {
         if (Up.pos.got)  v1 = _f.trk [Up.pos.tr].e [Up.pos.p].valu;  // init v1
         else            {cx = co.cx + th  *  Up.pos.cp;
                          v1 = (ubyte)(127 * (Up.pos.xp - cx) / (th-1));}
         if (x >= Up.pos.xp)                                   // offset by xpos
               {if ((x - Up.pos.xp) >= (127-v1))  v1  = 127;
                else v1 += (x - Up.pos.xp);}
         else  {if ((Up.pos.xp - x) <=      v1 )  v1  = 0;
                else v1 -= (Up.pos.xp - x);}
         StrAp (s2, StrFmt (s, " value=`d", (ct=='s') ? ((sbyt4)v1-64) : v1));
      }
      Info (s2);
   }
   if (Up.pos.drg == 'm')
      {DragRc ();   Up.pos.x2 = x;   Up.pos.y2 = y;   DragRc ();}
   if (Up.pos.drg == 'd')
      {DragRc ();   Up.pos.y2 = y;                    DragRc ();}
   if (Up.pos.drg == 'n')
      {DragRc ();   Up.pos.x1 = x - Up.pos.xo;
                    Up.pos.x2 =     Up.pos.x1 + W_NT;
// NAW keep time/dur same Up.pos.y1 = y - Up.pos.yo;
//                        Up.pos.y2 = Up.pos.y1 + W_NT*2;
                                                      DragRc ();}
}


void Song::MsUp (Qt::MouseButton b, sbyt2 x, sbyt2 y)
{ TStr  st, c, s1, s2;
  ubyte t, v1;
  ubyt2 cx, th = Up.txH;
  char  ct;
  PagDef *pg;
  ColDef  co;
//DBG("MsUp x=`d y=`d b=`d", x, y, b);   DbgPos ();
   if (! (b == Qt::LeftButton))  return;

   if (Up.pos.drg)  Up.drag.setWidth (0);        // clear it out

   if ((Up.pos.drg == 'q') || (Up.pos.drg == 'r')) {  // cue
      Up.pos.y2 = Up.pos.y1 = y;
      Info (CC(""));
      if (Up.pos.got && (ABSL (y - Up.pos.yp) > DRAG)) {
      // move existing cue
         pg = & _pag [Up.pos.pg];
         MemCp (& co, & pg->col [Up.pos.co], sizeof (co));
         Up.pos.tm = Y2Tm (y, & co);
         if (Up.pos.drg == 'q') {
            if (_f.cue [Up.pos.p].tend) {
               if (Up.pos.tm >= _f.cue [Up.pos.p].tend)
                   Up.pos.tm  = _f.cue [Up.pos.p].tend - M_WHOLE/2;
               StrFmt (Up.pos.str, "/`d`s",
                       _f.cue [Up.pos.p].tend-Up.pos.tm, _f.cue [Up.pos.p].s);
            }
         }
         else { // _drg == 'r'
            if (Up.pos.tm <= _f.cue [Up.pos.p].time)
                Up.pos.tm  = _f.cue [Up.pos.p].time + M_WHOLE/2;
            StrFmt (Up.pos.str, "/`d`s",
               Up.pos.tm - _f.cue [Up.pos.p].time, _f.cue [Up.pos.p].s);
            Up.pos.tm = _f.cue [Up.pos.p].time;
         }
         _f.cue.Del (Up.pos.p);        // del+ins to keep sorted :/
         if (*Up.pos.str == '(')       // chop section time to bar
            Up.pos.tm = Bar2Tm (Tm2Bar (Up.pos.tm));
         TxtIns (Up.pos.tm, Up.pos.str, & _f.cue, 'c');
      }
      else {
         MemCp (& Up.posx, & Up.pos, sizeof (Up.pos));
         emit sgUpd ("dCue");          // dlg fer ins/upd/del cue
      }
      _pg = 0;
      if (! Up.pos.pPoz) Poz (false);
   }
   if (Up.pos.drg == 'x') {            // control
      pg = & _pag [Up.pos.pg];
      MemCp (& co, & pg->col [Up.pos.co], sizeof (co));
      Up.pos.tm = Y2Tm (y, & co);
      StrCp (c, CtlSt (Up.pos.ct));
      for (t = 0;  t < NMCC;  t++)  if (! StrCm (MCC [t].s, c))  break;
      ct = (t >= NMCC) ? 'u' : MCC [t].typ;
     TrkEv *e = Up.pos.got ? & _f.trk [Up.pos.tr].e [Up.pos.p] : nullptr;
      if (ct == 'x') {                 // if ctl type x, pop dlg;  else use x
         CtlX2Str (st, c, e);
         StrCp  (Up.pos.str, st);
         StrFmt (Up.pos.etc, "setCtl `d `d `d `s ",
                 Up.pos.got ? Up.pos.tr : 128, Up.pos.p, Up.pos.tm, c);
//DBG("HEY str=`s etc=`s st=`s", Up.pos.str, Up.pos.etc, st);
         if      (! StrCm (c, CC("ksig")))  {emit sgUpd ("dKSg");   *st = '\0';}
         else if (! StrCm (c, CC("tsig")))  {emit sgUpd ("dTSg");   *st = '\0';}
         else if (! StrCm (c, CC("tmpo")))  {emit sgUpd ("dTpo");   *st = '\0';}
         else                               StrCp (st, CC("*"));     // prog
      }
      else {
         if (e)  v1 = e->valu;         // init v1
         else   {cx = co.cx + th  *  Up.pos.cp;
                 v1 = (ubyte)(127 * (Up.pos.xp - cx) / (th-1));}
         if (x >= Up.pos.xp)           // offset by xpos
               {if ((x - Up.pos.xp) >= (127-v1))  v1  = 127;
                else v1 += (x - Up.pos.xp);}
         else  {if ((Up.pos.xp - x) <=      v1 )  v1  = 0;
                else v1 -= (Up.pos.xp - x);}
         StrFmt (st, "`d", v1);
      }
      if (*st)
         Cmd (StrFmt (s2, "setCtl `d `d `d `s `s",
                      Up.pos.got ? Up.pos.tr : 128, Up.pos.p, Up.pos.tm,
                      c, st));
      _pg = 0;
      if (! Up.pos.pPoz) Poz (false);
   }
   if (Up.pos.drg == 'm') {            // rect[M]ov
      Up.pos.x2 = x;   Up.pos.y2 = y;
      if ((ABSL (Up.pos.x2 - Up.pos.x1) > DRAG) ||
          (ABSL (Up.pos.y2 - Up.pos.y1) > DRAG))  emit sgUpd ("dMov");
      if (! Up.pos.pPoz) Poz (false);
   }
   if (Up.pos.drg == 'd') {            // [d]ur
      Up.pos.y2 = y;   if (! RCRD)  NtDur ();
      if (! Up.pos.pPoz) Poz (false);
   }
   if (Up.pos.drg == 'n') {            // [n]oteHop
      Up.pos.x1 = x - Up.pos.xo;   Up.pos.y1 = y - Up.pos.yo;
      if (ABSL (x - Up.pos.xp) > DRAG)  NtHop ();
      else             if (! RCRD)  PreQua ();
      if (! Up.pos.pPoz) Poz (false);
   }
//DBG("MsUp end");
   Up.pos.drg = '\0';
   ReDo ();
}
