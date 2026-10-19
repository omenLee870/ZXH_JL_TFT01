# Third-party components

This application uses dynamically linked third-party libraries. The executable
folder includes package metadata and available upstream license files under
`_internal`. Keep these notices with copies of the application.

* Qt for Python / PySide6 Essentials 6.12.0 and Shiboken6 6.12.0:
  LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only (per installed package metadata).
  Project and source: https://code.qt.io/cgit/pyside/pyside-setup.git/
  Licensing: https://doc.qt.io/qtforpython-6/licenses.html
* Qt 6 libraries: see https://www.qt.io/licensing/open-source-lgpl-obligations
  and the corresponding module license information. Source:
  https://code.qt.io/cgit/qt/qtbase.git/
* pySerial 3.5: BSD license. Source and license:
  https://github.com/pyserial/pyserial/tree/v3.5
* Python 3.13 runtime: Python Software Foundation License.
  https://docs.python.org/3/license.html
* PyInstaller 6.22.3 bootloader: GPL with an exception permitting bundling of
  applications under their own license.
  https://pyinstaller.org/en/stable/license.html

Qt libraries remain in separate DLL files and can be replaced with compatible
builds. The application source is in `desktop/vehicle_simulator` in the project;
build instructions are in README.md.
