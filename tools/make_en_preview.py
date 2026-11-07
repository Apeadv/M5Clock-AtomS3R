from PIL import Image, ImageDraw, ImageFont

W = H = 128
img = Image.new("RGB", (W, H), (8, 16, 24))
d = ImageDraw.Draw(img)
SM = ImageFont.truetype(r"C:\Windows\Fonts\consola.ttf", 8)
MD = ImageFont.truetype(r"C:\Windows\Fonts\consola.ttf", 16)
BIG = None
for size in range(34, 20, -1):
    f = ImageFont.truetype(r"C:\Windows\Fonts\consola.ttf", size)
    if d.textlength("12:34:56", font=f) <= 120:
        BIG = f
        break

CYAN = (35, 213, 230)
AMBER = (255, 181, 71)
WHITE = (234, 246, 255)
GREY = (132, 160, 176)
DIM = (74, 89, 100)
GREEN = (53, 224, 138)
PANEL = (16, 32, 42)

# Title: WORLD TIME, split like the firmware's Font2 layout.
d.text((6, 1), "WORLD", font=MD, fill=CYAN)
d.text((52, 1), "TIME", font=MD, fill=CYAN)
d.text((87, 5), "UTC", font=SM, fill=AMBER)
for i in range(4):
    h = 3 + i * 2
    d.rectangle([106 + i * 4, 13 - h, 107 + i * 4, 13], fill=GREEN)

d.text((6, 19), "2026-11-07", font=SM, fill=GREY)
d.text((70, 19), "SAT", font=SM, fill=GREY)
d.text((112, 19), "M3", font=SM, fill=GREEN)

full = "12:34:56"
tw = d.textlength(full, font=BIG)
x = int((128 - tw) / 2)
d.text((x, 31), "12:34", font=BIG, fill=WHITE)
hmw = d.textlength("12:34", font=BIG)
d.text((x + hmw, 31), ":", font=BIG, fill=AMBER)
cw = d.textlength(":", font=BIG)
d.text((x + hmw + cw, 31), "56", font=BIG, fill=AMBER)
d.line([8, 63, 120, 63], fill=(41, 69, 85))

rows = [
    ("BEIJING", "+8", "12:34", AMBER),
    ("LONDON", "+0", "04:34", CYAN),
    ("NEW YORK", "-5", "23:34", CYAN),
]
for i, (name, off, tm, accent) in enumerate(rows):
    y = 64 + i * 20
    if i % 2 == 0:
        d.rectangle([0, y, 127, y + 18], fill=PANEL)
    d.rectangle([0, y + 2, 1, y + 16], fill=accent)
    d.text((6, y + 3), name, font=SM, fill=WHITE)
    d.text((6, y + 11), off, font=SM, fill=GREY)
    tw2 = d.textlength(tm, font=MD)
    d.text((122 - tw2, y + 2), tm, font=MD, fill=WHITE)

d.ellipse([60, 125, 62, 127], fill=CYAN)
d.ellipse([66, 125, 68, 127], fill=DIM)
img.resize((512, 512), Image.Resampling.NEAREST).save("dist/preview-s3r-en.png")
print("ok")
