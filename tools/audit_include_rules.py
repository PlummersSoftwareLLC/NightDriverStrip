#!/usr/bin/env python3

# +--------------------------------------------------------------------------
#
# File:        audit_include_rules.py
#
# NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
#
# This file is part of the NightDriver software project.
#
#    NightDriver is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    NightDriver is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Nightdriver.  It is normally found in copying.txt
#    If not, see <https://www.gnu.org/licenses/>.
#
#
# Description:
#
#    Include graph rule checks for fragile header cycles and IWYU violations.
#
# ---------------------------------------------------------------------------

import os
import re
import sys
from collections import deque, defaultdict

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
INCLUDE_DIRS = [
    os.path.join(PROJECT_ROOT, 'include'),
    os.path.join(PROJECT_ROOT, 'src'),
]

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*(["<])([^">]+)([">])')


def collect_files():
    files = []
    for base in INCLUDE_DIRS:
        for root, _, filenames in os.walk(base):
            # Skip third-party source directories
            if 'src/uzlib' in root or '.pio' in root:
                continue
            for name in filenames:
                if name.endswith(('.h', '.hpp', '.cpp', '.c')):
                    files.append(os.path.join(root, name))
    return files


def build_index(files):
    by_rel = {}
    by_base = defaultdict(list)
    for path in files:
        rel_path = os.path.relpath(path, PROJECT_ROOT)
        by_rel[rel_path] = path
        by_base[os.path.basename(path)].append(path)
    return by_rel, by_base


def resolve_include(current_file, include_path, by_rel, by_base):
    if include_path.startswith('/'):
        return None

    candidates = [
        os.path.join(os.path.dirname(current_file), include_path),
        os.path.join(PROJECT_ROOT, include_path),
        os.path.join(PROJECT_ROOT, 'include', include_path),
        os.path.join(PROJECT_ROOT, 'src', include_path),
    ]

    for candidate in candidates:
        if os.path.isfile(candidate):
            return candidate

    # If it's a bare filename and unique in project, resolve it.
    base = os.path.basename(include_path)
    if base == include_path and base in by_base and len(by_base[base]) == 1:
        return by_base[base][0]

    return None


def parse_includes(files, by_rel, by_base):
    graph = defaultdict(set)
    for path in files:
        try:
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                for line in f:
                    match = INCLUDE_RE.match(line)
                    if not match:
                        continue
                    include_path = match.group(2)
                    # Only track local/known includes
                    resolved = resolve_include(path, include_path, by_rel, by_base)
                    if resolved:
                        graph[path].add(resolved)
        except OSError:
            continue
    return graph


def find_path(graph, start, targets):
    queue = deque([start])
    parent = {start: None}

    while queue:
        node = queue.popleft()
        if node in targets and node != start:
            # Build path
            path = [node]
            while parent[path[-1]] is not None:
                path.append(parent[path[-1]])
            path.reverse()
            return path
        for neighbor in graph.get(node, []):
            if neighbor not in parent:
                parent[neighbor] = node
                queue.append(neighbor)
    return None


def rel(path):
    return os.path.relpath(path, PROJECT_ROOT)


def audit_include_ordering(path, rel_path, violations):
    """
    Check that includes within contiguous blocks are correctly tiered and alphabetized.
    Tiers: Globals > System <...> > Local "..."
    Skips effects/ directory as those are often legacy or complex.
    """
    if 'include/effects/' in rel_path:
        return

    try:
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
            
            current_block = []
            
            for line_no, line in enumerate(lines, 1):
                match = INCLUDE_RE.match(line)
                if match:
                    delim_start = match.group(1)
                    include_file = match.group(2)
                    current_block.append({
                        'file': include_file,
                        'delim': delim_start,
                        'line': line.strip(),
                        'no': line_no
                    })
                elif line.strip().startswith("//"):
                    # Allow comments within a block
                    continue
                else:
                    # End of a block (empty line or other code)
                    if len(current_block) > 1:
                        verify_block_order(rel_path, current_block, violations)
                    current_block = []
            
            # Check last block
            if len(current_block) > 1:
                verify_block_order(rel_path, current_block, violations)
                
    except OSError:
        pass

def verify_block_order(rel_path, block, violations):
    # Rule 0: globals.h must be absolute first if present
    if block[0]['file'] == "globals.h":
        globals_item = block[0]
        # Check if globals.h appears again later (redundant)
        for other in block[1:]:
            if other['file'] == "globals.h":
                violations.append(f"Style violation: Redundant globals.h in {rel_path} at line {other['no']}")
        block = block[1:]
    else:
        # If globals.h is in the block but NOT first
        for i, item in enumerate(block):
            if item['file'] == "globals.h":
                violations.append(f"Style violation: globals.h must be the FIRST include in {rel_path} at line {item['no']}")
                # We don't return here so we can find other issues in the block
        
    if len(block) <= 1:
        return

    # Rule 1: Tiered precedence: System <...> MUST precede Local "..."
    # Rule 2: Each tier must be alphabetized independently.
    
    systems = [b for b in block if b['delim'] == '<']
    locals = [b for b in block if b['delim'] == '"']
    
    # Check if any local precedes a system
    first_local_idx = -1
    for i, b in enumerate(block):
        if b['delim'] == '"':
            first_local_idx = i
            break
            
    if first_local_idx != -1:
        for i in range(first_local_idx + 1, len(block)):
            if block[i]['delim'] == '<':
                violations.append(f"Style violation: System header {block[i]['file']} must precede local headers in {rel_path} at line {block[i]['no']}")
                break

    # Check alphabetization within systems
    if systems:
        sys_names = [s['file'] for s in systems]
        sorted_sys = sorted(sys_names, key=str.lower)
        for i, (a, b) in enumerate(zip(sys_names, sorted_sys)):
            if a != b:
                item = systems[i]
                violations.append(f"Style violation: System includes not alphabetized in {rel_path} at line {item['no']}\n  Found: {a}, Expected: {b}")
                break

    # Check alphabetization within locals
    if locals:
        loc_names = [l['file'] for l in locals]
        sorted_loc = sorted(loc_names, key=str.lower)
        for i, (a, b) in enumerate(zip(loc_names, sorted_loc)):
            if a != b:
                item = locals[i]
                violations.append(f"Style violation: Local includes not alphabetized in {rel_path} at line {item['no']}\n  Found: {a}, Expected: {b}")
                break

def main():
    files = collect_files()
    by_rel, by_base = build_index(files)
    graph = parse_includes(files, by_rel, by_base)

    # heavy_headers that should be kept in .cpp files or decoupled
    heavy_headers = {
        os.path.join(PROJECT_ROOT, 'include', 'socketserver.h'),
        os.path.join(PROJECT_ROOT, 'include', 'ledbuffer.h'),
        os.path.join(PROJECT_ROOT, 'include', 'gfxbase.h'),
        os.path.join(PROJECT_ROOT, 'include', 'hub75gfx.h'),
        os.path.join(PROJECT_ROOT, 'include', 'ws281xgfx.h'),
        os.path.join(PROJECT_ROOT, 'include', 'screen.h'),
        os.path.join(PROJECT_ROOT, 'include', 'jsonserializer.h'),
        os.path.join(PROJECT_ROOT, 'include', 'ledstripeffect.h'),
    }

    # Core system headers that must remain slim
    core_headers = [
        'include/nd_network.h',
        'include/systemcontainer.h',
        'include/taskmgr.h',
        'include/interfaces.h',
        'include/types.h'
    ]

    transitive_rules = []
    
    # Rules for core headers
    for core in core_headers:
        core_path = os.path.join(PROJECT_ROOT, core)
        if not os.path.isfile(core_path):
            continue
            
        # Target specific heavy headers that shouldn't leak into core
        targets = heavy_headers.copy()
        
        transitive_rules.append({
            'from': core_path,
            'targets': targets,
            'label': f'{core} must not include heavy display, effect, or serialization headers'
        })

    violations = []

    # Check transitive rules
    for rule in transitive_rules:
        start = rule['from']
        path = find_path(graph, start, rule['targets'])
        if path:
            chain = ' -> '.join(rel(p) for p in path)
            violations.append(f"Transitive include violation: {rule['label']}\n  Path: {chain}")

    # Self-Contained Header and Pragma Once checks
    for path in files:
        rel_path = rel(path)
        
        # Style Rule: Alphabetical includes
        audit_include_ordering(path, rel_path, violations)

        if not path.endswith('.h'):
            continue
        
        # Files that are allowed to not include globals.h
        skip_globals_check = [
            'include/globals.h',
            'include/interfaces.h',
            'include/secrets.h',
            'include/secrets.example.h',
            'include/amoled/lv_conf.h'
        ]
        
        # Files that are allowed to skip #pragma once
        skip_pragma_check = [
            'include/secrets.h',
            'include/secrets.example.h',
            'include/amoled/lv_conf.h'
        ]

        try:
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                
                # Rule 1: Mandatory #pragma once (except exempted headers)
                if rel_path not in skip_pragma_check:
                    if '#pragma once' not in content:
                        violations.append(f"Header violation: Missing #pragma once in {rel_path}")
                
                # Rule 2: Mandatory globals.h inclusion (except leaf effects and exempted headers)
                if rel_path not in skip_globals_check and 'include/effects/' not in rel_path:
                    if '#include "globals.h"' not in content and '#include <globals.h>' not in content:
                         violations.append(f"Header violation: Missing globals.h inclusion in {rel_path}")
        except OSError:
            continue

    if violations:
        print('Include rule violations found:')
        for v in violations:
            print(v)
        sys.exit(1)

    print('No include rule violations found.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
