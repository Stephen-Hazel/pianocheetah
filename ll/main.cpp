// ll.cpp - list long like unix (but no sortin, only midi)

#include "stv/os.h"
#include "stv/midi.h"

char Ext, *ExS = CC("alll\0"   "midi\0"   "song\0");
TStr Top;
File F;

void DoDir (char *dir)
{ FDir d;
  TStr fn, a;
  char df;
   if ((df = d.Open (fn, dir))) {
      do {
         if (df == 'f') {
           ulong ln = StrLn (fn);
            if      (Ext == 0) {       // alll files (minus any cache files)
               if ( (ln < 10) || StrCm (& fn [ln-9], "cache.txt") )
                  {F.Put (& fn [StrLn (Top)+1]);   F.Put ("\n");}
            }
            else if (Ext == 1) {       // .mid/.kar/etc file
               if (FnMid (fn))
                  {F.Put (& fn [StrLn (Top)+1]);   F.Put ("\n");}
            }
            else                       // dirs w an a.song
               if ( (ln > 7) && (! StrCm (& fn [ln-7], "/a.song")) )
                  {StrCp (a, fn);   Fn2Path (a);
                   F.Put (& a  [StrLn (Top)+1]);   F.Put ("\n");}
         }
         else  DoDir (fn);
      } while ((df = d.Next (fn)));
      d.Shut ();
   }
}


int main (int argc, char *argv [])
{ ulong p;
  TStr  fn, to;
DBGTH("ll");
   App.Init ();
   if (argc < 3)                           return 99;
   if (! (Ext = PosInZZ (argv [1], ExS)))  return 99;
   Ext--;
   StrCp (Top,           argv [2]);
DBG("bgn ext=`s top=`s", argv[1], argv[2]);

// make output file n fill it
   p = StrLn (Top)+1;
   StrFmt (fn, "`s/__`scache.txt",  Top,  Ext?((Ext==1)?"midi":"song"):"");
   if (F.Open (fn, "w"))  {DoDir (Top);   F.Shut ();}

// replace any old cache
   StrCp (to, fn);   StrCp (& to [p], & to [p+1]);
   if (F.Size (to))  F.Kill (to);
   F.ReNm (fn, to);
DBG("end");
   return 0;
}
