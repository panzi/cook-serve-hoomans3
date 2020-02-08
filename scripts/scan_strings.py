#!/usr/bin/env python3

import struct
from game_maker import *

def is_magic(magic):
	return magic.isalnum()

def read_STRG(fp, area_size):
	start_offset = fp.tell()
	end_offset = start_offset + area_size

	count, = struct.unpack("<I", fp.read(4))
	data = fp.read(4 * count)
	offsets = struct.unpack("<%dI" % count, data)

	tbl = {}
	for index, offset in enumerate(offsets):
		fp.seek(offset, 0)
		data = fp.read(4)
		size, = struct.unpack("<I", data)
		data = fp.read(size).rstrip(b"\0")
		text = data.decode()
		tbl[offset + 4] = text

	fp.seek(end_offset, 0)
	return tbl

def read_strings(fp, area_size: int) -> (dict, int):
	size = area_size
	tbl  = None

	if area_size >= 8:
		head = fp.read(8)

		if len(head) == 8:
			magic, size = struct.unpack("<4sI", head)
			if size + 8 <= area_size and is_magic(magic):
				rem = size
				size += 8

				if magic == b'STRG':
					if tbl is None:
						tbl = read_STRG(fp, rem)
					else:
						tbl.update(read_STRG(fp, rem))
				else:
					while rem > 0:
						child_tbl, child_size = read_strings(fp, rem)
						if child_tbl is not None:
							if tbl is None:
								tbl = child_tbl
							else:
								tbl.update(child_tbl)
						rem -= child_size

				return tbl, size
			else:
				fp.seek(area_size - 8, 1)
				size = area_size
		else:
			size = len(head)
	else:
		fp.seek(area_size, 1)

	return tbl, size

def _scan_strings_in_data(offset: int, data: bytes, section, tbl: dict):
	data_len = len(data)
	for sub_offset in range(data_len - 3):
		val, = struct.unpack_from("<I", data, sub_offset)
		if val in tbl:
			val_offset = offset + sub_offset
			text = tbl[val]
			lines = text.split("\n", 1)
			first_line = lines[0]
			if len(lines) > 1 or len(first_line) > 140:
				text = first_line[:140] + '…'
			else:
				text = first_line
			print('%4s %10d %10d -> %s' % (section or '', val_offset, val, text))

def _scan_strings(fp, area_size: int, tbl: dict, section = None):
	size   = area_size
	offset = fp.tell()

	if area_size >= 8:
		head = fp.read(8)

		if len(head) == 8:
			magic, size = struct.unpack("<4sI", head)
			if size + 8 <= area_size and is_magic(magic):
				rem = size
				size += 8

				if magic == b'STRG' or magic == b'AUDO' or magic == b'TXTR':
					# skip
					fp.seek(rem, 1)
					return size
				else:
					magic = magic.decode()
					while rem > 0:
						child_size = _scan_strings(fp, rem, tbl, magic)
						rem -= child_size

				return size
			else:
				fp.seek(-8, 1)
				data = fp.read(area_size)
				_scan_strings_in_data(offset, data, section, tbl)
				size = area_size
		else:
			#raise Exception("%s %d area_size=%d len(head)=%d" % (section, offset, area_size, len(head)))
			_scan_strings_in_data(offset, head, section, tbl)
			size = len(head)
	else:
		fp.seek(area_size, 1)

	return size

def scan_strings(fp):
	fp.seek(0, 2)
	file_size = fp.tell()
	fp.seek(0, 0)

	tbl, size = read_strings(fp, file_size)
	fp.seek(0, 0)

#	for offset in sorted(tbl):
#		print("%10d -> %r" % (offset, tbl[offset]))
	
	_scan_strings(fp, file_size, tbl)


if __name__ == '__main__':
	import argparse

	parser = argparse.ArgumentParser()

	parser.add_argument('archives', nargs='*')
	parser.add_argument('--wine', action='store_true')

	args = parser.parse_args()

	if args.archives:
		archives = args.archives
	elif args.wine:
		archives = [find_archive_wine()]
	else:
		archives = [find_archive()]

	for archive in archives:
		print('archive:', archive)
		with open(archive,"rb") as fp:
			scan_strings(fp)
		print()
