from PIL import Image

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

def render(hh, mm, ss, width=24):
    img = Image.new("1", (width, 8), 0)
    def put(x, y):
        if 0 <= x < width and 0 <= y < 8:
            img.putpixel((x, y), 1)
    def digit(x, d):
        for row in range(7):
            bits = DIGITS[d][row]
            for col in range(3):
                if bits & (0b100 >> col):
                    put(x + col, row)
    def colon(x):
        put(x, 2); put(x, 5)
    if width >= 24:
        digit(0, hh // 10); digit(4, hh % 10); colon(7)
        digit(8, mm // 10); digit(12, mm % 10); colon(15)
        digit(16, ss // 10); digit(20, ss % 10)
    else:
        digit(0, hh // 10); digit(4, hh % 10); colon(7)
        digit(8, mm // 10); digit(12, mm % 10)
    return img

for t in [(12, 34, 56), (00, 00, 00), (23, 59, 59)]:
    img = render(*t)
    img.resize((img.width * 16, img.height * 16), Image.Resampling.NEAREST).save(f"mono_{t[0]:02d}{t[1]:02d}{t[2]:02d}.png")
print("ok")
