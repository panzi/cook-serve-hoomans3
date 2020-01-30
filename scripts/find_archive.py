#!/usr/bin/env python3

from game_maker import find_archive, find_archive_wine
import sys

if len(sys.argv) > 1 and sys.argv[1] == '--wine':
	print(find_archive_wine())
else:
	print(find_archive())
