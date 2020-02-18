CONCEPTS
========

This file describes the binary format of RNTuple data.
RNTuple data is stored as a set of blobs.
The blobs are typically compressed.
The container for the blobs can be a file or an object store.
RNTuple data are a series of C++ objects ("entries", "rows", or "events") in columnar layout.
The C++ objects are decomposed into simple types and vectors of simple types ("columns").
For instance, to store entries of the object

    struct Event
       float px;
       float py;
       float pz;
    };

there would three columns of type float, which store one member each ("columnar").
Columns are partitioned into pages of typically a few tens of kilobyte.
A cluster is a virtual unit and describes a collection of pages that contain all the data for a certain entry range.
Clusters are typically tens of megabytes in size.

There are the blob categories _header_, _page_, _cluster summary_, and _footer_.
The categories header, cluster summary, and footer are the _meta-data blobs_.

Header
------
Every RNTuple has exactly one header.
The header contains the schema of the data, that is the fields and columns.

Page
----
A page contains consecutive elements of a column.
Page elements all of the same fundamental type.
There is a fixed set of such types.
Pages can differ in their number of elements.
A page is not stored together with its meta-data.
A page comprises only the bare byte range of (compressed) data and requires a cluster summary in order to be interpreted.

Cluster Summary
---------------
A cluster summary contains for a certain set of clusters location informaton (e.g., file offsets or object key)
of all the pages in that cluster.

Footer
------
Every RNTuple has exactly one footer.
The footer contains the locations of the cluster summaries.


PHYSICAL LAYOUT
===============

This section describes the format of the different blob types.
Note that it does not describe how the RNTuple blobs are embedded in a container format, e.g. in a TFile.

Meta-data types
---------------

RNTuple meta-data blobs use a fixed set of types
consisting of little-endian integers of different size, collections, and extensible records.

| Type               | Description
|--------------------|------------
| `char`             | Single byte, ASCII character
| `byte`             | Single byte of an opaque byte stream
| `[u]int8/16/32/64` | Signed/unsigned, little endian integer of 1, 2, 4, or 8 bytes
| `collection`       | A `uint32` giving the items of elements followed by the items
| `string`           | A `collection` of `char` items
| `record`           | A `uint32` giving the length in bytes followed by the data

Note that records can be extended with new fields without breaking older readers.


Envelope
--------

Meta-data blobs are encoded in an _envelope_.
The envelope has the following layout

  - Style
  - 1 byte compression, 2 bytes reserved
  - 32bit compressed size
  - 32bit uncompressed size
  - Contents
  - CRC32


Page Layout
-----------

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
