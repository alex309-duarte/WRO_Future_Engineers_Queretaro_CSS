#!/usr/bin/env python3
"""Linux replica of build_gcc.ps1 (Hiwonder RosRobotControllerM4 -> GCC).

Reproduces the same source list, the same order and the same flags as the
original PowerShell script, so that the images are comparable.

Usage:
  build_gcc.py <RosRobotControllerM4-armclang-tree> <linker.ld> <output> [--order-only]
"""
import os, re, sys, subprocess, hashlib
import xml.etree.ElementTree as ET

def win_rel(project_root, abs_path):
    """Path relative to projectRoot with Windows separators, so it sorts the same way."""
    return os.path.relpath(abs_path, project_root).replace('/', '\\')

def collect(project_root):
    uv = os.path.join(project_root, 'MDK-ARM', 'RosRobotControllerM4.uvprojx')
    tree = ET.parse(uv)
    root = tree.getroot()

    # --- include dirs ---
    inc_text = None
    for vc in root.iter('VariousControls'):
        ip = vc.find('IncludePath')
        if ip is not None and ip.text:
            inc_text = ip.text
            break
    includes = []
    for entry in (inc_text or '').split(';'):
        entry = entry.strip()
        if not entry:
            continue
        rel = entry.replace('\\', '/')
        if rel.startswith('../'):
            cand = os.path.join(project_root, rel[3:])
        else:
            cand = os.path.join(project_root, 'MDK-ARM', rel)
        cand = cand.replace('portable/RVDS/ARM_CM4F', 'portable/GCC/ARM_CM4F')
        if os.path.isdir(cand):
            includes.append(os.path.realpath(cand))
    for extra in ('Drivers/CMSIS/Core/Include', 'Drivers/CMSIS/Device/ST/STM32F4xx/Include'):
        cand = os.path.join(project_root, extra)
        if os.path.isdir(cand):
            includes.append(os.path.realpath(cand))
    # Upper/lower-case shims: the tree comes from a Windows FS
    # (case-insensitive) and on Linux some includes do not resolve.
    for extra in filter(None, os.environ.get('EXTRA_INC', '').split(':')):
        if os.path.isdir(extra):
            includes.append(os.path.realpath(extra))
    includes = sorted(set(includes), key=str.lower)

    # --- sources ---
    srcs = []
    for node in root.iter('FilePath'):
        entry = (node.text or '').replace('\\', '/')
        if os.path.splitext(entry)[1].lower() != '.c':
            continue
        if entry.endswith('Hiwonder/Misc/syscall.c'):      # ps1 line 63
            continue
        if entry.startswith('../'):
            cand = os.path.join(project_root, entry[3:])
        else:
            cand = os.path.join(project_root, 'MDK-ARM', entry)
        cand = cand.replace('portable/RVDS/ARM_CM4F/port.c', 'portable/GCC/ARM_CM4F/port.c')
        if not os.path.exists(cand):
            raise SystemExit(f"Source not found: {entry} -> {cand}")
        srcs.append(os.path.realpath(cand))
    # PowerShell's Sort-Object -Unique: on the absolute path, case-insensitive.
    srcs = sorted(set(srcs), key=lambda p: win_rel(project_root, p).lower())
    srcs.append(os.path.realpath(os.path.join(
        project_root, 'Core', 'Startup', 'startup_stm32f407vetx.s')))
    return includes, srcs

def reorder(srcs, order_dir):
    """Imposes the canonical order of the reference build (obj/NNN_name.o).

    PowerShell's Sort-Object is culture-aware and cannot be reproduced
    reliably in Python; the Windows build already fixed the order in the
    numbered names, so it is reused as is.
    """
    want = []
    for n in sorted(os.listdir(order_dir)):
        m = re.match(r'^(\d{3})_(.+)\.o$', n)
        if m:
            want.append((int(m.group(1)), m.group(2)))
    want.sort()
    by_stem = {}
    for s in srcs:
        by_stem.setdefault(os.path.splitext(os.path.basename(s))[0], []).append(s)
    out, used = [], set()
    for _, stem in want:
        cands = [c for c in by_stem.get(stem, []) if c not in used]
        if not cands:
            raise SystemExit(f"Reference order requires '{stem}' and it is not in the tree")
        out.append(cands[0]); used.add(cands[0])
    missing = [s for s in srcs if s not in used]
    if missing:
        raise SystemExit("Sources with no place in the reference order: " +
                         ", ".join(os.path.basename(m) for m in missing))
    return out

COMMON = ['-mcpu=cortex-m4', '-mthumb', '-mfpu=fpv4-sp-d16', '-mfloat-abi=hard',
          '-DUSE_HAL_DRIVER', '-DSTM32F407xx', '-DLV_CONF_INCLUDE_SIMPLE',
          '-DLV_LVGL_H_INCLUDE_SIMPLE', '-ffunction-sections', '-fdata-sections',
          '-fcommon', '-fno-strict-aliasing', '-Os', '-g3', '-Wall', '-Wextra',
          '-Wno-unused-parameter', '-Wno-sign-compare']

def main():
    project_root = os.path.realpath(sys.argv[1])
    linker = os.path.realpath(sys.argv[2])
    out_root = os.path.realpath(sys.argv[3])
    order_only = '--order-only' in sys.argv
    order_dir = None
    if '--order-from' in sys.argv:
        order_dir = os.path.realpath(sys.argv[sys.argv.index('--order-from') + 1])

    includes, srcs = collect(project_root)
    if order_dir:
        srcs = reorder(srcs, order_dir)
    obj_root = os.path.join(out_root, 'obj')
    os.makedirs(obj_root, exist_ok=True)

    names = []
    for i, s in enumerate(srcs, 1):
        names.append(f"{i:03d}_{os.path.splitext(os.path.basename(s))[0]}.o")
    if order_only:
        for n in names:
            print(n)
        return

    gcc = os.environ.get('CC', 'arm-none-eabi-gcc')
    objcopy = os.environ.get('OBJCOPY', 'arm-none-eabi-objcopy')
    size = os.environ.get('SIZE', 'arm-none-eabi-size')
    cflags = COMMON + [f'-I{d}' for d in includes]

    objects = []
    for i, (s, n) in enumerate(zip(srcs, names), 1):
        obj = os.path.join(obj_root, n)
        lang = ['-std=gnu11'] if s.lower().endswith('.c') else ['-x', 'assembler-with-cpp']
        print(f"[{i}/{len(srcs)}] {os.path.basename(s)}", flush=True)
        r = subprocess.run([gcc] + cflags + lang + ['-c', s, '-o', obj],
                           capture_output=True, text=True)
        if r.returncode != 0:
            sys.stderr.write(r.stdout + r.stderr)
            raise SystemExit(f"Compilation failed: {s}")
        objects.append(obj)

    stem = os.path.join(out_root, 'RosRobotControllerM4')
    elf, mapf, hexf, binf = stem + '.elf', stem + '.map', stem + '.hex', stem + '.bin'
    link = ['-mcpu=cortex-m4', '-mthumb', '-mfpu=fpv4-sp-d16', '-mfloat-abi=hard',
            f'-T{linker}', '-Wl,--gc-sections', f'-Wl,-Map={mapf}',
            '--specs=nano.specs', '--specs=nosys.specs', '-o', elf] + objects + \
           ['-Wl,--start-group', '-lc', '-lm', '-Wl,--end-group']
    r = subprocess.run([gcc] + link, capture_output=True, text=True)
    sys.stderr.write(r.stderr)
    if r.returncode != 0:
        raise SystemExit("Link failed")
    subprocess.run([objcopy, '-O', 'ihex', elf, hexf], check=True)
    subprocess.run([objcopy, '-O', 'binary', elf, binf], check=True)
    print(subprocess.run([size, elf], capture_output=True, text=True).stdout)
    for f in (hexf, binf):
        h = hashlib.sha256(open(f, 'rb').read()).hexdigest()
        print(f"SHA256  {h}  {os.path.basename(f)}")

if __name__ == '__main__':
    main()
