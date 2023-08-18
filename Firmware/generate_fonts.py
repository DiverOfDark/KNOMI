from os.path import isfile, join, splitext
from os import makedirs, listdir, remove
import shutil

Import("env")

usedFont = "Montserrat-Regular.ttf"

# You must have nodejs/npm installed
env.Execute(f"npx lv_font_conv --no-compress --no-prefilter --bpp 2 --size 20 --font fonts/{usedFont} -r 0x20-0x7F,0xB0,0x2022 --format lvgl -o src/generated/font_20.c --force-fast-kern-format")
env.Execute(f"npx lv_font_conv --no-compress --no-prefilter --bpp 2 --size 24 --font fonts/{usedFont} -r 0x20-0x7F,0xB0,0x2022 --format lvgl -o src/generated/font_24.c --force-fast-kern-format")
env.Execute(f"npx lv_font_conv --no-compress --no-prefilter --bpp 2 --size 28 --font fonts/{usedFont} -r 0x20-0x7F,0xB0,0x2022 --format lvgl -o src/generated/font_28.c --force-fast-kern-format")
env.Execute(f"npx lv_font_conv --no-compress --no-prefilter --bpp 2 --size 32 --font fonts/{usedFont} -r 0x20-0x7F,0xB0,0x2022 --format lvgl -o src/generated/font_32.c --force-fast-kern-format")
env.Execute(f"npx lv_font_conv --no-compress --no-prefilter --bpp 2 --size 48 --font fonts/{usedFont} -r 0x20-0x7F,0xB0,0x2022 --format lvgl -o src/generated/font_48.c --force-fast-kern-format")