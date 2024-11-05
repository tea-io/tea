#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <csignal>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <list>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/statfs.h>
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

TEST_CASE("unlink") {
    int fd = open("project-dir/unlink.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    close(fd);
    int err = unlink("mount-dir/unlink.txt");
    REQUIRE(err == 0);
    struct stat new_stat;
    err = stat("project-dir/unlink.txt", &new_stat);
    REQUIRE(err == -1);
    REQUIRE(errno == ENOENT); 
}

TEST_CASE("rmdir") {
    int err = mkdir("project-dir/rmdir", 0755);
    REQUIRE(err == 0);
    err = rmdir("mount-dir/rmdir");
    REQUIRE(err == 0);
    struct stat new_stat;
    err = stat("project-dir/rmdir", &new_stat);
    REQUIRE(err == -1);
    REQUIRE(errno == ENOENT);
}

TEST_CASE("rename") {
    SECTION("normal"){
        int fd = open("project-dir/rename.txt", O_RDWR | O_CREAT, 0644);
        REQUIRE(fd >= 0);
        close(fd);
        int err = rename("mount-dir/rename.txt", "mount-dir/rename-new.txt");
        REQUIRE(err == 0);
        struct stat new_stat;
        err = stat("project-dir/rename.txt", &new_stat);
        REQUIRE(err == -1);
        REQUIRE(errno == ENOENT);
        err = stat("project-dir/rename-new.txt", &new_stat);
        REQUIRE(err == 0);
        remove("project-dir/rename-new.txt");
    }
    SECTION("exchange"){
        int fd = open("project-dir/exchange1.txt", O_RDWR | O_CREAT, 0644);
        REQUIRE(fd >= 0);
        int err = write(fd, "1", 1);
        REQUIRE(err == 1);
        close(fd);
        fd = open("project-dir/exchange2.txt", O_RDWR | O_CREAT, 0644);
        REQUIRE(fd >= 0);
        err = write(fd, "2", 1);
        REQUIRE(err == 1);
        close(fd);
        err = syscall(SYS_renameat2, AT_FDCWD, "project-dir/exchange1.txt", AT_FDCWD, "project-dir/exchange2.txt", RENAME_EXCHANGE);
        REQUIRE(err == 0);
        fd = open("project-dir/exchange1.txt", O_RDWR, 0644);
        REQUIRE(fd >= 0);
        char buffer[1];
        err = read(fd, buffer, 1);
        REQUIRE(err == 1);
        REQUIRE(buffer[0] == '2');
        close(fd);
        fd = open("project-dir/exchange2.txt", O_RDWR, 0644);
        REQUIRE(fd >= 0);
        err = read(fd, buffer, 1);
        REQUIRE(err == 1);
        REQUIRE(buffer[0] == '1');
        close(fd);
        remove("project-dir/exchange1.txt");
        remove("project-dir/exchange2.txt");
    }
}

TEST_CASE("chmod") {
    int fd = open("project-dir/chmod.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    close(fd);
    int err = chmod("mount-dir/chmod.txt", 0755);
    REQUIRE(err == 0);
    struct stat new_stat;
    err = stat("project-dir/chmod.txt", &new_stat);
    REQUIRE(err == 0);
    REQUIRE(new_stat.st_mode == 0100755);
    remove("project-dir/chmod.txt");
}

TEST_CASE("truncate") {
    int fd = open("project-dir/truncate.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    int err = write(fd, "123456789", 9);
    REQUIRE(err == 9);
    close(fd);
    err = truncate("mount-dir/truncate.txt", 5);
    REQUIRE(err == 0);
    fd = open("project-dir/truncate.txt", O_RDWR, 0644);
    REQUIRE(fd >= 0);
    char buffer[10];
    err = read(fd, buffer, 10);
    REQUIRE(err == 5);
    REQUIRE(strncmp(buffer, "12345", 5) == 0);
    close(fd);
    remove("project-dir/truncate.txt");
}

TEST_CASE("mknod") {
    int err = mknod("mount-dir/mknod.pipe", S_IFIFO | 0644, 0);
    REQUIRE(err == 0);
    struct stat new_stat;
    err = stat("project-dir/mknod.pipe", &new_stat);
    REQUIRE(err == 0);
    REQUIRE(S_ISFIFO(new_stat.st_mode));
    remove("project-dir/mknod.pipe");
}

TEST_CASE("link") {
    int fd = open("project-dir/link.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    close(fd);
    int err = link("mount-dir/link.txt", "mount-dir/link-new.txt");
    REQUIRE(err == 0);
    struct stat new_stat;
    err = stat("project-dir/link.txt", &new_stat);
    REQUIRE(err == 0);
    err = stat("project-dir/link-new.txt", &new_stat);
    REQUIRE(err == 0);
    remove("project-dir/link.txt");
    remove("project-dir/link-new.txt");
}

TEST_CASE("symlink"){
    int fd = open("project-dir/symlink.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    close(fd);
    int err = symlink("/symlink.txt", "mount-dir/symlink-new.txt");
    REQUIRE(err == 0);
    struct stat new_stat;
    err = lstat("project-dir/symlink-new.txt", &new_stat);
    REQUIRE(err == 0);
    REQUIRE(S_ISLNK(new_stat.st_mode));
    remove("project-dir/symlink.txt");
    remove("project-dir/symlink-new.txt");
}

TEST_CASE("readlink") {
    int fd = open("project-dir/symlink.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    close(fd);
    int err = symlink("/symlink.txt", "mount-dir/symlink-new.txt");
    REQUIRE(err == 0);
    std::string path = std::filesystem::canonical("mount-dir/symlink-new.txt");
    std::string orginal = std::filesystem::canonical("mount-dir/symlink-new.txt");
    REQUIRE(path == orginal);
    remove("project-dir/symlink.txt");
    remove("project-dir/symlink-new.txt");
}

TEST_CASE("statfs") {
    int fd = open("project-dir/statfs.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    close(fd);
    struct statfs mount_stat;
    int err = statfs("mount-dir", &mount_stat);
    REQUIRE(err == 0);
    struct statfs project_stat;
    err = statfs("project-dir", &project_stat);
    REQUIRE(err == 0);
    REQUIRE(mount_stat.f_bsize == project_stat.f_bsize);
    REQUIRE(mount_stat.f_blocks == project_stat.f_blocks);
    REQUIRE(mount_stat.f_bfree == project_stat.f_bfree);
    REQUIRE(mount_stat.f_bavail == project_stat.f_bavail);
    REQUIRE(mount_stat.f_frsize == project_stat.f_frsize);
    REQUIRE(mount_stat.f_files == project_stat.f_files);
    REQUIRE(mount_stat.f_ffree == project_stat.f_ffree);
    REQUIRE(mount_stat.f_namelen == project_stat.f_namelen);
    remove("project-dir/statfs.txt");
}

TEST_CASE("fsync") {
    int fd = open("project-dir/fsync.txt", O_RDWR | O_CREAT, 0644);
    REQUIRE(fd >= 0);
    close(fd);
    fd = open("mount-dir/fsync.txt", O_RDWR, 0644);
    REQUIRE(fd >= 0);
    int err = fsync(fd);
    REQUIRE(err == 0);
    close(fd);
    remove("project-dir/fsync.txt");
}
