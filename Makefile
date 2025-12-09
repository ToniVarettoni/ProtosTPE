bin:
	@mkdir -p bin

server: bin
	@$(MAKE) -C src/server all

client: bin
	@$(MAKE) -C src/client all

all: server client manager

clean:
	$(MAKE) -C src/server clean
	$(MAKE) -C src/client clean
	$(MAKE) -C src/manager clean

.PHONY: bin server client manager clean all