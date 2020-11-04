#ifndef CLEMULATOR_PROCESS_UTIL
#define CLEMULATOR_PROCESS_UTIL
void remove_zombies(int n);
int perform_redirect(char* filename, int strem_fd, int flags);
int check_and_perform_redirect(char* argv[], command_modifier cmd_mod);
void perform_cd_command(const char* dir);
void perform_command_w_pid(char* argv[], int pid, command_modifier cmd_mod);
void perform_single_command(char** argv, command_modifier cmd_mod);
int wait_pid_arr(int* pids, int size);
void perform_pipe(char*** piped, int num_pipes, command_modifier cmd_mod);
void perform_command(char* argv[], command_modifier cmd_mod);

#endif
