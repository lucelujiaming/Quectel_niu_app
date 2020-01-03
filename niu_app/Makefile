PWD := $(shell pwd)


.PHONY: all

all:
	make -C $(PWD)/niu_common/libmga/mbedtls
	make -C $(PWD)/niu_common/libmga
	make -C $(PWD)/niu_mgmt
	make -C $(PWD)/niu_daemon

clean:
	rm -rf $(PWD)/niu_mgmt/.objs
	rm -rf $(PWD)/niu_mgmt/niu_mgmtd
	rm -rf $(PWD)/niu_daemon/niu_daemon
	rm -rf $(PWD)/niu_daemon/.objs
	rm -rf $(PWD)/niu_common/libmga/mbedtls/bin/*
	rm -rf $(PWD)/niu_common/libmga/mbedtls/src/*.o
	rm -rf $(PWD)/niu_common/libmga/libmbedagps.a
	rm -rf $(PWD)/niu_common/libmga/.objs

