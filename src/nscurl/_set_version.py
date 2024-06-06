import subprocess
from datetime import datetime, timezone
import re
import argparse

# github workflow calls us with --version=1.2.3.4
parser = argparse.ArgumentParser()
parser.add_argument("-v", "--version", type=str, default="")
args = parser.parse_args()
if not args.version:
    date = datetime.now(tz=timezone.utc)
    args.version = f"{date.year}.{date.month}.{date.day}.0"
print(f"-- version is {args.version}")

v = args.version.split('.')
if len(v) != 4: raise ValueError(f"invalid version {args.version}")

'''
# execute: git log -1 --format=%ai
# output: b'2024-05-31 17:00:52 +0300\n'
output = subprocess.check_output(["git", "log", "-1", "--format=%ai"])
date = datetime.fromisoformat(output.decode('utf-8').replace('\n', ''))
print(f"-- last commit was on {date}")
'''

# Replace strings in file
regexes = {
    re.compile(r'^\s*FILEVERSION\s+([\d \t,]+)*$') : f"{v[0]},{v[1]},{v[2]},{v[3]}",
    re.compile(r'^\s*VALUE\s+"FileVersion"\s*,\s*"([\d\.]+)"\s*$') : f"{v[0]}.{v[1]}.{v[2]}.{v[3]}",
    re.compile(r'^\s*VALUE\s+"LegalCopyright"\s*,\s*"\D+\d+-(\d+).*$') : f"{datetime.now().year}"
}

lines = []
replaced = 0

with open('resource.rc') as infile:
    print(f"-- replacing in \"{infile.name}\":")
    for line in infile:
        found = False
        for regex, value in regexes.items():
            groups = regex.match(line)
            if groups != None:
                found = True
                line2 = line.replace(groups[1], value)
                if line != line2:
                    print(f"replaced \"{groups[1]}\" with \"{value}\":")
                    print("  {}".format(line2.replace('\n', '')))
                    replaced = replaced + 1
                lines.append(line2)
                break
        if not found:
            lines.append(line)

if replaced > 0:
    with open(infile.name, 'w') as outfile:
        for line in lines:
            # print("{}".format(line.replace('\n', '')))
            outfile.write(line)
else:
    print("resource file already up-to-date")
