#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <list>
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

TEST_CASE("open and close") {
    int fd = open("project-dir/test.txt", O_RDWR | O_CREAT, 0644);
    close(fd);
    fd = open("mount-dir/test.txt", O_RDWR, 0644);
    REQUIRE(fd >= 0);
    int err = close(fd);
    REQUIRE(err == 0);
    remove("project-dir/test.txt");
}

TEST_CASE("readdir") {
    mkdir("project-dir/test-dir", 0755);
    int fd = open("project-dir/test.txt", O_RDWR | O_CREAT, 0644);
    close(fd);
    // check readdir
    DIR *dir = opendir("mount-dir");
    REQUIRE(dir != nullptr);
    struct dirent *entry = readdir(dir);
    REQUIRE(entry != nullptr);
    std::list<std::string> entries;
    while (entry != nullptr) {
        entries.push_back(entry->d_name);
        entry = readdir(dir);
    }
    REQUIRE(entry == nullptr);
    REQUIRE(entries.size() == 4);
    REQUIRE(std::find(entries.begin(), entries.end(), "test.txt") != entries.end());
    REQUIRE(std::find(entries.begin(), entries.end(), "test-dir") != entries.end());
    REQUIRE(std::find(entries.begin(), entries.end(), ".") != entries.end());
    REQUIRE(std::find(entries.begin(), entries.end(), "..") != entries.end());
    remove("project-dir/test.txt");
    remove("project-dir/test-dir");
}

TEST_CASE("read") {
    int fd = open("project-dir/read.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    char buffer[10] = "123456789";
    int err = write(fd, buffer, 10);
    REQUIRE(err == 10);
    close(fd);
    fd = open("mount-dir/read.txt", O_RDWR, 0644);
    REQUIRE(fd >= 0);
    char read_buffer[10];
    err = read(fd, read_buffer, 10);
    REQUIRE(strncmp(read_buffer, buffer, 10) == 0);
    REQUIRE(err == 10);
    close(fd);
    remove("project-dir/read.txt");
}

TEST_CASE("write") {
    int fd = open("project-dir/write.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    close(fd);
    fd = open("mount-dir/write.txt", O_RDWR, 0644);
    REQUIRE(fd >= 0);
    char buffer[10] = "123456789";
    int err = write(fd, buffer, 10);
    REQUIRE(err == 10);
    close(fd);
    fd = open("project-dir/write.txt", O_RDWR, 0644);
    REQUIRE(fd >= 0);
    char read_buffer[10];
    err = read(fd, read_buffer, 10);
    REQUIRE(strncmp(read_buffer, buffer, 10) == 0);
    REQUIRE(err == 10);
    close(fd);
    remove("project-dir/write.txt");
}

TEST_CASE("create") {
    int fd = creat("mount-dir/create.txt", 0644);
    REQUIRE(fd >= 0);
    fd = open("project-dir/create.txt", O_RDWR, 0644);
    REQUIRE(fd >= 0);
    close(fd);
    remove("project-dir/create.txt");
}

TEST_CASE("mkdir") {
    int err = mkdir("mount-dir/mkdir", 0755);
    REQUIRE(err == 0);
    struct stat new_stat;
    err = stat("project-dir/mkdir", &new_stat);
    REQUIRE(err == 0);
    REQUIRE(S_ISDIR(new_stat.st_mode));
    remove("project-dir/mkdir");
}
