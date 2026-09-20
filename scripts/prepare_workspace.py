#!/usr/bin/env python3
"""Install contest app and reviewed BES configuration; never overwrite conflicts."""
from pathlib import Path
import argparse
import filecmp
import shutil
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('workspace', type=Path)
parser.add_argument('--check', action='store_true', help='Validate only; do not modify')
args = parser.parse_args()
root = args.workspace.resolve()
repo = Path(__file__).resolve().parents[1]
app = repo / 'app/knowledge_cards'
dest = root / 'packages/demos/knowledge_cards'
bes = root / 'vendor/bes'
patch = repo / 'board/bes2800bp/knowledge-cards.patch'
assert (root / 'packages/ai_agent/include/agent_config.h').is_file(), 'Missing packages/ai_agent'
assert (root / 'build.sh').is_file(), 'Not an openvela workspace'

# Preflight every mutation before changing any file.
if dest.exists() and dest.resolve() != app.resolve():
    for source in app.rglob('*'):
        if source.is_file() and source.suffix in ('.c', '.h'):
            other = dest / source.relative_to(app)
            if not other.is_file() or not filecmp.cmp(source, other, shallow=False):
                raise SystemExit(f'Existing application differs: {other}. Keep a backup and use the manifest link.')
    print('Existing application sources match; leaving directory in place.')
board_src = repo / 'board/bes2800bp/board_cfg.cmake'
board_dest = bes / 'boards/best1700_ep/aos_evb/board_cfg.cmake'
if board_dest.exists() and not filecmp.cmp(board_src, board_dest, shallow=False):
    raise SystemExit(f'Conflicting board config: {board_dest}')

def check(reverse=False):
    cmd = ['git', '-C', str(bes), 'apply', '--check']
    if reverse: cmd.append('--reverse')
    return subprocess.run(cmd + [str(patch)], capture_output=True).returncode == 0

applied = check(reverse=True)
if not applied and not check():
    raise SystemExit('BES baseline differs; review board/bes2800bp/knowledge-cards.patch before applying.')
if not args.check:
    if not dest.exists(): dest.symlink_to(app, target_is_directory=True)
    if not board_dest.exists(): shutil.copy2(board_src, board_dest)
    if not applied:
        subprocess.run(['git', '-C', str(bes), 'apply', str(patch)], check=True)
print('Workspace preparation: OK' + (' (check only)' if args.check else ''))
