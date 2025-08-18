#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

int system_spawn_cmd(const char *cmd) {
    wordexp_t p;
    if (wordexp(cmd, &p, 0) != 0) {
        fprintf(stderr, "Failed to parse command\n");
        return -1;
    }

    if (p.we_wordc == 0) {
        wordfree(&p);
        return -1;
    }

    pid_t pid;
    int status;

    // 用 posix_spawn 执行
    if (posix_spawn(&pid, p.we_wordv[0], NULL, NULL, p.we_wordv, environ) != 0) {
        perror("posix_spawn");
        wordfree(&p);
        return -1;
    }

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        wordfree(&p);
        return -1;
    }

    wordfree(&p);
    return status;
}

int main(void){
    int status = system_spawn_cmd("/usr/bin/g++ /home/shared/OJ_sys/OJ_Judge/test_isolate/test.cpp -o /home/shared/OJ_sys/OJ_Judge/test_isolate/exec/test");
    if(status == 0){
        fprintf(stderr, "编译成功\n");
        system_spawn_cmd("/usr/local/bin/isolate --init --box-id=0");
        system_spawn_cmd("/usr/bin/cp /home/shared/OJ_sys/OJ_Judge/test_isolate/exec/test /var/local/lib/isolate/0/box/");
        system_spawn_cmd("/usr/local/bin/isolate --box-id=0 \
            --processes=1 \
            --mem=65536 \
            --time=2 \
            --stdout=output.txt \
            --stderr=error.txt \
            --run /box/test");
        system_spawn_cmd("/usr/bin/cp /var/local/lib/isolate/0/box/output.txt ./output.txt");
        system_spawn_cmd("/usr/local/bin/isolate --cleanup --box-id=1");
        fprintf(stderr, "完成");
    }
    else{
        fprintf(stderr, "编译失败\n");
    }
    return 0;
}