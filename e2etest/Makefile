default: test

include ../common.mk

.PHONY: presubmit test

presubmit:
	cargo run

test:
	cargo run
	make -C ..
	make -C .. apps
	./http_client.py
	./ip_assignment_on_qemu.py
	./ping_to_router_on_qemu.py
	./udp_client.py
	./udp_server.py
	echo "All End-to-end tests PASSed"

