#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){
    int status = system("g++ /home/shared/OJ_sys/OJ_Judge/test_isolate/test.cpp -o /home/shared/OJ_sys/OJ_Judge/test_isolate/exec/test");
    if(status == 0){
        fprintf(stderr, "编译成功\n");
        system("isolate --init --box-id=0");
        system("cp /home/shared/OJ_sys/OJ_Judge/test_isolate/exec/test /var/local/lib/isolate/0/box/")
        system("isolate --box-id=1 \
        --processes=1 \
        --mem=65536 \
        --time=2 \
        --stdout=output.txt \
        --stderr=error.txt \
        --run /path/to/your/executable");
        cp(/var/local/lib/isolate/0/box/output.txt)
        isolate --cleanup --box-id=1

    }
    else{
        fprintf(stderr, "编译失败\n");
    }
    return 0;
}