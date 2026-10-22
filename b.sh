#!/bin/php
<?php # b.sh - build with flatpak-builder n stuff
      # args:  (none)  build w debug and leave user dir as is

// app triplet n fpak cmds
   $app = "app.shaz.pianocheetah";   $f = "flatpak";   $fb = "$f-builder";

echo "...uninstall old one\n";
   system ("$f uninstall -y $app");

echo "...compile/install\n";           // always Debug build
   system ("mkdir _build");
   system ("$fb --user --force-clean --install _build _fpak/fpak", $rc);
   system ("$f install --reinstall --user --assumeyes ".
           "/home/sh/src/pianocheetah/.$fb/cache $app");
   system ("$f install --reinstall --user --assumeyes ".
           "/home/sh/src/pianocheetah/.$fb/cache $app".".Debug");
   system ("rm -fr _build .$fb");      // wipe
   system ("echo x >/home/sh/.var/app/$app/config/dbg.txt");

// system ("rm -fr ~/.var/app/$app");
