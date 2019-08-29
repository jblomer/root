CONCEPTS
========

Header, Cluster Summary, Checkpoint, Footer

PHYSICAL LAYOUT
===============

Types
-----

  - int, descriptor id (64bit int), string, collections, records

Envelope
--------

  - 1 byte style, 1 byte compression, 2 bytes reserved
  - 32bit copmressed size
  - 32bit uncompressed size
  - Contents
  - CRC32

Frame
-----

Wraps around a record
  - 2 byte minimum version
  - 2 byte type
  - 4 bytes length


HEADER
======

Fixed information: name, frame versions of writer (field, column, cluster)

Field
-----
  - field name
  - type name
  - type alias
  - field / type version number
  - number of columns
  - list of children field ids

Columns
-------
  - type
  - element size
  - is sorted
  - column order
  - column id implicitly given by order

Cluster Summary
===============

locator of previous cluster

List of clusters
  - cluster locator [relative to range of clusters]
  - List of column ranges / page ranges
  - Page types, page settings [offsets relative to start of cluster start]

Column range: number of elements (first element index implicit by order of clusters)

Per cluster and column:
  - Page setting: compression, crc32
  - Page type (medium type): files

  - Raw File: offset [crc32]
  - ROOT file: offset [crc32]
  - Object store: key name, size, crc32


Checkpoint
==========

~ pointer to last cluster summary

FOOTER
======

variable information: namespace + key --> value [string, bytes, int]
  - ROOT namespace reserved, otherwise open to user-provided metadata

list of cluster summaries end their entry ranges
