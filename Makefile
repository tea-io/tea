CC := g++
C_FLAGS := -g3 -Wall -Wextra -pedantic -std=c++20 `pkg-config --cflags --libs protobuf` -pthread \
	-fvisibility=hidden -fno-strict-overflow -Wno-strict-overflow \
	-funwind-tables -fasynchronous-unwind-tables -rdynamic -fno-dwarf2-cfi-asm -fvar-tracking-assignments \
	-Wduplicated-branches -Wduplicated-cond -Wshadow -Wfloat-equal -Wold-style-cast -Wzero-as-null-pointer-constant \
	-Wuseless-cast -Wextra-semi -Woverloaded-virtual -Wcast-qual -Wstrict-null-sentinel -Wformat-security -Wformat-signedness \
	-Wmissing-declarations -Wformat=2 -Wlogical-op -Wmissing-include-dirs \
	-Wnull-dereference -Wpointer-arith -Wredundant-decls -Wswitch-enum -Wundef -Wuninitialized -Wwrite-strings -fno-common \
	-fanalyzer -fanalyzer-transitivity \
	-fsanitize=address,leak,undefined,null,return,signed-integer-overflow -fsanitize-trap=undefined -fno-sanitize-recover=all

FS_FLAGS := -lfuse3 -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=30
SERVER_FLAGS := 
COMMON := common/log.cpp common/io.cpp common/header.cpp
SERVER_FILES := server/tcp.cpp 
FS_FILES := filesystem/tcp.cpp 
PROTO := proto/messages.proto

TEST_FLAGS := -g3 -Wall -Wextra -pedantic -std=c++20 `pkg-config --cflags --libs protobuf` -pthread `pkg-config --cflags catch2-with-main`
TEST_LIBS := `pkg-config --libs catch2-with-main`
TEST_DIR := tests
TEST_FILES := $(wildcard $(TEST_DIR)/**/*.cpp $(TEST_DIR)/*.cpp)

all: build

.PHONY: build
build: filesystem server test

.PHONY: clean
clean: filesystem-clean server-clean proto-clean test-clean

.PHONY: filesystem-run
filesystem-run: filesystem 
	./filesystem/filesystem -f test/

.PHONY: filesystem 
filesystem: filesystem/filesystem

filesystem/filesystem: proto/proto.pb.o
	$(CC) $(C_FLAGS) $(FS_FLAGS) -o $@ filesystem/main.cpp $(COMMON) $(FS_FILES) proto/proto.pb.o

.PHONY: filesystem-clean
filesystem-clean:
	rm -f filesystem/filesystem

.PHONY: server-run
server-run: server
	./server/server ~/name

.PHONY: server
server: server/server

server/server: proto/proto.pb.o
	$(CC) $(C_FLAGS) $(SERVER_FLAGS) -o $@ server/main.cpp $(COMMON) $(SERVER_FILES) proto/proto.pb.o

.PHONY: server-clean
server-clean:
	rm -f server/server

.PHONY: proto
proto: proto/proto.pb.o

proto/proto.pb.o:
	protoc -I=proto/ --cpp_out=proto/ $(PROTO)
	$(CC) $(CFLAGS) -g `pkg-config --cflags --libs protobuf` -c proto/*.pb.cc -o proto/proto.pb.o

.PHONY: proto-clean
proto-clean:
	rm -rf proto/proto.pb.o proto/*.pb.*

format: 
	clang-format -i proto/*.proto **/*.h **/*.cpp

check-format:
	clang-format --dry-run --Werror proto/*.proto **/*.h **/*.cpp

.PHONY: test-run
test-run: test
	./tests/test-runner

.PHONY: test
test: $(TEST_FILES) proto/proto.pb.o
	$(CC) $(TEST_FLAGS) -o tests/test-runner $(TEST_FILES) $(COMMON) $(SERVER_FILES) $(FS_FILES) proto/proto.pb.o $(TEST_LIBS)

.PHONY: test-clean
test-clean:
	rm -f tests/test-runner
