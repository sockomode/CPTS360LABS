#include "functions.c"

char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", 
"rm", "reload", "save", "quit", 0};  // fill with list of commands
// other global variables

int find_command(char *user_command)
{
	int i = 0;
	while(cmd[i]){
		if (strcmp(user_command, cmd[i])==0){
            if(i == 2){
                return i;
            }
			return i;
		}
		i++;
	}
	return -1;
}

int main() {

	initialize();
	char *input = malloc(sizeof(char) * 100), *cmd = malloc(sizeof(char) * 8), *pathname = malloc(sizeof(char) * 1000);
	char *token = malloc(sizeof(char) * 100); const char buffer[2] = " "; char *newline_char=NULL;

	int i = 0; int found; int success;
	while(1)
    {
		strcpy(input, "");
		printf("Enter command: ");
		fgets(input,100,stdin);
		if((newline_char=strchr(input,'\n')) != NULL){
    		*newline_char = '\0';
  		}

        cmd = strtok(input, " ");
        cmd[strcspn(cmd, "\n")] = 0;
        pathname = strtok(NULL," ");

		if(pathname == NULL)
        {
			pathname == malloc(sizeof(char) * 100);
			strcpy(pathname,cmd);
		}
        if(pathname != NULL)
        {
            pathname[strcspn(pathname, "\n")] = 0;
        }

        found = find_command(cmd);
		switch(found){
			case 0: //mkdir function
				my_mkdir_function(cwd,pathname);
				break;
			case 1: //rmdir function
                my_rmdir_function(pathname,cwd);
				break;
			case 2: //ls function
				my_ls_function(cwd,pathname);
				break;
			case 3: //cd function
                my_cd_function(&cwd,pathname);
				break;
			case 4: //pwd function
                my_pwd_function(cwd);
				break;
			case 5: //creat function
                my_creat_function(cwd,pathname);
				break;
			case 6: //rm function
                my_rm_function(cwd,pathname);
				break;
			case 7: //reload function
				break;
			case 8: //save function 
				break;
			case 9: // quit
				return 0;
			default: 
				printf("command not found\n");
				break;
		}
		i++;
	}
	return 0;
}
