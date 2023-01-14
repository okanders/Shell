#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "jobs.h"

job_list_t *joblist;
pid_t shellPgid;
int jidCounter;
char buffer[1024];
char *inputs;
char *outputs;
char *appendages;
char *argv[512];
int boolean;
char *execFile;
int ampersand;

/*
 * Function that will identify resdirections, which is necessary for my parse
 * function
 *
 * Parameters:
 *  - Char redirections like ("<", ">", ">>")
 *
 * Returns:
 *  - 1 if input redirection is found
 *  - 2 if output redirection is found
 *  - 3 if appendage redirection is found
 *  - 0 if no redirection is found
 */
int identifyRedirection(char *redirection) {
    // if redirection and input is found
    if (strcmp(redirection, "<") == 0) {
        return 1;
    }
    // if redirection and output is found
    else if (strcmp(redirection, ">") == 0) {
        return 2;
    }
    // if redirection and appendage is found
    else if (strcmp(redirection, ">>") == 0) {
        return 3;
    }
    return 0;
}

/*
 * Function that will check if consecutive redirections are shown in my read and
 * will then report error based on return int
 *
 * Parameters:
 *  - Char of redirection ("<", ">", ">>")
 *
 * Returns:
 *  - 0 if no consecutive redirection is found
 *  - 1 if a consecutive redirection is found
 */
int consecutiveRedirections(char *redirection) {
    if (identifyRedirection(redirection) == 1 ||
        identifyRedirection(redirection) == 2 ||
        identifyRedirection(redirection) == 3) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Function that will check for ampersand at last non-white space
 *
 * Parameters:
 *  - Char of ampersand ("&")
 *
 * Returns:
 *  - 0 if ampersand is found
 *  - 1 if no ampersand is found
 */
int identifyBackground(char *ampersand) {
    // if ampersand is found
    if (strcmp(ampersand, "&") == 0) {
        return 0;
    } else {
        return 1;
    }
}

/*
 * Function that will be parse through my buffer and will populate my argv,
 * output, input, and appendage
 *
 * Parameters:
 *  - None, global variables are altered
 *
 * Returns:
 *  - Nothing, the function is void and will only alter global variables
 *  -Will alter my arrays and strings.
 */
void parse() {
    char *myToken = strtok(buffer, " \t\n");
    // check to see if the first token found in the string will not be null
    // if that first token is not null then continue to parse
    if (myToken != NULL) {
        // create index for my argv array
        int argvIndex = 0;
        // start by looping through from the first term until the end
        while (myToken != NULL) {
            // search for redirections and then strtok to locate and fill
            // inputs, outputs, and appendages
            if (identifyRedirection(myToken) == 1) {
                myToken = strtok(NULL, " \t\n");
                // go find token after redirection
                while (myToken != NULL) {
                    // if this redirection another redirection, then raise an
                    // error
                    if (consecutiveRedirections(myToken) == 1) {
                        fprintf(stderr, "consecutive redirection\n");
                        cleanup_job_list(joblist);
                        exit(1);
                    }
                    // store input
                    inputs = myToken;
                    break;
                }
                // store boolean to make sure input is not stored in argv
                boolean = 1;
                continue;
            }
            // search for output
            else if (identifyRedirection(myToken) == 2) {
                myToken = strtok(NULL, " \t\n");
                while (myToken != NULL) {
                    if (consecutiveRedirections(myToken) == 1) {
                        fprintf(stderr, "consecutive redirection\n");
                        cleanup_job_list(joblist);
                        exit(1);
                    }
                    // make sure this error exits so I am not improperly
                    // indexing
                    outputs = myToken;
                    break;
                }
                // store boolean to make sure output is not stored in argv
                boolean = 2;
                continue;

            }
            // search for apppendage
            else if (identifyRedirection(myToken) == 3) {
                myToken = strtok(NULL, " \t\n");
                while (myToken != NULL) {
                    if (consecutiveRedirections(myToken) == 1) {
                        fprintf(stderr, "consecutive redirection\n");
                        cleanup_job_list(joblist);
                        exit(1);
                    }
                    // make sure this error exits so I am not improperly
                    // indexing
                    appendages = myToken;
                    break;
                }
                // store boolean to make sure appendage is not stored in argv
                boolean = 3;
                continue;
            }
            // parse ampersand
            else if (identifyBackground(myToken) == 0) {
                ampersand = identifyBackground(myToken);
                break;
            } else {
                // loop again if myToken is currently a stored input, output,
                // appendage
                if (boolean != 0) {
                    boolean = 0;
                    myToken = strtok(NULL, " \t\n");
                    continue;
                }
                // store argv
                argv[argvIndex] = myToken;
                argvIndex++;
                myToken = strtok(NULL, " \t\n");
            }
        }
        // null terminate my argv and tokesn array
        argv[argvIndex] = '\0';
        execFile = argv[0];
        char *ptr = strrchr(argv[0], '/');
        // if a backslash comes up in its search,
        if (ptr != NULL) {
            argv[0] = ptr + 1;
        }
        if (outputs != NULL && appendages != NULL) {
            fprintf(stderr, "syntax error: multiple output files\n");
            cleanup_job_list(joblist);
            exit(1);
        }
    }
}
/*
 * Function will exit safely
 *
 * Parameters:
 *  - None
 *
 * Returns:
 *  - Nothing, the function is void and will only exit the program.
 */
void myExit() {
    cleanup_job_list(joblist);
    exit(0);
}

/*
 * Function will change the directory
 *
 * Parameters:
 *  - Char argv1, where argv[1] will be passed in and used to change that
 * directory
 *
 * Returns:
 *  - Nothing, the function is void and will only change the directory of the
 * argv[1]
 */
void cd(char *argv1) {
    // if argv[1] is null
    if (argv1 == NULL) {
        fprintf(stderr, "cd: syntax error\n");
    } else {
        // change directory, and calle error if less than -1
        if (chdir(argv1) == -1) {
            perror("No such file or directory");
            // printf(" %s: no such directory\n", argv);
        }
    }
}

/*
 * Function will remove the file named in argv[1]
 *
 * Parameters:
 *  - Char argv1, which is referring the file argv[1] that will be removed and
 * unlinked
 *
 * Returns:
 *  - Nothing, the function is void and will only remove the file
 */
void rm(char *argv1) {
    if (unlink(argv1) == -1) {
        perror("unlink failed");
        // printf(" %s: no such directory\n", errno); //print error
    }
}
/*
 * Function will link the file named in argv[1] and argv[2]
 *
 * Parameters:
 *  - Char argv1, which is referring the file argv[1] that will be linked
 *  - Char argv2 which is referring to the file argv[2] that will be linked
 *
 * Returns:
 *  - Nothing, the function is void and will only link the files
 */
void ln(char *argv1, char *argv2) {
    if (link(argv1, argv2) == -1) {
        perror("link failed");
    }
}
/*
 * Function will open and read my input file when a redirection is found. This
 * is done to STDIN.
 *
 * Parameters:
 *  - Char inputs, which refers to the input that will be redirected that was
 * caught in the parse method.
 *
 * Returns:
 *  - Nothing, the function is void and will only close file STDIN and open it
 * to be written into by the file inputs.
 */
void myInputFile(char *inputs) {
    if (inputs != NULL) {
        if (close(0) < 0) {
            perror("close input");
            cleanup_job_list(joblist);
            exit(1);
        }
        int inputFile;
        if ((inputFile = open(inputs, O_RDONLY | O_CREAT, 0777 & ~(022))) ==
            -1) {
            perror("open input");
            cleanup_job_list(joblist);
            exit(1);
        }
    }
}
/*
 * Function will open and write my output file when a redirection is found. This
 * is done to STDOUT. The function may also look to append to a file, opening it
 * and appending it to STDOUT
 *
 * Parameters:
 *  - Char output, which refers to the output that will be redirected that was
 * caught in the parse method.
 *  - Char appendage, which refers to the appendage that will be redirected that
 * was caught in the parse method.
 *
 * Returns:
 *  - Nothing, the function is void and will only close file STDOUT and open it
 * to be written into by the file outputs or appended to.
 */

void myOutputOrAppendageFile(char *outputs, char *appendages) {
    // handle output redirection
    if (outputs != NULL) {
        if (close(1) < 0) {
            perror("close output");
            cleanup_job_list(joblist);
            exit(1);
        }
        // instantiate output file
        int outputFile;
        // write in the output file
        if ((outputFile = open(outputs, O_WRONLY | O_CREAT | O_TRUNC,
                               0777 & ~(022))) == -1) {
            perror("open output");
            cleanup_job_list(joblist);
            exit(1);
        }
    }
    // hanlde appendage redirection
    else if (appendages != NULL) {
        if (close(1) < 0) {
            perror("close appendage");
            cleanup_job_list(joblist);
            exit(1);
        }
        // instantiate appendage file
        int appendageFile;
        // append to the output file
        if ((appendageFile = open(appendages, O_WRONLY | O_APPEND | O_CREAT,
                                  0777 & ~(022))) == -1) {
            perror("open appendage");
            cleanup_job_list(joblist);
            exit(1);
        }
    }
}
/*
 * Function will take a particular job that has been suspended and place it as a background process
 *
 * Parameters:
 *  - int of the job id
 *
 * Returns:
 *  - Nothing, the function is void and will only place the suspended job into the background.
 */
void bg(int jid) {
    //take control to shell 
    if (tcsetpgrp(STDIN_FILENO, shellPgid) == -1) {
        perror("tcsetprgp failed");
        cleanup_job_list(joblist);
        exit(1);
    }
    //send the SIGCONT to the particular process id to place into background
    if (kill(-(get_job_pid(joblist, jid)), SIGCONT) == -1) {
        perror("bg");
        cleanup_job_list(joblist);
        exit(1);
    }
}
/*
 * Function will take a particular job that has been suspended and place it as a foreground process
 *
 * Parameters:
 *  - int of the job id
 *
 * Returns:
 *  - Nothing, the function is void and will only place the suspended job into the foreground.
 */
void fg(int jid) {
    //give terminal control to this process group
    if (tcsetpgrp(STDIN_FILENO, get_job_pid(joblist, jid)) == -1) {
        perror("tcsetprgp failed");
        cleanup_job_list(joblist);
        exit(1);
    }
    //send the SIGCONT to the foreground process to continue
    if (kill(-(get_job_pid(joblist, jid)), SIGCONT) == -1) {
        perror("fg");
        cleanup_job_list(joblist);
        exit(1);
    }
    //update the job to now be running
    update_job_jid(joblist, jid, RUNNING);
    int foregroundPid;
    int status;
    // wait for foreground processes
    if ((foregroundPid =
             waitpid(get_job_pid(joblist, jid), &status, WUNTRACED)) == -1) {
        perror("waitpid foreground");
        cleanup_job_list(joblist);
        exit(1);
    }
    // check exit
    if (WIFEXITED(status)) {
        if (remove_job_jid(joblist, jid) == -1) {
            fprintf(stderr, "remove job failed\n");
            cleanup_job_list(joblist);
            exit(1);
        }
    }
    // check termination foreground
    if (WIFSIGNALED(status)) {
        printf("[%d] (%d) terminated by signal %d\n", jid, foregroundPid,
               WTERMSIG(status));
        if (remove_job_jid(joblist, jid) == -1) {
            fprintf(stderr, "remove job failed\n");
            cleanup_job_list(joblist);
            exit(1);
        }
    }
    // check suspension of foreground
    if (WIFSTOPPED(status)) {
        printf("[%d] (%d) suspended by signal %d\n", jid, foregroundPid,
               WSTOPSIG(status));
    }
    // restore control back to shell once foreground process is complete
    if (tcsetpgrp(STDIN_FILENO, shellPgid) == -1) {
        perror("tcsetprgp failed");
        cleanup_job_list(joblist);
        exit(1);
    }
}

/* jobs command, prints out the jobs list */
void myJobs() { jobs(joblist); }

/*
 * Function will kill my background zombie processes that have terminated or recieved a signal so that space is 
 * more efficiently used
 *
 * Parameters:
 *  - None
 *
 * Returns:
 *  - Nothing, the function is void and will only check for any children process that may have terminated or recieved a signal
 */
void reaping() {
    // do reaping here
    int statusBackground;
    int backgroundPid;
    // loop through all of my children and see why one exited strangley for
    // background processes
    while ((backgroundPid = waitpid(-1, &statusBackground,
                                    WUNTRACED | WNOHANG | WCONTINUED)) > 0) {
        // check valid exit status
        if (WIFEXITED(statusBackground)) {
            printf("[%d] (%d) terminated with exit status %d\n",
                   get_job_jid(joblist, backgroundPid), backgroundPid,
                   WEXITSTATUS(statusBackground));
            if (remove_job_jid(joblist, get_job_jid(joblist, backgroundPid)) ==
                -1) {
                fprintf(stderr, "remove job failed\n");
                cleanup_job_list(joblist);
                exit(1);
            }
        }
        // check termination
        if (WIFSIGNALED(statusBackground)) {
            printf("[%d] (%d) terminated by signal %d\n",
                   get_job_jid(joblist, backgroundPid), backgroundPid,
                   WTERMSIG(statusBackground));
            if (remove_job_jid(joblist, get_job_jid(joblist, backgroundPid)) ==
                -1) {
                fprintf(stderr, "remove job failed");
                cleanup_job_list(joblist);
                exit(1);
            }
        }
        // check suspension of children
        if (WIFSTOPPED(statusBackground)) {
            if (update_job_jid(joblist, get_job_jid(joblist, backgroundPid),
                               STOPPED) == -1) {
                fprintf(stderr, "update job failed");
                cleanup_job_list(joblist);
                exit(1);
            }
            printf("[%d] (%d) suspended by signal %d\n",
                   get_job_jid(joblist, backgroundPid), backgroundPid,
                   WSTOPSIG(statusBackground));
            jidCounter = jidCounter + 1;
        }
        // check continued
        if (WIFCONTINUED(statusBackground)) {
            if (update_job_jid(joblist, get_job_jid(joblist, backgroundPid),
                               RUNNING) == -1) {
                fprintf(stderr, "update job failed");
                cleanup_job_list(joblist);
                exit(1);
            }
            printf("[%d] (%d) resumed\n", get_job_jid(joblist, backgroundPid),
                   backgroundPid);
        }
    }
}
/*
 * Function will set up my signal handlers to their ignore mode, except for SIGQUIT
 *
 * Parameters:
 *  - None
 *
 * Returns:
 *  - Nothing, the function is void and will only set my signals to their ignore mode
 */
void setDefaultSignals() {
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR) {
        perror("SIGTSTP error");
        cleanup_job_list(joblist);
        exit(1);
    }
    // termination error
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        perror("SIGINT error");
        cleanup_job_list(joblist);
        exit(1);
    }
    // SIGTTOU error
    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
        perror("singal error");
        cleanup_job_list(joblist);
        exit(1);
    }
    if (signal(SIGQUIT, SIG_DFL) == SIG_ERR) {
        perror("singal error");
        cleanup_job_list(joblist);
        exit(1);
    }
}
/*
 * Function will set up my signal handlers to their default mode
 *
 * Parameters:
 *  - None
 *
 * Returns:
 *  - Nothing, the function is void and will only set my signals to their default mode
 */
void setChildSignals() {
    // suspension error
    if (signal(SIGTSTP, SIG_DFL) == SIG_ERR) {
        perror("SIGTSTP error");
        cleanup_job_list(joblist);
        exit(1);
    }

    // termination error
    if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
        perror("SIGINT error");
        cleanup_job_list(joblist);
        exit(1);
    }
    // continue error
    if (signal(SIGCONT, SIG_DFL) == SIG_ERR) {
        perror("SIGCONT error");
        cleanup_job_list(joblist);
        exit(1);
    }

    // SIGTTOU error
    if (signal(SIGTTOU, SIG_DFL) == SIG_ERR) {
        perror("singal error");
        cleanup_job_list(joblist);
        exit(1);
    }
}

/*
 * Main function: this will read the buffer and parse it to then execute any
 * commands and files with the child process, or UNIX commands
 *
 * Parameters:
 *  - None, will alter the global variables and local varibales submitted
 *
 * Returns:
 *  - 0 if program exits correctly, exit(1) if there is an error
 */

int main() {
    joblist = init_job_list();
    jidCounter = 1;
    // suspension error
    setDefaultSignals();
    while (1) {
        reaping();
#ifdef PROMPT
        if (printf("33sh> ") < 0) {
            fprintf(stderr, "33sh\n");
            cleanup_job_list(joblist);
            return 1;
        }
        if (fflush(stdout) < 0) {
            perror("stdout");
            cleanup_job_list(joblist);
            return 1;
        }
#endif
        ssize_t readFile;
        // loop through my readFile and all the buffer read in
        readFile = read(0, buffer, sizeof(buffer));
        if (readFile < 0) {
            perror("control d\n");
            cleanup_job_list(joblist);
            return 1;
        } else if (readFile == 0) {
            cleanup_job_list(joblist);
            return 0;
        }
        // reset argv array
        memset(argv, '\0', 512 * sizeof(*argv));
        // must reset my input, output, and appendage variables
        inputs = NULL;
        outputs = NULL;
        appendages = NULL;
        execFile = NULL;
        ampersand = 2;
        // execute parse function
        parse();
        // check to see if file path is null
        if (execFile == NULL) {
            memset(buffer, '\0', 1024);
            continue;
        }
        // implement cd
        if (strcmp(execFile, "cd") == 0) {
            cd(argv[1]);
            memset(buffer, '\0', 1024);
            continue;
        }
        // implement exit
        else if (strcmp(argv[0], "exit") == 0) {
            myExit();
        }
        // implement rm
        else if (strcmp(execFile, "rm") == 0) {
            rm(argv[1]);
            memset(buffer, '\0', 1024);
            continue;
        }
        // implement ln
        else if (strcmp(argv[0], "ln") == 0) {
            ln(argv[1], argv[2]);
            memset(buffer, '\0', 1024);
            continue;
        } else if (strcmp(argv[0], "bg") == 0) {
            int jid;
            if (argv[1][0] != '%') {
                fprintf(stderr, "no percetage sign\n");
                cleanup_job_list(joblist);
                exit(1);
            } else {
                jid = atoi(argv[1] + 1);
                if (get_job_pid(joblist, jid) == -1) {
                    fprintf(stderr, "job not found\n");
                    memset(buffer, '\0', 1024);
                    continue;
                }
                bg(jid);
                memset(buffer, '\0', 1024);
                continue;
            }
        } else if (strcmp(argv[0], "fg") == 0) {
            int jid;
            if (argv[1][0] != '%') {
                fprintf(stderr, "no percetage sign\n");
                cleanup_job_list(joblist);
                exit(1);
            } else {
                jid = atoi(argv[1] + 1);
                if (get_job_pid(joblist, jid) == -1) {
                    fprintf(stderr, "job not found\n");
                    memset(buffer, '\0', 1024);
                    continue;
                }
                fg(jid);
                memset(buffer, '\0', 1024);
                continue;
            }

        } else if (strcmp(argv[0], "jobs") == 0) {
            myJobs();
            memset(buffer, '\0', 1024);
            continue;
        }

        pid_t pid;
        //get my shell process group id
        shellPgid = getpgrp(); 
        int status;
        // fork and begin to handle my redirections
        if ((pid = fork()) == 0) {
            // set the pgid to pid so that each child process has unique group
            // process id
            pid_t otherPid = getpid();
            if (setpgid(otherPid, otherPid) == -1) {
                perror("setpgid failed");
                cleanup_job_list(joblist);
                exit(1);
            }
            // givng terminal control to the group processing id, must make sure
            // this is only for foreground
            if (ampersand != 0) {
                if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) {
                    perror("tcsetprgp failed");
                    cleanup_job_list(joblist);
                    exit(1);
                }
            }
            //set child signals
            setChildSignals();
            // handle input redirection
            myInputFile(inputs);
            // hanlde outputs and appendages redirection
            myOutputOrAppendageFile(outputs, appendages);
            execv(execFile, argv);
            perror("execv");
        }
        // check for foregorund control; no ampersand
        if (ampersand != 0) {
            int foregroundPid;
            // wait for foreground processes
            if ((foregroundPid = waitpid(pid, &status, WUNTRACED)) == -1) {
                perror("waitpid foreground");
                cleanup_job_list(joblist);
                exit(1);
            }
            // check termination foreground
            if (WIFSIGNALED(status)) {
                printf("[%d] (%d) terminated by signal %d\n", jidCounter,
                       foregroundPid, WTERMSIG(status));
            }
            // check suspension of foreground
            if (WIFSTOPPED(status)) {
                // if suspended then add to list
                if (add_job(joblist, jidCounter, foregroundPid, STOPPED,
                            execFile) == -1) {
                    fprintf(stderr, "add job failed\n");
                    cleanup_job_list(joblist);
                    exit(1);
                }
                printf("[%d] (%d) suspended by signal %d\n", jidCounter,
                       foregroundPid, WSTOPSIG(status));
                jidCounter = jidCounter + 1;
            }
            if(WIFCONTINUED(status)){
                if(update_job_jid(joblist, get_job_jid(joblist,
                foregroundPid), RUNNING) ==-1){
                    fprintf(stderr, "update job failed");
                    cleanup_job_list(joblist);
                    exit(1);
                }
                printf("[%d] (%d) resumed\n", get_job_jid(joblist,
                foregroundPid), foregroundPid);
            }
        }
        // restore control back to shell once foreground process is complete
        if (tcsetpgrp(STDIN_FILENO, shellPgid) == -1) {
            perror("tcsetprgp failed");
            cleanup_job_list(joblist);
            exit(1);
        }
        // for background processes after execution add job
        if (ampersand == 0) {
            if (add_job(joblist, jidCounter, pid, RUNNING, execFile) == -1) {
                fprintf(stderr, "add job failed");
                cleanup_job_list(joblist);
                exit(1);
            }
            printf("[%d] (%d)\n", jidCounter, getpid());
            jidCounter = jidCounter + 1;
        }
        memset(buffer, '\0', 1024);
    }
    cleanup_job_list(joblist);
    return 0;
}
