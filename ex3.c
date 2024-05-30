/*
name: elya athlan
id: 318757200
exersice: 3
########################## impotant compilation info ##########################
################# compilation command: gcc ex3.c -o ex3 ###################
###############################################################################
mini shell, 
in addition the program store all the command history in file : file.txt
supourt all regular terminal command exept "cd",
support history command execution use "!<i>" when i is the index of command stored in the file.txt file,
support pipe using '|' or '||'
support '&' at end of command (last character only) - run in background
support 'nohup' at the beginning of the command - the command will be excuting and ignoring all signals or inputs from the user, the output of the command can be found in "nohup.txt"
special command "done" summerize total commands and total pipes of the current run and terminate the program
*/
#define N 510
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
//#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

void count_arguments(const char*, int*, int*);      //recive string count words and set flag for diffrant type of command         
char** str_to_array(const char*, int);              //split string by space and store the words in array
void free_str_array(char**, int);                   //free dynamicly allocated strint array
void input_str(char*);                              //get user input
void read_from_buffer();                            //use to pipe output from son to parent process  (buffer filename = tmp)
void write_to_history_file(const char*);            // func to write the inputed string to the history file ("file.txt")
void print_current_dir();                           //print "current_directory>"
void print_done(int, int);                          //print when done typed
int get_nd_line_from_history(int, char*);           //get the <i>nd command from file.txt return 1 if successes
int extract_positive_int_from_str(char*);           //extract num from "!num" command
void print_history();                               //func to print history command
void history_case(int*);                            // traet history command
int get_num_of_pipes(char* input);                  // get the num of pipe in inputed command
void free_bg_data(int);
int power(int, int);
int nohup(const char* input);
int bg_pro = 0, bg_pid = 0, nohup_state=0;                         // global variables to store '&' state case and nohup

int main(){
    int words_counter = 0, commands_counter =0, history_successes = 1, pipes_counter=0, status; // history_successes - boolean to store status of get_nd_line_from_history() func
    while(1){
        print_current_dir();
        char** command;
        char input[N+1];
        input_str(input);
        commands_counter++;
        if(input[0] == ' ' || input[strlen(input)-1] == ' '){
            perror("no input\n");
            continue;
        }
        if(input[strlen(input)-1]=='&'){
            bg_pro=1;
            input[strlen(input)-1]='\0';
            signal(SIGCHLD, free_bg_data);
        }
        nohup_state=nohup(input);
        if(nohup_state){
            //set mask to ignore all signal
            sigset_t mask;  
            sigfillset(&mask);
            sigprocmask(SIG_SETMASK, &mask, NULL);
            //remove the 'nohup' from the command for execution
            int i=0;
            while(i<strlen(input)-6){
                input[i]=input[i+6];
                i++;
            }
            input[i]='\0';
        }
        int words = 0, flag = 0;                        //init word counter of current command, init flag for type of command [0-regular,1-cd,2-done,3-!num] 
        int num_of_pipes= get_num_of_pipes(input);
        if(num_of_pipes)                                //pipe command case    
            flag = 5;
        else              
            count_arguments(input, &words, &flag);
        if(flag < 2)                           //not "done" or "!num" case
            words_counter += words;   
        if(flag==4){
            history_case(&words_counter);
            continue;
        }
        if(flag == 3){                          //history command "!num"
            int command_index = extract_positive_int_from_str(input);
            if(command_index<1){
                printf("invalid history argumant\n");
                continue;
            }
            history_successes = get_nd_line_from_history(command_index, input);
            count_arguments(input, &words, &flag);              //recalc for the new string
            if(history_successes){
                printf("%s\n", input);
                if(flag==4){
                    history_case(&words_counter);
                    continue;
                }
                words_counter += words;
            }
            flag = 0;                                            //back to regular case after get the desired caommand
            if(get_num_of_pipes(input)>0)                        //check if the command is pipe command
                flag=5;
        }        
        if(!flag){                                              //regular case flag=0
            command = str_to_array(input, words);                   //calc the command array
            pid_t parent_process = fork();                      // create new process to run the command
            if(parent_process<0){
                perror("fork\n");
                exit(1);
            }
            if(parent_process == 0){
                int fd = open("tmp", O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);    //creat buffer file for output
                dup2(fd, 1);                                                //pipe the output to the file
                close(fd);                                                  //pipe configured close fd
                execvp(command[0], command);                                //execute the command
                //in case of wrong input
                printf("invalid command!\n");
                free_str_array(command, words+1);
                raise(SIGINT);
                return 1;
            }
            if(!bg_pro){
                wait(NULL);
                read_from_buffer();
                }
            else
                bg_pid = parent_process;

            free_str_array(command, words+1);                   //free the last command array
                                             // print the output from the buffer file "tmp"
        }
        if(flag == 1){                                      //in case special input "cd"
            printf("command not supported (Yet)\n");
            continue;
        }
        if(flag ==2 ){                                       //done case
            print_done(commands_counter, pipes_counter);
            break;
        }
        if(flag==5){                                            //pipe case
            num_of_pipes = get_num_of_pipes(input);                     
            int args[3], flag[3], to_arr_ind=0, pipe_ind=0;   //init variables for each commend in the input
            char to_arr[3][N];
            
            //extarct the commands and store them in array for execution
            int i=0;
            while(i<strlen(input)){

                if(input[i]=='|'){
                    i++;
                    if(input[i]=='|')                   //check if pipe inputed in '||' format
                        i++;
                    to_arr[pipe_ind][to_arr_ind]='\0';
                    pipe_ind++;
                    to_arr_ind=0;
                }
                else
                    to_arr[pipe_ind][to_arr_ind++]=input[i++];
            }
            to_arr[pipe_ind][to_arr_ind]='\0';
            //convert string to cmd array include !<num> case
            for(i=0;i<num_of_pipes+1;i++)
                if(to_arr[i][0]=='!' || to_arr[i][1]=='!' ){
                    num_of_pipes++;
                    int command_index = extract_positive_int_from_str(to_arr[i]);
                    history_successes = get_nd_line_from_history(command_index, to_arr[i]);
                    if(get_num_of_pipes(to_arr[i])){
                        int j=0;
                        char tmp_cmd[N];
                        if(i==0)
                            strcpy(tmp_cmd, to_arr[i+1]);
                        while(to_arr[i][j]!='|')
                            j++;
                        int last_of_first = j;
                        j++;
                        int k=0;
                        while(j< strlen(to_arr[i]))
                            to_arr[i+1][k++] = to_arr[i][j++];
                        to_arr[i+1][k]='\0', to_arr[i][last_of_first]='\0';
                        if(i==0)
                            strcpy(to_arr[i+2], tmp_cmd);
                        
                    }
                }
            for(i=0;i<num_of_pipes+1;i++)
                count_arguments(to_arr[i], &args[i], &flag[i]);

            char*** cmd = (char***)malloc(sizeof(char**)*num_of_pipes);             //init array of the commands
            if(cmd==NULL){
                perror("malloc");
                exit(1);
            }

            for(int i=0;i<num_of_pipes+1;i++){
                count_arguments(to_arr[i], &args[i], &flag[i]);
                cmd[i] = str_to_array(to_arr[i], args[i]);
            }
            //store original STDIN STDOUT for the restoring after command executing
            int tempin = dup(0);
            int tempout = dup(1);
            
            int pid;
            if(num_of_pipes==2){            //in case of 2 pipes
                int p1[2];
                pipe(p1);
                dup2(p1[1],1);              //STDOUT -> write pipe
                close(p1[1]);               //copied, the original can be closed
                pid = fork();
                if(pid<0){
                    perror("fork");
                    exit(1);
                }
                if(pid==0){
                    execvp(cmd[0][0],cmd[0]);
                    perror("execvp");
                    //free the command array if execvp filed
                    for(int i=0;i<num_of_pipes+1;i++)
                        free_str_array(cmd[i], args[i]);
                    free(cmd);
                    raise(SIGINT);
                return 1;
                }
                dup2(p1[0],0);          //STDIN -> read pipe
                close(p1[0]);           //copied, the original can be closed
                waitpid(-1, &status, 0);
                if(WIFSIGNALED(status))
                    continue;
            }
            int p2[2];
            pipe(p2);
            dup2(p2[1],1);              //STDOUT -> write pipe
            close(p2[1]);               //copied, the original can be closed
            pid = fork();
            if(pid<0){
                perror("fork");
                exit(1);
            }
            if(pid==0){
                execvp(cmd[num_of_pipes-1][0],cmd[num_of_pipes-1]);
                perror("execvp");
                //free the command array if execvp filed
                for(int i=0;i<num_of_pipes+1;i++)
                    free_str_array(cmd[i], args[i]);
                free(cmd);
                raise(SIGINT);
                return 1;
            }
            dup2(p2[0],0);      //STDIN -> read pipe
            close(p2[0]);       //copied, the original can be closed
            waitpid(-1, &status, 0);
            if(WIFSIGNALED(status))
                continue;
            
            //prepare temporary file for the final output
            int tmp = open("tmp", O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
            dup2(tmp,1);        //STDOUT -> file
            close(tmp);         //copied, the original can be closed

            pid = fork();
            if(pid<0){
                perror("fork");
                exit(1);
            }
            if(pid==0){
                execvp(cmd[num_of_pipes][0],cmd[num_of_pipes]);
                perror("execvp");
                //free the command array if execvp filed
                for(int i=0;i<num_of_pipes+1;i++)
                    free_str_array(cmd[i], args[i]);
                free(cmd);
                raise(SIGINT);
                return 1;
            }
            //restore original STDIN STDOUT
            dup2(tempin,0);
            dup2(tempout,1);
            if(!bg_pro)
                waitpid(-1, &status, 0);
                if(WIFSIGNALED(status))
                    continue;
            

            for(int i=0; i<num_of_pipes+1;i++)
                free_str_array(cmd[i], args[i]);
            
            read_from_buffer();
            strcpy(input, to_arr[0]);
            for(int i=1;i<num_of_pipes+1;i++){
                strcat(input, "|");
                strcat(input, to_arr[i]);
            }
            pipes_counter += num_of_pipes;
        }
        if(history_successes && !WIFSIGNALED(status) )
            write_to_history_file(input);                       //add typed command to the history file
    }
    return 0;
}
void free_bg_data(int num){                                     // free resources of background prossess
    if(bg_pid>0){
        waitpid(bg_pid,NULL,0);
        bg_pid=0;
        bg_pro=0;
        signal(SIGCHLD, SIG_DFL);
        read_from_buffer();
    }
}
char** str_to_array(const char* str, int words){
    char** res;
    res = (char**)malloc(sizeof(char*)*(words+1));
    if(res == NULL){
        perror("malloc\n");
        exit(1);
    }    
    char a[strlen(str)+1];              //copy the original string for strtok() func
    strcpy(a, str);
    int i=0;                            //init index for the array
    //iterate over the string allocate dynamicly string for each word and strore the word
    char* token = strtok(a, " ");
    while( token != NULL ) {
        res[i] = (char*)malloc(sizeof(char)*(strlen(token)+1));
        if(res[i]==NULL){
            free_str_array(res, i+1);       //free all the previous allocations 
            perror("malloc\n");
            exit(1);
        }
        strcpy(res[i],token);
        i++;
        token = strtok(NULL, " ");
    }
    res[i] = NULL;              //put NULL at the last index to respect the execvp command format 
    return res;
}
void input_str(char* a){

    fgets (a, N+1, stdin);

    a[strlen(a)-1]='\0'; //remove the '\n' char from the string

}
int nohup(const char* input){
    char copy[N+1];                     //init string to for strtok to avoid aliasing on original input
    strcpy(copy, input);
    //using strtok() func to split the input with space delimiter
    char* token = strtok(copy, " ");
    if(!strcmp(token,"nohup"))
        return 1;
    return 0;
}
void count_arguments(const char* a, int* arguments, int* flag){
    char copy[N+1];                     //init string to for strtok to avoid aliasing on original input
    strcpy(copy, a);
    //using strtok() func to split the input with space delimiter
    char* token = strtok(copy, " ");
    char* last_word;
    int i=0;
    while( token != NULL ) {            //iterate over all the arguments
        i++;                            //increment arguments counter
        last_word = token;              //save the last word for chack history/exit case
        if(i==1)
            if(!strcmp(last_word,"cd")) //check "cd" command
                *flag = 1;
            if(token[0] == '!')         //check history command
                *flag =3;
        token = strtok(NULL, " ");
    }
    *(arguments) = i;                   
    if(i == 1){
        if(!strcmp(last_word,"done"))       //done case
            *flag = 2;
        if(!strcmp(last_word,"history"))
            *flag = 4;                      //history case
    }
}

void free_str_array(char** to_free, int size){
    for(int i=0;i<size;i++)
        free(to_free[i]);
    free(to_free);
}
void read_from_buffer(){
    FILE* tmp = fopen("tmp", "r");          //open buffer file
    if(tmp == NULL){
        perror("open file tmp\n");
    }
    char c;
    int saved_stdout;
    if(nohup_state){                                                       //in nohup case we redirect the output to nohup.txt
        int fd = open("nohup.txt", O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);     //creat nohup.txt file for output
        saved_stdout = dup(1);                                             // store original STDOUT fd
        dup2(fd, 1);                                                       //pipe the output to the file
        close(fd);                                                         //pipe configured close fd
    }
        //read buffer file
        while((c=fgetc(tmp))!=EOF)
            printf("%c",c);
        fclose(tmp);
        remove("tmp");                      //delete buffer file
    if(nohup_state){    
        //restore original STDOUT fd            
        dup2(saved_stdout,1);              
        close(saved_stdout);
        //restore signals
        sigset_t mask;  
        sigemptyset(&mask);
        sigprocmask(SIG_SETMASK, &mask, NULL);
        nohup_state=0;
    }
}
void write_to_history_file(const char* to_write){
    FILE* history = fopen("file.txt","a");
    if(history==NULL){
        perror("open file file.txt\n");
        exit(1);
    }
    if(history == NULL) printf("update history file failed\n");
    fprintf(history, "%s\n",to_write);
    fclose(history);
}
void print_current_dir(){
    char current_dir[N];
    getcwd(current_dir, N);
    printf("%s>",current_dir);
}
void print_done(int commands_counter, int pipes_counter){
    printf("Number of commands: %d\n",commands_counter-1);
    printf("Number of pipes: %d\n",pipes_counter);
    printf("See you Next time !\n");
}
int get_nd_line_from_history(int nd, char* dest){
    nd--;                       //normalize index
    char *line_buf = NULL;
    size_t line_buf_size = 0;
    int line_count = 0;
    ssize_t line_size;
    FILE* history = fopen("file.txt", "r");
    if(history==NULL){
        perror("open file file.txt\n");
        exit(1);
    }
    line_size = getline(&line_buf, &line_buf_size, history);
    //iterate over the file lines , stop and return when index==nd
    while (line_size >= 0){
        if(line_count == nd){
            fclose(history);
            strcpy(dest, line_buf);
            if(dest[strlen(dest)-1] = '\n')
                dest[strlen(dest)-1] = '\0';     //remove '\n' char
            raise(SIGINT);
                return 1;
        }
        if (line_count > nd)
            break;
        line_size = getline(&line_buf, &line_buf_size, history);
        line_count++;
    }
    strcpy(dest, "echo NOT IN. HISTORY");
    fclose(history);
    return 0;
}

int extract_positive_int_from_str(char* input){
    int n= strlen(input), res =0, exponent = 0;
    if(input[n-1]==' ')
        n--;
    for(int i=n-1;i>0 && input[i] != '!';i--){
        if(!isdigit(input[i]))
            return -1;
        res += (input[i]-'0')*power(10, exponent++);
    }
    return res;
}
int power(int base, int exp){
    if(base==0)
        return 0;
    int res=1;
    for(int i =0 ; i<exp;i++)
        res*=base;
    return res;
}
void print_history(){
    char to_print[N+1];
    FILE* history = fopen("file.txt", "r");
    if(history == NULL)
        return;
    while(fgets(to_print, N+1, history) != NULL)
        printf("%s", to_print);
    fclose(history);
}
void history_case(int* words_counter){
    *words_counter++;
    print_history();
    write_to_history_file("history");
}

int get_num_of_pipes(char* input){
    int count=0;
    for(int i=0; i<strlen(input); i++)
        if(input[i]=='|'){
            count++;
            if(i+1< strlen(input) && input[i+1]=='|')
                i++;
        }
    return count;
}
