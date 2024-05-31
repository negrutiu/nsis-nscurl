import subprocess
from datetime import datetime
import re

# execute: git log -1 --format=%ai
# output: b'2024-05-31 17:00:52 +0300\n'
output = subprocess.check_output(["git", "log", "-1", "--format=%ai"])
commit = datetime.fromisoformat(output.decode('utf-8').replace('\n', ''))
print(f"-- last commit was on {commit}")

# Replace strings in file
majver = 1
regexes = {
    re.compile(r'^\s*FILEVERSION\s+([\d \t,]+)*$') : f"{majver},{commit.year},{commit.month},{commit.day}",
    re.compile(r'^\s*VALUE\s+"FileVersion"\s*,\s*"([\d\.]+)"\s*$') : f"{majver}.{commit.year}.{commit.month}.{commit.day}",
    re.compile(r'^\s*VALUE\s+"LegalCopyright"\s*,\s*"\D+\d+-(\d+).*$') : f"{commit.year}"
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
            print("{}".format(line.replace('\n', '')))
            outfile.write(line)
else:
    print("resource file already up-to-date")
