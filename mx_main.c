#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include "mxa_functions.h"

extern int mgrip_cmd_internal(int argc, char *argv[]);

#define MAX_COMMAND_LEN 1024
#define MAX_ARGS 32

int mx_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
    return 0;
}

int mx_ls(int argc, char *argv[]){
    DIR *d;
    struct dirent *dir;
    char *p=".";
    if(argc > 1) p=argv[1];

    if(!(d=opendir(p))){
        perror("mx ls");
        return 1;
    }
    while((dir=readdir(d))!=NULL) {
        if(strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0) {
            printf("%s\n",dir->d_name);
        }
    }
    closedir(d);
    return 0;
}

int mx_mkdir(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr,"mx mkdir: missing operand\n");
        return 1;
    }
    if(mkdir(argv[1], S_IRWXU | S_IRWXG | S_IRWXO) != 0){
        perror("mx mkdir");
        return 1;
    }
    return 0;
}

int mx_rm(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr,"mx rm: missing operand\n");
        return 1;
    }
    if(unlink(argv[1]) != 0){
        perror("mx rm");
        return 1;
    }
    return 0;
}

int mx_rmdir(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr,"mx rmdir: missing operand\n");
        return 1;
    }
    if(rmdir(argv[1]) != 0){
        perror("mx rmdir");
        return 1;
    }
    return 0;
}

int mx_cat(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "mx cat: missing operand\n");
        return 1;
    }
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("mx cat");
        return 1;
    }
    int c;
    while ((c = fgetc(fp)) != EOF) {
        putchar(c);
    }
    fclose(fp);
    return 0;
}

int mx_cd(int argc, char *argv[]) {
    const char *target_dir;
    if (argc < 2 || strcmp(argv[1], "~") == 0) {
        target_dir = getenv("HOME");
        if (target_dir == NULL) {
            target_dir = "/";
        }
    } else {
        target_dir = argv[1];
    }

    if (chdir(target_dir) != 0) {
        perror("mx cd");
        return 1;
    }
    return 0;
}


int mx_exit(int argc, char *argv[]) {
    int exit_code = 0;
    if (argc > 1) {
        exit_code = atoi(argv[1]);
    }
    exit(exit_code);
}

int mx_list(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    printf("Minxbox BETA v0.9 commands:\n");
    printf("  echo\n");
    printf("  ls\n");
    printf("  mkdir\n");
    printf("  rm\n");
    printf("  rmdir\n");
    printf("  cat\n");
    printf("  cd\n");
    printf("  grep (enhanced by mgrip!)\n");
    printf("  list\n");
    printf("  mxa pack\n");
    printf("  mxa unpack\n");
    printf("  exit\n");
    return 0;
}

int mxa_dispatch_command(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pack|unpack> [arguments...]\n", argv[0]);
        return 1;
    }
    const char *sub_command = argv[1];
    if (strcmp(sub_command, "pack") == 0) {
        return mxa_pack_cmd(argc - 1, argv + 1);
    } else if (strcmp(sub_command, "unpack") == 0) {
        return mxa_unpack_cmd(argc - 1, argv + 1);
    } else {
        fprintf(stderr, "Error: Unknown mxa command '%s'\n", sub_command);
        return 1;
    }
}


int main(int argc, char *argv[]){
    char *prog_name = basename(argv[0]);
    if (strcmp(prog_name, "grep") == 0) return mgrip_cmd_internal(argc, argv);
    if (strcmp(prog_name, "mxa") == 0) return mxa_dispatch_command(argc, argv);
    if (strcmp(prog_name, "cat") == 0) return mx_cat(argc, argv);
    if (strcmp(prog_name, "ls") == 0) return mx_ls(argc, argv);
    if (strcmp(prog_name, "cd") == 0) return mx_cd(argc, argv);
    if (strcmp(prog_name, "echo") == 0) return mx_echo(argc, argv);
    if (strcmp(prog_name, "mkdir") == 0) return mx_mkdir(argc, argv);
    if (strcmp(prog_name, "rm") == 0) return mx_rm(argc, argv);
    if (strcmp(prog_name, "rmdir") == 0) return mx_rmdir(argc, argv);
    if (strcmp(prog_name, "list") == 0) return mx_list(argc, argv);

    char line[MAX_COMMAND_LEN];
    char *token;
    char *args[MAX_ARGS];
    int status = 0;

    while (1) {
        char cwd[MAX_COMMAND_LEN];
 
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("mx:%s$ ", cwd);
        } else {
            perror("getcwd");
            printf("mx$ ");
        }
        
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) {
            continue;
        }

        int i = 0;
        token = strtok(line, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;
        int num_args = i;

        if (num_args == 0) {
            continue;
        }

        const char *cmd = args[0];

        if (strcmp(cmd, "echo") == 0) {
            status = mx_echo(num_args, args);
        } else if (strcmp(cmd, "ls") == 0) {
            status = mx_ls(num_args, args);
        } else if (strcmp(cmd, "mkdir") == 0) {
            status = mx_mkdir(num_args, args);
        } else if (strcmp(cmd, "rm") == 0) {
            status = mx_rm(num_args, args);
        } else if (strcmp(cmd, "rmdir") == 0) {
            status = mx_rmdir(num_args, args);
        } else if (strcmp(cmd, "cat") == 0) {
            status = mx_cat(num_args, args);
        } else if (strcmp(cmd, "cd") == 0) {
            status = mx_cd(num_args, args);
        } else if (strcmp(cmd, "grep") == 0) {
            status = mgrip_cmd_internal(num_args, args);
        } else if (strcmp(cmd, "list") == 0) {
            status = mx_list(num_args, args);
        } else if (strcmp(cmd, "mxa") == 0) {
            status = mxa_dispatch_command(num_args, args);
        } else if (strcmp(cmd, "exit") == 0) {
            mx_exit(num_args, args);
        } else {
            fprintf(stderr, "mx: %s: command not found\n", cmd);
            status = 127;
        }
    }
    
    return status;
}
