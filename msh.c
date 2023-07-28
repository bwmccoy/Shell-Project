#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>


#define MAX_PLUGINS 10

typedef struct {
    char name[20];
    char path[201];
} Plugin;

Plugin plugins[MAX_PLUGINS]; // array to store plugins
int plugin_count = 0; // int to hold the current number of plugins stored

void add_plugin(char* plugin_name, char* plugin_path) {
    if (plugin_count >= MAX_PLUGINS) {
        printf("Error: Maximum number of plugins reached!\n");
        return;
    }

    strcpy(plugins[plugin_count].name, plugin_name);
    strcpy(plugins[plugin_count].path, plugin_path);
    plugin_count++;
}

char* find_plugin(char* plugin_name) {
    for (int i = 0; i < plugin_count; i++) {
        if (strcmp(plugins[i].name, plugin_name) == 0) {
            return plugins[i].path;
        }
    }
    return NULL;
}

int execute_plugin(char* plugin_name, char** args) {
    char* plugin_path = find_plugin(plugin_name);
    if (plugin_path == NULL) { // plugin not found
        return 0;
    }


    
}