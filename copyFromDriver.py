import os
import shutil
from contextlib import suppress
from pathlib import Path

with suppress(FileNotFoundError):
    os.remove(Path("include/packet.h"))
    print("Removed include/packet.h")
print(f'Copied: {shutil.copy(Path("driver/serial/packet.h"), Path("include/packet.h"))}')
with suppress(FileNotFoundError):
    os.remove(Path("include/converters.h"))
    print("Removed include/converters.h")
print(f'Copied: {shutil.copy(Path("driver/serial/converters.h"), Path("include/converters.h"))}')

with suppress(FileNotFoundError):
    os.remove(Path("src/packet.cpp"))
    print("Removed src/packet.cpp")
print(f'Copied: {shutil.copy(Path("driver/serial/packet.cpp"), Path("src/packet.cpp"))}')
