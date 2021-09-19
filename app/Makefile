include ../common.mk

APPS=\
	 argstest/argstest.bin \
	 browser-rs/browser-rs.bin \
	 browser/browser.bin \
	 dig/dig.bin \
	 fizzbuzz/fizzbuzz.bin \
	 guitest/guitest.bin \
	 guitestrust/guitestrust.bin \
	 hello/hello.bin \
	 httpclient/httpclient.bin \
	 httpserver/httpserver.bin \
	 pi/pi.bin \
	 ping/ping.bin \
	 readtest/readtest.bin \
	 rusttest/rusttest.bin \
	 shelium/shelium.bin \
	 udpclient/udpclient.bin \
	 udpserver/udpserver.bin \
	 a/a.bin \
	 # dummy line

default: $(APPS)

%.bin : .FORCE
	make -C $(dir $@)

%_clean : .FORCE
	make -C $(dir $*) clean

%_test : .FORCE
	make -C $(dir $*) test

.PHONY : .FORCE clean deply test

clean: $(addsuffix _clean, $(APPS))

deploy : $(APPS)
	cp $(APPS) ../mnt/

test: $(addsuffix _test, $(APPS))
	@echo "All app test PASSed."
