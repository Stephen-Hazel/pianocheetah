#!/bin/sh
# fp-build.sh - build n install everything into /app for flatpak run
# from top of source tree (with stv checked out inside it)

cp -pr _fpak/share /app || exit 1

for d in pianocheetah midicfg initme mid2song txt2song mod2song ll \
         sfz2syn synsnd midimp song2wav; do
   ln -sfn ../stv $d/stv                                      &&
   cmake -G Ninja -S $d -B _b/$d -DCMAKE_BUILD_TYPE=Debug \
                                 -DCMAKE_INSTALL_PREFIX=/app  &&
   cmake --build _b/$d                                        &&
   cmake --install _b/$d                                      || exit 1
done
