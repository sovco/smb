#define NOB_IMPLEMENTATION
#include "build/deps/nob/nob.h"

static inline bool file_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = { 0 };

    nob_cmd_append(&cmd, "mkdir", "-p", "build/deps/stf");
    if (!nob_cmd_run(&cmd)) return 1;
    if (!file_exists("build/deps/stf/stf.h")) {
        nob_cmd_append(&cmd, "curl", "-Lo", "build/deps/stf/stf.h", "https://raw.githubusercontent.com/sovco/stf/refs/heads/master/include/stf/stf.h");
        if (!nob_cmd_run(&cmd)) return 1;
    }
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-std=c11", "-D_POSIX_C_SOURCE=200112L", "-o3", "-o", "build/smb-test", "-lm", "-Iinclude", "-Ibuild/deps", "test/smb-test.c");
    if (!nob_cmd_run(&cmd)) return 1;
    return 0;
}
