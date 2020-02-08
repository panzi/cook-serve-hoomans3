#!/usr/bin/env python3

from game_maker import *
from os import rename
from os.path import isfile, exists
import sys

if __name__ == '__main__':
	import argparse

	parser = argparse.ArgumentParser()

	parser.add_argument('archives', nargs='*')
	parser.add_argument('--wine', action='store_true')
	parser.add_argument('-t', '--target', default=None)

	args = parser.parse_args()

	if args.archives:
		archives = args.archives
	elif args.wine:
		archives = [find_archive_wine()]
	elif args.target and args.target.lower() in ('win32', 'win64') and \
	     platform.system() != 'Windows' and not platform.system().startswith('CYGWIN'):
		archives = [find_archive_wine()]
	else:
		archives = [find_archive()]

	for archive in archives:
		backup = archive + '.backup'
		if isfile(backup):
			print('Reverting patch...')
			print('Source:     ', backup)
			print('Destination:', archive)
			rename(backup, archive)
		elif exists(backup):
			sys.stderr.write('*** Error: Backup exists but is not a file: %s\n' % backup)
			sys.exit(1)
		else:
			sys.stderr.write('*** Error: Backup does not exist: %s\n' % backup)
			sys.exit(1)
