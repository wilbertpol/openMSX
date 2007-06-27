# $Id$

include build/node-start.mk

SRC_HDR:= \
	File \
	FileContext \
	FileBase \
	LocalFile \
	FileOperations \
	CompressedFileAdapter \
	GZFileAdapter \
	ZipFileAdapter \
	ReadDir \
	FilePool \
	PreCacheFile

HDR_ONLY:= \
	FileException

include build/node-end.mk

