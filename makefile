MAKEFLAGS += --no-print-directory
.PHONY: build_start

build_start: build start

clean:
	rm -rf build bin

build:
	mkdir -p build
	@if [ -d bin ]; then \
		echo "Removing bin folder";\
    	rm -rf bin;\
  	fi;\
  	cd build;\
  	cmake .. -DCMAKE_BUILD_TYPE=Debug;\
  	make -j$(shell nproc)

start:
	cd bin;\
	./viewer