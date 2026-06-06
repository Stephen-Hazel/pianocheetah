// sLoad.cpp - song loadin junk for song2wav

int EvCmp (void *p1, void *p2)  // TrkEv sortin by time,ctrl,up/prs/dn
{ TrkEv *e1 = (TrkEv *)p1, *e2 = (TrkEv *)p2;
  int t;
   if ((t = e1->time - e2->time))  return t;
   if ((t = e1->ctrl - e2->ctrl))  return t;
   if ((t = (e1->valu & 0x80) - (e2->valu & 0x80)))  return t;  // up / pr,dn
   return   (e2->val2 & 0x80) - (e1->val2 & 0x80);              // pr / dn
}


char *SongRec (char *buf, ubyt2 len, ubyt4 pos, void *ptr)
{ static ubyte TB;                     // parse a record of the .song file
  static TStr  Msg;
  STable *s = (STable *)ptr;
  ubyte   i, nc;
   (void)len;   (void)ptr;
   if (pos == 0)  TB = TB_DSC;         // always start in descrip section
   if ((*buf == '-') || (*buf == '\0'))  return NULL;      // skip comment/empty
   for (ubyte t = 0;  t < TB_MAX;  t++)     // new section/table ?
      if (! StrCm (buf, s [t].Name ()))  {TB = t;  return NULL;}
   if ((s [TB].NRow () + 1) > s [TB].MaxRow ())
      return  StrFmt (Msg, "line=`d `s too many lines", pos+1, s [TB].Name ());
   if ((nc = s [TB].NCol ()) == 1)  s [TB].Add (buf);
   else {
     ColSep ss (buf, nc-1);
      for (i = 0;  i < nc;  i++)  s [TB].Add (ss.Col [i]);
   }
   return NULL;
}


int TSigCmp (void *p1, void *p2)  // by .bar
{ TSgRow *t1 = (TSgRow *)p1, *t2 = (TSgRow *)p2;
   return (int)(t1->bar) - (int)(t2->bar);
}


void Song::Load (char *fn)
// ok, go thru the BRUTAL HELL to load in a song, etc...
{ TStr  buf;
  ubyte nt, t, i;
  ubyt4 ln, e, ne, pe, te, tm;
  char *m, *p, ud;
  File  f;
  TrkEv *e2;
  STable st [TB_MAX];
DBG("Load fn=`s", fn);
   Wipe ();
   if (! f.Size (fn)) {DBG("Load  file size=0");   return;}

   _nEv = 0;
   st [TB_DSC].Init (CC("Descrip:"), 1, MAX_DSC);
   st [TB_TRK].Init (CC("Track:")  , 4, MAX_TRK);
   st [TB_DRM].Init (CC("DrumMap:"), 7, MAX_DRM);
   st [TB_LYR].Init (CC("Lyric:")  , 2, MAX_LYR);
   st [TB_EVT].Init (CC("Event:")  , 2, MAX_EVT);

   if ((m = f.DoText (fn, & st, SongRec)))
      {DBG("Load  DoText err=`s", m);   return;}
//st [TB_TRK].Dump (); st [TB_DRM].Dump (); st [TB_EVT].Dump ();

DBG(" init tsg, ctl, trk, ev");
   nt = (ubyte)(_trk.Ln = st [TB_TRK].NRow ());
   ne =                   st [TB_EVT].NRow ();
   _ctl.Ln = 3;   StrCp (_ctl [0], CC("Tmpo"));
                  StrCp (_ctl [1], CC("TSig"));
                  StrCp (_ctl [2], CC("KSig"));
   _tSg.Ln = 0;
   for (t = 0, pe = e = 0;  e < ne;  e++) {
      if (! StrCm (st [TB_EVT].Get (e, 0), CC("EndTrack"))) {
         if (t >= nt)  {DBG("Load  EndTrack>nt");   return;}
         _trk [t].ne = (e-pe);   pe = e+1;
         _trk [t].e = & _ev [_nEv];   _nEv += _trk [t].ne;
         t++;
      }
      if ('!' ==       st [TB_EVT].Get (e, 1) [0]) {
         StrCp (buf, & st [TB_EVT].Get (e, 1) [1]);
         if ((m = StrCh (buf, '=')))  *m = '\0';
         else  {DBG("Load  !ctrl no = at trk=`d ev#=`d", t+1, e);   return;}

         for (i = 0;  i < _ctl.Ln;  i++)  if (! StrCm (buf, _ctl [i]))  break;
         if (i >= _ctl.Ln) {           // new ctl s
            if (_ctl.Ln >= 128)  {DBG("Load  >128 ctls");   return;}
            _ctl.Ln++;   StrCp (_ctl [i], buf);
         }
         if ((i == 1) && (! _tSg.Full ())) {
            StrCp (buf, ++m);   if ((m = StrCh (buf, '/')))  *m++ = '\0';
            te = _tSg.Ins ();
            _tSg [te].num = (ubyte)Str2Int (buf);
            _tSg [te].den = (ubyte)Str2Int (m);
            _tSg [te].bar = (ubyt2)Str2Int (st [TB_EVT].Get (e, 0));
         }
      }
   }
   if (t < nt)  {DBG("Load  EndTrack<ntrk");   return;}

// init initial tSg - sort by bar n calc .time
   Sort (_tSg.Ptr (), _tSg.Ln, _tSg.Siz (), TSigCmp);
   if ((_tSg.Ln == 0) || (_tSg [0].bar > 1))
      {_tSg.Ins (0);   _tSg [0].num = 4;   _tSg [0].den = 4;
                                           _tSg [0].bar = 1;}
   for (tm = 0, t = 0;  t < _tSg.Ln;  t++) {
      _tSg [t].time = tm;
      if ((ubyt4)(t+1) < _tSg.Ln)
         tm += (_tSg [t+1].bar - _tSg [t].bar) * M_WHOLE * _tSg [t].num /
                                                           _tSg [t].den;
   }

// NOW we can convert TmSt n ctrls (n everything) into _trk[].e[]
DBG(" set events");
   _tEnd = 0;
   for (e2 = _trk [t = 0].e, e = 0;  e < ne;  e++) {
      StrCp (buf, st [TB_EVT].Get (e, 0));
      if (! StrCm (buf, CC("EndTrack")))  {e2 = _trk [++t].e;   continue;}

      e2->time = Str2Tm (buf);         // ok parse the rec - all start w time
      if (e2->time > _tEnd)  _tEnd = e2->time;

      StrCp (buf, st [TB_EVT].Get (e, 1));
      if (*buf == '!') {               // ctl
         if ((m = StrCh (buf, '=')))  *m++ = '\0';
         else  {DBG("Load  !ctl no = in trk=`d ev#=`d", t+1, e);   return;}
         for (i = 0;  i < _ctl.Ln;  i++)
            if (! StrCm (& buf [1], _ctl [i]))
               {e2->ctrl = (ubyte)(0x80 | i);   break;}
         if      (i == 0) {            // tmpo
           ubyt2 tp;
            tp = (ubyt2)Str2Int (m);
            e2->valu = tp & 0x00FF;   e2->val2 = tp >> 8;
         }
         else if (i == 1) {            // tsig
           ubyte n, d, s, x;
            if (! (p = StrCh (m, '/')))
               {DBG("Load  TSig missing / trk=`d ev#=`d", t+1, e);   return;}
            n = (ubyte)Str2Int (m);
            d = (ubyte)Str2Int (++p);
            s = 1;
            if ((m = StrCh (p, '/')))  s = (ubyte)Str2Int (++m);
            for (x = 0;  x < 8;  x++)  if ((1 << x) == d)  break;
            if (x >= 8)
               {DBG("Load  TSig bad denom trk=`d ev#=`d", t+1, e);   return;}
            e2->valu = n;   e2->val2 = x | ((s-1) << 4);
         }
         else if (i == 2) {            // ksig
           char *map = CC("b2#b#b2#b#b2");
            e2->valu = i = MNt (m);
            if (m [StrLn (m)-1] == 'm')  e2->val2 = 1;  // minor
            if (map [i] != '2')
                  {if (map [i] == 'b')  e2->val2 |= 0x80;}  // b not #
            else  {if (m [1]   == 'b')  e2->val2 |= 0x80;}
         }
         else {
            e2->valu = (ubyte)Str2Int (m, & p);
            e2->val2 = (ubyte)(*p ? Str2Int (p) : 0);
         }
      }
      else {                           // note
//TStr db1;DBG("  tr=`d `s `s", t, TmS (db1, e2->time), buf);
         i = MDrm (p = buf);
         if (i == (ubyte)128) i = MKey (buf, & p);   else p += 4;
         ud = *p++;
         e2->ctrl = i;
         e2->valu = (ubyte)Str2Int (p, & p) | ((ud == '_') ? 0x80 : 0);
         e2->val2 = (ud == '~') ? 0x80 : 0;
      }
      e2++;
   }

// init trk: .p,chn,grp,shh,name,snd
   for (t = 0;  t < nt;  t++) {
      _trk [t].p = 0;
      StrCp (_trk [t].snd,  st [TB_TRK].Get (t,1));        // sound
      StrCp (_trk [t].name, st [TB_TRK].Get (t,3));        // trackname
      if ((p = StrCh (_trk [t].name, '#')))  *p = '\0';

   // set .grp,.shh outa track mode thingy
      StrCp (buf, st [TB_TRK].Get (t,2));                  // mode
      _trk [t].grp = (*buf ==    '+') ? true : false;
      _trk [t].shh = StrCh (buf, '#') ? true : false;
      _trk [t].chn = MemCm (_trk [t].snd, CC("Drum/"), 5) ? 0xFF : 9;
   }
  ubyte mc = 0;                        // finalize _chn n get maxchans for syn
   for (t = 0;  t < nt;  t++)  if (_trk [t].chn != 9)
      _trk [t].chn = (_trk [t].grp && t) ? _trk [t-1].chn : mc++;

DBG(" sortin trks");
   for (t = 0;  t < nt;  t++)          // sort trk events
      Sort (_trk [t].e, _trk [t].ne, sizeof (TrkEv), EvCmp);

   _drm.Ln = st [TB_DRM].NRow ();      // load _drm so DrumExp can sep drm trks
   for (e = 0;  e < _drm.Ln;  e++) {
      _drm [e].ctl = MDrm (st [TB_DRM].Get (e, 0));   // defaults
      _drm [e].snd [0] = '\0';
      _drm [e].shh = false;
      StrCp (buf, st [TB_DRM].Get (e, 4));            // overwrite w drummap
      if (*buf == '#')  _drm [e].shh = true;
      StrCp (buf, st [TB_DRM].Get (e, 1));  // sound
      if (*buf == '.')  MDrm2StG (buf, _drm [e].ctl);
      StrCp (_drm [e].snd, buf);
DBG("   dmap `d ctl=`d snd=`s", e, _drm [e].ctl, buf);
   }
TStr t1;
DBG("drm shh ctl snd");
for (t = 0; t < _drm.Ln; t++)  DBG("`d/`d `b `s `s",
t, _drm.Ln, _drm [t].shh,
MDrm2Str(t1,_drm [t].ctl), _drm [t].snd);

// tell syn it's sounds
  ubyt4 s, ns = 0;
  TStr  ts;
DBG(" SetBnk");
   for (s = 0;  s < 256;  s++)  _bnk [s][0] = '\0';
   for (t = 0;  t < _trk.Ln;  t++) {
      if (_trk [t].chn != 9) {         // melodic
         StrCp (ts, _trk [t].snd);
DBG("   tr=`d melo `s", t, ts);
         if (*ts == '\0')  continue;

         for (s = 0;  s < ns;  s++)  if (! StrCm (_bnk [s], ts))  break;
         if (s >= ns) {
            if (ns >= 128)  Die ("too many melodic sounds fer syn");
            StrCp (_bnk [ns++], ts);
         }
DBG("      prog=`d", s);
      }
      else                             // drum (only 1 track)
         for (s = 0;  s < _drm.Ln;  s++) {
DBG("   tr=`d drum ctl=`d snd=`s", t, _drm [s].ctl, _drm [s].snd);
            if (_drm [s].snd [0] && (! _drm [s].shh))
               StrCp (_bnk [128+_drm [s].ctl], _drm [s].snd);
         }
   }
   Sy.LoadSnd (_bnk, mc);

// do progch's for ALL song melo chans
   for (ubyte t = 0;  t < _trk.Ln;  t++)  if (! _trk [t].grp)  SetChn (t);
DBG("Load end");
   Dump ();
}
