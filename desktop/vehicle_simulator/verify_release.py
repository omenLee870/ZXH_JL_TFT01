"""Start the packaged app, render its real Windows widget tree, and exit."""
import os
from pathlib import Path
import subprocess
import sys

exe=Path(sys.argv[1]).resolve()
png=Path(sys.argv[2]).resolve()
env=os.environ.copy()
env['QT_QPA_PLATFORM']='windows'
subprocess.run([str(exe),'--screenshot',str(png)],env=env,check=True,timeout=20)
if not png.is_file() or png.stat().st_size<10000:
    raise RuntimeError('Packaged application did not produce its preview')
print(f'Packaged Windows startup and rendering PASS: {png}')
