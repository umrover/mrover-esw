#!/usr/bin/env python3
import sys
import os
import urllib.request
import subprocess
import pathlib

venv_dir = sys.argv[1]
pyproject_toml = sys.argv[2]
venv_pip = sys.argv[3]

pyvenv_cfg = os.path.join(venv_dir, "pyvenv.cfg")
venv_exists = os.path.exists(pyvenv_cfg)
timestamp_file = os.path.join(venv_dir, "package.timestamp")

if venv_exists and os.path.exists(timestamp_file):
    if os.path.getmtime(timestamp_file) >= os.path.getmtime(pyproject_toml):
        print("STATUS: venv is already up-to-date (cached)")
        sys.exit(0)

has_internet = False
try:
    urllib.request.urlopen("https://pypi.org", timeout=3)
    has_internet = True
except Exception:
    pass

if has_internet:
    try:
        if not venv_exists:
            subprocess.run([sys.executable, "-m", "venv", venv_dir], check=True, capture_output=True)

        subprocess.run(
            [venv_pip, "install", "--upgrade", "pip", "setuptools", "wheel"], check=True, capture_output=True
        )
        subprocess.run(
            [venv_pip, "install", "--editable", ".[dev]"],
            cwd=os.path.dirname(pyproject_toml),
            check=True,
            capture_output=True,
        )

        pathlib.Path(timestamp_file).touch()

        print("STATUS: env successfully synced with internet")
        sys.exit(0)
    except subprocess.CalledProcessError as e:
        print(f"FATAL: failed to install packages\n{e.stderr.decode('utf-8')}")
        sys.exit(1)
else:
    if not venv_exists:
        print(f"FATAL: venv does not exist at {venv_dir} and cannot be created offline")
        sys.exit(1)

    try:
        result = subprocess.run([venv_pip, "list", "--outdated"], capture_output=True, text=True, timeout=4)
        pathlib.Path(timestamp_file).touch()

        if result.returncode == 0 and result.stdout.strip():
            print(f"WARNING: offline mode, packages out of date:\n{result.stdout}")
        else:
            print("STATUS: offline mode, env matches local state")
        sys.exit(0)
    except Exception:
        pathlib.Path(timestamp_file).touch()
        print("WARNING: entirely offline, skipping version comparison checks")
        sys.exit(0)
