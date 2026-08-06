#!/usr/bin/env python3
"""Offline smoke: load profile weights and run one image (no ROS).

  conda activate ros_env
  cd …/aero-hub
  python ros/src/ah_yolo/scripts/smoke_infer.py --profile coco80 \\
      --image /path/to.jpg
  python ros/src/ah_yolo/scripts/smoke_infer.py --profile tank \\
      --weights yolo-models/mini_tank_0308.pt --image zAttachments/tank_alone.jpeg
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Allow import without install
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from ah_yolo.ah_yolo_node import resolve_weights  # noqa: E402


def main() -> int:
    ap = argparse.ArgumentParser(description="ah_yolo offline smoke inference")
    ap.add_argument("--profile", default="coco80", choices=("tank", "coco80"))
    ap.add_argument("--weights", default="", help="Override weights path")
    ap.add_argument("--image", required=True, help="Image path")
    ap.add_argument("--conf", type=float, default=0.25)
    args = ap.parse_args()

    try:
        from ultralytics import YOLO
    except ImportError:
        print("ERROR: ultralytics not installed. In ros_env: pip install ultralytics", file=sys.stderr)
        return 1

    weights = resolve_weights(args.profile, args.weights)
    print(f"weights={weights}")
    model = YOLO(str(weights))
    results = model.predict(source=args.image, conf=args.conf, verbose=False)
    r0 = results[0]
    n = 0 if r0.boxes is None else len(r0.boxes)
    print(f"detections={n}")
    if n:
        names = r0.names or {}
        for b in r0.boxes:
            cid = int(b.cls.item())
            print(f"  {names.get(cid, cid)} conf={float(b.conf):.3f} xyxy={b.xyxy.tolist()}")
    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
