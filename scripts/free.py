#!/usr/bin/env python3

import os
from os.path import join as pjoin, abspath, dirname
from PIL import Image

FOLDERS = ('CUST_SPR_AllNewFTC_A', 'CUST_SPR_AllNewFTC_B')

def free():
	free_map = {folder: set(range(50)) for folder in FOLDERS}
	prjdir = pjoin(dirname(abspath(__file__)), '..')

	for folder in FOLDERS:
		spritedir = pjoin(prjdir, 'sprites', folder)
		free_set = free_map[folder]
		for filename in os.listdir(spritedir):
			parts = filename.split(".")
			if len(parts) == 2 and parts[1] == "png":
				try:
					number = int(parts[0], 10)
				except ValueError:
					pass
				else:
					free_set.remove(number)

	for folder in FOLDERS:
		free_slots = free_map[folder]
		if free_slots:
			print(folder + ':')
			for number in sorted(free_slots):
				filename = '%s/%d.png' % (folder, number)
				filepath = pjoin(prjdir, 'dump', folder, '%d.png' % number)
				width, height = Image.open(filepath).size
				print('\t%-30s %d x %d' % (filename, width, height))
		print()

if __name__ == '__main__':
	free()
