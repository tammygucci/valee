#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# (Vallée) The feature tour, web/brand/vallee_demo.mp4: the browser build (web/emu/build_clang.sh) driven headless in
# Node (tools/film/film.mjs: the screen and the audio, as the unit makes them), captioned at 1920 x 1080
# (tools/film/compose.py), encoded with ffmpeg. Run from the repo root after the emulator's build.
set -e
cd "$(dirname "$0")/../.."
node --max-old-space-size=4096 tools/film/film.mjs build/film
N=$(python3 -c "import json;print(json.load(open('build/film/captions.json'))['frames'])")
DUR=$(python3 -c "print($N/30)"); FO=$(python3 -c "print($N/30-2.5)")
python3 tools/film/compose.py 0 "$N" | ffmpeg -v error -y -f rawvideo -pix_fmt rgb24 -s 1920x1080 -r 30 -i - \
  -f f32le -ar 44100 -ac 2 -i build/film/audio.f32 -af "volume=1.9,afade=t=out:st=$FO:d=2.5" \
  -c:v libx264 -crf 16 -preset veryfast -pix_fmt yuv420p -tune animation -c:a aac -b:a 192k -movflags +faststart \
  -t "$DUR" build/film/vallee_demo.tmp.mp4
mv build/film/vallee_demo.tmp.mp4 build/film/vallee_demo.mp4
ffmpeg -v error -y -ss 46 -i build/film/vallee_demo.mp4 -frames:v 1 -vf scale=1280:720 -q:v 3 build/film/poster.jpg
echo "film: build/film/vallee_demo.mp4, build/film/poster.jpg"
