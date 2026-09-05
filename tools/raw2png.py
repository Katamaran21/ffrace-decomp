"""Convert an ffrace --dump raw RGB565 frame (240x320, little endian) to PNG."""
import sys

from PIL import Image

W, H = 240, 320


def convert(src, dst):
    with open(src, "rb") as fp:
        blob = fp.read()
    if len(blob) != W * H * 2:
        raise SystemExit("%s: expected %d bytes, got %d" % (src, W * H * 2, len(blob)))
    img = Image.new("RGB", (W, H))
    px = img.load()
    for y in range(H):
        row = y * W * 2
        for x in range(W):
            v = blob[row + x * 2] | (blob[row + x * 2 + 1] << 8)
            r = (v >> 11) & 0x1F
            g = (v >> 5) & 0x3F
            b = v & 0x1F
            px[x, y] = (r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2)
    img.save(dst)


if __name__ == "__main__":
    for path in sys.argv[1:]:
        convert(path, path.rsplit(".", 1)[0] + ".png")
        print(path.rsplit(".", 1)[0] + ".png")
