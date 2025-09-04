#!/usr/bin/env python3
import argparse, json, os, shlex, subprocess, sys, pathlib, hashlib
from concurrent.futures import ThreadPoolExecutor, as_completed

def load_db(path):
    with open(path, "r") as f:
        return json.load(f)

def ensure_dir(p):
    pathlib.Path(p).mkdir(parents=True, exist_ok=True)

def find_token(args, pred):
    for i, a in enumerate(args):
        if pred(a): return i
    return -1

def strip_ccache(args):
    return args[1:] if (args and "ccache" in args[0]) else args

def to_list_command(entry):
    # Prefer "arguments" if present (already split), else split "command"
    if "arguments" in entry and entry["arguments"]:
        return list(entry["arguments"])
    return shlex.split(entry["command"])

def derive_ptx_path(entry, ptx_root):
    """
    Use the entry's recorded output path to derive a unique PTX path:
      <ptx_root>/<relative(object_output) with .o -> .ptx>
    This avoids basename collisions.
    """
    cwd = entry.get("directory", os.getcwd())
    obj_out = entry.get("output")
    if not obj_out:
        # Fallback: ptx/<basename>.ptx
        base = os.path.splitext(os.path.basename(entry["file"]))[0] + ".ptx"
        return os.path.join(cwd, ptx_root, base)

    # Normalize: object output is often relative to cwd
    if os.path.isabs(obj_out):
        rel = os.path.relpath(obj_out, start=cwd)
    else:
        rel = obj_out

    rel_noext = os.path.splitext(rel)[0]
    return os.path.join(cwd, ptx_root, rel_noext + ".ptx")

def transform_to_ptx(args, out_path):
    args = list(args)
    # replace -c with -ptx (or append if missing)
    i = find_token(args, lambda a: a == "-c")
    if i != -1:
        args[i] = "-ptx"
    else:
        if "-ptx" not in args:
            args.append("-ptx")
    # rewrite -o
    i = find_token(args, lambda a: a == "-o")
    if i != -1 and i + 1 < len(args):
        args[i+1] = out_path
    else:
        i = find_token(args, lambda a: a.startswith("-o"))
        if i != -1:
            args[i] = "-o" + out_path
        else:
            args.extend(["-o", out_path])
    # stability: drop -keep / --keep-dir
    args = [a for a in args if a != "-keep" and not a.startswith("--keep-dir")]
    return args

def build_task(entry, ptx_root):
    cwd = entry.get("directory", os.getcwd())
    src = entry.get("file", "")
    if not src.endswith(".cu"):
        return None  # skip non-CUDA
    args = strip_ccache(to_list_command(entry))
    if not args or "nvcc" not in args[0]:
        return None  # skip non-nvcc entries
    out_path = derive_ptx_path(entry, ptx_root)
    ensure_dir(os.path.dirname(out_path))
    cmd = transform_to_ptx(args, out_path)
    return cwd, cmd, out_path, src

def uniquify(path, seen):
    if path not in seen:
        seen.add(path)
        return path, None  # no collision
    stem, ext = os.path.splitext(path)
    i = 2
    while True:
        alt = f"{stem}-{i}{ext}"
        if alt not in seen:
            seen.add(alt)
            return alt, path  # collision, original -> alt
        i += 1

def run(cmd, cwd):
    p = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    return p.returncode, p.stdout

def main():
    ap = argparse.ArgumentParser(description="Regenerate PTX via nvcc from compile_commands.json")
    ap.add_argument("--db", default="cmake-build-local/compile_commands.json", help="Path to compile_commands.json")
    ap.add_argument("--ptx-root", default="ptx_out", help="Root folder (under each entry's directory) for PTX outputs")
    ap.add_argument("-j", "--jobs", type=int, default=8, help="Parallel jobs (default: 8)")
    ap.add_argument("--dry-run", action="store_true", help="Print commands only, do not execute")
    args = ap.parse_args()

    db = load_db(args.db)
    planned = []
    for entry in db:
        t = build_task(entry, args.ptx_root)
        if t:
            planned.append((entry, *t))  # (entry, cwd, cmd, out_path, src)

    if not planned:
        print("No CUDA nvcc entries found.", file=sys.stderr)
        return 2

    # Prevent overwrites by uniquifying output paths
    seen = set()
    collisions = []
    deduped = []
    for entry, cwd, cmd, out_path, src in planned:
        uniq_path, orig = uniquify(out_path, seen)
        if orig:
            # Patch the command's -o to the new unique path
            i = find_token(cmd, lambda a: a == "-o")
            if i != -1 and i + 1 < len(cmd):
                cmd[i+1] = uniq_path
            else:
                oi = find_token(cmd, lambda a: a.startswith("-o"))
                if oi != -1:
                    cmd[oi] = "-o" + uniq_path
                else:
                    cmd.extend(["-o", uniq_path])
            collisions.append((orig, uniq_path, src))
        deduped.append((cwd, cmd, uniq_path, src))

    print(f"Planned: {len(planned)}  |  After de-dupe: {len(deduped)}  |  Collisions resolved: {len(collisions)}")
    if collisions:
        print("Examples of collisions (orig -> unique):")
        for a, b, s in collisions[:5]:
            print(f"  {a} -> {b}   ({os.path.basename(s)})")

    if args.dry_run:
        for cwd, cmd, out_path, src in deduped:
            print("CWD:", cwd)
            print("CMD:", " ".join(shlex.quote(c) for c in cmd))
        return 0

    failures = 0
    ok = 0
    with ThreadPoolExecutor(max_workers=args.jobs) as ex:
        futs = {ex.submit(run, cmd, cwd): (cwd, cmd, out, src) for cwd, cmd, out, src in deduped}
        for fut in as_completed(futs):
            cwd, cmd, out, src = futs[fut]
            rc, outtxt = fut.result()
            if rc != 0 or (not os.path.exists(out)):
                failures += 1
                print("\n=== FAILED ===")
                print("CWD:", cwd)
                print("CMD:", " ".join(shlex.quote(c) for c in cmd))
                print(outtxt)
            else:
                ok += 1
                print(f"[OK] {out}")
    skipped = len(planned) - len(deduped)

    print(f"\nSummary:")
    print(f"  planned:   {len(planned)}")
    print(f"  unique:    {len(deduped)}")
    print(f"  ok:        {ok}")
    print(f"  failed:    {failures}")
    print(f"  skipped:   {skipped}")
    print(f"  collisions:{len(collisions)}")

    # Return nonzero if any failures
    return 1 if failures else 0

if __name__ == "__main__":
    sys.exit(main())
