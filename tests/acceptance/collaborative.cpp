#include <catch2/catch_test_macros.hpp>
#include <unistd.h>
#include <fcntl.h>

TEST_CASE("offset per client") {
    int fd1 = open("mount-dir/offset.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd1 >= 0);
    int err = write(fd1, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec a diam lectus. Sed sit amet ipsum mauris. Maecenas congue lig", 120);
    REQUIRE(err == 120);
    int fd2 = open("mount-dir/offset.txt", O_RDWR, 0644);
    REQUIRE(fd2 >= 0);
    char buffer[5];
    err = read(fd2, buffer, 5);
    REQUIRE(err == 5);
    REQUIRE(strncmp(buffer, "Lorem", 5) == 0);
    err = write(fd2, "test", 4);
    REQUIRE(err == 4);
    err = lseek(fd2, 0, SEEK_SET);
    REQUIRE(err == 0);
    char read_buffer[9];
    err = read(fd2, read_buffer, 9);
    REQUIRE(strncmp(read_buffer, "Loremtest", 9) == 0);
    close(fd1);
    close(fd2);
    remove("project-dir/offset.txt");
}

