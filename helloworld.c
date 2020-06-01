#include <stdio.h>

int main(int argc, char ** argv){
    printf("Hello world!!!!\n");
    if (argc > 1) {
        printf("Arg: %s\n", argv[0]);
    }
    return 0;
}
