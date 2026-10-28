#!/usr/bin/env python3
#
# This script is used to generate the list of bugs per compiler version
# from the list of bugs.
# It updates the list in place. The command-line test
# test/cmdlineTests/~update_bugs_by_version runs it and fails if the list
# changed, which checks in CI that the list is up to date.
# A bug without "fixed" is not fixed in any release yet and is listed for
# every version from "introduced" on.

import json
import re
import sys
from pathlib import Path

def comp(version_string):
    return [int(c) for c in version_string.split('.')]

root_path = Path(__file__).resolve().parent.parent

bugs = json.loads((root_path / 'docs/bugs.json').read_text(encoding='utf8'))

versions = {}
with (root_path / 'Changelog.md').open(encoding='utf8') as changelog:
    for line in changelog:
        m = re.search(r'^### (\S+) \((\d+-\d+-\d+)\)$', line)
        if m:
            versions[m.group(1)] = {}
            versions[m.group(1)]['released'] = m.group(2)

if not versions:
    sys.exit('No released versions in Changelog.md: expected headings such as "### 0.2.0 (2026-10-19)".')

for key, value in versions.items():
    value['bugs'] = []
    for bug in bugs:
        if 'introduced' in bug and comp(bug['introduced']) > comp(key):
            continue
        if 'fixed' in bug and comp(bug['fixed']) <= comp(key):
            continue
        value['bugs'] += [bug['name']]

(root_path / 'docs/bugs_by_version.json').write_text(json.dumps(
    versions,
    sort_keys=True,
    indent=4,
    separators=(',', ': ')
) + '\n', encoding='utf8')
