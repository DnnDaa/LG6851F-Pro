from importlib.machinery import SourceFileLoader
from pathlib import Path
import struct
work=Path('/root/codex-rebuild/rx82-clean');p=work/'modules/mt_wifi.ko';Elf=SourceFileLoader('audit',str(work/'repairs/elf-audit.py')).load_module().Elf;e=Elf(p);f=e.functions()['CFG80211DRV_OpsVifAdd'];off=e.sh[f['section']][4]+f['value']+0x220;v=struct.unpack_from('<I',e.data,off)[0];assert v==0x540003e2;data=bytearray(e.data);struct.pack_into('<I',data,off,0x540003ea);p.write_bytes(data);print('CLIENT_PATCH_OK')
