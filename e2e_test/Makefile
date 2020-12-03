default: test

include ../common.mk

test: .FORCE stop_docker
	./ip_assignment_on_qemu.py
	./ping_to_router_on_qemu.py
	./udp_client.py
	echo "All End-to-end tests PASSed"

.PHONY: .FORCE
