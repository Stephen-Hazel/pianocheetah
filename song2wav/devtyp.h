// devtyp.h - syn's controls (no need fo sounds)

class DevTyp {                         // ctls used in syn
public:
   void Init ()                        // always syn
   // cache devTyp's sound.txt and ccout.txt files into mem
   { TStr fn;
     File f;
DBG("load controls for syn");
      _nCc = 0;
      App.Path (fn, 'd');   StrAp (fn, "/device/syn/ccout.txt");
      f.DoText (fn, this, CcRec);
   }

   ubyt2 CCID (char *name)
   {  for (ubyte c = 0; c < _nCc; c++)
         if (! StrCm (name, _cc [c].map))  return _cc [c].raw;
      return 0;
   }

   void SetCCMap (TStr *lst, ubyte len)
   { ubyte i;
      MemSet (CCMap, 0, sizeof (CCMap));
      for (i = 0; i < len; i++)  CCMap [i] = CCID (lst [i]);
   }
   ubyt2 CCMap [128];
private:
   static char *CcRec (char *buf, ubyt2 len, ubyt4 pos, void *ptr)
   // parse a record of the ccout.txt file
   { static TStr Msg;
     DevTyp     *d = (DevTyp *)ptr;
      if (pos >= 128)            Die ("DevTyp::CcRec >= 128 rows");
     ColSep ss (buf, 2);
      if (ss.Col[1][0] == '\0')  Die ("DevTyp::DoCc missing cols");
      StrCp (d->_cc [d->_nCc].map, ss.Col[0]);
      d->_cc        [d->_nCc].raw = MCtl (ss.Col[1]);
                     d->_nCc++;
      return NULL;
   }
   CcDef _cc [128];   ubyte _nCc;
};
