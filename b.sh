#!/bin/php
<?php # b.sh - build with flatpak-builder n stuff
      # args:  (none)  build w debug and leave user dir as is
      #        p       build for prod - strippin debugging and clearing user dir
   $arg = '';   if ($argc > 1)  $arg = $argv [1];

// app triplet n fpak cmds
   $app = "app.shaz.pianocheetah";   $f = "flatpak";   $fb = "$f-builder";

echo "...uninstall old one\n";
   system ("$f uninstall -y $app");
   system ("rm -fr _build .$fb");      // wipe

   if ($arg != 'p') {                  // Debug build by default
      system ("mkdir _build");
      system ("$fb --user --force-clean --install _build fpak.dbg", $rc);
      system ("$f install --reinstall --user --assumeyes ".
              "/home/sh/src/pianocheetah/.$fb/cache $app");
      system ("$f install --reinstall --user --assumeyes ".
              "/home/sh/src/pianocheetah/.$fb/cache $app".".Debug");
      system ("echo x >/home/sh/.var/app/$app/config/dbg.txt");
   // cant remember gdb commands to save my life
      echo "
flatpak run --command=sh --devel --filesystem=$(pwd) $app
gdb /app/bin/pianocheetah
set logging enabled on
thread apply all bt
run
-- hit y and ...make it blow up
where
thread apply all bt
bt full\n";
      exit;
   }

// source => _build => install
echo "...compile/install\n";
   system ("$fb --user --install _build fpak", $rc);
#  system ("$fb --user --install --force-clean --ccache
#               --keep-build-dirs _build fpak", $rc);
   if ($rc != 0)  exit;                // build error :(

echo "...cleanup\n";
   system ("rm -fr _build .$fb");
   system ("rm -fr ~/.var/app/$app");
