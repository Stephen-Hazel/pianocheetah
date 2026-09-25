// initme.cpp - init fer pcheetah

#include "initme.h"

TStr DirPC;

void InitMe::Init ()
{  StrCp (DirPC, "/var/data/pianocheetah");
   App.CfgPut ("d", DirPC);
   Gui.WinLoad ();

/* NO SOUP FOR YOU - this is flatpak land.  ...you want this?
** then either
**    ln -s ~/.var/app/app.shaz.pianocheetah/data/pianocheetah  ./pcheetah
** or
**    flatseal me with filesystem=host
**    mv    ~/.var/app/app.shaz.pianocheetah/data/pianocheetah  /.../pcheetah
**    vi    ~/.var/app/app.shaz.pianocheetah/config/d.cfg n set /.../pcheatah
**
  TStr dir;
DBG("Init bgn");
   Gui.Hey (
      "Oh hi :)\n\n"
      "I need you to pick a directory for your pianocheetah files.\n"
      "Your home dir is fine.\n\n"
      "I'll make a pianocheetah dir there.");
   StrCp (dir, getenv ("HOME"));
DBG("home=`s", dir);
   if (Gui.AskDir (dir, "Pick a dir to put the pianocheetah dir into")) {
      StrAp (dir, "/pianocheetah");
      StrCp (DirPC, dir);
DBG("picked=`s", DirPC);
      App.CfgPut ("d", DirPC);
      Gui.WinLoad ();
   }
DBG("Init end");
*/
}

void InitMe::Quit ()  {}

int main (int argc, char *argv [])
{ QApplication app (argc, argv);
  InitMe       win;
  File   f;
  Setup *s  = nullptr;
  int    rc = 0;
DBGTH("InitMe");   DBG("bgn");
   App.Init ();   Gui.Init (& app, & win, "InitMe");   win.Init ();
   if (*DirPC && (! f.Size (DirPC))) { // Size makes sure it's notta file by acc
      s  = new Setup ();
DBG("came back from thread");
      rc = Gui.Loop ();
DBG("gui loop done");
      delete s;
   }
   win.Quit ();
DBG("end");
   return rc;
}
