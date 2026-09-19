"""Assemble checked Alpakit archives into the versioned TeleportLogistics release set."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import stat
from pathlib import Path, PurePosixPath
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


ROOT = Path(__file__).resolve().parents[1]


def safe_files(archive: ZipFile) -> list[ZipInfo]:
    files: list[ZipInfo] = []
    seen: set[str] = set()
    for info in archive.infolist():
        path = PurePosixPath(info.filename)
        if path.is_absolute() or ".." in path.parts or "\\" in info.filename or ":" in info.filename:
            raise SystemExit(f"Unsafe archive path: {info.filename}")
        normalized = str(path).casefold()
        if normalized in seen:
            raise SystemExit(f"Duplicate archive path: {info.filename}")
        seen.add(normalized)
        if stat.S_ISLNK(info.external_attr >> 16):
            raise SystemExit(f"Symlink in release archive: {info.filename}")
        if not info.is_dir():
            files.append(info)
    return files


def verify_archive(path: Path, version: str, required: tuple[str, ...]) -> None:
    with ZipFile(path) as archive:
        files = safe_files(archive)
        names = {info.filename for info in files}
        missing = set(required) - names
        if missing:
            raise SystemExit(f"{path} is missing: {', '.join(sorted(missing))}")
        manifest = json.loads(archive.read("TeleportLogistics.uplugin"))
        if manifest.get("SemVersion") != version:
            raise SystemExit(
                f"{path} contains TeleportLogistics {manifest.get('SemVersion')}, expected {version}"
            )
        expected = json.loads((ROOT / "TeleportLogistics/TeleportLogistics.uplugin").read_text())
        for key in ("Version", "VersionName", "FriendlyName", "GameVersion", "Plugins", "Modules"):
            if manifest.get(key) != expected.get(key):
                raise SystemExit(f"{path}: packaged {key} differs from source manifest")
        bad = archive.testzip()
        if bad:
            raise SystemExit(f"CRC failure in {path}: {bad}")


def write_merged(client: Path, server: Path, destination: Path) -> None:
    # Match Alpakit's ArchiveMergedStagedPlugin layout: each complete staged
    # plugin sits below its FinalCookPlatform directory.
    with ZipFile(destination, "w", ZIP_DEFLATED, compresslevel=9) as output:
        for platform, source in (("Windows", client), ("WindowsServer", server)):
            with ZipFile(source) as archive:
                for info in safe_files(archive):
                    merged = ZipInfo(f"{platform}/{info.filename}", info.date_time)
                    merged.compress_type = ZIP_DEFLATED
                    merged.external_attr = info.external_attr
                    merged.create_system = info.create_system
                    output.writestr(merged, archive.read(info), compresslevel=9)


def digest(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    archive_dir = ROOT / ".toolchains/sml-project/Saved/ArchivedPlugins/TeleportLogistics"
    parser.add_argument("--client", type=Path, default=archive_dir / "TeleportLogistics-Windows.zip")
    parser.add_argument(
        "--server", type=Path, default=archive_dir / "TeleportLogistics-WindowsServer.zip"
    )
    parser.add_argument(
        "--allow-stale",
        action="store_true",
        help="package archives older than the plugin sources (an editor-only or partial rebuild)",
    )
    args = parser.parse_args()

    manifest = json.loads((ROOT / "TeleportLogistics/TeleportLogistics.uplugin").read_text())
    version = manifest["SemVersion"]

    # ficsit.app rejects an upload whose integer Version is not the SemVer major, and
    # whose VersionName disagrees with SemVersion. Checked here as well as in the test
    # suite, because this is the path a release actually goes through.
    if manifest["Version"] != int(version.split(".")[0]):
        raise SystemExit(
            f'Version {manifest["Version"]} must equal the SemVersion major of {version}'
        )
    if manifest["VersionName"] != version:
        raise SystemExit(f'VersionName {manifest["VersionName"]} does not match SemVersion {version}')

    # Every other check here proves an archive is internally consistent, not that it came
    # from the build just run. Alpakit replaces only the platform zip it writes, so a
    # partial or editor-only run leaves the other platform's zip behind, and at an
    # unchanged version nothing downstream can tell.
    ignored = {"Binaries", "Intermediate", "Saved", "__pycache__"}
    newest = max(
        path.stat().st_mtime
        for path in (ROOT / "TeleportLogistics").rglob("*")
        if path.is_file() and not ignored.intersection(path.parts)
    )
    for source in (args.client, args.server):
        if source.stat().st_mtime < newest and not args.allow_stale:
            raise SystemExit(
                f"{source} predates the plugin sources it packages; rebuild it, "
                f"or pass --allow-stale if you know the archive is current."
            )
    required_client = (
        "TeleportLogistics.uplugin",
        "Resources/SFUIKIT-NOTICE.md",
        "Resources/NAMEPLATE-NOTICE.md",
        "Resources/MODELING-NOTICE.md",
        "Resources/ICONS-NOTICE.md",
        "Binaries/Win64/FactoryGameSteam-TeleportLogistics-Win64-Shipping.dll",
        "Binaries/Win64/FactoryGameEGS-TeleportLogistics-Win64-Shipping.dll",
        "Content/Paks/Windows/TeleportLogisticsFactoryGame-Windows.pak",
        "Content/Paks/Windows/TeleportLogisticsFactoryGame-Windows.utoc",
        "Content/Paks/Windows/TeleportLogisticsFactoryGame-Windows.ucas",
    )
    required_server = (
        "TeleportLogistics.uplugin",
        "Resources/SFUIKIT-NOTICE.md",
        "Resources/NAMEPLATE-NOTICE.md",
        "Resources/MODELING-NOTICE.md",
        "Resources/ICONS-NOTICE.md",
        "Binaries/Win64/FactoryServer-TeleportLogistics-Win64-Shipping.dll",
        "Content/Paks/WindowsServer/TeleportLogisticsFactoryGame-WindowsServer.pak",
        "Content/Paks/WindowsServer/TeleportLogisticsFactoryGame-WindowsServer.utoc",
        "Content/Paks/WindowsServer/TeleportLogisticsFactoryGame-WindowsServer.ucas",
    )
    verify_archive(args.client, version, required_client)
    verify_archive(args.server, version, required_server)

    dist = ROOT / "dist"
    dist.mkdir(exist_ok=True)
    client_out = dist / f"TeleportLogistics-{version}-Windows.zip"
    server_out = dist / f"TeleportLogistics-{version}-WindowsServer.zip"
    merged_out = dist / f"TeleportLogistics-{version}.zip"
    shutil.copy2(args.client, client_out)
    shutil.copy2(args.server, server_out)
    write_merged(client_out, server_out, merged_out)
    verify_archive(client_out, version, required_client)
    verify_archive(server_out, version, required_server)
    with ZipFile(merged_out) as archive:
        merged_names = {info.filename for info in safe_files(archive)}
        for prefix, required in (("Windows", required_client), ("WindowsServer", required_server)):
            missing = {f"{prefix}/{name}" for name in required} - merged_names
            if missing:
                raise SystemExit(f"{merged_out} is missing: {', '.join(sorted(missing))}")
        bad = archive.testzip()
        if bad:
            raise SystemExit(f"CRC failure in {merged_out}: {bad}")

    # Hash only what this run produced. Superseded archives left in dist/ are not
    # part of the release, and package-source.py writes its zip before this runs.
    source_out = dist / f"TeleportLogistics-{version}-source.zip"
    produced = [(digest(path), path) for path in (client_out, server_out, merged_out, source_out)
                if path.is_file()]
    (dist / "SHA256SUMS").write_text(
        "".join(f"{value}  {path.name}\n" for value, path in produced))
    for value, path in produced:
        print(f"{value}  {path}")


if __name__ == "__main__":
    main()
