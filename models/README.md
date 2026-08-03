# AeroHub YOLO weights

Used by `ah_yolo` (Task_33+). Directory is exported as `AERO_HUB_MODELS` by `ros/init_ah_ros_in_terminal.sh`.

| Profile | Default files (first found wins) | Notes |
|---------|----------------------------------|-------|
| **coco80** | `yolo11n.pt` | 80 COCO classes — virtual world / X-Plane via OBS |
| **tank** | `mini_tank_0308.pt`, then `mini_tank*.pt`, `tank.pt`, … | cyberbrick mini tank (Ultralytics `.pt`) |

Large `*.pt` files are gitignored — keep them local under this folder.

## Lab: mini tank (current)

Weights on disk: **`mini_tank_0308.pt`**.

**Runtime switch (Dashboard):** Ops → Tracking → **YOLO model** → Mini tank / COCO-80 → **Apply model**  
(calls `ah/yolo/set_profile`; `ah_yolo` must already be running).

```bash
# after init_ah_ros_in_terminal.sh (sets AERO_HUB_MODELS)
ros2 run ah_yolo ah_yolo_node --ros-args -p profile:=tank
# or start on coco80 and switch from the dashboard
ros2 run ah_yolo ah_yolo_node --ros-args -p profile:=coco80
```

Startup log should show something like:

```text
ah_yolo ready: profile=tank weights=…/models/mini_tank_0308.pt …
```

Explicit path (optional):

```bash
ros2 run ah_yolo ah_yolo_node --ros-args \
  -p profile:=tank \
  -p weights_path:=$AERO_HUB_MODELS/mini_tank_0308.pt
```

Or env override:

```bash
export AERO_HUB_YOLO_TANK_WEIGHTS=$AERO_HUB_MODELS/mini_tank_0308.pt
ros2 run ah_yolo ah_yolo_node --ros-args -p profile:=tank
```

Reload without restarting the process (after swapping files):

```bash
ros2 service call /ah/yolo/reload std_srvs/srv/Trigger
```

## Lab: COCO baseline (virtual / multi-class)

```bash
ros2 run ah_yolo ah_yolo_node --ros-args -p profile:=coco80
# → models/yolo11n.pt
```

## Adding a newer tank checkpoint

Prefer a dated name so versions stay clear:

```bash
cp /path/to/runs/detect/.../weights/best.pt \
  ~/Documents/projects/pix-eagle-stack/aero-hub/models/mini_tank_MMDD.pt
```

`profile:=tank` prefers **`mini_tank_0308`**, then other `mini_tank*`, then `tank.pt`.  
Or point `weights_path` / `AERO_HUB_YOLO_TANK_WEIGHTS` at the new file.

Stable alias (optional):

```bash
cp mini_tank_0308.pt tank.pt
```

## `.pt` vs `.safetensors`

| Format | Support |
|--------|---------|
| **`.pt`** | Preferred. Native Ultralytics checkpoint. |
| **`.safetensors`** | Path may resolve, but Ultralytics often cannot load it as a full YOLO model. |
| **`.onnx`** | Optional later; not the default runtime. |

## Env overrides

```bash
export AERO_HUB_MODELS=/absolute/path/to/models
export AERO_HUB_YOLO_TANK_WEIGHTS=/absolute/path/to/weights.pt
```

Do **not** commit large weight binaries unless the team agrees.
