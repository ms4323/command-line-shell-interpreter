/*
CS543 Assignment 2 : Command Line Shell Interpreter
Name: Mahshid Shahmohammadian
Email: ms4323@drexel.edu
Drexel University
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 80 /* 80 chars per line, per command */
#define MAX_ALIAS 100 /* maximum number of aliases */

int parallel;
int indx;
int exec_command;
int should_run;
int num_hist;
int current;
char alias_names[MAX_ALIAS][MAX_LINE+1];
char alias_funcs[MAX_ALIAS][MAX_LINE+1];
int num_alias = 0;
int num_directories = 0;
int diagnostics_on = 0;


/*
 The following function constructs the full path for the execv function given the array of directories defined by set path command
 */
char* construct_directories(int index,char **args,char directories[10][MAX_LINE/2 + 1]){
    
    char *mypath;
    if (strstr(directories[index],args[0]) != NULL){ // the directory already contains the command for example "/bin/ls"
        size_t dir_size = strlen(directories[index]);
        mypath = (char*) malloc(dir_size);
        strcpy(mypath, directories[index]); // just return the directory and not concat anything to it
    }
    else {
        size_t dir_size = strlen(directories[index]);
        size_t size = strlen(args[0]);
        mypath = (char*) malloc(size+dir_size+1);
        strcpy(mypath, directories[index]);
        strcat(mypath, "/");
        strcat(mypath, args[0]);
    }
    
    return mypath;
}

/*
 The following function executes the command (using fork and execv) based on the directories that are defined before in set path, if the command is not special (like history , !! , ! , help , alias , etc)
 
 */
void execute_command(int index,char **args,char directories[10][MAX_LINE/2 + 1]){
    
    int found = 0;
    char *path = construct_directories(index,args,directories);
    
    while ( index < num_directories ){ // search in the defined directories in order to find the executable, if found break the while loop
        if (access(path,F_OK) == -1){
            path = construct_directories(index,args,directories);
            if (diagnostics_on){
                printf("%d) searching for the executable file in:\n",index+1);
                printf("%s\n",path);
            }
            index++; // next directory
        }
        else {
            found = 1;
            break;
        }
        
    } // if the file is not found in all the directories the while loop will end after checking all the directories and then next tries to execute with fork and execv (if not found, it will print error no such command)
    
    pid_t pid_child = fork();
    int status_child;
    
    if (pid_child < 0) {
        // error forking child process
        printf("Error: forking child process failed\n");
        exit(1);
    } else if (pid_child == 0) {
        // fork a child process
        if(execv(path, args) < 0){ //
            printf("Error: no such command, execution failed\n");
            exit(1);
        }
        
    } else { // parent process
        
        if (parallel == 0){
            while (wait(&status_child) != pid_child); // waiting for the child
        }
        else { // if user puts an '&' at the end of the command
            printf("child process is running in parallel\n");
        }
    }
    
    
}

/*
 The following function prints the list of last 10 history of commands in  descending order
 */
void history_list(char hist[12][MAX_LINE+1], int current, int num_hist){

    int hist_indx = num_hist-1;
    int temp = current-2;
    int i;
    for (i = temp; i >= 0; i--){
      if (hist[i]) {
        printf("   %d) %s\n", hist_indx, hist[i]);
        hist_indx--;
      }
    }

}
/*
 The following function tokenizes the command into an array of each word to process the command afterwards
 */
int split_commands(char *comm, char *unwanted , char **args){
    char *tok;
    int l = 0;
    tok = strtok(comm,unwanted); // separate the command line tokens with unwanted characters
    while (tok != NULL) { // until the end of command line
        args[l] = tok; // put every seperate tokens in the args
        l++;
        tok = strtok(NULL,unwanted);
    }// end of command line
    args[l] = NULL;
    return l;
    
}
/*
 The following function defines an alias for any command, after defining the alias the name will run the command only
 */

void alias_func(char *args[MAX_LINE/2 + 1]){
    
    if ((args[1] == NULL) || (strcmp(args[1],"&") == 0)){
        // typing alias is not enough
        printf("Please include the alias name and command\n");
        printf("Usage: alias name \"command\"\n");
    }
    else {
        
        if ((args[2] == NULL) || (strcmp(args[2],"&") == 0)){
            //typing alias name is not enough
            printf("Please include the command\n");
            printf("Usage: alias name \"command\"\n");
        }
        else {
            
            if (strncmp(args[2], "\"", 1) == 0) { // first character is " or not?
                
                memcpy(alias_names[num_alias],args[1],MAX_LINE+1);
                memmove(args[2], args[2]+1, strlen(args[2])); //remove " character from args[2] to get the command (or portion of command)
                
                size_t size2 = strlen(args[2]);
                if ( (args[3] == NULL) || (strcmp(args[3],"&") == 0)){
                    args[2][size2-1] = '\0';
                    memcpy(alias_funcs[num_alias],args[2],MAX_LINE+1); // the command is just one word
                }
                else { // the command is not one word then go for the rest of the command
                    size_t size3 = strlen(args[3]);
                    if ( (args[4] == NULL) || (strcmp(args[4],"&") == 0)){
                        args[3][size3-1] = '\0'; //remove " character from the end of args[3]
                        
                        char *str = (char*) malloc(size2+size3+1);
                        strcpy(str, args[2]);
                        strcat(str, " ");
                        strcat(str, args[3]); // concat args[2] and args[3] to get the command
                        
                        memcpy(alias_funcs[num_alias],str,MAX_LINE+1);
                        free(str);
                    }
                    else { // doing the same for the next words
                        size_t size4 = strlen(args[4]);
                        if ((args[5] == NULL) || (strcmp(args[5],"&") == 0)){
                            args[4][size4-1] = '\0'; //remove " character from the end of args[4]
                            
                            char *str = (char*) malloc(size2+size3+size4+1);
                            strcpy(str, args[2]);
                            strcat(str, " ");
                            strcat(str, args[3]);
                            strcat(str, " ");
                            strcat(str, args[4]); // concat args[2] and args[3] and args[4] to get the command
                            memcpy(alias_funcs[num_alias],str,MAX_LINE+1);
                            free(str);
                        }
                        else {
                            printf("commad is too long!");
                        }
                    }
                }
                
                num_alias++; // one alias has been added to the data structures
                
            }
            else {
                printf("Please put the command in double quotes\n");
                printf("Usage: alias name \"command\"\n");
            }
            
            
            
        }
        
    }
    // it was a special command so later does not execute it with fork and execv
    exec_command = 0;
}

void set_path_func(char *args[MAX_LINE/2 + 1],char temp_line[MAX_LINE],char directories[10][MAX_LINE/2 + 1],int should_print){
    
            // set path is not enough
            if ( (args[2] == NULL) || (strcmp(args[2],"&") == 0)){
                if (should_print){
                    printf("Please include the directories\n");
                    printf("Usage: set path = (directories seperated with space)\n");
                }
            }
            else {
                if(strcmp (args[2],"=") == 0){
                    // user should follow the structure of the command to be run
                    if ( (args[3] == NULL) || (strcmp(args[3],"&") == 0)){
                        if (should_print){
                            printf("Please include the directories\n");
                            printf("Usage: set path = (directories seperated with space)\n");
                        }
                    }
                    else {
                        
                        memmove(temp_line, temp_line+12, MAX_LINE+1); //remove "set path = (" from command to have the directories
                        int ret = split_commands(temp_line," )",args);// split the path and get the number of paths
                        int i;
                        for (i = 0 ; i < num_directories; i++ ){
                            memset(directories[i],0,MAX_LINE+1); // first clear the dirctories to replace the directories to the existing ones
                        }
                        
                        int ii;
                        num_directories = -1; // for the following loop
                        for (ii = 0 ; ii < ret; ii++ ){
                            
                            memcpy(directories[ii],args[ii],MAX_LINE+1); // for each of the paths, put them in the directories data structure
                            num_directories++;
                        }
                        
                        
                    }
                    
                }
                else {
                    if (should_print){
                        printf("Usage: set path = (directories seperated with space)\n");
                    }
                }
                
            }
    // it was a special command so later does not execute it with fork and execv
    exec_command = 0;
    }

void set_verbose_func(char *args[MAX_LINE/2 + 1],char temp_line[MAX_LINE],char directories[10][MAX_LINE/2 + 1],int should_print){
    
    // set verbose is not enough
    if ( (args[2] == NULL) || (strcmp(args[2],"&") == 0)){
        if (should_print){
            printf("Please include \"on\" or \"off\" after \"set verbose\"\n");
            printf("Usage: set verbose on\n");
            printf("Or: set verbose off\n");
        }
    }
    else {
        if(strcmp (args[2],"on") == 0){ // so turning on the dignostics of the shell
            
            if ( (args[3] == NULL) || (strcmp(args[3],"&") == 0)){
                if(diagnostics_on){
                    if (should_print){
                        printf("It is already on!\n");
                    }
                }
                else {
                    diagnostics_on = 1; // in the execute_command function, based on this value decides to print each directories in which it is searchig or not
                    if (should_print){
                        printf("Diagnostics off now\n");
                    }
                }
                
            }
            else {
                if (should_print){
                    printf("Usage: set verbose on\n");
                    printf("Or: set verbose off\n");
                    printf("Additional words not allowed after set verbose on/off\n");
                }
                
            }
            
        }
        else if(strcmp (args[2],"off") == 0){ // so turning on the dignostics of the shell
            if ( (args[3] == NULL) || (strcmp(args[3],"&") == 0)){
                if(!diagnostics_on){
                    if (should_print){
                        printf("It is already off!\n");
                    }
                }
                else {
                    diagnostics_on = 0; // in the execute_command function, based on this value decides to print each directories in which it is searchig or not
                    if (should_print){
                        printf("Diagnostics off now\n");
                    }
                }
                
            }
            else {
                if (should_print){
                    printf("Usage: set verbose on\n");
                    printf("Or: set verbose off\n");
                    printf("Additional words not allowed after set verbose on/off\n");
                }
                
            }
        }
        else {
            if (should_print){
                printf("Usage: set verbose on\n");
                printf("Or: set verbose off\n");
            }
        }
        
    }
    // it was a special command so later does not execute it with fork and execv
    exec_command = 0;
    
    }




void interpret_commands(char *args[MAX_LINE/2 + 1],char hist[12][MAX_LINE+1], char res_command[MAX_LINE+1],char temp_line[MAX_LINE],char directories[10][MAX_LINE/2 + 1]){


        ///////////* handle special commands to interpret *//////

        /*

         1) if first token of command is "exit"
         then do not execute the command and get out of while loop
         (exit the shell)

        */
        if (strcmp(args[0],"exit") == 0){

            should_run = 0; // so do not continue the while loop in main function which gets new commands
 
        }

        /*

         2) if first token of command is "history"
         then list the history of commands or depending on the next token show the most recent command, nth command
        */
        else if(strcmp(args[0],"history") == 0){
            if ( (args[1] == NULL) || (strcmp(args[1],"&") == 0) ){
                history_list(hist,current,num_hist); // call the history printing function
            }
            else {
                printf("No such command, additional argument is not allowed for history command.\n");
                printf("Usage: history \n");
            }
            
            exec_command = 0; // we do not run anything, just printing
        }

        /*

         3) if first token of command is "!!"
         then the last command will be printed

        */
        else if ((strcmp(args[0],"!!") == 0)){

            if (args[1] == NULL) { // it should not have the next word
                int hist_indx = current-2;

                if (hist_indx < 0 ){
                    printf("No commands found in history\n");
                    // it was a special command so later does not execute it with fork and execv
                    exec_command = 0;
                }
                else{
                    while (strcmp(hist[hist_indx],"!!") == 0) {
                        hist_indx--;
                    }
                    
                    printf("%s\n", hist[hist_indx]);
                    res_command = hist[hist_indx]; // latest command stored

                    int ret = split_commands(res_command," \n\t\r",args); // tokenize the command
                    interpret_commands(args,hist,res_command,temp_line,directories); // process it

                }
            }
            else {
                printf("No such command, additional argument is not allowed for !! command.\n");
                printf("Usage: !! \n");
                // it was a special command so later does not execute it with fork and execv
                exec_command = 0;

            }


        }

        /*

         4) if first token of command includes "!" , meaning !n
         then the nth command will be printed

        */
        else if (strncmp(args[0], "!", 1) == 0){ // first character is ! or not?

            char temp_arg = args[0];
            memmove(args[0], args[0]+1, strlen(args[0])); //remove ! character from args[0] to get the number
            if (args[1] == NULL){

                int nth = atoi(args[0]); // convert it to integer
                int start = 1; // the range of the histories
                if (num_hist > 10){
                    start = num_hist-10 +1;
                }
                int end = num_hist; // end of the range of the histories

                if (atoi(args[0]) == NULL){ // index is not a number
                    printf("index must be a number\n");
                    printf("Usage: !number \n");
                    exec_command = 0;
                }
                else { // index is a number
                    args[0] = temp_arg; // bring the ! character back to store in history correctly
                    if (nth > end){
                        printf("No such command in history\n");
                        // it was a special command so later does not execute it with fork and execv
                        exec_command = 0;
                    }
                    else if (nth < start){
                        printf("No such command in history\n");
                        // it was a special command so later does not execute it with fork and execv
                        exec_command = 0;
                    }
                    
                    //two cases when n bigger than 10 or less than 10
                    else if (nth < 10){
                        int hist_indx;
                        if (start < 0) {
                            hist_indx = nth;
                        }
                        else if (start == 1){
                            hist_indx = nth - start;
                        }
                        else {
                            hist_indx = nth - start + 1;
                        }
                        // now print the command found, and then interpret it again
                        printf("%s\n", hist[hist_indx]);
                        res_command = hist[hist_indx];
                        int ret = split_commands(res_command," \n\t\r",args);
                        interpret_commands(args,hist,res_command,temp_line,directories);
                    }
                    else if (nth >= 10){
                        int hist_indx = nth - start +1;
                        // now print the command found, and then interpret it again
                        printf("%s\n", hist[hist_indx]);
                        res_command = hist[hist_indx];
                        int ret = split_commands(res_command," \n\t\r",args);
                        interpret_commands(args,hist,res_command,temp_line,directories);
                    }
                }

            }
            else {
                printf("No such command, additional argument is not allowed for !n command.\n");
                printf("Usage: !number \n");
                // it was a special command so later does not execute it with fork and execv
                exec_command = 0;
            }

        }


        /*

         5) if first token of command is "cd"
         then we look at the second argument to determine the directory to change to

         */
        else if (strcmp (args[0],"cd") == 0){
            int ret;
            if ((args[1] == NULL) || (strcmp(args[1],"&") == 0)){
                printf("Please include the path to change directory\n");
                // it was a special command so later does not execute it with fork and execv
                exec_command = 0;
            } else {
                ret = chdir(args[1]); // change directory using chdir
                // it was a special command so later does not execute it with fork and execv
                exec_command = 0;
                if (ret != 0){ // could not find the folder in the existing path
                  printf("No such directory\n");
                }
            }
        }

        /*

         6) if first token of command is "alias"
         then store the name and the functionality of the alias in .cs543rc file

        */
        else if (strcmp (args[0],"alias") == 0) {

            alias_func(args);
        }
    
        /*
         
         7) if first token of command is "set"
         7.a) and second token is "path" then
         set path to the directory user defines
         7.b) and second token is "verbose" and third token is "on or off" then
         turns the dignostics on or off
        */
    
        else if(strcmp (args[0],"set") == 0){
            
            if ( (args[1] == NULL) || (strcmp(args[1],"&") == 0)){
                printf("No such command\n");
                printf("Do you mean \"set path\" or \"set verbose\" ?\n");
            }
            else {
                
                if(strcmp (args[1],"path") == 0){ // set path function
            
                    set_path_func(args,temp_line,directories,1); // print anything (last argument) , since it is not initializing from .cs543rc file
                }
                
                else if(strcmp (args[1],"verbose") == 0){ // set verbose function
                    set_verbose_func(args,temp_line,directories,1); // print anything (last argument) , since it is not initializing from .cs543rc file
                }
                
                else {
                    printf("No such command\n");
                }
            }
            // it was a special command just to print and not to run anything
            exec_command = 0;
        }

        /*

         8) help function to print the defined and available commands to execute

        */

        else if(strcmp (args[0],"help") == 0){
          if ( (args[1] != NULL) && (strcmp(args[1],"&") != 0)){
             printf("Usage: help\n");
          }
          else {
            printf("Bellow is the list of defined commands:\n");
            printf("Note: like linux shell the commands are case sensitive.\n");
            printf("    exit  =>  exits the shell\n");
            printf("    cd path => changes the directory to path\n");
            printf("    history => lists the last 10 commands\n");
            printf("    !!  =>  shows the lastest command and executes it\n");
            printf("    !n  =>  shows the nth command in history, if it exists in 10 element sized buffer, and executes it\n");
            printf("    alias name \"command\"  =>  assigns an alias command called name for the command user defines\n");
            printf("    (Defined functions via alias are executed using the name that user assigns for alias.)\n");
            printf("    set path = (directories seperated with space) => sets directories in which shell searches for executables for the command\n");
            printf("    Commands found in the PATH (or set path directories) are also executable eg. ls, pwd, date, ...\n");
            printf("    set verbose on/off => turns on/off the diagnostics, so when searching for an executables in the directories, it prints the corresponding directories\n");
            printf("    bash file.sh => read a file with .sh extension, and run commands in the file line by line\n");
          }
          // it was a special command just to print and not to run anything
          exec_command = 0;
        }
    
        /*
         
         9) bash function to run the commands from a .sh file just like bash -x command in linux
         
         */
    
        else if(strcmp (args[0],"bash") == 0){
            if ( (args[1] == NULL) && (strcmp(args[1],"&") == 0)){
                // needing the name of the file
                printf("Please include the name of the file to run\n");
                printf("Usage: bash file.sh\n");
                exec_command = 0;
            }
            else {
                if ( strstr(args[1],".sh") != NULL ){
                    
                    FILE *file;
                    file = fopen (args[1],"r"); // open the file to read the commands
                    if (file == NULL){
                        printf("Error: could not open the bash file\n");
                        exit(1);
                    }
                    
                    char *fline = NULL;
                    size_t len = 0;
                    ssize_t read;
                    char *fline_args[MAX_LINE/2 + 1];
                    char temp_fline[MAX_LINE+1];
                    
                    int setpath_exists = 0;
                    while( (read = getline(&fline, &len, file)) != -1){ // read commands line by line
                        
                        strcpy(temp_fline,fline);
                        int ret = split_commands(fline," \n\t\r",fline_args); // tokenize the command into its words
                        
                        printf("+ "); // when we use bash -x file.sh in linux terminal, it print "+" for each command, so I used it also
                        printf("%s\n",temp_fline);
                        interpret_commands(fline_args,hist,res_command,temp_fline,directories); // see if special command or not?
                        if (exec_command){
                            // if not special commans execute it using executables in paths
                            execute_command(0,fline_args,directories);
                        }
                        
                    }
                    fclose(file); // finish reading the .sh file
                    
                }
                else {
                    printf("Please include a .sh file\n");
                    printf("Usage: bash file.sh\n");
                    // it was a special command so later does not execute it with fork and execv
                    exec_command = 0;
                }
            }
            
            
            
        }

        /*

         10) now check for the user defined aliases

        */

        else {

            int alias_indx;

            for (alias_indx = MAX_ALIAS-1 ; alias_indx>= 0; alias_indx--){ // search in descending order to execute the overwriten aliases as the latest one

                if(alias_names[alias_indx] != NULL){

                    if (strcmp (args[0],alias_names[alias_indx]) == 0){ // if command matches each alias, call interpret_commands for the function of the alias
                        
                        strcpy(res_command , alias_funcs[alias_indx]);
                        int ret = split_commands(res_command," \n\t\r",args);
                        interpret_commands(args,hist,res_command,temp_line,directories);

                    }
                }

            }

        }


}



int main(void){

    fclose(fopen(".cs543rc","a")); // make sure file exists

    char directories[10][MAX_LINE/2 + 1]; // directories to search for the executables
    
    /*
     first read the initialization file for "alias" or "set path" or "set verbose" commands
     */
    FILE *rfile;
    rfile = fopen (".cs543rc","r");
    if (rfile == NULL){
        printf("Error: could not open the .cs543rc file\n");
        exit(1);
    }
    
    char *fline = NULL;
    size_t len = 0;
    ssize_t read;
    char *fline_args[MAX_LINE/2 + 1];
    char temp_fline[MAX_LINE+1];

    int setpath_exists = 0;
    while( (read = getline(&fline, &len, rfile)) != -1){ // read file line by line and process
        
        strcpy(temp_fline,fline);
        int ret = split_commands(fline," \n\t\r",fline_args); // tokenize the command

        /*
         initialize "set path" and "set verbose" commands
        */
        if (strcmp(fline_args[0],"set") == 0){
            
            if ( (fline_args[1] != NULL) && (strcmp(fline_args[1],"&") != 0)){
                if(strcmp (fline_args[1],"path") == 0){ // set path function
                    
                    set_path_func(fline_args,temp_fline,directories,0); // but do not print anything (last argument)!
                    setpath_exists = 1;
                }
                
                else if(strcmp (fline_args[1],"verbose") == 0){ // set verbose function
                    set_verbose_func(fline_args,temp_fline,directories,0); // but do not print anything (last argument)!
                }
            }
            
        }

        /*
         initialize alias commands
         */
        else if (strcmp(fline_args[0],"alias") == 0){
            alias_func(fline_args);
        }
        
    }
    fclose(rfile); // finish reading the initialization file
    
    if (!setpath_exists) { // if there was no "set path" definition in the file, the shell has to know where to search for executables, so I set the following directories to the path directories at first
        
        memcpy(directories[0],"/bin",MAX_LINE+1);
        memcpy(directories[1],"/usr/bin",MAX_LINE+1);
        num_directories+=2;

    
    }

    char *args[MAX_LINE/2 + 1];    /* command line (of 80) has max of 40 arguments */
    should_run = 1;
    char *line = NULL;
    char temp_line[MAX_LINE+1];
    size_t size = 0;
    ssize_t ssize = 0;
    char hist[12][MAX_LINE+1]; // for keeping history records
    char temp_hist[12][MAX_LINE+1];
    num_hist = 0;
    current = 0; // keeping track of history
    exec_command = 1; // executable command (being fork() and execvp())


    while(should_run){ // getting command

        parallel = 0;
        printf("osh> ");
        fflush(stdout);
        exec_command = 1; // first we suppose the commans is executable and after processing the words of the commans we decide to execute with fork and execv or treat it as a special command
        ssize = getline(&line, &size, stdin); // read the command line
        strcpy(temp_line,line);
        int args_size = sizeof(args);
        char res_command[MAX_LINE+1];
        
        // Enter means run again
        if (strcmp(line,"\n") == 0)
        {
            continue;
        }

        indx = split_commands(line," \n\t\r",args); // tokenize the command into its words
        memcpy(hist[current],temp_line,MAX_LINE+1); // puting the current command in the history storage

        
        /*
         after 10 commands, command 0 will be discarded,
         other commands will be moved one step higher,
         then new command will be pushed to the hist
         
         */
        int ii, jj;
        if (current >= 11){
            current = 11;
            for (ii = 1; ii < 12; ii++){
                for (jj = 0; jj < MAX_LINE+1 ; jj++){
                    temp_hist[ii-1][jj] = hist[ii][jj];
                }
            }
            for (ii = 0 ; ii < 12; ii++){
                for (jj = 0; jj < MAX_LINE+1 ; jj++){
                    hist[ii][jj] = temp_hist[ii][jj];
                }
            }
        }
        else{
            current++;
        }

        num_hist++;

        /*
         
         if "&" is at the end of the command
         so it should not wait on the process
         and just execute in parallel with the chid
         
         */
        
        if ( strcmp(args[indx-1],"&") == 0){
            parallel = 1;
        }
        
        // see if the command is special (defined by me)
        interpret_commands(args,hist,res_command,temp_line,directories);

        // if not special, exec_command will be one
        if (should_run && exec_command){

            // if not special, execute using fork and execv (searching for the executables in the specified directories)
            execute_command(0,args,directories);

        }

    }

    fclose(fopen(".cs543rc","w")); // clear the file, because the aliases and the paths should only remain till the end of shell's life
    return 0;
}
