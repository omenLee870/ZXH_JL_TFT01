# -*- mode: python ; coding: utf-8 -*-
from pathlib import Path
from PyInstaller.utils.hooks import copy_metadata

source = Path(SPECPATH)
a = Analysis([str(source / 'app.py')], pathex=[str(source)],
    datas=copy_metadata('PySide6-Essentials') + copy_metadata('shiboken6') + copy_metadata('pyserial'),
    binaries=[], hiddenimports=[], hookspath=[], hooksconfig={}, runtime_hooks=[],
    excludes=[], noarchive=False, optimize=0)

# Qt 6.12 uses Windows' native ICU (unsuffixed ucnv_* exports). A different ICU
# from the build host PATH exports version-suffixed symbols and can be picked
# accidentally by the binary scanner. Use the OS implementation on Windows 10+.
a.binaries = [entry for entry in a.binaries
              if Path(entry[0]).name.lower() not in ('icuuc.dll', 'icuin.dll', 'icu.dll', 'icudt78.dll')]
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, [], exclude_binaries=True, name='ZXH-Vehicle-Lab',
          debug=False, bootloader_ignore_signals=False, strip=False, upx=False,
          console=False, disable_windowed_traceback=False)
coll = COLLECT(exe, a.binaries, a.datas, strip=False, upx=False, name='ZXH-Vehicle-Lab')
