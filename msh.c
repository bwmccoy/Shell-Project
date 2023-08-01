#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_INPUT 201 // 201 instead of 200 since fgets() adds a newline char at end of input (which I replace with null terminator for strtok()) for corner case where input is the max length of 200
#define MAX_ARGS 20
#define MAX_PLUGINS 10

typedef struct {
    char name[20];
    int (*run)(char**);
} Plugin;

Plugin plugins[MAX_PLUGINS]; // array to store plugins
int plugin_count = 0; // counter to hold the current number of plugins stored - also tells where to add a plugin in the plugins array

void parse_input(char* input);
void load_plugin(char* plugin_name);
void add_plugin(char* plugin_name, int (*run)(char**));
int find_plugin_index(char* plugin_name);
int execute_plugin(char* plugin_name, char** args);
int fork_exec(char** arguments);

int main(void) {
    // array to hold the user input
    char input[MAX_INPUT];

    // shell loop
    while(true){
        printf("> ");
        fgets(input, sizeof(input), stdin); // only getting the first 200 chars of input to avoid buffer overflow

        // checking if user input exceeds maximum input size
        int input_length = strcspn(input, "\n");
        if (input_length >= 200) {
            continue;
        }

        // removing newline char at end of input and replacing it with null terminator to denote end of string
        input[input_length] = '\0';


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

    // check if maximum allowed arguments was exceeded
    if (i < MAX_ARGS) {
        arguments[i] = NULL;
    } else {
        return;
    }

    // check if the length of any arguments exceedes the max allowed length
    for (int j = 0; j < i; j++) {
        if (strlen(arguments[j]) >= 20) {
            return;
        }
    }
    

    // executing
    if (strcmp(arguments[0], "exit") == 0) { // if the command is exit
        unload_plugins();
        exit(0);
    } else if (strcmp(arguments[0], "load") == 0) { // if the command is load
        if (arguments[1] == NULL) { // making sure the plugin is present
            printf("Error: Plugin %s initialization failed!\n", arguments[1]);
            return;
        }
        // loading the plugin
        load_plugin(arguments[1]);
    } else if (fork_exec(arguments) != 0 && find_plugin_index(arguments[0]) >= 0) { // try to run it as an executable, if its not, check if its a loaded plugin 
        // if its a loaded plugin
        execute_plugin(arguments[0], arguments);
    } else {
        return;
    }
}

void load_plugin(char* plugin_name) {
    // checking if the plugin is already loaded
    if (find_plugin_index(plugin_name) >= 0)  {
        printf("Error: Plugin %s initialization failed!\n", plugin_name);
        return;
    }

    // construct the plugin filename/path
    char plugin_filename[256];
    snprintf(plugin_filename, sizeof(plugin_filename), "./%s.so", plugin_name);

    // load the shared object
    void* handle = dlopen(plugin_filename, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: Plugin %s initialization failed!\n", plugin_name); 
        return;
    }

    // load the plugin's intitialize function
    int (*initialize)();
    initialize = dlsym(handle, "initialize");

    // check if initialize failed to load
    char* init_error = dlerror();
    if (init_error) {
        fprintf(stderr, "%s\n", init_error);
        dlclose(handle);
        return;
    }

    // load the plugin's run function
    int (*run)(char**);
    run = dlsym(handle, "run");

    // check if run failed to load
    char* run_error = dlerror();
    if (run_error) {
        fprintf(stderr, "%s\n", run_error);
        dlclose(handle);
        return;
    }

    // intitialize the plugin
    int init_result = initialize();

    // check if initialize was successful
    if (init_result != 0) {
        fprintf(stderr, "Error: Plugin %s initialization failed!\n", plugin_name);
        dlclose(handle);
        return;
    }

    add_plugin(plugin_name, run);
}

void add_plugin(char* plugin_name, int (*run)(char**)) {
    if (plugin_count >= MAX_PLUGINS) {
        printf("Error: Maximum number of plugins reached!\n");
        return;
    }

    strcpy(plugins[plugin_count].name, plugin_name);
    plugins[plugin_count].run = run;
    plugin_count++;
}

int find_plugin_index(char* plugin_name) {
    for (int i = 0; i < plugin_count; i++) {
        if (strcmp(plugins[i].name, plugin_name) == 0) {
            return i;
        }
    }
    return -1;
}

int execute_plugin(char* plugin_name, char** argv) {
    // get the index of the plugin
    int plugin_index = find_plugin_index(plugin_name);
    if (plugin_index < 0) { // if plugin not in plugins array (isn't loaded)
        return 0;
    }

    // call run function of the plugin
    int run_result = plugins[plugin_index].run(argv);
    return run_result;
}

int fork_exec(char** argv) {
    pid_t pid = fork();
    int status = 0;
    if (pid == 0) { // child process
        // exec the command
        if (execvp(argv[0], argv) < 0) { // check if theres an error from execvp (returns -1 if error and nothing if successful)
            //printf("Error: exec failed!\n");
            exit(1); // exit with a 1 to indicate error to parent
        }

    } else if (pid > 0){ // parent process
        wait(&status);
    } else { // error forking since pid < 0
        fprintf(stderr, "Error forking!\n");
    }
    return status;
}

