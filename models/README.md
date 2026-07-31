# AeroHub YOLO weights

Used by `ah_yolo` (Task_33+).

| Profile | Default files (first found wins) | Notes |
|---------|----------------------------------|-------|
| **coco80** | `yolo11n.pt` (present here) | 80 COCO classes — virtual world / X-Plane via OBS |
| **tank** | `tank.pt` (after Ultralytics retrain) | cyberbrick mini tank — **not** MLX/safetensors |

**Lab now:** use **`yolo11n.pt`** for stack integration (Task_33–35). Retrain the mini-tank detector with **Ultralytics** (not MLX) and drop `best.pt` here as `tank.pt` when ready.

## Interim test (no tank weights yet)

```bash
export AERO_HUB_MODELS=~/Documents/projects/pix-eagle-stack/aero-hub/models
# coco80 → models/yolo11n.pt automatically
ros2 run ah_yolo ah_yolo_node --ros-args -p profile:=coco80

# or force the same weights on any profile name:
ros2 run ah_yolo ah_yolo_node --ros-args \
  -p profile:=coco80 \
  -p weights_path:=$AERO_HUB_MODELS/yolo11n.pt
```

## After Ultralytics tank retrain

```bash
# Ultralytics train writes runs/detect/.../weights/best.pt
cp /path/to/runs/detect/.../weights/best.pt \
  ~/Documents/projects/pix-eagle-stack/aero-hub/models/tank.pt

ros2 run ah_yolo ah_yolo_node --ros-args -p profile:=tank
```

## `.pt` vs `.safetensors`

| Format | Support |
|--------|---------|
| **`.pt`** | Preferred. Native Ultralytics checkpoint (`best.pt` / `last.pt` from a YOLO train). |
| **`.safetensors`** | Resolved by path search, but **Ultralytics may not load it** as a full YOLO model (needs architecture + heads, not just tensors). |
| **`.onnx`** | Optional later path; not the default runtime. |

### If you only have `*.safetensors`

1. **Look next to the safetensors file** in the training run directory for **`best.pt` or `last.pt`** — Ultralytics usually writes those even when something else also exports safetensors.
2. Point ah_yolo at the `.pt` explicitly:

```bash
export AERO_HUB_YOLO_TANK_WEIGHTS=/absolute/path/to/best.pt
# or
ros2 run ah_yolo ah_yolo_node --ros-args \
  -p profile:=tank \
  -p weights_path:=/absolute/path/to/best.pt
```

3. Copy into models for defaults:

```bash
cp /path/to/best.pt \
  ~/Documents/projects/pix-eagle-stack/aero-hub/models/tank.pt
```

4. If there is **truly no `.pt`**, tell us which tool produced the `.safetensors` (Ultralytics version, Hugging Face, custom export). Loading often needs a matching model YAML/class names — we can wire a converter once we know the source.

### Env overrides

```bash
export AERO_HUB_MODELS=/absolute/path/to/models
export AERO_HUB_YOLO_TANK_WEIGHTS=/absolute/path/to/weights.pt   # or .safetensors
```

## Optional: local nano COCO weights

```bash
cd ~/Documents/projects/pix-eagle-stack/aero-hub/models
# Ultralytics downloads on first coco80 run if missing; or copy from PixEagle lab (read-only):
# cp ../../PixEagle/models/yolo11n.pt .
```

Do **not** commit large weight binaries unless the team agrees.
