CC := g++
C_FLAGS := -g3 -Wall -Wextra -pedantic -std=c++20 `pkg-config --cflags --libs protobuf` -pthread
FS_FLAGS := -lfuse3 -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=30
SERVER_FLAGS := 
COMMON := common/log.cpp
SERVER_FILES := 
FS_FILES :=
PROTO := proto/packets.proto

TEST_FLAGS := `pkg-config --cflags catch2-with-main`
TEST_LIBS := `pkg-config --libs catch2-with-main`
TEST_DIR := tests/catch2
TEST_FILES := $(wildcard $(TEST_DIR)/*.cpp)

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
	$(CC) $(CFLAGS) -c proto/*.pb.cc -o proto/proto.pb.o

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
	$(CC) $(C_FLAGS) $(TEST_FLAGS) -o tests/test-runner $(TEST_FILES) $(COMMON) proto/proto.pb.o $(TEST_LIBS)

.PHONY: test-clean
test-clean:
	rm -f tests/test-runner
