#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# (Vallée) The two films on the installer page and the README, made from the browser build (web/emu/build_clang.sh)
# driven headless in Node (the screen and the audio as the unit makes them), captioned at 1920 x 1080
# (tools/film/compose.py), encoded with ffmpeg:
#   tour     web/brand/vallee_demo.mp4 + poster.jpg            (tools/film/film.mjs: GEN, the sounds, THE SPELL, ALEMBIC, TEEN)
#   perform  web/brand/vallee_perform.mp4 + poster_perform.jpg (tools/film/perform.mjs: a song written, then played live)
# Run from anywhere after the emulator's build: tools/film/make.sh [tour|perform|all] (default all). Writes build/film*/;
# copy the results into web/brand/ to publish them.
set -e
cd "$(dirname "$0")/../.."
film() {   # name script out poster_second
    node --max-old-space-size=4096 "tools/film/$2" "$3"
    N=$(python3 -c "import json;print(json.load(open('$3/captions.json'))['frames'])")
    DUR=$(python3 -c "print($N/30)"); FO=$(python3 -c "print($N/30-2.5)")
    FILM_OUT="$3" python3 tools/film/compose.py 0 "$N" | ffmpeg -v error -y -f rawvideo -pix_fmt rgb24 -s 1920x1080 -r 30 -i - \
      -f f32le -ar 44100 -ac 2 -i "$3/audio.f32" -af "volume=1.9,afade=t=out:st=$FO:d=2.5" \
      -c:v libx264 -crf 16 -preset veryfast -pix_fmt yuv420p -tune animation -c:a aac -b:a 192k -movflags +faststart \
      -t "$DUR" "$3/$1.tmp.mp4"
    mv "$3/$1.tmp.mp4" "$3/$1.mp4"
    ffmpeg -v error -y -ss "$4" -i "$3/$1.mp4" -frames:v 1 -vf scale=1280:720 -q:v 3 "$3/$5"
    echo "film: $3/$1.mp4, $3/$5"
}
case "${1:-all}" in
    tour|all) film vallee_demo film.mjs build/film 46 poster.jpg ;;
esac
case "${1:-all}" in
    perform|all) film vallee_perform perform.mjs build/film_perform 42 poster_perform.jpg ;;
esac
