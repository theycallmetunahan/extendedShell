#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

// passes all cases except repeaters
// nisan 7 23:01 2024

void execute_command(parsed_input *input);
void execute_subshell_command(parsed_input *input);
void execute_sequential_new(single_input* inputs, int num_inputs);

int main() {
    char line[1024];    
    
   
    while (1) {
        printf("/> ");
        if (!fgets(line, sizeof(line), stdin)) break;        
        if (strcmp(line, "quit\n") == 0) break;

        parsed_input input;
        if (parse_line(line, &input)) {
            execute_command(&input);
        } 
        else {
            ;
        }

        free_parsed_input(&input);
    }

    return 0;
}



void execute_single_command_without_fork(command *cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(cmd->args[0], cmd->args);
        
        exit(0);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        exit(-1);
        // did not do error handling
    }

}

void execute_pipeline_new(single_input* inputs, int num_inputs);
void execute_command_based_on_input_type(single_input* input);
void execute_command_based_on_input_type_direct(single_input* input);




void execute_subshell(char* subshell) {
    parsed_input input;
    if (parse_line(subshell, &input)) {
        execute_subshell_command(&input);
    } else {
        ; // did not do error handling
    }
    free_parsed_input(&input);
}


void execute_subshell_command(parsed_input *input) {
    if (input->separator == SEPARATOR_SEQ) {
        execute_sequential_new(input->inputs, input->num_inputs);
        return;
    } else if (input->separator == SEPARATOR_PIPE) {
        execute_pipeline_new(input->inputs, input->num_inputs);
        return;
    }
    else if (input->separator == SEPARATOR_PARA) {
        for (int i = 0; i < input->num_inputs; i++) {
            execute_command_based_on_input_type(&input->inputs[i]);
        }
        return;
    }
    else {
        ; // did not do error handling
    }
}




void execute_pipeline_new(single_input* inputs, int num_inputs) {
    if (num_inputs < 2) {
        return;
    }

    int (*pipes)[2] = malloc((num_inputs - 1) * sizeof(int[2]));
    pid_t* pids = malloc(num_inputs * sizeof(pid_t));


    for (int i = 0; i < num_inputs - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            
            exit(-1);
            // not necessary 
        }
    }
    for (int i = 0; i < num_inputs; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {                       
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
                       if (i < num_inputs - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
                       for (int j = 0; j < num_inputs - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            execute_command_based_on_input_type_direct(&inputs[i]);
            exit(0);
        }
    }
    for (int i = 0; i < num_inputs - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    for (int i = 0; i < num_inputs; i++) {
        waitpid(pids[i], NULL, 0);
    }

    free(pipes);
    free(pids);
}

void execute_command_based_on_input_type_direct(single_input* input) { 
    if (input->type == INPUT_TYPE_COMMAND) {
        command *cmd = &input->data.cmd;
        execvp(cmd->args[0], cmd->args);
    }
    else if (input->type == INPUT_TYPE_SUBSHELL) {
        execute_subshell(input->data.subshell);
    } else {
        ; // did not do error handling
    }
}


void execute_command_based_on_input_type(single_input* input) {
    if (input->type == INPUT_TYPE_COMMAND) {
        execute_single_command_without_fork(&input->data.cmd);

    }
    else if (input->type == INPUT_TYPE_SUBSHELL) {
        execute_subshell(input->data.subshell);
    } 
    else {
        ;
        // did not do error handling
    }
}


void execute_parallel_new(single_input* inputs, int num_inputs) {
    for (int i = 0; i < num_inputs; i++) {
        if (inputs[i].type == INPUT_TYPE_PIPELINE) {
            parsed_input* new_input = malloc(sizeof(parsed_input));
            new_input->num_inputs = inputs[i].data.pline.num_commands;
            new_input->separator = SEPARATOR_PIPE;
            for (int j = 0; j < inputs[i].data.pline.num_commands; j++) {
                new_input->inputs[j].type = INPUT_TYPE_COMMAND;
                new_input->inputs[j].data.cmd = inputs[i].data.pline.commands[j];
            }
            execute_command(new_input);
            free(new_input);

        } 
        else {
            execute_command_based_on_input_type(&inputs[i]);
        }
    }
    
    
}




void execute_sequential_new(single_input* inputs, int num_inputs) {
    for (int i = 0; i < num_inputs; i++) {
        if (inputs[i].type == INPUT_TYPE_PIPELINE){
            parsed_input* new_input = malloc(sizeof(parsed_input));
            new_input->num_inputs = inputs[i].data.pline.num_commands;
            new_input->separator = SEPARATOR_PIPE;
            for (int j = 0; j < inputs[i].data.pline.num_commands; j++) {
                new_input->inputs[j].type = INPUT_TYPE_COMMAND;
                new_input->inputs[j].data.cmd = inputs[i].data.pline.commands[j];
            }
            execute_command(new_input);
            free(new_input);
        } 
        
        else {
            execute_command_based_on_input_type(&inputs[i]);
        }
    } 
           
}

void execute_command(parsed_input *input) {
    if (input->num_inputs == 1 && input->inputs[0].type == INPUT_TYPE_COMMAND) {
        
        execute_single_command_without_fork(&input->inputs[0].data.cmd);
    }
    else if (input->num_inputs == 1 && input->inputs[0].type == INPUT_TYPE_SUBSHELL) {
        execute_subshell(input->inputs[0].data.subshell);
    }
    else {
        switch (input->separator) {
            case SEPARATOR_SEQ:
                execute_sequential_new(input->inputs, input->num_inputs);
                break;
            case SEPARATOR_PARA:
                execute_parallel_new(input->inputs, input->num_inputs);
                break;
            case SEPARATOR_PIPE:
                execute_pipeline_new(input->inputs, input->num_inputs);
                break;
            default:
                ; // not necessary

        }
    }
}
