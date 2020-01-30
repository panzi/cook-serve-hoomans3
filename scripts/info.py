#!/usr/bin/env python3

import struct
from game_maker import *

def is_magic(magic):
	return magic.isalnum()

def _dump_TXTR(fp, area_size, nesting):
	start_offset = fp.tell()
	end_offset = start_offset + area_size
	count, = struct.unpack("<I", fp.read(4))
	data = fp.read(4 * count)
	info_offsets = struct.unpack("<%dI" % count, data)
	file_infos = []
	indent1 = " " * (nesting * 3)
	indent2 = " " * (10 - (nesting * 3))

	for offset in info_offsets:
		if offset < start_offset or offset + 8 > end_offset:
			raise FileFormatError("illegal TXTR info offset: %d" % offset)
		fp.seek(offset, 0)
		data = fp.read(4 * 3)
		info = struct.unpack("<III", data)
		file_infos.append(info)

	print("TXTR count:", len(info_offsets))
	for index, (unknown1, unknown2, offset) in enumerate(file_infos):
		if offset < start_offset or offset > end_offset:
			raise FileFormatError("illegal TXTR data offset: %d" % offset)
		fp.seek(offset, 0)
		try:
			info = parse_png_info(fp)
		except FileFormatError:
			fp.seek(offset, 0)
			details = "first bytes: %r" % fp.read(4)
			print("%s(data)%s %10d %10s %4d %d %d %s" % (indent1, indent2, offset, "N/A", index, unknown1, unknown2, details))
		else:
			details = "%dx%d" % (info.width, info.height)
			if offset + info.filesize > end_offset:
				raise FileFormatError("PNG file at offset %d overflows TXTR data section end" % offset)
			print("%s PNG  %s %10d %10d %4d %d %d %s" % (indent1, indent2, offset, info.filesize, index, unknown1, unknown2, details))

	fp.seek(end_offset, 0)

def _dump_AUDO(fp, area_size, nesting):
	start_offset = fp.tell()
	end_offset = start_offset + area_size
	count, = struct.unpack("<I", fp.read(4))
	data = fp.read(4*count)
	offsets = struct.unpack("<%dI" % count, data)
	indent1 = " " * (nesting * 3)
	indent2 = " " * (10 - (nesting * 3))

	for offset in offsets:
		fp.seek(offset, 0)
		data = fp.read(4)
		offset += 4
		size, = struct.unpack("<I",data)
		magic = fp.read(min(4,size))
		fp.seek(offset, 0)

		info = None
		try:
			if magic == b'RIFF':
				info = parse_riff_info(fp)
			elif magic == b'OggS':
				info = parse_ogg_info(fp)
		except FileFormatError as e:
			print("at offset %d: %s" % (offset, e))

		if info is None:
			what     = "(data)"
			details  = "first bytes: %r" % magic
		else:
			what     = " %-4s " % info.what
			filesize = info.filesize
			details  = "filesize: %10d %s" % (filesize, info.details)

			if filesize > size:
				raise FileFormatError("%s file at offset %d overflows AUDO data section end" % (
					info.what, offset))

		print("%s%s%s %10d %10d %s" % (indent1, what, indent2, offset, size, details))

	fp.seek(end_offset,0)

all_tpag_offsets = set()

def _dump_SPRT(fp,area_size,nesting):
	start_offset = fp.tell()
	end_offset = start_offset + area_size
	indent1 = " " * (nesting * 3)
	indent2 = " " * (10 - (nesting * 3))

	count, = struct.unpack("<I", fp.read(4))
	data = fp.read(4 * count)
	offsets = struct.unpack("<%dI" % count, data)

	field_count = 20
	record_size = field_count * 4
	pfmt = "%s(data)%s %10d %10d" + (' %5d' * field_count) + ' '
	sfmt = '<' + ('I' * field_count)
	for index, offset in enumerate(offsets):
		size = (offsets[index + 1] if index + 1 < len(offsets) else end_offset) - offset
		fp.seek(offset, 0)
		data = fp.read(record_size)
		vals = struct.unpack(sfmt, data)
		tpag_count = vals[-1]

		data = fp.read(tpag_count * 4)
		tpag_offsets = struct.unpack('<%dI' % tpag_count, data)
		all_tpag_offsets.update(tpag_offsets)

		strptr = vals[0]
		fp.seek(strptr - 4, 0)
		strlen, = struct.unpack('<I', fp.read(4))
		name = fp.read(strlen).decode()

		msg = pfmt % ((indent1, indent2, offset, size) + vals)
		msg += name
		if record_size > size:
			msg += " (assumed record size is too big)"
		print(msg)
		print('%s      %s                       %s' % (indent1, indent2, ' '.join(str(tpag_offset) for tpag_offset in tpag_offsets)))

	fp.seek(end_offset,0)

def _dump_BGND(fp,area_size,nesting):
	start_offset = fp.tell()
	end_offset = start_offset + area_size
	indent1 = " " * (nesting * 3)
	indent2 = " " * (10 - (nesting * 3))

	count, = struct.unpack("<I", fp.read(4))
	data = fp.read(4 * count)
	offsets = struct.unpack("<%dI" % count, data)

	pfmt = "%s(data)%s %10d %10d" + (" %5d" * 5) + ' %s'
	sfmt = '<IIIII'
	for index, offset in enumerate(offsets):
		size = (offsets[index + 1] if index + 1 < len(offsets) else end_offset) - offset
		fp.seek(offset, 0)
		data = fp.read(4 * 5)
		vals = struct.unpack(sfmt, data)
		strptr = vals[0]
		fp.seek(strptr - 4, 0)
		strlen, = struct.unpack('<I', fp.read(4))
		name = fp.read(strlen).decode()

		print(pfmt % ((indent1, indent2, offset, size) + vals + (name, )))

	fp.seek(end_offset,0)

def _dump_TPAG(fp,area_size,nesting):
	start_offset = fp.tell()
	end_offset = start_offset + area_size
	indent1 = " " * (nesting * 3)
	indent2 = " " * (10 - (nesting * 3))

	count, = struct.unpack("<I", fp.read(4))
	data = fp.read(4 * count)
	offsets = struct.unpack("<%dI" % count, data)

	pfmt = "%s(data)%s %10d %10d" + (' %5d' * 11)
	sfmt = '<' + ('H' * 11)
	for index, offset in enumerate(offsets):
		size = (offsets[index + 1] if index + 1 < len(offsets) else end_offset) - offset
		fp.seek(offset, 0)
		data = fp.read(2 * 11)
		vals = struct.unpack(sfmt, data)
		print(pfmt % ((indent1, indent2, offset, size) + vals))

	offsets = set(offsets)
	for offset in all_tpag_offsets:
		if offset not in offsets:
			print("ERROR: invalid TPAG offset read from SPRT:", offset)

	fp.seek(end_offset,0)

def _dump_OBJT(fp,area_size,nesting):
	start_offset = fp.tell()
	end_offset = start_offset + area_size
	indent1 = " " * (nesting * 3)
	indent2 = " " * (10 - (nesting * 3))

	count, = struct.unpack("<I", fp.read(4))
	data = fp.read(4 * count)
	offsets = struct.unpack("<%dI" % count, data)

	for index, offset in enumerate(offsets):
		size = (offsets[index + 1] if index + 1 < len(offsets) else end_offset) - offset
		print("%s(data)%s %10d %10d" % (indent1, indent2, offset, size))

	fp.seek(end_offset, 0)

def _dump_STRG(fp,area_size,nesting):
	start_offset = fp.tell()
	end_offset = start_offset + area_size
	indent1 = " " * (nesting * 3)
	indent2 = " " * (10 - (nesting * 3))

	count, = struct.unpack("<I", fp.read(4))
	data = fp.read(4 * count)
	offsets = struct.unpack("<%dI" % count, data)

	for index, offset in enumerate(offsets):
		fp.seek(offset, 0)
		data = fp.read(4)
		size, = struct.unpack("<I", data)
		data = fp.read(size).rstrip(b"\0")
		lines = data.decode().split("\n", 1)
		first_line = lines[0]
		if len(lines) > 1 or len(first_line) > 140:
			text = first_line[:140] + '…'
		else:
			text = first_line
		print("%s(data)%s %10d %10d %4d %s" % (indent1, indent2, offset, size, index, text))

	fp.seek(end_offset, 0)

GEN8_STRING_INDICES = {1, 2, 10, 25}

def _dump_GEN8(fp,area_size,nesting):
	indent1 = " " * (nesting * 3)
	indent2 = " " * (10 - (nesting * 3))
	start_offset = fp.tell()
	end_offset = start_offset + area_size
	data = fp.read(area_size)
	gen8 = struct.unpack("<%dI" % (area_size // 4), data)
	offset = start_offset
	for index, value in enumerate(gen8):
		msg = "%s      %s %10d %10d %10d" % (indent1, indent2, offset, 4, value)
		if index in GEN8_STRING_INDICES:
			fp.seek(value - 4, 0)
			data = fp.read(4)
			strlen, = struct.unpack("<I", data)
			data = fp.read(strlen)
			msg += " -> %s" % data.decode()
		print(msg)
		offset += 4

	fp.seek(end_offset, 0)

def _dump_info(fp,area_size,nesting):
	magic   = None
	size    = area_size
	offset  = fp.tell()
	indent1 = " " * (nesting * 3)
	indent2 = " " * (10 - (nesting * 3))

	if area_size >= 8:
		head = fp.read(8)

		if len(head) == 8:
			magic, size = struct.unpack("<4sI", head)
			if size + 8 <= area_size and is_magic(magic):
				rem = size
				nesting += 1
				size += 8
				print("%s %s %s %10d %10d" % (indent1, magic.decode(), indent2, offset, size))

				if magic == b'GEN8':
					_dump_GEN8(fp,rem,nesting)
				elif magic == b'TXTR':
					_dump_TXTR(fp,rem,nesting)
				elif magic == b'AUDO':
					_dump_AUDO(fp,rem,nesting)
				elif magic == b'SPRT':
					_dump_SPRT(fp,rem,nesting)
				elif magic == b'BGND':
					_dump_BGND(fp,rem,nesting)
				elif magic == b'TPAG':
					_dump_TPAG(fp,rem,nesting)
#				elif magic == b'OBJT':
#					_dump_OBJT(fp,rem,nesting)
				elif magic == b'STRG':
					_dump_STRG(fp,rem,nesting)
				else:
					while rem > 0:
						nondata, child_magic, child_size = _dump_info(fp,rem,nesting)
						rem -= child_size

				return True, magic, size
			else:
				fp.seek(area_size - 8, 1)
				size = area_size
		else:
			size = len(head)
	else:
		fp.seek(area_size, 1)

	print("%s(data)%s %10d %10d" % (indent1, indent2, offset, size))
	return False, magic, size

def dump_info(fp):
	fp.seek(0,2)
	file_size = fp.tell()
	fp.seek(0,0)

	print('type                 offset       size details')
	nondata, magic, size = _dump_info(fp,file_size,0)
	if not nondata:
		raise ValueError("no file magic at start of file")

	if size < file_size:
		raise ValueError("file size underflow: file size = %d, read size = %d" % (
			file_size, size))

	elif size > file_size:
		raise ValueError("file size overflow: file size = %d, read size = %d" % (
			file_size, size))

if __name__ == '__main__':
	import sys

	if len(sys.argv) > 1:
		archive = sys.argv[1]

		if archive == '--wine':
			archive = find_archive_wine()
	else:
		archive = find_archive()

	with open(archive,"rb") as fp:
		dump_info(fp)
