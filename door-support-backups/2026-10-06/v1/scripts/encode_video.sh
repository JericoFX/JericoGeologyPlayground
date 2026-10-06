#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
ffmpeg -hide_banner -loglevel error -y -framerate 12 -i output/evidence/video_frames/%04d.png -c:v libx264 -preset medium -crf 19 -pix_fmt yuv420p -movflags +faststart output/evidence/jolt-support-fall-working.mp4
python - <<'PY'
from pathlib import Path
from evidence import freeze_file
print(freeze_file(Path('output/evidence/jolt-support-fall-working.mp4'),'jolt-support-fall'))
PY
