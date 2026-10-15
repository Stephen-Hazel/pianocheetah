#!/bin/php
<?php # b.sh - build with flatpak-builder n stuff
      # args:  (none)  build w debug and leave user dir as is

// app triplet n fpak cmds
   $app = "app.shaz.pianocheetah";   $f = "flatpak";   $fb = "$f-builder";

echo "...uninstall old one\n";
   system ("$f uninstall -y $app");
   system ("rm -fr _build .$fb");      // wipe
   system ("mkdir  _build");

// Debug build by default
echo "...compile/install\n";
   system ("$fb --user --force-clean --install _build fpak", $rc);
   system ("$f install --reinstall --user --assumeyes ".
           "/home/sh/src/pianocheetah/.$fb/cache $app");
   system ("$f install --reinstall --user --assumeyes ".
           "/home/sh/src/pianocheetah/.$fb/cache $app".".Debug");
   system ("rm -fr _build .$fb");      // wipe
   system ("echo x >/home/sh/.var/app/$app/config/dbg.txt");
// cant remember gdb commands to save my life
   echo "
$f run --command=sh --devel --filesystem=$(pwd) $app
gdb /app/bin/theapp
set logging enabled on
thread apply all bt
run
-- hit y and ...make it blow up
where
thread apply all bt
bt full\n";

// system ("rm -fr ~/.var/app/$app");
