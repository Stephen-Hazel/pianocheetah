// midimp.cpp - midi import

#include "stv/os.h"

bool Did;
TStr DirF, DirT;

void FixFn (char *s)
// do merge check and tack on a number if needed
{ TStr  t;
  char *p;
  ubyt4 n;
  FDir  d;
   StrCp (t, DirT);   StrAp (t, CC("/"));   StrAp (t, s);
   if (d.Got (t)) {
      StrAp (t, CC("_2"));   StrAp (s, CC("_2"));
      while (d.Got (t)) {              // gotta return s but test w t
         for (p = & s [StrLn (s)-2];  *p != '_';  p--)  ;
         n = Str2Int (p+1);   StrFmt (p+1, "`d", ++n);
         StrCp (t, DirT);   StrAp (t, CC("/"));   StrAp (t, s);
      }
   }
//DBG(s);
}


char *Move (char *fn, ubyt2 len, ubyt4 pos, void *ptr)
// move and Mid2Song each mid.  renamin non .mid to .mid
{ TStr fr, to, fnx, c;
  bool mod = false;
  File f;
   (void)len;   (void)pos;   (void)ptr;
// take off .ext, FixFn does merge check
   if (! StrCm (& fn [StrLn (fn)-4], CC(".mod")))  mod = true;
   StrCp (fnx, fn);   StrAp (fnx, CC(""), 4);   FixFn (fnx);
   StrFmt (fr, "`s/`s",       DirF, fn);
   StrFmt (to, "`s/`s/a.mid", DirT, fnx);
   if (mod)  StrAp (to, CC("mod"), 3);
// move n mid2song
   f.Copy (fr, to);   f.Kill (fr);
   App.Run (StrFmt (c, "`s2song `p", mod?"mod":"mid", to));
   return nullptr;
}


char *Wipe (char *fn, ubyt2 len, ubyt4 pos, void *ptr)
{ ubyte i;
  TStr  fr, to, fnx;
  File  f;                             // move flattened fn to _junk dir
   (void)len;   (void)pos;   (void)ptr;
   Did = true;                         // settin this leave junk dir there
   StrCp (fnx, fn);
   for (i = 0;  i < StrLn (fnx);  i++)  if (fnx [i] == '/')  fnx [i] = '_';
   StrFmt (fr, "`s/`s", DirF, fn);
   StrFmt (to, "`s/`s", DirT, fnx);
   f.Copy (fr, to);   f.Kill (fr);
   return nullptr;
}


int main (int argc, char *argv [])
{ TStr ds, c, s;
  File f;
  Path d;  (void)argc;   (void)argv;
DBGTH("MidImp");
TRC("bgn");
   App.Init ();
// from picked dir
   StrCp (DirF, argv [1]);   FnName (ds, DirF);
// to ..pianocheetah/3_queue/botdirpicked
   StrFmt (DirT, "`s/3_queue/`s", App.Path (s, 'd'), ds);   d.Make (DirT);
DBG("DirFr=`s DirTo=`s", DirF, DirT);

// list midi files in midi_import n move+mid2song em
   App.Run (StrFmt (c, "ll midi `p", DirF));
   StrCp (s, DirF);   StrAp (s, CC("/_midicache.txt"));
   f.DoText (s, nullptr, Move);

// list off leftovers n move to midi_junk
   App.Run (StrFmt (c, "ll alll `p", DirF));
   StrAp (DirT, CC("/_junk"));   d.Make (DirT);
   StrCp (s, DirF);   StrAp (s, CC("/_cache.txt"));
   f.DoText (s, nullptr, Wipe);
   if (! Did)  d.Kill (DirT);          // kill it if didn't put nothin in
   d.Kill (DirF);                      // kill it it's empty
TRC("end");
   return 0;
}
