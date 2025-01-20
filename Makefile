CC := g++
C_FLAGS := -std=c++20 `pkg-config --cflags --libs protobuf` -pthread
DEV_C_FLAGS := -g3 -Wall -Wextra -pedantic -std=c++20 `pkg-config --cflags --libs protobuf` -pthread \
	-fvisibility=hidden -fno-strict-overflow -Wno-strict-overflow \
	-funwind-tables -fasynchronous-unwind-tables -rdynamic -fno-dwarf2-cfi-asm -fvar-tracking-assignments \
	-Wduplicated-branches -Wduplicated-cond -Wshadow -Wfloat-equal -Wold-style-cast -Wzero-as-null-pointer-constant \
	-Wuseless-cast -Wextra-semi -Woverloaded-virtual -Wcast-qual -Wstrict-null-sentinel -Wformat-security -Wformat-signedness \
	-Wmissing-declarations -Wformat=2 -Wlogical-op -Wmissing-include-dirs -Wno-old-style-cast\
	-Wnull-dereference -Wpointer-arith -Wredundant-decls -Wswitch-enum -Wundef -Wuninitialized -Wwrite-strings -fno-common \
	-fprofile-exclude-files=/usr/.* -Wno-analyzer-use-of-uninitialized-value \
	-fsanitize=address,leak,undefined,null,return,signed-integer-overflow -fsanitize-trap=undefined -fno-sanitize-recover=all

FS_FLAGS := -lfuse3 -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=31
SERVER_FLAGS := 
COMMON := common/log.cpp common/io.cpp common/header.cpp
SERVER_FILES := server/tcp.cpp server/fs.cpp server/lsp.cpp
FS_FILES := filesystem/tcp.cpp filesystem/fs.cpp filesystem/log.cpp filesystem/lsp.cpp
PROTO := proto/messages.proto

UNIT_FLAGS := -g3 -Wall -Wextra -pedantic -std=c++20 `pkg-config --cflags --libs protobuf` -pthread `pkg-config --cflags catch2-with-main`
ACCEPTANCE_FLAGS := -g3 -Wall -Wextra -pedantic -std=c++20 `pkg-config --cflags catch2-with-main`
TEST_LIBS := `pkg-config --libs catch2-with-main`
UNIT_FILES := $(wildcard tests/unit/*.cpp)
ACCEPTANCE_FILES := $(wildcard tests/acceptance/*.cpp)

all: build

.PHONY: debug
debug: C_FLAGS = $(DEV_C_FLAGS)
debug: build

.PHONY: build
build: filesystem server

.PHONY: install
install: filesystem-install server-install

.PHONY: clean
clean: filesystem-clean server-clean proto-clean test-clean

.PHONY: filesystem-run
filesystem-run: filesystem 
	./filesystem/filesystem --host=127.0.0.1 -f mount-dir/

.PHONY: filesystem-install
filesystem-install: filesystem
	cp filesystem/filesystem /usr/local/bin/tea-fs

.PHONY: filesystem 
filesystem: filesystem/filesystem

.PHONY: filesystem-debug
filesystem-debug: C_FLAGS = $(DEV_C_FLAGS)

filesystem/filesystem: proto/proto.pb.o
	$(CC) $(C_FLAGS) $(FS_FLAGS) -o $@ filesystem/main.cpp $(COMMON) $(FS_FILES) proto/proto.pb.o

.PHONY: filesystem-clean
filesystem-clean:
	rm -f filesystem/filesystem

.PHONY: server-install
server-install: server
	cp server/server /usr/local/bin/tea-server

.PHONY: server-run
server-run: server
	./server/server project-dir

.PHONY: server
server: server/server

.PHONY: server-debug
server-debug: C_FLAGS = $(DEV_C_FLAGS)

server/server: proto/proto.pb.o
	$(CC) $(C_FLAGS) $(SERVER_FLAGS) -o $@ server/main.cpp $(COMMON) $(SERVER_FILES) proto/proto.pb.o

.PHONY: server-clean
server-clean:
	rm -f server/server

.PHONY: proto
proto: proto/proto.pb.o

proto/proto.pb.o:
	protoc -I=proto/ --cpp_out=proto/ $(PROTO)
	$(CC) $(CFLAGS) `pkg-config --cflags --libs protobuf` -c proto/*.pb.cc -o proto/proto.pb.o

.PHONY: proto-clean
proto-clean:
	rm -rf proto/proto.pb.o proto/*.pb.*

format: 
	clang-format -i proto/*.proto **/*.h **/*.cpp

check-format:
	clang-format --dry-run --Werror proto/*.proto **/*.h **/*.cpp

.PHONY: unit-run
unit-run: unit
	./tests/unit-runner

.PHONY: unit 
unit: proto/proto.pb.o
	$(CC) $(UNIT_FLAGS) $(FS_FLAGS) -o tests/unit-runner $(UNIT_FILES) $(COMMON) $(SERVER_FILES) $(FS_FILES) proto/proto.pb.o $(TEST_LIBS)

.PHONY: acceptance-run
acceptance-run: acceptance
	./tests/acceptance-runner

.PHONY: acceptance
acceptance: proto/proto.pb.o
	$(CC) $(ACCEPTANCE_FLAGS) -o tests/acceptance-runner $(ACCEPTANCE_FILES) $(TEST_LIBS)

.PHONY: test-clean
test-clean:
	rm -f tests/unit-runner
	rm -f tests/acceptance-runner
