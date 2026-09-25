#!/bin/sh
# fp-build.sh - build n install everything into /app n /var for flatpak

# load up /app/share and /var/data
cp -pr _fpak/share /app || exit 1

# build each app with common "stv" include code
for d in pianocheetah midicfg mid2song txt2song mod2song ll \
         sfz2syn synsnd midimp song2wav; do
   ln -sfn ../stv $d/stv           &&
   cmake -G Ninja -S $d -B _b/$d --log-level=WARNING \
      -DCMAKE_BUILD_TYPE=$1 \
      -DCMAKE_INSTALL_MESSAGE=NEVER \
      -DCMAKE_INSTALL_PREFIX=/app  &&
   cmake --build   _b/$d           &&
   cmake --install _b/$d           || exit 1
done
