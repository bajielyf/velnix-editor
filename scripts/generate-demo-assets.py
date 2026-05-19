#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "docs" / "assets" / "main-window.png"
LOGO = ROOT / "docs" / "assets" / "logo-app.png"
OUT = ROOT / "docs" / "assets" / "demo"

FONT_REGULAR = [
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/System/Library/Fonts/HelveticaNeue.ttc",
    "/System/Library/Fonts/SFNS.ttf",
]
FONT_MEDIUM = [
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Medium.ttc",
    "/System/Library/Fonts/HelveticaNeue.ttc",
    "/System/Library/Fonts/SFNS.ttf",
]
FONT_BOLD = [
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc",
    "/System/Library/Fonts/HelveticaNeue.ttc",
    "/System/Library/Fonts/SFNS.ttf",
]
FONT_MONO = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/System/Library/Fonts/Menlo.ttc",
    "/System/Library/Fonts/SFNSMono.ttf",
]


def font(paths: list[str], size: int) -> ImageFont.FreeTypeFont:
    for path in paths:
        if Path(path).exists():
            return ImageFont.truetype(path, size)
    return ImageFont.load_default(size=size)


F_TITLE = font(FONT_BOLD, 42)
F_SUB = font(FONT_REGULAR, 22)
F_BODY = font(FONT_REGULAR, 21)
F_MONO = font(FONT_MONO, 21)
F_BADGE = font(FONT_MEDIUM, 18)
F_KEY = font(FONT_MONO, 23)


def base_frame() -> Image.Image:
    image = Image.open(BASE).convert("RGBA")
    return image.resize((1280, 954), Image.Resampling.LANCZOS)


def overlay_tint(image: Image.Image, opacity: int = 82) -> Image.Image:
    layer = Image.new("RGBA", image.size, (14, 24, 34, opacity))
    return Image.alpha_composite(image, layer)


def rounded(draw: ImageDraw.ImageDraw, box, fill, outline=None, width=1, radius=18):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def text_box(image: Image.Image, title: str, subtitle: str, accent=(48, 120, 214)):
    draw = ImageDraw.Draw(image)
    x, y, w, h = 54, 696, 650, 188
    rounded(draw, (x, y, x + w, y + h), (255, 255, 255, 242), radius=20)
    draw.rectangle((x, y, x + 9, y + h), fill=accent)
    draw.text((x + 34, y + 28), title, fill=(22, 28, 36), font=F_TITLE)
    draw.text((x + 36, y + 92), subtitle, fill=(70, 80, 92), font=F_SUB)


def badge(image: Image.Image, text: str, pos=(948, 72), fill=(36, 93, 180)):
    draw = ImageDraw.Draw(image)
    x, y = pos
    bbox = draw.textbbox((0, 0), text, font=F_BADGE)
    w = bbox[2] - bbox[0] + 34
    h = 38
    rounded(draw, (x, y, x + w, y + h), (*fill, 238), radius=18)
    draw.text((x + 17, y + 7), text, fill=(255, 255, 255), font=F_BADGE)


def spotlight(image: Image.Image, box, color=(48, 120, 214), alpha=96):
    draw = ImageDraw.Draw(image)
    x1, y1, x2, y2 = box
    rounded(draw, (x1, y1, x2, y2), (*color, 34), outline=(*color, alpha + 80), width=4, radius=10)


def cursor(image: Image.Image, x: int, y: int):
    draw = ImageDraw.Draw(image)
    points = [(x, y), (x, y + 42), (x + 12, y + 31), (x + 23, y + 55), (x + 34, y + 50), (x + 23, y + 27), (x + 40, y + 27)]
    draw.polygon(points, fill=(255, 255, 255), outline=(28, 36, 46))


def code_panel(image: Image.Image, lines: list[str], pos=(252, 190), size=(800, 438), active_line=2):
    draw = ImageDraw.Draw(image)
    x, y = pos
    w, h = size
    rounded(draw, (x, y, x + w, y + h), (252, 253, 255, 248), outline=(201, 210, 221), radius=12)
    for i, line in enumerate(lines):
        yy = y + 30 + i * 36
        if i == active_line:
            draw.rectangle((x + 14, yy - 4, x + w - 18, yy + 28), fill=(255, 247, 213))
        draw.text((x + 24, yy), f"{i + 1:>2}", fill=(142, 151, 164), font=F_MONO)
        draw.text((x + 84, yy), line, fill=(38, 50, 64), font=F_MONO)


def save_gif(name: str, frames: list[Image.Image], duration=230):
    path = OUT / f"{name}.gif"
    frames[0].save(
        path,
        save_all=True,
        append_images=frames[1:],
        duration=duration,
        loop=0,
        disposal=2,
        optimize=True,
    )


def save_cover(name: str, frame: Image.Image):
    frame.save(OUT / f"{name}.png")


def scene_startup():
    frames = []
    logo = Image.open(LOGO).convert("RGBA").resize((150, 150), Image.Resampling.LANCZOS)
    for i in range(9):
        scale = 0.72 + i * 0.035
        bg = Image.new("RGBA", (1280, 954), (245, 247, 250, 255))
        draw = ImageDraw.Draw(bg)
        draw.text((520, 602), "Velnix Editor", fill=(30, 39, 52), font=F_TITLE)
        draw.text((470, 660), "Fast launch, ready to edit", fill=(82, 93, 106), font=F_SUB)
        size = int(150 * scale)
        icon = logo.resize((size, size), Image.Resampling.LANCZOS)
        bg.alpha_composite(icon, (640 - size // 2, 418 - size // 2))
        frames.append(bg)
    final = overlay_tint(base_frame(), 18)
    text_box(final, "Startup Speed", "From icon to editor window with a lightweight native feel")
    badge(final, "Launch")
    frames.extend([final] * 5)
    save_cover("startup-speed", final)
    save_gif("startup-speed", frames, 150)


def scene_open_file():
    frames = []
    for i in range(8):
        im = overlay_tint(base_frame(), 38)
        spotlight(im, (16, 32, 78 + i * 18, 68), alpha=95)
        if i > 2:
            draw = ImageDraw.Draw(im)
            rounded(draw, (42, 76, 286, 264), (255, 255, 255, 245), outline=(197, 206, 217), radius=8)
            for n, item in enumerate(["New", "Open...", "Recent Files", "Save", "Close"]):
                yy = 96 + n * 32
                if item == "Open...":
                    draw.rectangle((52, yy - 3, 276, yy + 26), fill=(225, 237, 255))
                draw.text((66, yy), item, fill=(38, 48, 62), font=F_BODY)
        cursor(im, 188, 116 + min(i, 4) * 10)
        frames.append(im)
    final = overlay_tint(base_frame(), 36)
    code_panel(final, ["#include <gtk/gtk.h>", "", "int main() {", "  gtk_main();", "  return 0;", "}"])
    text_box(final, "Open File", "Menus, recent files, and syntax highlighting in one flow", accent=(27, 138, 103))
    badge(final, "Open File", fill=(27, 138, 103))
    frames.extend([final] * 5)
    save_cover("open-file", final)
    save_gif("open-file", frames, 170)


def scene_editing():
    typed = ["const editor = new Velnix();", "editor.open('README.md');", "editor.focus();"]
    frames = []
    for i in range(12):
        im = overlay_tint(base_frame(), 30)
        progress = int(len(typed[0]) * min(1, i / 7))
        lines = [typed[0][:progress], typed[1] if i > 6 else "", typed[2] if i > 9 else ""]
        code_panel(im, lines, active_line=min(2, i // 5))
        draw = ImageDraw.Draw(im)
        caret_x = 340 + min(progress, 26) * 13
        draw.rectangle((caret_x, 226, caret_x + 3, 254), fill=(30, 92, 180))
        frames.append(im)
    final = frames[-1].copy()
    text_box(final, "Editing Experience", "Line numbers, active line, indentation, and clear code input", accent=(202, 89, 50))
    badge(final, "Editing", fill=(202, 89, 50))
    save_cover("editing-experience", final)
    save_gif("editing-experience", frames + [final] * 5, 145)


def scene_session_restore():
    tabs = ["README.md", "notes.txt", "src/main.cpp"]
    frames = []
    for i in range(10):
        im = overlay_tint(base_frame(), 34)
        draw = ImageDraw.Draw(im)
        rounded(draw, (228, 126, 1056, 666), (252, 253, 255, 246), outline=(199, 209, 220), radius=12)
        draw.text((264, 158), "Restoring previous session", fill=(34, 45, 58), font=F_SUB)
        for n, tab in enumerate(tabs):
            y = 222 + n * 96
            alpha = 90 + min(145, max(0, i - n * 2) * 36)
            rounded(draw, (268, y, 1014, y + 66), (244, 247, 251, alpha), outline=(205, 214, 225, alpha), radius=10)
            draw.text((302, y + 18), tab, fill=(38, 50, 64, alpha), font=F_BODY)
            if i > n * 2 + 1:
                draw.text((882, y + 18), "restored", fill=(27, 138, 103, alpha), font=F_BADGE)
        if i > 6:
            code_panel(im, ["# Velnix Editor", "", "Session state restored.", "Cursor, scroll, and tabs return."],
                       pos=(300, 474), size=(680, 160), active_line=2)
        frames.append(im)
    final = frames[-1].copy()
    text_box(final, "Session Restore", "Open tabs, unsaved text, cursor, and scroll state return on launch", accent=(82, 105, 186))
    badge(final, "Restore", fill=(82, 105, 186))
    save_cover("session-restore", final)
    save_gif("session-restore", frames + [final] * 5, 165)


def scene_macros():
    steps = ["Start recording", "Type text", "Run search", "Save macro", "Play macro"]
    frames = []
    for i in range(12):
        im = overlay_tint(base_frame(), 40)
        draw = ImageDraw.Draw(im)
        code_panel(im, ["for item in items:", "    process(item)", "", "find: TODO", "replace: DONE"],
                   pos=(214, 166), size=(590, 380), active_line=min(4, i // 3))
        rounded(draw, (844, 146, 1128, 548), (255, 255, 255, 246), outline=(199, 209, 220), radius=12)
        draw.text((876, 176), "Macro Workflow", fill=(34, 45, 58), font=F_SUB)
        for n, step in enumerate(steps):
            y = 232 + n * 54
            active = n <= min(4, i // 2)
            fill = (48, 120, 214, 238) if active else (228, 233, 240, 238)
            text_fill = (255, 255, 255) if active else (86, 98, 112)
            rounded(draw, (878, y, 1092, y + 36), fill, radius=9)
            draw.text((900, y + 6), step, fill=text_fill, font=F_BADGE)
        if i % 2 == 0:
            draw.rectangle((416, 236, 419, 264), fill=(30, 92, 180))
        frames.append(im)
    final = frames[-1].copy()
    text_box(final, "Macros", "Record editing and search actions, then save, manage, and replay them", accent=(164, 92, 44))
    badge(final, "Automation", fill=(164, 92, 44))
    save_cover("macros", final)
    save_gif("macros", frames + [final] * 5, 155)


def scene_markdown():
    md_lines = ["# Release Notes", "", "- Fast native startup", "- Multi-tab editing", "- Markdown-friendly text flow", "", "```cpp", "auto editor = VelnixEditor{};", "```"]
    frames = []
    for i in range(10):
        im = overlay_tint(base_frame(), 28)
        code_panel(im, md_lines[: max(1, i)], pos=(236, 150), size=(826, 520), active_line=min(i, 2))
        frames.append(im)
    final = frames[-1].copy()
    draw = ImageDraw.Draw(final)
    for n, label in enumerate(["Heading", "List", "Code block"]):
        y = 208 + n * 68
        rounded(draw, (806, y, 1018, y + 46), (255, 255, 255, 238), outline=(199, 209, 220), radius=12)
        draw.text((834, y + 10), label, fill=(54, 65, 80), font=F_BADGE)
    text_box(final, "Markdown Editing", "Write structured Markdown now; preview belongs in plugins", accent=(88, 109, 67))
    badge(final, "Source Editing", fill=(88, 109, 67))
    save_cover("markdown", final)
    save_gif("markdown", frames + [final] * 5, 160)


def make_index():
    rows = [
        ("Startup Speed", "startup-speed"),
        ("Open File", "open-file"),
        ("Editing Experience", "editing-experience"),
        ("Session Restore", "session-restore"),
        ("Macros", "macros"),
        ("Markdown", "markdown"),
    ]
    content = ["# Velnix Editor Demo Assets", ""]
    for label, slug in rows:
        content.append(f"- {label}: `{slug}.gif`, `{slug}.png`")
    content.append("")
    content.append("These assets are generated from `docs/assets/main-window.png` by `scripts/generate-demo-assets.py`.")
    (OUT / "README.md").write_text("\n".join(content), encoding="utf-8")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    scene_startup()
    scene_open_file()
    scene_editing()
    scene_session_restore()
    scene_macros()
    scene_markdown()
    make_index()


if __name__ == "__main__":
    main()
