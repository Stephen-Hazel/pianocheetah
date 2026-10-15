// ctlNt.cpp - note widget - do editing on da pixmap

#include "pcheetah.h"


void CtlNt::RePM ()
// moved to new screen?  resize pixmap
{  Up.pm = nullptr;
  QString scr = Gui.W ()->windowHandle ()->screen ()->name ();
  QList<QScreen *> scrLs = QGuiApplication::screens ();
   for (int i = 0;  i < scrLs.size ();  i++)  if (scrLs [i]->name () == scr)
      {Up.pm = new QPixmap (scrLs [i]->size ());   break;}
   if (Up.pm == nullptr)  DBG("couldn't alloc CtlNt pixmap :(");
}

void CtlNt::resizeEvent (QResizeEvent *e)
{  Up.w = (ubyt2)e->size ().width  (); // maybe we changed screens
   Up.h = (ubyt2)e->size ().height (); // otherwise we should always be beeg nuf
   if ((Up.pm == nullptr) || (Up.w > Up.pm->width  ()) ||
                             (Up.h > Up.pm->height ()))  RePM ();
TRC("resize w=`d h=`d", Up.w, Up.h);
   emit sgReSz ();
}

void CtlNt::keyPressEvent (QKeyEvent *e)
{ KeyMap km;
  key    k;
   if ((k = km.Map (e->modifiers (), e->key ())))  DBG("nt key=`s", km.Str (k));
}

void CtlNt::wheelEvent (QWheelEvent *e)
{ int j = e->angleDelta ().y () / 40;
DBG("nt wheel=`d", j);
}

void CtlNt::mousePressEvent   (QMouseEvent *e)
{  Up.gp = e->globalPosition ();
  sbyt2 x = e->pos ().x (),
        y = e->pos ().y ();   emit sgMsDn (e->button  (), x, y); }

void CtlNt::mouseMoveEvent    (QMouseEvent *e)
{ sbyt2 x = e->pos ().x (),
        y = e->pos ().y ();   emit sgMsMv (e->buttons (), x, y);
}

void CtlNt::mouseReleaseEvent (QMouseEvent *e)
{ sbyt2 x = e->pos ().x (),
        y = e->pos ().y ();   emit sgMsUp (e->button  (), x, y); }


//______________________________________________________________________________
void CtlNt::paintEvent (QPaintEvent */*e*/)
{  if (Up.pm == nullptr)  {DBG("no pm");   return;}
  Canvas c (this);
//DBG("CtlNt::paintEvent w=`d h=`d  tx=`d ty=`d tw=`d th=`d",
//Up.w, Up.h, Up.tpos.left(), Up.tpos.top(), Up.tpos.width(), Up.tpos.height());
   c.Blt (*Up.pm,   0, 0,  0, 0,  Up.w, Up.h);
   c.Blt (*Up.tpm,  Up.tpos.left (),  Up.tpos.top (),  0, 0,
                    Up.tpos.width (), Up.tpos.height ());
   if (Up.drag.width ()) {
     QPainter *p = c.Painter ();
      p->setCompositionMode (QPainter::RasterOp_SourceXorDestination);
      p->setPen (QColor (0xff, 0xff, 0xff));
      p->drawRect (Up.drag);
   }
}

void CtlNt::Cur ()
{ Qt::CursorShape c;
//DBG("ntCur `c", Up.ntCur);
   switch (Up.ntCur) {
   case '|': c = Qt::SizeVerCursor;    break;
   case 'X': c = Qt::SizeAllCursor;    break;
   case '+': c = Qt::CrossCursor;      break;
   case 'H': c = Qt::OpenHandCursor;   break;
   default:  c = Qt::ArrowCursor;
   }
   setCursor (c);
}
