#!/bin/php
<?php # b.sh - build with flatpak-builder n stuff
      # args:  c  wipe config dir - for full reinstall
      #        d  build with debugging so ya can gdb inside it
   $arg = '';   if ($argc > 1)  $arg = $argv [1];

// app triplet n fpak cmds
   $app = "app.pianocheetah.pianocheetah";
   $f = "flatpak";   $fb = "$f-builder";

   system ("rm -fr _build .$fb");      // wipe

echo "...uninstall old one\n";
   system ("$f uninstall -y $app");

   if ($arg == 'd') {
//    DEBUGGIN !
      system ("mkdir _build");
      system ("$fb --user --force-clean --install _build fpak.dbg", $rc);
      system ("$f install --reinstall --user --assumeyes ".
              "/home/sh/src/pianocheetah/.$fb/cache $app");
      system ("$f install --reinstall --user --assumeyes ".
              "/home/sh/src/pianocheetah/.$fb/cache $app".".Debug");
      system ("echo x " .
             ">/home/sh/.var/app/app.pianocheetah.pianocheetah/config/dbg.txt");
      echo "
flatpak run --command=sh --devel --filesystem=$(pwd) app.pianocheetah.pianocheetah
gdb /app/bin/pianocheetah
set logging enabled on
thread apply all bt
run        (usually gotta type y)
           (make it blow up)
where
thread apply all bt
bt full\n";
      exit;
   }

// source => _build => install
echo "...compilin n installin\n";
   system ("$fb --user --install _build fpak", $rc);
   if ($rc != 0)  exit;                // build error :(

echo "...cleanup\n";
   system ("rm -fr _build .$fb");
   system ("echo x " .
             ">/home/sh/.var/app/app.pianocheetah.pianocheetah/config/dbg.txt");
   if ($arg != 'c')  exit;

// cleanup .var/app/ dir for full reset (wipe config files etc)
echo "...reset config files\n";
   system ("rm -fr ~/.var/app/$app");
