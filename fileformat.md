Game Maker Archive File Format
==============================

This document describes parts of the Game Maker archive format as reverse
engineered from the game data shipped with Cook, Serve, Delicious! All of this
was reverse engineered using a hex editor and Python. No code was analyzed.

Because this is all from one game I don't actually know if there are any game
specific parts or if this applies generally to all Game Maker games.

**TODO:** Find out where a file format version or something is stored, because
there are differences between the file formats used in Cook, Serve, Delicious!
1 and 2.

Overall Structure
-----------------

Byte-order: Little-Endian

    ┌──────────────────────────────────────────┐
    │                                          │
    │  Root Chunk Magic: "FORM"                │
    │  Root Chunk Content Size                 │
    │                                          │
    │  ┌────────────────────────────────────┐  │
    │  │                                    │  │
    │  │  Chunk Magic                       │  │
    │  │  Chunk Content Size                │  │
    │  │                                    │  │
    │  │  ┌──────────────────────────────┐  │  │
    │  │  │                              │  │  │
    │  │  │  (Content)                   │  │  │
    │  │  │                              │  │  │
    │  │  └──────────────────────────────┘  │  │
    │  ├────────────────────────────────────┤  │
    │  │                                    │  │
    │  │  ...                               │  │
    │  │                                    │  │
    │  └────────────────────────────────────┘  │
    │                                          │
    └──────────────────────────────────────────┘

This looks like each chunk where self-contained, but there are absolute offsets
in many chunks, some even pointing into other chunks.

Obviously this file format is somewhat based on IFF, except it uses
little-endian encoding of integers instead big-endian.

Chunks
------

Basic chunk layout:

    Offset  Size  Type        Description
         0     4  char[4]     chunk magic
         4     4  uint32_t    chunk content size (N)
         8     N  uint8_t[N]  content

This means the overall size of a chuck is N + 8.

Chunk hierarchy as found in the Cook, Serve, Delicious! archive (data.win or
game.unx).

* FORM
  * GEN8
  * OPTN
  * EXTN
  * SOND
  * SPRT
  * BGND
  * PATH
  * SCPT
  * SHDR
  * FONT
  * TMLN
  * OBJT
  * ROOM
  * DAFL
  * TPAG
  * CODE
  * VARI
  * FUNC
  * STRG
  * TXTR
  * AUDO
  * LANG
  * GLOB

### FORM

Main chunk spanning the whole file, containing all the other chunks. None of the
other chunks seem to contain sub-chunks.

### GEN8

Some meta data about the game archive. These are the values from game.unx. Some
of the values are obviously absolute offsets pointing into the string table
(STRG).

     Offset  Size  Type               Value
          0     4  ?                   4353
          4     4  uint32_t        21965010 -> "CSD3_Foodtruck"
          8     4  uint32_t        21965029 -> "default"
         12     4  ?                 100033
         16     4  ?               10000000
         20     4  ?                      0
         24     4  ?                      0
         28     4  ?                      0
         32     4  ?                      0
         36     4  ?                      0
         40     4  uint32_t        21965010 -> "CSD3_Foodtruck"
         44     4  ?                      2
         48     4  ?                      0
         52     4  ?                      0
         56     4  ?                      0
         60     4  ?                   1920
         64     4  ?                   1080
         68     4  ?                  16440
         72     4  ?             1557395764
         76     4  ?                   2806
         80     4  ?                      0
         84     4  ?                      0
         88     4  ?                      0
         92     4  ?             1580336379
         96     4  ?                      0
        100     4  uint32_t        21965041 -> "Cook, Serve, Delicious! 3?!"
        104     4  ?                      0
        108     4  ?                      0
        112     4  ?             1845440180
        116     4  ?             2017505949
        120     4  ?             4293967266
        124     4  ?                  51268
        128     4  ?                      9
        132     4  ?                      0
        136     4  ?                      1
        140     4  ?                      2
        144     4  ?                      3
        148     4  ?                      4
        152     4  ?                      5
        156     4  ?                      6
        160     4  ?                      7
        164     4  ?                      8
        168     4  ?             1011005649
        172     4  ?              107019637
        176     4  ?             1296761863
        180     4  ?             1294314652
        184     4  ?             3050135175
        188     4  ?             2905032698
        192     4  ?              766865593
        196     4  ?             2121004061
        200     4  ?             2060813204
        204     4  ?             2120607617
        208     4  ?             1114636288
        212     4  ?                      1
        216     4  ?              773649331
        220     4  ?             1240623512
        224     4  ?             1482811789
        228     4  ?             2470682114
        232     4  ?                      0
        236     4  ?                      0

### STRG

String table. Other chunks point right into this one whenever they refer to a
simple string (so whole scripts aren't stored in this, only identifier names and
such).

     Offset  Size  Type         Description
          0     4  char[4]      chunk magic: 'STRG'
          4     4  uint32_t     chunk content size
          8     4  uint32_t     number of strings (N)
         12   4*N  uint32_t[N]  absolute offsets of strings
     12+4*N     ?  String[N]    strings

Because of the absolute offsets I wouldn't expect the strings to actually start
right after the offset table.

#### String

Absolute offsets to strings in other chunks point directly to the characters
(not to the string length).

     Offset  Size  Type         Description
          0     4  uint32_t     string length (N) excluding the terminating NULL
          4     N  char[N]      characters
        4+N     1  char         '\0'

### SPRT

Sprite info.

    Offset  Size  Type         Description
         0     4  char[4]      chunk magic: 'SPRT'
         4     4  uint32_t     chunk body size
         8     4  uint32_t     number of sprites (N)
        12   4*N  uint32_t[N]  sprite offsets

#### Sprite

    Offset  Size  Type         Description
         0     4  uint32_t     sprite name (offset into STRG)
         4    60  ?            ?
        76     4  uint32_t     number of sub-sprites (N)
        80     4  uint32_t[N]  offsets into TPAG
    80+N*4     ?  ?            some more data, dunno

### TPAG

Contains coordinates for sprites in textures files.

    Offset  Size  Type         Description
         0     4  char[4]      chunk magic: 'TPAG'
         4     4  uint32_t     chunk body size
         8     4  uint32_t     number of records (N)
        12   4*N  uint32_t[N]  record offsets

#### Record

    Offset  Size  Type         Description
         0     2  uint16_t     x
         2     2  uint16_t     y
         4     2  uint16_t     width
         6     2  uint16_t     height
         8     2  uint16_t     maybe an x-offset? (mostly 0)
        10     2  uint16_t     maybe an y-offset? (mostly 0)
        12     2  uint16_t     maybe a sub-width?
        14     2  uint16_t     maybe a sub-height?
        16     2  uint16_t     maybe an enclosing-width?
        18     2  uint16_t     maybe an enclosing-height?
        20     2  uint16_t     TXTR index

### TXTR

I brute-force searched all the other chunks and haven't found any values that
are offsets to any file data chunks in this chunk.

     Offset  Size  Type         Description
          0     4  char[4]      chunk magic: 'TXTR'
          4     4  uint32_t     chunk content size
          8     4  uint32_t     number of textures (N)
         12   4*N  uint32_t[N]  absolute FileInfo offsets
     12+8*N   8*N  FileInfo[N]  FileInfos
    12+12*N     ?  N/A          zero padding (35 bytes in this case)
          ?     ?  uint8_t[?]   file data

#### FileInfo

     Offset  Size  Type         Description
          0     4  uint32_t     most of the time 1, sometimes 0 in Cook, Serve,
                                Delicious! 2 (maybe a file type? "enabled" flag?)
          4     4  uint32_t     always 0 in Cook, Serve, Delicious! 2
          8     4  uint32_t     absolute offset of file data

There is no information about the size of the texture files anywhere in the
archive file format. But all of the textures in Cook, Serve, Delicious! are PNG
files and thus it is easy to parse them out of the archive. A PNG file has a
certain signature and then consists of chunks, each containing a chunk-magic and
the last having the magic "IEND". See more about the PNG file format here:
http://www.w3.org/TR/PNG-Structure.html
http://www.w3.org/TR/PNG/

### AUDO

I brute-force searched all the other chunks and haven't found any values that
are offsets to any file data chunks in this chunk.

     Offset  Size  Type         Description
          0     4  char[4]      chunk magic: 'AUDO'
          4     4  uint32_t     chunk content size
          8     4  uint32_t     number of audio files (N)
         12   4*N  uint32_t[N]  absolute AudioFile offsets
     12+4*N     ?  AudioFile[N] AudioFiles

#### AudioFile

     Offset  Size  Type         Description
          0     4  uint32_t     size (N)
          4     N  uint8_t[N]   file data

The only audio file formats found in the Cook, Serve, Delicious! game archive
where RIFF WAVE and Ogg Vorbis.
