import os, shutil, subprocess
from pathlib import Path
os.environ["RESUME"]="1"
p=Path("/root/codex-rebuild/rx82-clean-build-lib.py")
s=p.read_text().split("if RESUME:\n",1)[0]
exec(compile(s,str(p),"exec"),globals())
assert not WORK.exists()
WORK.mkdir(parents=True); LOG.touch()
shutil.copytree(LANACCEL / "src", SOURCE, ignore=shutil.ignore_patterns("*.o", "*.ko", ".*.cmd", "*.mod", "*.mod.c", "modules.order", "Module.symvers", "*.symvers", ".tmp_versions", "*.a", "*.so"))
shutil.copytree(VENDOR / "include", INCLUDE)
shutil.copytree(Path("/root/codex-rebuild/driver-full-20261021/repairs"), WORK/"repairs")
OUTPUT.mkdir()
subprocess.run(["python3","/root/codex-rebuild/rx82-patch.py",str(SOURCE/"wifi/mt_wifi/ap/ap_data.c")],check=True)
for n in ("cmn","coex","wifi"):run_make(n)
print("CLEAN_WIFI_BUILD_OK",flush=True)
