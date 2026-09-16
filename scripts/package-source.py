"""Create a review/build handoff archive, explicitly not an installable mod."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import json

root = Path(__file__).resolve().parents[1]
version = json.loads((root / "TeleportLogistics" / "TeleportLogistics.uplugin").read_text())["SemVersion"]
out = root / "dist" / f"TeleportLogistics-{version}-source.zip"
out.parent.mkdir(exist_ok=True)
# reports/model-renders is 42 MB of per-version PNG sheets; the dated markdown
# records next to them are the evidence a reviewer needs.
excluded = {"Binaries", "Intermediate", "Saved", "DerivedDataCache", "__pycache__",
            "node_modules", "model-renders"}
with ZipFile(out, "w", ZIP_DEFLATED) as archive:
    for folder in ("TeleportLogistics", "scripts", "tests", "docs", "reports", "assets", "design", "tools"):
        for path in sorted((root / folder).rglob("*")):
            if path.is_file() and not excluded.intersection(path.parts):
                archive.write(path, path.relative_to(root))
    for name in ("LICENSE", "README.md", "CHANGELOG.md", "THIRD-PARTY-NOTICES.md",
                 "sdk-lock.json", ".clang-format"):
        archive.write(root / name, name)
print(out)
