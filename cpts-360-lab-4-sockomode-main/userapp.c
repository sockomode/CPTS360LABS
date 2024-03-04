#include "userapp.h"

void register_process(unsigned int pid) 
{
    char cmd[100];
    memset(cmd, '\0', 100);
    sprintf(cmd, "echo %u > /proc/kmlab/status", pid); //writes to /proc/kmlab/status
    system(cmd);
}

int main(int argc, char* argv[])
{
    int expire = 10;
    time_t start_time = time(NULL);

    if (argc == 2) {
        expire = atoi(argv[1]);
    }

    register_process(getpid());

    // Terminate user application if the time is expired
    while (1) {
        if ((int)(time(NULL) - start_time) > expire) {
            break;
        }
    }

	return 0;
}