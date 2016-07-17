CFLAGS = -I/usr/local/opt/openssl/include/ -O3 -flto -fomit-frame-pointer
LDFLAGS = -L/usr/local/opt/openssl/lib -lcrypto

all: forceitbox

forceitbox: forceitbox.c
	$(CC) $(CFLAGS) -o $@  $+ $(LDFLAGS) 
	strip $@

clean:
	rm -f forceitbox