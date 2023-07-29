#include <stdio.h>

int initialize() {
    //printf("Plugin 1 initializing...\n");

    return 0;
}

int run(char **argv) {
    printf("running plugin 1!\n");

    return 0;
}