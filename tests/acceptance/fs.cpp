#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

TEST_CASE("stat") {
    SECTION("file") {
        int fd = open("project-dir/test.txt", O_RDWR | O_CREAT, 0644);
        REQUIRE(fd >= 0);
        close(fd);
        struct stat orginal_stat;
        int err = stat("project-dir/test.txt", &orginal_stat);
        REQUIRE(err == 0);
        struct stat new_stat;
        err = stat("mount-dir/test.txt", &new_stat);
        REQUIRE(err == 0);
        REQUIRE(orginal_stat.st_mode == new_stat.st_mode);
        REQUIRE(orginal_stat.st_size == new_stat.st_size);
        REQUIRE(orginal_stat.st_nlink == new_stat.st_nlink);
        REQUIRE(orginal_stat.st_atime == new_stat.st_atime);
        REQUIRE(orginal_stat.st_mtime == new_stat.st_mtime);
        REQUIRE(orginal_stat.st_ctime == new_stat.st_ctime);
        remove("project-dir/test.txt");
    }
    SECTION("dir") {
        int err = mkdir("project-dir/test-dir", 0755);
        REQUIRE(err == 0);
        struct stat orginal_stat;
        err = stat("project-dir/test-dir", &orginal_stat);
        REQUIRE(err == 0);
        struct stat new_stat;
        err = stat("mount-dir/test-dir", &new_stat);
        REQUIRE(err == 0);
        REQUIRE(orginal_stat.st_mode == new_stat.st_mode);
        REQUIRE(orginal_stat.st_size == new_stat.st_size);
        REQUIRE(orginal_stat.st_nlink == new_stat.st_nlink);
        REQUIRE(orginal_stat.st_atime == new_stat.st_atime);
        REQUIRE(orginal_stat.st_mtime == new_stat.st_mtime);
        REQUIRE(orginal_stat.st_ctime == new_stat.st_ctime);
        remove("project-dir/test-dir");
    }
    SECTION("enoent") {
        struct stat new_stat;
        int err = stat("mount-dir/not-exist", &new_stat);
        REQUIRE(err == -1);
        REQUIRE(errno == ENOENT);
    };
}
