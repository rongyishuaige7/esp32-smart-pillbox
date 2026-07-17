#!/usr/bin/env python3
"""Fail on credentials, private paths, generated state, or unreviewed binaries."""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

SKIP_DIRS = {'.git', '.pio', '.dart_tool', '.gradle', '.idea', '.vscode', 'build', 'dist', '__pycache__', 'ephemeral'}
TEXT_SUFFIXES = {
    '', '.c', '.cc', '.cpp', '.h', '.hpp', '.ini', '.md', '.py', '.txt', '.yml', '.yaml',
    '.json', '.csv', '.html', '.xml', '.plist', '.dart', '.gradle', '.kts', '.sh', '.svg',
    '.properties', '.xcconfig', '.cmake', '.pbxproj', '.m', '.mm', '.swift', '.java', '.kt',
}
FORBIDDEN_SUFFIXES = {'.apk', '.aab', '.bin', '.elf', '.map', '.o', '.a', '.pem', '.key', '.p12', '.jks', '.zip', '.7z', '.tar', '.gz'}
FORBIDDEN_NAMES = {'.env', 'local.properties', 'id_rsa', 'id_ed25519', '.flutter-plugins-dependencies'}
PATTERNS = [
    ('private key', re.compile(r'-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----')),
    ('GitHub token', re.compile(r'\b(?:gh[opusr]_[A-Za-z0-9_]{20,}|github_pat_[A-Za-z0-9_]{20,})\b')),
    ('AWS access key', re.compile(r'\bAKIA[0-9A-Z]{16}\b')),
    ('OpenAI-style key', re.compile(r'\bsk-[A-Za-z0-9_-]{16,}\b')),
    ('private LAN literal', re.compile(r'\b(?:192\.168\.(?!4\.1\b)\d{1,3}\.\d{1,3}|10\.\d{1,3}\.\d{1,3}\.\d{1,3}|172\.(?:1[6-9]|2\d|3[01])\.\d{1,3}\.\d{1,3})\b')),
    ('local absolute path', re.compile(r'/(?:home|Users|mnt)/[^\s`"\']+')),
    ('Windows user path', re.compile(r'(?i)\b[A-Z]:\\Users\\[^\\\s]+')),
    ('generic assigned secret', re.compile(r'''(?ix)\b(api[_-]?key|access[_-]?token|auth[_-]?token|secret|password|passwd|pwd)\b\s*[:=]\s*["'](?!YOUR_|EXAMPLE|REPLACE|CHANGEME|REDACTED|\[REDACTED\]|<YOUR_|\.\.\.)([A-Za-z0-9+/=_!@#$%^&*.-]{8,})["']''')),
]
# Provenance paths are documentation of the only audited source locations, not runtime dependencies.
ALLOWED_EXACT_LINES = {
    ('docs/SOURCE_PROVENANCE.md', '/home/rongyi/桌面/ESP32智能药盒'),
    ('docs/SOURCE_PROVENANCE.md', '/mnt/shared/2026项目/ESP32智能药盒.zip'),
    ('docs/SOURCE_PROVENANCE.md', '/home/rongyi/桌面/esp32-smart-pillbox'),
}
ALLOWED_SUBSTRINGS = {'YOUR_WIFI_SSID', 'YOUR_WIFI_PASSWORD', '[REDACTED]', '<YOUR_'}


def tracked_files(root: Path) -> list[Path]:
    try:
        raw = subprocess.run(['git', '-C', str(root), 'ls-files', '-z'], check=True, capture_output=True).stdout
    except (FileNotFoundError, subprocess.CalledProcessError):
        raw = b''
    if raw:
        return [root / item.decode('utf-8', 'surrogateescape') for item in raw.split(b'\0') if item]
    return sorted(
        path for path in root.rglob('*')
        if path.is_file() and not any(part in SKIP_DIRS for part in path.relative_to(root).parts)
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', default='.')
    root = Path(parser.parse_args().root).resolve()
    errors: list[str] = []
    self_path = Path(__file__).resolve()
    for path in tracked_files(root):
        rel = path.relative_to(root)
        if path.name in FORBIDDEN_NAMES:
            errors.append(f'{rel}: forbidden local/config file')
        if any(part in SKIP_DIRS for part in rel.parts):
            errors.append(f'{rel}: forbidden generated directory')
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f'{rel}: forbidden binary/archive/key artifact')
        if path.stat().st_size > 5 * 1024 * 1024:
            errors.append(f'{rel}: file exceeds 5 MiB')
        if path.resolve() == self_path or path.suffix.lower() not in TEXT_SUFFIXES or path.stat().st_size > 2_000_000:
            continue
        try:
            text = path.read_text(encoding='utf-8')
        except (UnicodeDecodeError, OSError):
            continue
        for number, line in enumerate(text.splitlines(), 1):
            if (rel.as_posix(), line.strip()) in ALLOWED_EXACT_LINES or any(value in line for value in ALLOWED_SUBSTRINGS):
                continue
            for label, pattern in PATTERNS:
                if label == 'local absolute path' and rel.as_posix() == 'docs/SOURCE_PROVENANCE.md':
                    continue
                if pattern.search(line):
                    errors.append(f'{rel}:{number}: {label}')
    if errors:
        print('Secret scan: FAIL', file=sys.stderr)
        print('\n'.join(sorted(set(errors))), file=sys.stderr)
        return 1
    print('Secret scan: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
