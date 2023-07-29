#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdbool.h>

#define MAX_INPUT 201 // 201 instead of 200 since fgets() adds a newline char at end of input (which I replace with null terminator for strtok()) for corner case where input is the max length of 200
#define MAX_ARGS 20
#define MAX_PLUGINS 10

typedef struct {
    char name[20];
    char path[201];
    void* handle;
} Plugin;

Plugin plugins[MAX_PLUGINS]; // array to store plugins
int plugin_count = 0; // int to hold the current number of plugins stored - also tells where to add a plugin in the plugins array

void parse_input(char* input);
void load_plugin(char* plugin_name);
void add_plugin(char* plugin_name, char* plugin_path, void* handle);
char* find_plugin_path(char* plugin_name);
void* find_plugin_handle(char* plugin_name);
int execute_plugin(char* plugin_name, char* plugin_path, void* handle, char** args);
void fork_exec(char** arguments);

int main(void) {
    // array to hold the user input
    char input[MAX_INPUT];

    // shell loop
    while(true){
        printf("> ");
        fgets(input, sizeof(input), stdin); // only getting the first 200 chars of input to avoid buffer overflow

        // removing newline char at end of input and replacing it with null terminator to denote end of string
        input[strcspn(input, "\n")] = '\0';

        // parsing and executing the input
        parse_input(input);
    }


    return 0;
}

void parse_input(char* input) {
    // array to hold arguments
    char* arguments[MAX_ARGS];

    // using strtok to tokenize into arguments
    char* tokenized = strtok(input, " ");

    // looping through tokenized string and adding each argument to arguments array
    int i = 0;
    while (tokenized != NULL) {
        arguments[i] = tokenized;
        tokenized = strtok(NULL, " ");
        i++;
    }
    arguments[i] = NULL;

    // executing
    if (strcmp(arguments[0], "exit") == 0) { // if the command is exit
        exit(0);
    } else if (strcmp(arguments[0], "load") == 0) { // if the command is load
        if (arguments[1] == NULL) { // making sure the plugin is present
            printf("Error: plugin name not specified\n");
            return;
        }
        // loading the plugin
        load_plugin(arguments[1]);
    } else { // if the command isnt built in
        // check if its a loaded plugin
        char* plugin_path = find_plugin_path(arguments[0]);
        void* handle = find_plugin_handle(arguments[0]);
        if (plugin_path != NULL) { // if it is, execute it
            execute_plugin(arguments[0], plugin_path, handle, arguments);
        } else { // if its not, fork and exec
            fork_exec(arguments);
        }
        
    } 
}

void load_plugin(char* plugin_name) {
    // checking if the plugin is already loaded
    if (find_plugin_path(plugin_name) != NULL)  {
        printf("Error: Plugin %s is already loaded!\n", plugin_name);
        return;
    }

    // construct the plugin filename/path
    char plugin_filename[256];
    snprintf(plugin_filename, sizeof(plugin_filename), "./%s.so", plugin_name);

    // load the shared object
    void* handle = dlopen(plugin_filename, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
    }

    // load the plugin's intitialize function
    static int (*initialize)();
    initialize = dlsym(handle, "initialize");

    // check if initialize failed to load
    char* error = NULL;
    error = dlerror();
    if (error) {
        fprintf(stderr, "%s\n", error);
        return;
    }

    // intitialize the plugin
    int init_result = initialize();

    // check if initialize was successful
    if (init_result != 0) {
        fprintf(stderr, "Error: Plugin %s initialization failed!\n", plugin_name);
        return;
    }
    printf("initialization successful!\n");
    add_plugin(plugin_name, plugin_filename, handle);

    // close the plugin
    //dlclose(handle); // maybe this shouldn't be here since it potentially unloads the library and I want it to stay initialized and ready to run run() from the plugins array
}

void add_plugin(char* plugin_name, char* plugin_path, void* handle) {
    if (plugin_count >= MAX_PLUGINS) {
        printf("Error: Maximum number of plugins reached!\n");
        return;
    }

    strcpy(plugins[plugin_count].name, plugin_name);
    strcpy(plugins[plugin_count].path, plugin_path);
    plugins[plugin_count].handle = handle;
    plugin_count++;
}

char* find_plugin_path(char* plugin_name) {
    for (int i = 0; i < plugin_count; i++) {
        if (strcmp(plugins[i].name, plugin_name) == 0) {
            return plugins[i].path;
        }
    }
    return NULL;
}

void* find_plugin_handle(char* plugin_name) {
    for (int i = 0; i < plugin_count; i++) {
        if (strcmp(plugins[i].name, plugin_name) == 0) {
            return plugins[i].handle;
        }
    }
    return NULL;
}

int execute_plugin(char* plugin_name, char* plugin_path, void* handle, char** argv) {
    // construct the plugin filename/path
    //char plugin_filename[256];
    //snprintf(plugin_filename, sizeof(plugin_filename), "./%s.so", plugin_name);

    // or 

    // clear any potential previous errors
    dlerror();

    // load the plugin's run function
    static int (*run)(char** a);
    run = dlsym(handle, "run");

    // check for any errors loading run
    char* error = NULL;
    if (error) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }

    // call run
    int run_result = run(argv);


    return 0;
}

void fork_exec(char** arguments) {

}


// check if the command is built in using strcmp() to exit or load
// if it is, execute that command - exit will just exit and load will load the plugin
// if its not, check if its a valid loaded plugin and execute it if it is
// if its not a plugin, its likely an executable so fork and exec that program