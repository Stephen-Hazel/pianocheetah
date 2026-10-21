#!/bin/sh
# fp-build.sh - build n install everything into /app n /var for flatpak

# load up /app/share and /var/data
cp -pr _fpak/share /app || exit 1

# each app gets common "stv" include code
for d in pianocheetah midicfg mid2song txt2song mod2song ll \
         sfz2syn synsnd midimp song2wav; do
   ln -sfn ../stv $d/stv           || exit 1
done

# build all apps at once via top CMakeLists.txt
cmake -G Ninja -S . -B _b --log-level=WARNING \
   -DCMAKE_BUILD_TYPE=Debug \
   -DCMAKE_INSTALL_MESSAGE=NEVER \
   -DCMAKE_INSTALL_PREFIX=/app     &&
cmake --build   _b                 &&
cmake --install _b                 || exit 1
