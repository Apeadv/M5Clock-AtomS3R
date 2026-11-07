from PIL import Image, ImageDraw, ImageFont

# --- S3R preview -----------------------------------------------------------
W = H = 128
img = Image.new("RGB", (W, H), (8, 16, 24))
d = ImageDraw.Draw(img)
CN = ImageFont.truetype(r"C:\Windows\Fonts\msyhbd.ttc", 16, index=0)
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

d.text((6, 1), "世界时间", font=CN, fill=CYAN)
d.text((78, 5), "UTC", font=SM, fill=AMBER)
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
    ("北京", "+8", "12:34", AMBER),
    ("伦敦", "+0", "04:34", CYAN),
    ("纽约", "-5", "23:34", CYAN),
]
for i, (name, off, tm, accent) in enumerate(rows):
    y = 64 + i * 20
    if i % 2 == 0:
        d.rectangle([0, y, 127, y + 18], fill=PANEL)
    d.rectangle([0, y + 2, 1, y + 16], fill=accent)
    d.text((6, y + 2), name, font=CN, fill=WHITE)
    nw = d.textlength(name, font=CN)
    d.text((6 + nw + 4, y + 6), off, font=SM, fill=GREY)
    tw2 = d.textlength(tm, font=MD)
    d.text((122 - tw2, y + 2), tm, font=MD, fill=WHITE)

d.ellipse([60, 125, 62, 127], fill=CYAN)
d.ellipse([66, 125, 68, 127], fill=DIM)
img.resize((512, 512), Image.Resampling.NEAREST).save("dist/preview-s3r.png")

# --- Chain Mono preview ----------------------------------------------------
DIGITS = [
    [0b111,0b101,0b101,0b101,0b101,0b101,0b111],
    [0b010,0b110,0b010,0b010,0b010,0b010,0b111],
    [0b111,0b001,0b001,0b111,0b100,0b100,0b111],
    [0b111,0b001,0b001,0b111,0b001,0b001,0b111],
    [0b101,0b101,0b101,0b111,0b001,0b001,0b001],
    [0b111,0b100,0b100,0b111,0b001,0b001,0b111],
    [0b111,0b100,0b100,0b111,0b101,0b101,0b111],
    [0b111,0b001,0b001,0b010,0b010,0b010,0b010],
    [0b111,0b101,0b101,0b111,0b101,0b101,0b111],
    [0b111,0b101,0b101,0b111,0b001,0b001,0b111],
]
frame = [[0] * 24 for _ in range(8)]
def put(x, y):
    if 0 <= x < 24 and 0 <= y < 8:
        frame[y][x] = 1
def digit(x, n):
    for row in range(7):
        bits = DIGITS[n][row]
        for col in range(3):
            if bits & (0b100 >> col):
                put(x + col, row)
def colon(x):
    put(x, 2); put(x, 5)
digit(0, 1); digit(4, 2); colon(7)
digit(8, 3); digit(12, 4); colon(15)
digit(16, 5); digit(20, 6)

cell = 16
gap = 8
margin = 12
w = margin * 2 + 3 * 8 * cell + 2 * gap
h = margin * 2 + 8 * cell
im = Image.new("RGB", (w, h), (16, 16, 16))
dd = ImageDraw.Draw(im)
LED_ON = (255, 46, 38)
LED_OFF = (38, 10, 10)
for m in range(3):
    ox = margin + m * (8 * cell + gap)
    for y in range(8):
        for x in range(8):
            cx = ox + x * cell
            cy = margin + y * cell
            col = LED_ON if frame[y][m * 8 + x] else LED_OFF
            dd.ellipse([cx + 2, cy + 2, cx + cell - 2, cy + cell - 2], fill=col)
im.save("dist/preview-chain-mono.png")
print("ok", BIG.size, tw)
