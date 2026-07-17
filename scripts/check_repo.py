#!/usr/bin/env python3
"""Validate publication contracts that need no physical hardware."""
from __future__ import annotations

import argparse
import csv
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

REQUIRED = [
    '.github/workflows/validate.yml', '.gitignore', '.markdownlint-cli2.jsonc', 'HARDWARE.md', 'LICENSE',
    'README.md', 'SECURITY.md', 'THIRD_PARTY_NOTICES.md', 'docs/GITHUB_METADATA.md',
    'docs/HARDWARE_LAB_CARD.md', 'docs/PROJECT_STATUS.md', 'docs/PROTOCOL.md',
    'docs/SOURCE_PROVENANCE.md', 'docs/VERIFICATION.md', 'hardware/BOM.csv', 'hardware/wiring-diagram.svg',
    'firmware/platformio.ini', 'scripts/check_repo.py', 'scripts/secret_scan.py', 'scripts/verify.sh',
    'tests/test_source_contracts.py', 'app/pubspec.yaml', 'app/test/widget_test.dart',
]
FORBIDDEN_NAMES = {'.env', 'local.properties', 'id_rsa', 'id_ed25519', '.flutter-plugins-dependencies'}
FORBIDDEN_DIRS = {'.pio', '.gradle', '.dart_tool', '.idea', 'build', 'dist', 'ephemeral', '__pycache__', '.vscode'}
FORBIDDEN_SUFFIXES = {'.o', '.a', '.elf', '.bin', '.map', '.pyc', '.apk', '.aab', '.pem', '.key', '.zip', '.7z', '.tar', '.gz'}
MAX_FILE_SIZE = 5 * 1024 * 1024


def files(root: Path) -> list[Path]:
    try:
        raw = subprocess.run(['git', '-C', str(root), 'ls-files', '-z'], check=True, capture_output=True).stdout
    except (subprocess.CalledProcessError, FileNotFoundError):
        raw = b''
    if raw:
        return [root / item.decode('utf-8', 'surrogateescape') for item in raw.split(b'\0') if item]
    return sorted(
        path for path in root.rglob('*')
        if path.is_file() and not any(part in {'.git', *FORBIDDEN_DIRS} for part in path.relative_to(root).parts)
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', default='.')
    root = Path(parser.parse_args().root).resolve()
    errors: list[str] = []
    for rel in REQUIRED:
        if not (root / rel).is_file():
            errors.append(f'missing required file: {rel}')
    checked = files(root)
    for path in checked:
        rel = path.relative_to(root)
        if path.name in FORBIDDEN_NAMES:
            errors.append(f'forbidden local/config file: {rel}')
        if any(part in FORBIDDEN_DIRS for part in rel.parts):
            errors.append(f'forbidden generated directory: {rel}')
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f'forbidden binary/archive/key artifact: {rel}')
        if path.stat().st_size > MAX_FILE_SIZE:
            errors.append(f'file exceeds 5 MiB: {rel}')

    contracts = {
        'README.md': [
            '不是医疗器械', '无认证、无 TLS', '当前端到端真机复测未执行',
            '本次状态请求成功', '可选 SSD1306',
        ],
        'firmware/platformio.ini': ['platform = espressif32@6.13.0', 'board = esp32dev'],
        'firmware/src/main.cpp': [
            'reminderManager.begin(18, 13, 21, 22)',
            'sensorManager.begin(15, 25, 4, 2, 33, 32)',
            'configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com")',
        ],
        'firmware/src/wifi_manager.cpp': ['WiFi.softAP(ap_ssid)', 'const char* ap_ssid = "SmartPillbox"'],
        'firmware/src/webserver.cpp': [
            'doc["status"] = "local_response"', 'Access-Control-Allow-Origin',
            'server.send(404, "application/json", "{\\"error\\":\\"Not found\\"}")',
            'doc.remove("wifi_ssid")', 'doc.remove("wifi_password")',
        ],
        'app/lib/constants.dart': ["defaultDeviceHost = '192.168.4.1'", 'actual STA address'],
        'app/lib/pages/home_page.dart': ["'本次状态请求成功'", 'Timer.periodic'],
        'app/android/app/src/main/AndroidManifest.xml': ['android:usesCleartextTraffic="true"', 'android.permission.INTERNET'],
        'docs/SOURCE_PROVENANCE.md': [
            '23d03a0e4c7c4297c36e04554ba4c300b35e2e31ab4c6091ab77ace628adcd0b',
            '207b8a40c1b978d502aff9adca60d9b385a4f668284a83cd293a70650315ce52',
            '62f13bd53184f5b560533740c5fd4731f1fda681e86ee82bd0c781c0d09f943a',
        ],
    }
    for rel, values in contracts.items():
        path = root / rel
        if not path.is_file():
            continue
        text = path.read_text(encoding='utf-8')
        for value in values:
            if value not in text:
                errors.append(f'fact contract missing in {rel}: {value}')

    for rel in ['README.md', 'docs/PROJECT_STATUS.md', 'docs/HARDWARE_LAB_CARD.md']:
        path = root / rel
        text = path.read_text(encoding='utf-8').lower() if path.is_file() else ''
        for claim in ['system online', 'current hardware verified', 'hardware re-verified: pass', 'production ready']:
            if claim in text:
                errors.append(f'unsupported claim in {rel}: {claim}')
    try:
        ET.parse(root / 'hardware/wiring-diagram.svg')
    except (ET.ParseError, OSError) as exc:
        errors.append(f'invalid wiring SVG: {exc}')
    try:
        rows = list(csv.DictReader((root / 'hardware/BOM.csv').open(newline='', encoding='utf-8')))
        if len(rows) < 8:
            errors.append('BOM must contain at least 8 component rows')
    except (OSError, csv.Error) as exc:
        errors.append(f'invalid BOM.csv: {exc}')
    if errors:
        print('Repository check: FAIL', file=sys.stderr)
        for item in sorted(set(errors)):
            print(f'- {item}', file=sys.stderr)
        return 1
    print(f'Repository check: PASS ({len(checked)} files checked)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
