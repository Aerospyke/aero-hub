#!/usr/bin/env python3
"""Copy an existing AeroHub.app and stage ROS shared libraries into Frameworks.

Does NOT run macdeployqt — iterate on ROS bundling without a full Qt redeploy.

Usage (from aero-hub/):
  ./scripts/bundle_ros_into_app.py
  ./scripts/bundle_ros_into_app.py \\
      --src run/AeroHub.app \\
      --dst run/AeroHub-standalone.app

Prerequisites:
  - Working lab app already installed (cmake --install …) at --src
  - CONDA_PREFIX or default ~/miniconda3/envs/ros_env with ROS dylibs
  - ros/install (ah_msgs, etc.)
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

APP_NAME = "AeroHub"
RPATH_DEP_RE = re.compile(r"^\s*(@rpath/[^ ]+)")
LC_RPATH_RE = re.compile(r"^\s*path (.*) \(offset \d+\)\s*$")

# Seed globs under conda lib (dlopen plugins + Fast DDS stack).
CONDA_SEED_GLOBS = (
    "librmw_fastrtps*.dylib",
    "librmw_dds_common*.dylib",
    "librosidl_typesupport_fastrtps*.dylib",
    "librosidl_dynamic_typesupport_fastrtps*.dylib",
    "libfastrtps*.dylib",
    "libfastcdr*.dylib",
    "libfoonathan_memory*.dylib",
)


def log(msg: str) -> None:
    print(f"[bundle_ros] {msg}")


def run(cmd: list[str], *, check: bool = False) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, check=check, text=True, capture_output=True)


def otool_rpath_deps(binary: Path) -> list[str]:
    """Return @rpath/... load commands from otool -L."""
    proc = run(["otool", "-L", str(binary)])
    if proc.returncode != 0:
        return []
    deps: list[str] = []
    for line in proc.stdout.splitlines():
        m = RPATH_DEP_RE.match(line)
        if m:
            deps.append(m.group(1))
    return sorted(set(deps))


def otool_absolute_rpaths(binary: Path) -> list[str]:
    """Absolute LC_RPATH entries (lab paths to strip from the copy)."""
    proc = run(["otool", "-l", str(binary)])
    if proc.returncode != 0:
        return []
    paths: list[str] = []
    lines = proc.stdout.splitlines()
    for i, line in enumerate(lines):
        if "cmd LC_RPATH" in line and i + 2 < len(lines):
            m = LC_RPATH_RE.match(lines[i + 2])
            if m:
                paths.append(m.group(1))
    return paths


def install_name_tool(*args: str) -> None:
    run(["install_name_tool", *args])  # ignore failures (duplicate rpath, etc.)


def resolve_path(p: str | Path) -> Path:
    path = Path(p).expanduser()
    if path.is_absolute():
        return path.resolve()
    return (Path.cwd() / path).resolve()


def collect_search_dirs(conda_lib: Path, ros_install: Path) -> list[Path]:
    dirs = [conda_lib]
    if ros_install.is_dir():
        dirs.extend(sorted({p for p in ros_install.rglob("lib") if p.is_dir()}))
    return dirs


def find_lib(name: str, search_dirs: list[Path]) -> Path | None:
    for d in search_dirs:
        candidate = d / name
        if candidate.is_file():
            return candidate
    return None


def stage_lib(src: Path, frameworks: Path) -> None:
    name = src.name
    dest = frameworks / name
    if not dest.is_file():
        shutil.copy2(src, dest)
        log(f"+ {name}")
    install_name_tool("-id", f"@loader_path/../Frameworks/{name}", str(dest))


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent.parent
    home = Path.home()
    default_conda = Path(os.environ["CONDA_PREFIX"]) / "lib" if os.environ.get("CONDA_PREFIX") else home / "miniconda3/envs/ros_env/lib"

    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--src", type=str, default=str(root / "run" / "AeroHub.app"), help="Source .app (lab install)")
    p.add_argument(
        "--dst",
        type=str,
        default=str(root / "run" / "AeroHub-standalone.app"),
        help="Destination .app (copy; never overwrites --src logic beyond replace dst)",
    )
    p.add_argument("--conda-lib", type=str, default=str(default_conda), help="ROS/conda lib directory")
    p.add_argument("--ros-install", type=str, default=str(root / "ros" / "install"), help="colcon install prefix")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    src = resolve_path(args.src)
    dst = resolve_path(args.dst)
    conda_lib = resolve_path(args.conda_lib)
    ros_install = resolve_path(args.ros_install)

    exe = dst / "Contents" / "MacOS" / APP_NAME
    frameworks = dst / "Contents" / "Frameworks"

    if not (src / "Contents" / "MacOS").is_dir():
        print(f"error: source app missing or incomplete: {src}", file=sys.stderr)
        print("  First: cmake --build … --target AeroHub && cmake --install …", file=sys.stderr)
        return 1
    if not conda_lib.is_dir():
        print(f"error: conda ROS lib dir not found: {conda_lib}", file=sys.stderr)
        print("  Activate ros_env or pass --conda-lib", file=sys.stderr)
        return 1

    log(f"src={src}")
    log(f"dst={dst}")
    log(f"conda_lib={conda_lib}")
    log(f"ros_install={ros_install}")

    # Fresh copy — never mutate the lab install app.
    if dst.exists():
        shutil.rmtree(dst)
    # ditto preserves macOS resource forks / app layout better than copytree alone.
    subprocess.run(["ditto", str(src), str(dst)], check=True)
    frameworks.mkdir(parents=True, exist_ok=True)

    search_dirs = collect_search_dirs(conda_lib, ros_install)

    seeds: list[Path] = []
    for pattern in CONDA_SEED_GLOBS:
        seeds.extend(sorted(conda_lib.glob(pattern)))
    ah_msgs_lib = ros_install / "ah_msgs" / "lib"
    if ah_msgs_lib.is_dir():
        seeds.extend(sorted(ah_msgs_lib.glob("libah_msgs*.dylib")))

    for dep in otool_rpath_deps(exe):
        base = dep.removeprefix("@rpath/")
        if (frameworks / base).is_file():
            continue
        found = find_lib(base, search_dirs)
        if found:
            seeds.append(found)

    # Multi-pass: stage queue, then scan all Frameworks + main binary for missing @rpath.
    queue: list[Path] = list(dict.fromkeys(seeds))  # preserve order, unique
    for _pass in range(25):
        for src_lib in queue:
            if src_lib.is_file():
                stage_lib(src_lib, frameworks)
        queue = []
        candidates = list(frameworks.glob("*.dylib"))
        for binary in candidates + [exe]:
            for dep in otool_rpath_deps(binary):
                base = dep.removeprefix("@rpath/")
                if (frameworks / base).is_file():
                    continue
                found = find_lib(base, search_dirs)
                if found:
                    queue.append(found)
        # unique paths not yet staged
        next_q: list[Path] = []
        seen: set[str] = set()
        for p in queue:
            if p.name in seen or (frameworks / p.name).is_file():
                continue
            seen.add(p.name)
            next_q.append(p)
        queue = next_q
        if not queue:
            break

    # Rewrite @rpath → @loader_path among Frameworks dylibs
    for dst_lib in frameworks.glob("*.dylib"):
        for dep in otool_rpath_deps(dst_lib):
            base = dep.removeprefix("@rpath/")
            if (frameworks / base).is_file():
                install_name_tool("-change", dep, f"@loader_path/{base}", str(dst_lib))

    # Main binary: point remaining @rpath at Frameworks
    for dep in otool_rpath_deps(exe):
        base = dep.removeprefix("@rpath/")
        if (frameworks / base).is_file():
            install_name_tool("-change", dep, f"@loader_path/../Frameworks/{base}", str(exe))

    # Drop absolute lab rpaths so cold start exercises Frameworks only
    for rpath in otool_absolute_rpaths(exe):
        if rpath.startswith("/"):
            log(f"remove absolute rpath: {rpath}")
            install_name_tool("-delete_rpath", rpath, str(exe))

    install_name_tool("-add_rpath", "@loader_path/../Frameworks", str(exe))
    install_name_tool("-add_rpath", "@executable_path/../Frameworks", str(exe))

    missing = 0
    for dep in otool_rpath_deps(exe):
        base = dep.removeprefix("@rpath/")
        if not (frameworks / base).is_file():
            print(f"[bundle_ros] warning: still unresolved on main binary: {dep}", file=sys.stderr)
            missing = 1
    for dst_lib in frameworks.glob("*.dylib"):
        for dep in otool_rpath_deps(dst_lib):
            base = dep.removeprefix("@rpath/")
            if not (frameworks / base).is_file():
                print(f"[bundle_ros] warning: {dst_lib.name} still needs: {dep}", file=sys.stderr)
                missing = 1

    log("codesigning…")
    subprocess.run(["codesign", "--force", "--deep", "--sign", "-", str(dst)], check=False)

    count = len(list(frameworks.glob("*.dylib")))
    log(f"done: {dst}  ({count} dylibs in Frameworks)")
    if missing:
        print(
            "[bundle_ros] finished with unresolved warnings (app may still crash on missing plugins).",
            file=sys.stderr,
        )
        return 3
    log(f"test:  {exe}")
    log("note:  ament share/ is not copied; pure dylib bundling may still need AMENT_PREFIX_PATH for some ROS features.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
