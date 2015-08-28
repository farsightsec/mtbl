[![Build Status](https://travis-ci.org/farsightsec/mtbl.png?branch=master)](https://travis-ci.org/farsightsec/mtbl)

`mtbl`: immutable sorted string table library
=============================================

Introduction
------------

`mtbl` is a C library implementation of the Sorted String Table (SSTable) data
structure, based on the SSTable implementation in the open source [Google
LevelDB library](http://code.google.com/p/leveldb/). An SSTable is a file
containing an immutable mapping of keys to values. Keys are stored in sorted
order, with an index at the end of the file allowing keys to be located quickly.

`mtbl` is not a database library. It does not provide an updateable key-value
data store, but rather exposes primitives for creating, searching and merging
SSTable files. Unlike databases which use the SSTable data structure internally
as part of their data store, management of SSTable files -- creation, merging,
deletion, combining of search results from multiple SSTables -- is left to the
discretion of the `mtbl` library user.

`mtbl` SSTable files consist of a sequence of data blocks containing sorted
key-value pairs, where keys and values are arbitrary byte arrays. Data blocks
are optionally compressed using the
[zlib](http://www.zlib.net/), [LZ4](https://github.com/Cyan4973/lz4), or
[Snappy](http://google.github.io/snappy/) compression algorithms. The data
blocks are followed by an index block, allowing for fast searches over the
keyspace.

The basic `mtbl` interface is the writer, which receives a sequence of key-value
pairs in sorted order with no duplicate keys, and writes them to data blocks in
the SSTable output file. An index containing offsets to data blocks and the last
key in each data block is buffered in memory until the writer object is closed,
at which point the index is written to the end of the SSTable file. This allows
SSTable files to be written in a single pass with sequential I/O operations
only.
    
Once written, SSTable files can be searched using the `mtbl` reader interface.
Searches can retrieve key-value pairs based on an exact key match, a key prefix
match, or a key range. Results are retrieved using a simple iterator interface.

The `mtbl` library also provides two utility interfaces which facilitate a
sort-and-merge workflow for bulk data loading. The sorter interface receives
arbitrarily ordered key-value pairs and provides them in sorted order, buffering
to disk as needed. The merger interface reads from multiple SSTables
simultaneously and provides the key-value pairs from the combined inputs in
sorted order. Since `mtbl` does not allow duplicate keys in an SSTable file,
both the sorter and merger interfaces require a caller-provided merge function
which will be called to merge multiple values for the same key. These interfaces
also make use of sequential I/O operations only.
