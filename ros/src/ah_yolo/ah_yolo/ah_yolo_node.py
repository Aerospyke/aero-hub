#!/usr/bin/env python3
"""ah_yolo_node — Ultralytics YOLO on /ah/video/compressed → /ah/detections (JSON).

Profiles:
  tank   — mini-tank detector (models/mini_tank_*.pt, tank.pt, or param)
  coco80 — COCO-style 80-class baseline (yolo11n.pt by default)

Runtime: Python + Ultralytics (simplest path that loads custom .pt weights).
"""

from __future__ import annotations

import json
import os
import threading
import time
from pathlib import Path
from typing import Any

import cv2
import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.parameter import Parameter
from rclpy.qos import HistoryPolicy, QoSProfile, ReliabilityPolicy
from sensor_msgs.msg import CompressedImage
from std_msgs.msg import String
from std_srvs.srv import Trigger

try:
    from ah_msgs.srv import SetYoloProfile
except ImportError:  # pragma: no cover — package layout during early bootstrap
    SetYoloProfile = None  # type: ignore

# Lazy import ultralytics so --help / package install works without torch.
YOLO = None  # type: ignore


def _import_yolo():
    global YOLO
    if YOLO is None:
        from ultralytics import YOLO as _YOLO

        YOLO = _YOLO
    return YOLO


def _default_models_dir() -> Path:
    # Prefer AERO_HUB_MODELS, then aero-hub/models relative to common layouts.
    env = os.environ.get("AERO_HUB_MODELS", "").strip()
    if env:
        return Path(env).expanduser().resolve()
    here = Path(__file__).resolve()
    candidates = [
        here.parents[4] / "models",  # …/aero-hub/ros/install/... → not reliable
        Path.cwd() / "models",
        Path.cwd().parent / "models",
        Path.cwd().parent.parent / "models",
        Path.home() / "Documents/projects/pix-eagle-stack/aero-hub/models",
    ]
    for c in candidates:
        if c.is_dir():
            return c.resolve()
    return (Path.cwd() / "models").resolve()


# Accepted weight suffixes (Ultralytics prefers .pt; .safetensors also resolved).
_WEIGHT_SUFFIXES = (".pt", ".safetensors", ".onnx")


def _first_existing(paths: list[Path]) -> Path | None:
    for p in paths:
        if p.is_file():
            return p.resolve()
    return None


def resolve_weights(profile: str, weights_path: str) -> Path:
    """Resolve weights file for profile; raise FileNotFoundError if missing.

    Supports ``.pt`` (preferred for Ultralytics), ``.safetensors``, and ``.onnx``.
    """
    if weights_path.strip():
        p = Path(weights_path).expanduser().resolve()
        if not p.is_file():
            raise FileNotFoundError(f"weights_path not found: {p}")
        return p

    models = _default_models_dir()
    profile = profile.strip().lower()
    if profile == "tank":
        env = os.environ.get("AERO_HUB_YOLO_TANK_WEIGHTS", "").strip()
        if env:
            p = Path(env).expanduser().resolve()
            if p.is_file():
                return p
            raise FileNotFoundError(f"AERO_HUB_YOLO_TANK_WEIGHTS not found: {p}")

        # Prefer dated mini_tank checkpoints, then stable aliases. Never fall back
        # to COCO nanos (yolo11n / yolov8n) via a blind *.pt glob.
        basenames = (
            "mini_tank_0308",
            "mini_tank",
            "tank",
            "cyberbrick_tank",
            "cyberbrick",
            "best",
            "last",
        )
        candidates: list[Path] = []
        for base in basenames:
            for suf in _WEIGHT_SUFFIXES:
                candidates.append(models / f"{base}{suf}")
        if models.is_dir():
            # Newer dated exports first (mini_tank_0308.pt before mini_tank_0101.pt).
            for pattern in ("mini_tank*.pt", "mini_tank*.safetensors", "tank*.pt", "tank*.safetensors"):
                candidates.extend(sorted(models.glob(pattern), reverse=True))

        found = _first_existing(candidates)
        if found is not None:
            return found

        raise FileNotFoundError(
            "tank profile: place Ultralytics weights under models/ as "
            "mini_tank_0308.pt (or tank.pt), "
            f"(looked under {models}), or set AERO_HUB_YOLO_TANK_WEIGHTS / weights_path. "
            "See models/README.md."
        )

    if profile in ("coco80", "coco", "coco-80"):
        # Prefer local copy; else Ultralytics will download yolo11n.pt on first use.
        for name in ("yolo11n.pt", "yolov8n.pt"):
            p = models / name
            if p.is_file():
                return p
        # Relative to package — still let Ultralytics fetch by name.
        return Path("yolo11n.pt")

    raise ValueError(f"unknown profile '{profile}' (use tank | coco80)")


def pick_device(requested: str) -> str:
    req = (requested or "").strip().lower()
    if req:
        return req
    try:
        import torch

        if getattr(torch.backends, "mps", None) and torch.backends.mps.is_available():
            return "mps"
        if torch.cuda.is_available():
            return "0"
    except Exception:
        pass
    return "cpu"


class AhYoloNode(Node):
    def __init__(self) -> None:
        super().__init__("ah_yolo")

        self.declare_parameter("profile", "coco80")
        self.declare_parameter("weights_path", "")
        self.declare_parameter("conf_threshold", 0.25)
        self.declare_parameter("iou_threshold", 0.45)
        self.declare_parameter("frame_stride", 2)
        self.declare_parameter("image_topic", "ah/video/compressed")
        self.declare_parameter("detections_topic", "ah/detections")
        self.declare_parameter("device", "")

        self._model_lock = threading.Lock()
        self._model = None
        self._model_path: Path | None = None
        self._frame_count = 0
        self._last_infer_s = 0.0

        video_qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=1,
        )
        det_qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=1,
        )

        image_topic = self.get_parameter("image_topic").get_parameter_value().string_value
        detections_topic = (
            self.get_parameter("detections_topic").get_parameter_value().string_value
        )

        self._det_pub = self.create_publisher(String, detections_topic, det_qos)
        self._image_sub = self.create_subscription(
            CompressedImage, image_topic, self._on_image, video_qos
        )

        self._reload_srv = self.create_service(
            Trigger, "ah/yolo/reload", self._on_reload
        )
        if SetYoloProfile is not None:
            self._set_profile_srv = self.create_service(
                SetYoloProfile, "ah/yolo/set_profile", self._on_set_profile
            )
        else:
            self._set_profile_srv = None
            self.get_logger().warn(
                "ah_msgs SetYoloProfile unavailable — rebuild ah_msgs / ah_yolo; "
                "runtime profile switch disabled"
            )

        self._load_model()
        domain = os.environ.get("ROS_DOMAIN_ID", "(default 0)")
        self.get_logger().info(
            f"ah_yolo ready: profile={self._profile()} weights={self._model_path} "
            f"device={self._device()} domain={domain} sub={image_topic} pub={detections_topic}"
        )

    def _profile(self) -> str:
        return self.get_parameter("profile").get_parameter_value().string_value.strip().lower()

    def _device(self) -> str:
        return pick_device(
            self.get_parameter("device").get_parameter_value().string_value
        )

    def _load_model(self) -> None:
        profile = self._profile()
        weights_param = (
            self.get_parameter("weights_path").get_parameter_value().string_value
        )
        path = resolve_weights(profile, weights_param)
        yolo_cls = _import_yolo()
        device = self._device()
        self.get_logger().info(f"Loading YOLO weights={path} device={device} …")

        if path.suffix.lower() == ".safetensors":
            # Ultralytics primarily loads .pt checkpoints. Some training exports only
            # write safetensors; try YOLO() and surface a clear conversion hint on fail.
            self.get_logger().warn(
                "weights are .safetensors — Ultralytics usually wants .pt "
                "(same training run often has best.pt). Attempting load…"
            )

        try:
            model = yolo_cls(str(path))
        except Exception as ex:
            if path.suffix.lower() == ".safetensors":
                raise RuntimeError(
                    f"Failed to load {path} with Ultralytics ({ex}). "
                    "Prefer the training run's best.pt / last.pt, or convert: "
                    "pip install safetensors torch  # then see models/README.md. "
                    "You can also pass -p weights_path:=/path/to/best.pt"
                ) from ex
            raise

        with self._model_lock:
            self._model = model
            self._model_path = path
        self.get_logger().info(f"YOLO loaded: {path}")

    def _on_reload(
        self, _request: Trigger.Request, response: Trigger.Response
    ) -> Trigger.Response:
        try:
            self._load_model()
            response.success = True
            response.message = f"reloaded profile={self._profile()} path={self._model_path}"
        except Exception as ex:  # noqa: BLE001 — surface to operator
            response.success = False
            response.message = str(ex)
            self.get_logger().error(f"reload failed: {ex}")
        return response

    def _on_set_profile(
        self, request: "SetYoloProfile.Request", response: "SetYoloProfile.Response"
    ) -> "SetYoloProfile.Response":
        """Switch tank | coco80 (and optional weights_path) then reload weights."""
        raw = (request.profile or "").strip().lower()
        aliases = {
            "tank": "tank",
            "mini_tank": "tank",
            "minitank": "tank",
            "coco80": "coco80",
            "coco": "coco80",
            "coco-80": "coco80",
        }
        profile = aliases.get(raw, raw)
        if profile not in ("tank", "coco80"):
            response.success = False
            response.message = f"unknown profile '{request.profile}' (use tank | coco80)"
            response.active_profile = self._profile()
            response.weights_path = str(self._model_path) if self._model_path else ""
            return response

        weights_override = (request.weights_path or "").strip()
        try:
            self.set_parameters(
                [
                    Parameter("profile", Parameter.Type.STRING, profile),
                    Parameter("weights_path", Parameter.Type.STRING, weights_override),
                ]
            )
            self._load_model()
            response.success = True
            response.active_profile = self._profile()
            response.weights_path = str(self._model_path) if self._model_path else ""
            response.message = (
                f"profile={response.active_profile} weights={response.weights_path}"
            )
            self.get_logger().info(f"set_profile OK: {response.message}")
        except Exception as ex:  # noqa: BLE001
            response.success = False
            response.message = str(ex)
            response.active_profile = self._profile()
            response.weights_path = str(self._model_path) if self._model_path else ""
            self.get_logger().error(f"set_profile failed: {ex}")
        return response

    def _on_image(self, msg: CompressedImage) -> None:
        stride = max(1, int(self.get_parameter("frame_stride").value))
        self._frame_count += 1
        if self._frame_count % stride != 0:
            return

        with self._model_lock:
            model = self._model
        if model is None:
            return

        arr = np.frombuffer(msg.data, dtype=np.uint8)
        bgr = cv2.imdecode(arr, cv2.IMREAD_COLOR)
        if bgr is None or bgr.size == 0:
            self.get_logger().warning("JPEG decode failed")
            return

        h, w = bgr.shape[:2]
        conf = float(self.get_parameter("conf_threshold").value)
        iou = float(self.get_parameter("iou_threshold").value)
        device = self._device()

        # Use track() (ByteTrack/BoTSORT via Ultralytics) so detections carry a
        # persistent track_id across frames. Plain predict() has no multi-frame IDs.
        t0 = time.perf_counter()
        try:
            results = model.track(
                source=bgr,
                conf=conf,
                iou=iou,
                device=device,
                persist=True,
                verbose=False,
            )
        except Exception as ex:  # noqa: BLE001
            self.get_logger().error(f"track failed: {ex}")
            return
        self._last_infer_s = time.perf_counter() - t0

        detections: list[dict[str, Any]] = []
        if results:
            r0 = results[0]
            names = r0.names if hasattr(r0, "names") else {}
            boxes = getattr(r0, "boxes", None)
            if boxes is not None and len(boxes) > 0:
                xyxy = boxes.xyxy.cpu().numpy()
                confs = boxes.conf.cpu().numpy()
                clss = boxes.cls.cpu().numpy().astype(int)
                if boxes.id is not None:
                    track_ids = boxes.id.int().cpu().numpy()
                else:
                    track_ids = None
                for i in range(len(xyxy)):
                    x1, y1, x2, y2 = xyxy[i]
                    bw = max(0.0, float(x2 - x1))
                    bh = max(0.0, float(y2 - y1))
                    nx = float(x1) / float(w) if w else 0.0
                    ny = float(y1) / float(h) if h else 0.0
                    nw = bw / float(w) if w else 0.0
                    nh = bh / float(h) if h else 0.0
                    # Clamp to [0,1]
                    nx = min(1.0, max(0.0, nx))
                    ny = min(1.0, max(0.0, ny))
                    nw = min(1.0 - nx, max(0.0, nw))
                    nh = min(1.0 - ny, max(0.0, nh))
                    cid = int(clss[i])
                    cname = names.get(cid, str(cid)) if isinstance(names, dict) else str(cid)
                    tid = int(track_ids[i]) if track_ids is not None else -1
                    detections.append(
                        {
                            "track_id": tid,
                            "class_id": cid,
                            "class_name": cname,
                            "confidence": float(confs[i]),
                            "bbox_normalized": {
                                "x": nx,
                                "y": ny,
                                "w": nw,
                                "h": nh,
                            },
                        }
                    )

        stamp = float(msg.header.stamp.sec) + float(msg.header.stamp.nanosec) * 1e-9
        payload = {
            "profile": self._profile(),
            "weights": str(self._model_path) if self._model_path else "",
            "stamp": stamp,
            "infer_s": round(self._last_infer_s, 4),
            "frame_width": int(w),
            "frame_height": int(h),
            "detections": detections,
        }
        out = String()
        out.data = json.dumps(payload, separators=(",", ":"))
        self._det_pub.publish(out)

        if detections and (self._frame_count // stride) % 15 == 0:
            self.get_logger().info(
                f"detections={len(detections)} infer_s={self._last_infer_s:.3f}"
            )


def main(args: list[str] | None = None) -> None:
    rclpy.init(args=args)
    node = AhYoloNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
