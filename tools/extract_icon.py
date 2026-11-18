"""Pull the largest icon image out of the original Bejeweled.exe and save it as a PNG.

Usage: python extract_icon.py <Bejeweled.exe> <out.png>
The 1.87 exe carries a 256x256 PNG icon (RT_ICON entries that start with the PNG signature).
"""
import sys

import pefile

RT_ICON = 3


def main(exe: str, out: str) -> None:
    pe = pefile.PE(exe, fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_RESOURCE"]])
    best = None
    for entry in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        if entry.id != RT_ICON:
            continue
        for icon in entry.directory.entries:
            for lang in icon.directory.entries:
                data = pe.get_data(lang.data.struct.OffsetToData, lang.data.struct.Size)
                if data.startswith(b"\x89PNG") and (best is None or len(data) > len(best)):
                    best = data
    if best is None:
        sys.exit("no PNG icon found")
    with open(out, "wb") as f:
        f.write(best)
    print(f"{out}: {len(best)} bytes")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
