#!/usr/bin/env python3
"""
Generates the Windows time zone table of src/PlatformSpecific/SystemLocation.windows.cpp: each (Windows zone, territory)
pair of the Unicode CLDR windowsZones mapping with the coordinates of its first IANA zone in the tz database's tables.

Usage:
    python3 tools/windows-zones/generate.py <windowsZones.xml> <zoneinfo directory> > table.inc

Sources:
    - windowsZones.xml: https://github.com/unicode-org/cldr, common/supplemental/windowsZones.xml (Copyright © 2001-2026
      Unicode, Inc., Unicode License v3 — docs/third-party-notices.md).
    - The tz database's zone.tab, zone1970.tab and tzdata.zi (the links), public domain (/usr/share/zoneinfo on Linux).
A zone without a city (Etc/GMT+n, UTC) is left out: the engine falls back to its birthplace.
"""

import re
import sys


def iso6709(text):
	match = re.match(r'([+-])(\d{2})(\d{2})(\d{2})?([+-])(\d{3})(\d{2})(\d{2})?$', text)
	latitude = int(match.group(2)) + int(match.group(3)) / 60 + int(match.group(4) or 0) / 3600
	longitude = int(match.group(6)) + int(match.group(7)) / 60 + int(match.group(8) or 0) / 3600
	return (latitude if match.group(1) == '+' else -latitude, longitude if match.group(5) == '+' else -longitude)


def main():
	xml = open(sys.argv[1], encoding='utf-8').read()
	zoneinfo = sys.argv[2].rstrip('/')
	coordinates = {}

	for name in ('zone.tab', 'zone1970.tab'):
		for line in open(f'{zoneinfo}/{name}', encoding='utf-8'):
			if line.startswith('#'):
				continue
			fields = line.rstrip('\n').split('\t')
			if len(fields) >= 3 and fields[2] not in coordinates:
				coordinates[fields[2]] = iso6709(fields[1])

	links = {}
	for line in open(f'{zoneinfo}/tzdata.zi', encoding='utf-8'):
		if line.startswith('L '):
			_, target, alias = line.split()[:3]
			links[alias] = target

	rows = []
	for windows, territory, types in re.findall(r'<mapZone other="([^"]+)" territory="([^"]+)" type="([^"]+)"/>', xml):
		zone = types.split()[0]
		zone = zone if zone in coordinates else links.get(zone, zone)
		if zone in coordinates:
			rows.append((windows, territory, coordinates[zone]))

	for windows, territory, (latitude, longitude) in sorted(rows):
		print('\t\t{.windowsZone = "%s", .territory = "%s", .latitude = %.4f, .longitude = %.4f},' % (windows, territory, latitude, longitude))


if __name__ == '__main__':
	main()
