#!/usr/bin/env python3
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time


WORK = Path("/root/codex-rebuild/rx82-clean")
SOURCE = WORK / "source"
INCLUDE = WORK / "include"
OUTPUT = WORK / "modules"
LOG = WORK / "build.log"
VENDOR = Path("/root/build/lg6851f-vendor618")
LANACCEL = Path("/root/build/lg6851f-lanaccel")
KERNEL = Path(
    "/root/build/owrt/build_dir/"
    "target-aarch64_cortex-a55+neon-vfpv4_musl/"
    "linux-mediatek_mt6990/linux-6.18.44"
)
CROSS = (
    "/root/build/owrt/staging_dir/"
    "toolchain-aarch64_cortex-a55+neon-vfpv4_gcc-14.4.0_musl/"
    "bin/aarch64-openwrt-linux-musl-"
)
RESUME = os.environ.get("RESUME") == "1"


def emit(message):
    with LOG.open("a", encoding="utf-8") as stream:
        stream.write(message + "\n")
        stream.flush()


def load_command(name):
    return json.loads((VENDOR / f"{name}-command.json").read_text())


def replace_paths(value):
    if isinstance(value, list):
        return [replace_paths(item) for item in value]
    if not isinstance(value, str):
        return value
    value = value.replace(str(LANACCEL / "src"), str(SOURCE))
    value = value.replace(str(VENDOR / "src"), str(SOURCE))
    value = value.replace(str(LANACCEL / "include"), str(INCLUDE))
    value = value.replace(str(VENDOR / "include"), str(INCLUDE))
    return value


def run_make(name):
    args = replace_paths(load_command(name))
    for index, arg in enumerate(args):
        if arg.startswith("KCFLAGS="):
            flag = "-DCONFIG_MT_WIFI_DISABLE_EMLSR_T2L=0"
            if flag not in arg:
                args[index] = arg + " " + flag
    if not RESUME:
        emit(f"\n===== {name}: clean =====")
        clean_args = list(args)
        assert clean_args[-1] == "modules"
        clean_args[-1] = "clean"
        clean = subprocess.run(
            clean_args,
            cwd=KERNEL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
        )
        emit(clean.stdout)
        clean_cmd = WORK / f"{name}-clean.json"
        clean_cmd.write_text(json.dumps(clean_args, indent=2))

    emit(f"\n===== {name}: build =====")
    started = time.time()
    result = subprocess.run(
        args,
        cwd=KERNEL,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
    )
    emit(result.stdout)
    emit(f"===== {name}: exit={result.returncode} seconds={time.time()-started:.1f} =====")
    command_record = WORK / f"{name}-command.json"
    command_record.write_text(json.dumps(args, indent=2))
    if result.returncode:
        raise SystemExit(f"{name} failed with exit code {result.returncode}")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


if RESUME:
    if not WORK.exists():
        raise SystemExit(f"resume workspace missing: {WORK}")
else:
    if WORK.exists():
        shutil.rmtree(WORK)
    WORK.mkdir(parents=True)
    (WORK / "build.log").touch()
emit("LG6851F full vendor Wi-Fi driver build")
emit(time.strftime("%Y-%m-%d %H:%M:%S %z"))

if not RESUME:
    shutil.copytree(
        LANACCEL / "src",
        SOURCE,
        copy_function=shutil.copy2,
        ignore=shutil.ignore_patterns(
            "*.o",
            "*.ko",
            ".*.cmd",
            "*.mod",
            "*.mod.c",
            "modules.order",
            "Module.symvers",
            "*.symvers",
            ".*.symvers.cmd",
            ".*.ko.cmd",
            ".tmp_versions",
            "*.a",
            "*.so",
        ),
    )
    shutil.copytree(VENDOR / "include", INCLUDE)
    OUTPUT.mkdir()

emit(f"source={SOURCE}")
emit(f"kernel={KERNEL}")
emit("cross_compile=" + CROSS)

for module in ("cmn", "coex", "wifi", "warp", "hwifi"):
    run_make(module)

artifacts = [
    SOURCE / "mt_wifi_cmn/mt_wifi_cmn.ko",
    SOURCE / "wifi_md_coex/wifi_md_coex.ko",
    SOURCE / "wifi/mt_wifi_ap/mt_wifi.ko",
    SOURCE / "wifi/warp_driver/warp/mtk_warp.ko",
    SOURCE / "wifi/wlan_hwifi/connac_if.ko",
    SOURCE / "wifi/wlan_hwifi/mt7990.ko",
    SOURCE / "wifi/wlan_hwifi/mt7991.ko",
    SOURCE / "wifi/wlan_hwifi/mtk_hwifi.ko",
    SOURCE / "wifi/wlan_hwifi/mtk_pci.ko",
    SOURCE / "wifi/wlan_hwifi/mtk_wed.ko",
]
manifest = {}
for path in artifacts:
    if not path.is_file():
        raise SystemExit(f"missing build artifact: {path}")
    target = OUTPUT / path.name
    shutil.copy2(path, target)
    manifest[path.name] = {
        "size": target.stat().st_size,
        "sha256": sha256(target),
    }
    emit(f"{manifest[path.name]['sha256']}  {target.stat().st_size}  {target.name}")

(OUTPUT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
emit("BUILD_OK")
