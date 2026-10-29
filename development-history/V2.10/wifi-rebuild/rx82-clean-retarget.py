#!/usr/bin/env python3
from importlib.machinery import SourceFileLoader
from pathlib import Path
import hashlib
import json
import struct
import subprocess


WORK = Path("/root/codex-rebuild/rx82-clean")
REPAIRS = WORK / "repairs"
BASE = WORK / "source/wifi/mt_wifi_ap/mt_wifi.ko"
MERGED = WORK / "mt_wifi-merged-unstripped.ko"
REPAIRED = WORK / "mt_wifi-repaired-unstripped.ko"
FINAL = WORK / "modules/mt_wifi.ko"
TC = (
    "/root/build/owrt/staging_dir/"
    "toolchain-aarch64_cortex-a55+neon-vfpv4_gcc-14.4.0_musl/"
    "bin/aarch64-openwrt-linux-musl-"
)
Elf = SourceFileLoader("audit", str(REPAIRS / "elf-audit.py")).load_module().Elf


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def add_relocations(data, elf, relocation_section, extra):
    while len(data) % 8:
        data.append(0)
    offset = len(data)
    for relocation in sorted(extra):
        data.extend(struct.pack("<QQq", *relocation))
    header = struct.unpack_from("<16sHHIQQQIHHHHHH", data)
    section_index = elf.names.index(relocation_section)
    struct.pack_into(
        "<Q",
        data,
        header[6] + section_index * header[11] + 24,
        offset,
    )
    struct.pack_into(
        "<Q",
        data,
        header[6] + section_index * header[11] + 32,
        len(extra) * 24,
    )


assert BASE.is_file()
subprocess.run(
    [
        TC + "ld",
        "-r",
        str(BASE),
        str(REPAIRS / "parser-repair.o"),
        str(REPAIRS / "action-repair.o"),
        str(REPAIRS / "pmksa-repair.o"),
        "-o",
        str(MERGED),
    ],
    check=True,
)

old = Elf(BASE)
new = Elf(MERGED)
old_functions = old.functions()
new_functions = new.functions()
assert all(old.symbytes(symbol) == new.symbytes(new_functions[name])
           for name, symbol in old_functions.items())
assert old.bytes(old.names.index(".modinfo")) == new.bytes(new.names.index(".modinfo"))
new_imports = {
    symbol["name"]
    for symbol in new.symbols
    if symbol["section"] == 0
} - {
    symbol["name"]
    for symbol in old.symbols
    if symbol["section"] == 0
}
assert not new_imports, f"Unexpected kernel imports: {sorted(new_imports)}"

targets = {
    "defrag_multi_link_per_sta_profile":
        "lg6_defrag_multi_link_per_sta_profile",
    "parse_multi_link_per_sta_profile":
        "lg6_parse_multi_link_per_sta_profile",
    "parse_multi_link_ie": "lg6_parse_multi_link_ie",
    "PeerProtectEHTAction": "lg6_PeerProtectEHTAction",
    "CFG80211_OpsPmksaSet": "lg9_pmksa_set",
    "CFG80211_OpsPmksaFlush": "lg9_pmksa_flush",
    "get_rate_flag": "lg9_get_rate_flag",
}

text_index = new.names.index(".text")
rela_index = new.names.index(".rela.text")
relocation_bytes = new.bytes(rela_index)
relocations = [
    list(struct.unpack_from("<QQq", relocation_bytes, offset))
    for offset in range(0, len(relocation_bytes), 24)
]
data = bytearray(new.data)
entry_changes = []
for original_name, replacement_name in targets.items():
    original = new_functions[original_name]
    replacement = new_functions[replacement_name]
    assert original["section"] == text_index
    assert original["value"] % 4 == 0
    offset = original["value"]
    assert not any(offset <= relocation[0] < offset + 4 for relocation in relocations)
    file_offset = new.sh[text_index][4] + offset
    before = bytes(data[file_offset:file_offset + 4])
    struct.pack_into("<I", data, file_offset, 0x14000000)
    relocations.append([offset, (replacement["index"] << 32) | 282, 0])
    entry_changes.append({
        "original_symbol": original_name,
        "replacement_symbol": replacement_name,
        "entry_offset": offset,
        "instruction_before": before.hex(),
        "instruction_after": "00000014",
        "relocation": "R_AARCH64_JUMP26",
    })

old_relocations = old.bytes(old.names.index(".rela.text"))
wiphy_offsets = []
for offset in range(0, len(old_relocations), 24):
    relocation_offset, info, _ = struct.unpack_from("<QQq", old_relocations, offset)
    if old.symbols[info >> 32]["name"] == "wiphy_register":
        wiphy_offsets.append(relocation_offset)
assert len(wiphy_offsets) == 1, wiphy_offsets
wrapper = new_functions["lg9_wiphy_register"]
for offset in wiphy_offsets:
    matches = [relocation for relocation in relocations if relocation[0] == offset]
    assert len(matches) == 1
    relocation = matches[0]
    assert new.symbols[relocation[1] >> 32]["name"] == "wiphy_register"
    assert relocation[1] & 0xFFFFFFFF == 283
    relocation[1] = (wrapper["index"] << 32) | 283

add_relocations(data, new, ".rela.text", relocations)
REPAIRED.write_bytes(data)

final = Elf(REPAIRED)
final_functions = final.functions()
untouched = []
for name, symbol in old_functions.items():
    before = old.symbytes(symbol)
    after = final.symbytes(final_functions[name])
    if name in targets:
        assert before[4:] == after[4:]
        assert after[:4] == struct.pack("<I", 0x14000000)
    else:
        assert before == after, f"Unexpected function modification: {name}"
        untouched.append(name)

radar = final_functions["nl80211_send_event_radar_notify"]
radar_offset = final.sh[radar["section"]][4] + radar["value"] + 0x54
radar_before = bytes(data[radar_offset:radar_offset + 4])
assert radar_before == bytes.fromhex("e503132a"), radar_before.hex()
data[radar_offset:radar_offset + 4] = bytes.fromhex("65060051")
REPAIRED.write_bytes(data)

subprocess.run(
    [TC + "objcopy", "--strip-debug", str(REPAIRED), str(FINAL)],
    check=True,
)

result = {
    "method": (
        "clean full vendor rebuild; three bounded parser/action objects, "
        "AP EXTERNAL_AUTH PMKSA/rate objects, seven AArch64 entry redirections, "
        "one wiphy_register call relocation and the radar array-index correction"
    ),
    "source_sha256": digest(BASE),
    "output_sha256": digest(FINAL),
    "output_bytes": FINAL.stat().st_size,
    "patched_entries": entry_changes,
    "wiphy_registration_call_offsets": wiphy_offsets,
    "radar_patch": {
        "symbol": "nl80211_send_event_radar_notify",
        "file_offset": radar_offset,
        "instruction_before": radar_before.hex(),
        "instruction_after": "65060051",
    },
    "untouched_original_functions": len(untouched),
    "new_kernel_imports": [],
    "emlsr_emlmr_t2l_disabled": False,
    "pmkid_validation_bypassed": False,
    "hardware_load_tested": False,
}
(WORK / "driver-repair.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result, indent=2))
