APPS=\
	 argstest/argstest.bin \
	 fizzbuzz/fizzbuzz.bin \
	 hello/hello.bin \
	 httpclient/httpclient.bin \
	 httpserver/httpserver.bin \
	 pi/pi.bin \
	 ping/ping.bin \
	 readtest/readtest.bin \
	 udpserver/udpserver.bin \
	 udpclient/udpclient.bin \
	 browser/browser.bin \
	 # dummy line

default: $(APPS)

.FORCE :

deploy : $(APPS) .FORCE
	cp $(APPS) ../mnt/

%.bin : .FORCE
	make -C $(dir $@)

%_clean : .FORCE
	make -C $(dir $*) clean

clean: $(addsuffix _clean, $(APPS))

test: .FORCE
	# check if all apps can be built successfully
	make clean
	make


