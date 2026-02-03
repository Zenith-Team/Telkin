import json
from datetime import datetime, timezone
from pathlib import Path

def telkin_define_region(make_args: dict) -> str:
    header = (
        f"; Telkin Loader - Built on {datetime.now(timezone.utc).isoformat()}\r\n"
        "; Copyright (c) 2021-2025 Zenith Team - MPL-2.0-no-copyleft-exception\r\n"
        f"[{make_args['title_id']}]\r\n"
        f"moduleMatches = {make_args['hash']}\r\n\r\n"
        f"{make_args['inject_addr']} = b TelkinBootstrap\r\n"
    )

    return header + "\r\n"

def to_crlf(text: str) -> str:
    lf = text.replace("\r\n", "\n").replace("\r", "\n")
    return lf.replace("\n", "\r\n")

def create_patch(targets):
    file_content = Path("./launcher/cemu/main.asm").read_text(encoding="utf-8")
    out = []
    for tgt in targets:
        out.append(telkin_define_region(tgt))
    combined = "\r\n\r\n".join(out) + "\r\n\r\n" + file_content
    final = to_crlf(combined)
    Path("./out/Telkin/patch_Loader.asm").write_text(final, encoding="utf-8")

def create_rules(targets):
    file_content = Path("./launcher/cemu/rules.txt").read_text(encoding="utf-8")
    header = "[Definition]\r\ntitleIds = "
    for tgt in targets:
        header += f"{tgt['title_id']},"
    header = header.rstrip(",") + "\r\n\r\n"
    file_content = header + file_content
    final = to_crlf(file_content)
    Path("./out/Telkin/rules.txt").write_text(final, encoding="utf-8")


def main():
    Path("./out/Telkin").mkdir(parents=True, exist_ok=True)
    targets = json.loads(Path("./launcher/targets.json").read_text(encoding="utf-8")).get("targets", [])
    create_patch(targets)
    create_rules(targets)

if __name__ == "__main__":
    main()
