# $Id:$

include build/node-start.mk

SRC_HDR:= \
	CRC16 \
	CircularBuffer \
	Date \
	HostCPU \
	Math \
	MemoryOps \
	StringOp \
	Unicode \
	sha1

HDR_ONLY:= \
	FixedPoint \
	Subject Observer \
	ScopedAssign \
	checked_cast \
	likely \
	noncopyable \
	shared_ptr

include build/node-end.mk
