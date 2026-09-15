#!/bin/php
<?php # fhub.sh - test build for flathub

// repo name n app triplet
   $app = "app.shaz.pianocheetah";
   $f = "flatpak";   $fb = "$f-builder";

   system ("$f remove $app");

// source => _build + _repo and install
   system ("rm -fr _build _repo .$fb");
   system ("mkdir  _build _repo");
   system ("$fb --user --install --repo=_repo _build $app.json", $rc);
