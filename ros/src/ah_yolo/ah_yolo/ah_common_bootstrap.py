"""Bootstrap ROS domain/namespace via AhCommon shared library (same as ah_core).

Loads libah_common and calls ah_settings_bootstrap() so Python uses the C++
settings engine — not a duplicate INI parser.
"""

from __future__ import annotations

import ctypes
import os
import sys
from ctypes import c_char, c_int, c_uint8
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class RosRuntimeBootstrap:
    domain_id: int
    namespace: str
    settings_path: str
    models_dir: str
    loaded_from_file: bool


class _AhSettingsBootstrap(ctypes.Structure):
    _fields_ = [
        ("domain_id", c_uint8),
        ("loaded_from_file", c_int),
        ("ros_namespace", c_char * 256),
        ("settings_path", c_char * 1024),
        ("models_dir", c_char * 1024),
    ]


def _lib_candidates() -> list[Path]:
    # Shared lib with C ABI (OUTPUT_NAME ah_common_c).
    if sys.platform == "darwin":
        names = ["libah_common_c.dylib"]
    elif sys.platform.startswith("linux"):
        names = ["libah_common_c.so"]
    else:
        names = ["ah_common_c.dll", "libah_common_c.dll"]

    paths: list[Path] = []
    env = os.environ.get("AH_COMMON_LIB", "").strip()
    if env:
        paths.append(Path(env))

    try:
        from ament_index_python.packages import get_package_prefix

        prefix = Path(get_package_prefix("ah_common"))
        for n in names:
            paths.append(prefix / "lib" / n)
    except Exception:  # noqa: BLE001 — ament not available in some test contexts
        pass

    # Colcon workspace: install/ah_common/lib, or AhCommon build dirs.
    here = Path(__file__).resolve()
    for parent in here.parents:
        for n in names:
            paths.append(parent / "install" / "ah_common" / "lib" / n)
            paths.append(parent / "lib" / n)
            paths.append(parent / "build" / "ah_common" / "AhCommon" / n)
            paths.append(parent / "build" / "ah_core" / "AhCommon" / n)

    seen: set[str] = set()
    out: list[Path] = []
    for p in paths:
        key = str(p)
        if key not in seen:
            seen.add(key)
            out.append(p)
    return out


def _load_ah_common() -> ctypes.CDLL:
    last_err: Exception | None = None
    for candidate in _lib_candidates():
        if not candidate.is_file():
            continue
        try:
            return ctypes.CDLL(str(candidate))
        except OSError as ex:
            last_err = ex
            continue
    tried = "\n  ".join(str(p) for p in _lib_candidates()[:12])
    msg = "could not load AhCommon shared library (libah_common).\n  tried:\n  " + tried
    if last_err:
        msg += f"\n  last error: {last_err}"
    msg += "\n  Build/install: colcon build --packages-select ah_common (or build_ros)."
    raise RuntimeError(msg)


def apply_ros_runtime_from_ah_common() -> RosRuntimeBootstrap:
    """Call AhCommon C API; apply effective domain/namespace into this process."""
    lib = _load_ah_common()
    lib.ah_settings_bootstrap.argtypes = [ctypes.POINTER(_AhSettingsBootstrap)]
    lib.ah_settings_bootstrap.restype = c_int

    buf = _AhSettingsBootstrap()
    rc = lib.ah_settings_bootstrap(ctypes.byref(buf))
    if rc != 0:
        raise RuntimeError(f"ah_settings_bootstrap failed with code {rc}")

    domain_id = int(buf.domain_id)
    namespace = buf.ros_namespace.decode("utf-8", errors="replace")
    settings_path = buf.settings_path.decode("utf-8", errors="replace")
    models_dir = buf.models_dir.decode("utf-8", errors="replace")
    loaded = bool(buf.loaded_from_file)

    # C++ Load() setenv ROS_DOMAIN_ID when unset; ensure Python sees effective id.
    os.environ["ROS_DOMAIN_ID"] = str(domain_id)
    if models_dir and not os.environ.get("AERO_HUB_MODELS", "").strip():
        os.environ["AERO_HUB_MODELS"] = models_dir
    if settings_path and not os.environ.get("AERO_HUB_SETTINGS", "").strip():
        os.environ["AERO_HUB_SETTINGS"] = settings_path

    return RosRuntimeBootstrap(
        domain_id=domain_id,
        namespace=namespace,
        settings_path=settings_path,
        models_dir=models_dir,
        loaded_from_file=loaded,
    )
