/* ======= ENGG 126 Project 1 - Veneracion, Yap ======= */

#include <unistd.h>     // fork(), dup2()
#include <sys/wait.h>   // waitpid()
#include <stdio.h>      // printf(), fgets()
#include <string.h>     // strtok(), strcmp(), strdup()
#include <stdlib.h>     // free()
#include <fcntl.h>      // open(), creat(), close()

using namespace std;

const unsigned maxLineLength = 80;
const unsigned bufSize = 50;
const unsigned redSize = 2;
const unsigned pipeSize = 2;
const unsigned maxCommandName = 30;

void parseCommand(char input[], char* argv[], int* wait) {
   for (unsigned idx = 0; idx < bufSize; idx++) {
      argv[idx] = NULL;
   }

   // Check for trailing & and remove if exists
   if (input[strlen(input) - 1] == '&') {
      *wait = 1;
      input[strlen(input) - 1] = '\0';
   }
   else {
      *wait = 0;
   }

   // Perform tokenization on input string
   const char *delim = " ";
   unsigned idx = 0;

   char *token = strtok(input, delim);
   while (token != NULL) {
      argv[idx++] = token;
      token = strtok(NULL, delim);
   }

   argv[idx] = NULL;
}

void parseRedir(char* argv[], char* redir_argv[]) {
   unsigned idx = 0;
   redir_argv[0] = NULL;
   redir_argv[1] = NULL;

   while (argv[idx] != NULL) {

      // Check if command contains character <, >
      if (strcmp(argv[idx], "<") == 0 || strcmp(argv[idx], ">") == 0) {

         // Check for succeeded file name
         if (argv[idx + 1] != NULL) {

            // Move redirect type and file name to redirect arguments vector
            redir_argv[0] = strdup(argv[idx]); // duplicate string
            redir_argv[1] = strdup(argv[idx + 1]);
            argv[idx] = NULL;
            argv[idx + 1] = NULL;
         }
      }
      idx++;
   }
}

bool parsePipe(char* argv[], char *child01_argv[], char *child02_argv[]) {
   unsigned idx = 0, split_idx = 0;
   bool containsPipe = false;
   int cnt = 0;

   while (argv[idx] != NULL) {

      // Check if user_command contains pipe character |
      if (strcmp(argv[idx], "|") == 0) {
         split_idx = idx;
         containsPipe = true;
      }
      idx++;
   }
   
   if (!containsPipe) {
      return false;
   }
   
   // Copy arguments before split pipe position to child01_argv[]
   for (idx = 0; idx < split_idx; idx++) {
      child01_argv[idx] = strdup(argv[idx]); // duplicate string 
   }
   child01_argv[idx++] = NULL;
      
   // Copy arguments after split pipe position to child02_argv[]
   while (argv[idx] != NULL) {
      child02_argv[idx - split_idx - 1] = strdup(argv[idx]);
      idx++;
   }
   child02_argv[idx - split_idx - 1] = NULL;
   
   return true;
}

void childProcess(char* argv[], char* redir_argv[]) {
   int fd_out, fd_in;
   if (redir_argv[0] != NULL) {
         
      // Redirect output
      if (strcmp(redir_argv[0], ">") == 0) {

         // Get file description
         fd_out = creat(redir_argv[1], S_IRWXU);
         if (fd_out == -1) {
            perror("Failed to redirect output.");
            exit(EXIT_FAILURE);
         }

         // Replace stdout with output file
         dup2(fd_out, STDOUT_FILENO);

         // Check for error on close
         if (close(fd_out) == -1) {
            perror("Failed to close output.");
            exit(EXIT_FAILURE);
         }
      }

      // Redirect input
      else if (strcmp(redir_argv[0], "<") == 0) {
         fd_in = open(redir_argv[1], O_RDONLY);
         if (fd_in == -1) {
            perror("Failed to redirect input.");
            exit(EXIT_FAILURE);
         }

         dup2(fd_in, STDIN_FILENO);

         if (close(fd_in) == -1) {
            perror("Failed to close input.");
            exit(EXIT_FAILURE);
         }
      }
   }

   // Execute user command in child process
   if (execvp(argv[0], argv) == -1) {
      perror("Failed to execute command");
      exit(EXIT_FAILURE);
   }
}

void parentProcess(pid_t child_pid, int wait) {
   int status;
   printf("Parent <%d> spawned child <%d>.\n", getpid(), child_pid);
   switch (wait) {

      // Parent and child are running concurrently (&)
      case 0: {
         waitpid(child_pid, &status, 0);
         break;
      }

      // Parent waits for child process with PID to be terminated (no &)
      default: {
         waitpid(child_pid, &status, WUNTRACED);

         if (WIFEXITED(status)) {   
            printf("Child <%d> exited with status = %d.\n", child_pid, status);
         }
         break;
      }
   }
}

void execwithPipe(char* child01_argv[], char* child02_argv[]) {
   int pipefd[2];

   if (pipe(pipefd) == -1) {  
      /* Create a pipe with 1 input and 1 output file descriptor
      Index = 0 : read pipe; Index = 1 : write pipe
      */
      perror("Failed pipe()");
      exit(EXIT_FAILURE);
   }

   // Create 1st child   
   if (fork() == 0) {

      // Redirect STDOUT to output part of pipe 
      dup2(pipefd[1], STDOUT_FILENO);  // duplicate pip to STDOUT 
                                       // any writes to standard output will  
                                       // be sent to the pipe  
      close(pipefd[0]);     
      close(pipefd[1]);      

      execvp(child01_argv[0], child01_argv);   
      perror("Failed to execute first command.");
      exit(EXIT_FAILURE);
   }

   // Create 2nd child
   if (fork() == 0) {

      // Redirect STDIN to input part of pipe
      dup2(pipefd[0], STDIN_FILENO);            
      close(pipefd[1]);      
      close(pipefd[0]);       

      execvp(child02_argv[0], child02_argv);   
      perror("Failed to execute second command.");
      exit(EXIT_FAILURE);
   }

   close(pipefd[0]);
   close(pipefd[1]);
   // Wait for child 1
   wait(0);   
   // Wait for child 2
   wait(0);   
}

int main() {
   bool running = true;
   pid_t pid;
   int status = 0, history_count = 0, wait;
   char user_input[maxLineLength];
   char *argv[bufSize], *redir_argv[redSize];
   char *child01_argv[pipeSize], *child02_argv[pipeSize];

   while (running) {
      printf("osh>");
      fflush(stdout);
  
      // Read and parse user input
      while (fgets(user_input, maxLineLength, stdin) == NULL) {
         perror("Cannot read user input!");
         fflush(stdin);
      }

    	// Remove trailing endline character
      user_input[strcspn(user_input, "\n")] = '\0';

      // Check if user entered "exit"
      if (strcmp(user_input, "exit") == 0) {
         running = false;
         continue;
      }

      parseCommand(user_input, argv, &wait);
      parseRedir(argv, redir_argv);
      
      if (parsePipe(argv, child01_argv, child02_argv)) {
         execwithPipe(child01_argv, child02_argv);
         continue;
      }
      
      // Fork child process
      pid_t pid = fork();

      // Fork return twice on success: 0 - child process, > 0 - parent process
      switch (pid) {
         case -1:
            perror("fork() failed!");
            exit(EXIT_FAILURE);
      
         case 0:     // In child process
            childProcess(argv, redir_argv);
            exit(EXIT_SUCCESS);
      
         default:    // In parent process
            parentProcess(pid, wait);
      }
      
   }
   return 0;
}
