#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>
#include <glob.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>


#define C_EXTERNAL 0
#define C_EXIT 1
#define C_CD 2
#define C_JOBS 3
#define C_FG 4
#define C_BG 5

#define JOBS 10
#define PATH 1024
#define COMMAND 1024
#define TOKEN 128
#define TOKEN_DELIMITERS " \t\r\n\a"



#define BACKGROUND 0
#define FOREGROUND 1
#define PIPELINE 2


#define COLOR_GRAY "\033[1;30m"
#define COLOR_NONE "\033[m"
#define COLOR_GREENBACKGROUND "\[\033[42m\]"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_GREEN "\033[0;32;32m"

#define RUNNING_STATUS 0
#define DONE_STATUS 1
#define SUSPENDED_STATUS 2
#define CONTINUED_STATUS 3
#define TERMINATED_STATUS 4

#define P_F_ALL 0
#define P_F_DONE 1
#define P_F_REMAINING 2





const char* STATUS_STRING[] = {
    "running",
    "done",
    "suspended",
    "continued",
    "terminated"
};


struct shell_info {
  char cur_user[TOKEN];
  char cur_dir[PATH];
  char pw_dir[PATH];
  struct Job *jobs[JOBS + 1];
};


struct Job {
  int id;
  struct Process *root;
  char *command;
  pid_t pgid;
  int mode;
};


struct Process {
    char *command;
    int argc;
    char **argv;
    char *input_direction;
    char *output_direction;
    pid_t pid;
    int type;
    int status;
    struct Process *next;
};



struct shell_info *shell;

int getJobIdByPid(int pid) {
    int i;
    struct Process *proc;

    for (i = 1; i <= JOBS; i++) {
        if (shell->jobs[i] != NULL) {
            for (proc = shell->jobs[i]->root; proc != NULL; proc = proc->next) {
                if (proc->pid == pid) {
                    return i;
                }
            }
        }
    }

    return -1;
}

struct Job* getJobById(int id) {
    if (id > JOBS) {
        return NULL;
    }

    return shell->jobs[id];
}

int getPgidByJobId(int id) {
    struct Job *job = getJobById(id);

    if (job == NULL) {
        return -1;
    }

    return job->pgid;
}

int getProcCount(int id, int filter) {
    if (id > JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    int count = 0;
    struct Process *proc;
    for (proc = shell->jobs[id]->root; proc != NULL; proc = proc->next) {
        if (filter == P_F_ALL ||
            (filter == P_F_DONE && proc->status == DONE_STATUS) ||
            (filter == P_F_REMAINING && proc->status != DONE_STATUS)) {
            count++;
        }
    }

    return count;
}

int getNextJobId() {
    int i;

    for (i = 1; i <= JOBS; i++) {
        if (shell->jobs[i] == NULL) {
            return i;
        }
    }

    return -1;
}

int printProcessesOfJob(int id) {
    if (id > JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    printf("[%d]", id);

    struct Process *proc;
    for (proc = shell->jobs[id]->root; proc != NULL; proc = proc->next) {
        printf(" %d", proc->pid);
    }
    printf("\n");

    return 0;
}

int printJobStatus(int id) {
    if (id > JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    printf("[%d]", id);

    struct Process *proc;
    for (proc = shell->jobs[id]->root; proc != NULL; proc = proc->next) {
        printf("\t%d\t%s\t%s", proc->pid,
            STATUS_STRING[proc->status], proc->command);
        if (proc->next != NULL) {
            printf("|\n");
        } else {
            printf("\n");
        }
    }

    return 0;
}

int releaseJob(int id) {
    if (id > JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    struct Job *job = shell->jobs[id];
    struct Process *proc, *tmp;
    for (proc = job->root; proc != NULL; ) {
        tmp = proc->next;
        free(proc->command);
        free(proc->argv);
        free(proc->input_direction);
        free(proc->output_direction);
        free(proc);
        proc = tmp;
    }

    free(job->command);
    free(job);

    return 0;
}

int insertJob(struct Job *job) {
    int id = getNextJobId();

    if (id < 0) {
        return -1;
    }

    job->id = id;
    shell->jobs[id] = job;
    return id;
}

int removeJob(int id) {
    if (id > JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    releaseJob(id);
    shell->jobs[id] = NULL;

    return 0;
}

int isJobCompleted(int id) {
    if (id > JOBS || shell->jobs[id] == NULL) {
        return 0;
    }

    struct Process *proc;
    for (proc = shell->jobs[id]->root; proc != NULL; proc = proc->next) {
        if (proc->status != DONE_STATUS) {
            return 0;
        }
    }

    return 1;
}

int setProcessStatus(int pid, int status) {
    int i;
    struct Process *proc;

    for (i = 1; i <= JOBS; i++) {
        if (shell->jobs[i] == NULL) {
            continue;
        }
        for (proc = shell->jobs[i]->root; proc != NULL; proc = proc->next) {
            if (proc->pid == pid) {
                proc->status = status;
                return 0;
            }
        }
    }

    return -1;
}

int setJobStatus(int id, int status) {
    if (id > JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    int i;
    struct Process *proc;

    for (proc = shell->jobs[id]->root; proc != NULL; proc = proc->next) {
        if (proc->status != DONE_STATUS) {
            proc->status = status;
        }
    }

    return 0;
}

int waitForPid(int pid) {
    int status = 0;

    waitpid(pid, &status, WUNTRACED);
    if (WIFEXITED(status)) {
        setProcessStatus(pid, DONE_STATUS);
    } else if (WIFSIGNALED(status)) {
        setProcessStatus(pid, TERMINATED_STATUS);
    } else if (WSTOPSIG(status)) {
        status = -1;
        setProcessStatus(pid, SUSPENDED_STATUS);
    }

    return status;
}

int waitForJob(int id) {
    if (id > JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    int proc_count = getProcCount(id, P_F_REMAINING);
    int wait_pid = -1, wait_count = 0;
    int status = 0;

    do {
        wait_pid = waitpid(-shell->jobs[id]->pgid, &status, WUNTRACED);
        wait_count++;

        if (WIFEXITED(status)) {
            setProcessStatus(wait_pid, DONE_STATUS);
        } else if (WIFSIGNALED(status)) {
            setProcessStatus(wait_pid, TERMINATED_STATUS);
        } else if (WSTOPSIG(status)) {
            status = -1;
            setProcessStatus(wait_pid, SUSPENDED_STATUS);
            if (wait_count == proc_count) {
                printJobStatus(id);
            }
        }
    } while (wait_count < proc_count);

    return status;
}

int getCommandType(char *command) {
    if (strcmp(command, "exit") == 0) {
        return C_EXIT;
    } else if (strcmp(command, "cd") == 0) {
        return C_CD;
    } else if (strcmp(command, "jobs") == 0) {
        return C_JOBS;
    } else if (strcmp(command, "fg") == 0) {
        return C_FG;
    } else if (strcmp(command, "bg") == 0) {
        return C_BG;
    } else {
        return C_EXTERNAL;
    }
}

char* helperStrtrim(char* line) {
    char *head = line, *tail = line + strlen(line);

    while (*head == ' ') {
        head++;
    }
    while (*tail == ' ') {
        tail--;
    }
    *(tail + 1) = '\0';

    return head;
}

void update_cwd_info() {
    getcwd(shell->cur_dir, sizeof(shell->cur_dir));
}

int cd(int argc, char** argv) {
    if (argc == 1) {
        chdir(shell->pw_dir);
        update_cwd_info();
        return 0;
    }

    if (chdir(argv[1]) == 0) {
        update_cwd_info();
        return 0;
    } else {
        printf("shell: cd %s: No such file or directory\n", argv[1]);
        return 0;
    }
}

int jobs(int argc, char **argv) {
    int i;

    for (i = 0; i < JOBS; i++) {
        if (shell->jobs[i] != NULL) {
            printJobStatus(i);
        }
    }

    return 0;
}

int fg(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: fg <pid>\n");
        return -1;
    }

    int status;
    pid_t pid;
    int job_id = -1;

    if (argv[1][0] == '%') {
        job_id = atoi(argv[1] + 1);
        pid = getPgidByJobId(job_id);
        if (pid < 0) {
            printf("shell: fg %s: no such job\n", argv[1]);
            return -1;
        }
    } else {
        pid = atoi(argv[1]);
    }


    tcsetpgrp(0, pid);

    if (job_id > 0) {
        setJobStatus(job_id, CONTINUED_STATUS);
        printJobStatus(job_id);
        if (waitForJob(job_id) >= 0) {
            removeJob(job_id);
        }
    } else {
        waitForPid(pid);
    }

    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(0, getpid());
    signal(SIGTTOU, SIG_DFL);

    return 0;
}

int bg(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: bg <pid>\n");
        return -1;
    }

    pid_t pid;
    int job_id = -1;

    if (argv[1][0] == '%') {
        job_id = atoi(argv[1] + 1);
        pid = getPgidByJobId(job_id);
        if (pid < 0) {
            printf("shell: bg %s: no such job\n", argv[1]);
            return -1;
        }
    } else {
        pid = atoi(argv[1]);
    }


    if (job_id > 0) {
        setJobStatus(job_id, CONTINUED_STATUS);
        printJobStatus(job_id);
    }

    return 0;
}





int shell_exit() {
    printf("exit!\n");
    exit(0);
}

void check_zombie() {
    int status, pid;
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED|WCONTINUED)) > 0) {
        if (WIFEXITED(status)) {
            setProcessStatus(pid, DONE_STATUS);
        } else if (WIFSTOPPED(status)) {
            setProcessStatus(pid, SUSPENDED_STATUS);
        } else if (WIFCONTINUED(status)) {
            setProcessStatus(pid, CONTINUED_STATUS);
        }

        int job_id = getJobIdByPid(pid);
        if (job_id > 0 && isJobCompleted(job_id)) {
            printJobStatus(job_id);
            removeJob(job_id);
        }
    }
}

void sigintHandler(int signal) {
    printf("\n");
}

int executeBuiltinCommand(struct Process *proc) {
    int status = 1;

    switch (proc->type) {
        case C_EXIT:
            shell_exit();
            break;
        case C_CD:
            cd(proc->argc, proc->argv);
            break;
        case C_JOBS:
            jobs(proc->argc, proc->argv);
            break;
        case C_FG:
            fg(proc->argc, proc->argv);
            break;
        case C_BG:
            bg(proc->argc, proc->argv);
            break;
        default:
            status = 0;
            break;
    }

    return status;
}

int launchProcess(struct Job *job, struct Process *proc, int in_fd, int out_fd, int mode) {
    proc->status = RUNNING_STATUS;
    if (proc->type != C_EXTERNAL && executeBuiltinCommand(proc)) {
        return 0;
    }

    pid_t childpid;
    int status = 0;

    childpid = fork();

    if (childpid < 0) {
        return -1;
    } else if (childpid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        proc->pid = getpid();
        if (job->pgid > 0) {
            setpgid(0, job->pgid);
        } else {
            job->pgid = proc->pid;
            setpgid(0, job->pgid);
        }

        if (in_fd != 0) {
            dup2(in_fd, 0);
            close(in_fd);
        }

        if (out_fd != 1) {
            dup2(out_fd, 1);
            close(out_fd);
        }

        if (execvp(proc->argv[0], proc->argv) < 0) {
            printf("shell: %s: command not found\n", proc->argv[0]);
            exit(0);
        }

        exit(0);
    } else {
        proc->pid = childpid;
        if (job->pgid > 0) {
            setpgid(childpid, job->pgid);
        } else {
            job->pgid = proc->pid;
            setpgid(childpid, job->pgid);
        }

        if (mode == FOREGROUND) {
            tcsetpgrp(0, job->pgid);
            status = waitForJob(job->id);
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(0, getpid());
            signal(SIGTTOU, SIG_DFL);
        }
    }

    return status;
}

int launchJob(struct Job *job) {
    struct Process *proc;
    int status = 0, in_fd = 0, fd[2], job_id = -1;

    check_zombie();
    if (job->root->type == C_EXTERNAL) {
        job_id = insertJob(job);
    }

    for (proc = job->root; proc != NULL; proc = proc->next) {
        if (proc == job->root && proc->input_direction != NULL) {
            in_fd = open(proc->input_direction, O_RDONLY);
            if (in_fd < 0) {
                printf("shell: no such file or directory: %s\n", proc->input_direction);
                removeJob(job_id);
                return -1;
            }
        }
        if (proc->next != NULL) {
            pipe(fd);
            status = launchProcess(job, proc, in_fd, fd[1], PIPELINE);
            close(fd[1]);
            in_fd = fd[0];
        } else {
            int out_fd = 1;
            if (proc->output_direction != NULL) {
                out_fd = open(proc->output_direction, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                if (out_fd < 0) {
                    out_fd = 1;
                }
            }
            status = launchProcess(job, proc, in_fd, out_fd, job->mode);
        }
    }

    if (job->root->type == C_EXTERNAL) {
        if (status >= 0 && job->mode == FOREGROUND) {
            removeJob(job_id);
        } else if (job->mode == BACKGROUND) {
            printProcessesOfJob(job_id);
        }
    }

    return status;
}

struct Process* parseCommandChunk(char *chunk) {
    int bufsize = TOKEN;
    int index = 0;
    char *command = strdup(chunk);
    char *token;
    char **tokens = (char**) malloc(bufsize * sizeof(char*));

    if (!tokens) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(chunk, TOKEN_DELIMITERS);
    while (token != NULL) {
        glob_t glob_buffer;
        int glob_count = 0;
        if (strchr(token, '*') != NULL || strchr(token, '?') != NULL) {
            glob(token, 0, NULL, &glob_buffer);
            glob_count = glob_buffer.gl_pathc;
        }

        if (index + glob_count >= bufsize) {
            bufsize += TOKEN;
            bufsize += glob_count;
            tokens = (char**) realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        if (glob_count > 0) {
            int i;
            for (i = 0; i < glob_count; i++) {
                tokens[index++] = strdup(glob_buffer.gl_pathv[i]);
            }
            globfree(&glob_buffer);
        } else {
            tokens[index] = token;
            index++;
        }

        token = strtok(NULL, TOKEN_DELIMITERS);
    }

    int i = 0, argc = 0;
    char *input_direction = NULL, *output_direction = NULL;
    while (i < index) {
        if (tokens[i][0] == '<' || tokens[i][0] == '>') {
            break;
        }
        i++;
    }
    argc = i;

    for (; i < index; i++) {
        if (tokens[i][0] == '<') {
            if (strlen(tokens[i]) == 1) {
                input_direction = (char *) malloc((strlen(tokens[i + 1]) + 1) * sizeof(char));
                strcpy(input_direction, tokens[i + 1]);
                i++;
            } else {
                input_direction = (char *) malloc(strlen(tokens[i]) * sizeof(char));
                strcpy(input_direction, tokens[i] + 1);
            }
        } else if (tokens[i][0] == '>') {
            if (strlen(tokens[i]) == 1) {
                output_direction = (char *) malloc((strlen(tokens[i + 1]) + 1) * sizeof(char));
                strcpy(output_direction, tokens[i + 1]);
                i++;
            } else {
                output_direction = (char *) malloc(strlen(tokens[i]) * sizeof(char));
                strcpy(output_direction, tokens[i] + 1);
            }
        } else {
            break;
        }
    }

    for (i = argc; i <= index; i++) {
        tokens[i] = NULL;
    }

    struct Process *created_proc = (struct Process*) malloc(sizeof(struct Process));
    created_proc->command = command;
    created_proc->argv = tokens;
    created_proc->argc = argc;
    created_proc->input_direction = input_direction;
    created_proc->output_direction = output_direction;
    created_proc->pid = -1;
    created_proc->type = getCommandType(tokens[0]);
    created_proc->next = NULL;
    return created_proc;
}

struct job* parseCommand(char *line) {
    line = helperStrtrim(line);
    char *command = strdup(line); // return pointer of line

    struct Process *root_proc = NULL, *proc = NULL;
    char *line_cursor = line, *c = line, *seg;
    int seg_len = 0, mode = FOREGROUND;

    if (line[strlen(line) - 1] == '&') {
        mode = BACKGROUND;
        line[strlen(line) - 1] = '\0';
    }

    while (1) {
        if (*c == '\0' || *c == '|') {
            seg = (char*) malloc((seg_len + 1) * sizeof(char));
            strncpy(seg, line_cursor, seg_len);
            seg[seg_len] = '\0';

            struct Process* created_proc = parseCommandChunk(seg);
            if (!root_proc) {
                root_proc = created_proc;
                proc = root_proc;
            } else {
                proc->next = created_proc;
                proc = created_proc;
            }

            if (*c != '\0') {
                line_cursor = c;
                while (*(++line_cursor) == ' ');
                c = line_cursor;
                seg_len = 0;
                continue;
            } else {
                break;
            }
        } else {
            seg_len++;
            c++;
        }
    }

    struct Job *new_job = (struct Job*) malloc(sizeof(struct Job));
    new_job->root = root_proc;
    new_job->command = command;
    new_job->pgid = -1;
    new_job->mode = mode;
    return new_job;
}

char* readLine() {
    int bufsize = COMMAND;
    int index = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[index] = '\0';
            return buffer;
        } else {
            buffer[index] = c;
        }
        index++;

        if (index >= bufsize) {
            bufsize += COMMAND;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void printPromt() {
  printf(COLOR_CYAN "%s" COLOR_NONE " in " COLOR_YELLOW "%s" COLOR_NONE "\n", shell->cur_user, shell->cur_dir);
  printf(COLOR_GREENBACKGROUND "Seungjin>" COLOR_NONE " ");
}


void loop() {
    char *line;
    struct Job *job;
    int status = 1;

    while (1) {
        printPromt();
        line = readLine();
        if (strlen(line) == 0) {
            check_zombie();
            continue;
        }
        job = parseCommand(line);
        status = launchJob(job);
    }
}



void init() {
    struct sigaction sigint_action = {
        .sa_handler = &sigintHandler,
        .sa_flags = 0
    };
    sigemptyset(&sigint_action.sa_mask);
    sigaction(SIGINT, &sigint_action, NULL);

    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    pid_t pid = getpid();
    setpgid(pid, pid);
    tcsetpgrp(0, pid);

    shell = (struct shell_info*) malloc(sizeof(struct shell_info));
    getlogin_r(shell->cur_user, sizeof(shell->cur_user));

    struct passwd *pw = getpwuid(getuid());
    strcpy(shell->pw_dir, pw->pw_dir);

    int i;
    for (i = 0; i < JOBS; i++) {
        shell->jobs[i] = NULL;
    }

    update_cwd_info();
}

int main(int argc, char **argv) {
    init();
    loop();

    return EXIT_SUCCESS;
}
