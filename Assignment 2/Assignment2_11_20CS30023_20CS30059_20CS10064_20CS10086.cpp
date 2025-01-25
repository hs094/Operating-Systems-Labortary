#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <vector>
#include <fnmatch.h>
#include <algorithm>
#include <glob.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <string>
#include <signal.h>

using namespace std;

#define CMD_LEN 1024
#define IND_CMD_LEN 128
#define BUF_LEN 128
#define EVENT_BUF_LEN 1024
#define MAX_FILENAME_LENGTH 128
#define STD_INPUT 0
#define STD_OUTPUT 1
#define HISTFILESIZE 10000
#define HISTSIZE 1000

char *prompt = (char *)"\n$: ";
char *prompt1 = (char *)"$: ";
/**
 * Terminos has been used to take input character
 * by character with getchar in such a way that if tab
 * or Ctrl+R is pressed then it can be detected directly
 * without the need of pressing enter and processing the input
 */
struct termios saved_attr;
// function used to reset input to normal mode
void reset_input_mode(void)
{
  saved_attr.c_lflag |= (ICANON | ECHO);
  saved_attr.c_cc[VMIN] = 0;
  saved_attr.c_cc[VTIME] = 1;
  tcsetattr(STDIN_FILENO, TCSANOW, &saved_attr);
}
// command to set input to character by character input mode
void set_input_mode(void)
{
  struct termios tattr;
  tcgetattr(STDIN_FILENO, &saved_attr);
  atexit(reset_input_mode);
  tcgetattr(STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON | ECHO);
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

// global pids where the commands run,
// Kept here so that they can be signalled
// to continue when Ctrl Z is pressed
pid_t *pids;
int pipe_cnt;

// some flags to switch code to background
// or stop the multiWatch code
static volatile int runInBackgrnd = 0;

// Inotify File Descriptor that watches over temporary files
// during multiwatch. Kept Global so that it can be closed
// when Ctrl+C is pressed.

std::vector<std::string> list_files(const std::string &dir, const std::string &pattern)
{
  std::vector<std::string> files;
  DIR *dp;
  struct dirent *entry;
  if ((dp = opendir(dir.c_str())) == NULL)
  {
    std::cerr << "Cannot open directory: " << dir << std::endl;
    return files;
  }
  while ((entry = readdir(dp)) != NULL)
  {
    std::string file_name = entry->d_name;
    if (fnmatch(pattern.c_str(), file_name.c_str(), FNM_PATHNAME | FNM_PERIOD) == 0)
      files.push_back(file_name);
  }
  closedir(dp);
  return files;
}

map<string, string> getProcessDetails(int pid)
{
  map<string, string> details;

  // Open the status file for the process
  string filename = "/proc/" + to_string(pid) + "/status";
  ifstream statusFile(filename.c_str());
  if (!statusFile.is_open())
  {
    cout << "Error opening file: " << filename << endl;
    exit(1);
  }

  // Read the details from the status file
  string line;
  while (getline(statusFile, line))
  {
    int pos = line.find(':');
    if (pos != string::npos)
    {
      string key = line.substr(0, pos);
      string value = line.substr(pos + 1);
      details[key] = value;
    }
  }
  return details;
}

// Function to traverse the process tree
// string
// This code defines a recursive function named traverseProcessTree which recursively traverses the process tree and prints details about each process.

// The function takes four arguments:

// pid: the process ID of the current process being analyzed
// indent: an integer representing the number of spaces to use for indentation when printing details
// suggest: a boolean indicating whether the function should check for suspicious processes
// depth: an integer indicating the depth of the current process in the process tree
// Inside the function, the getProcessDetails function is called to retrieve details about the current process. The function then prints the process ID and name using the cout function.

// If the suggest flag is set to true, the function checks whether the current process is suspicious by checking the time spent by the process and the number of child processes it has. If the process has been running for more than 10 seconds, or if it has between 5 and 10 child processes, it is considered suspicious and a message is printed indicating this.

// Finally, the function recursively calls itself with the parent process ID to traverse the entire process tree. The recursion continues until the parent process ID is less than or equal to 0, indicating that the root process has been reached.
void traverseProcessTree(int pid, int indent, bool suggest, int depth)
{
  // Read the details of the process
  map<string, string> details = getProcessDetails(pid);

  // Print the process ID and name
  cout << string(indent, ' ') << "Process ID: " << pid << endl;
  cout << string(indent, ' ') << "Process Name: " << details["Name"] << endl;
  // printf("")
  // Check if the suggest flag is set
  if (suggest)
  {
    // Get the time spent by the process
    long time = atoi(details["Uptime"].c_str());

    // Get the number of child processes
    // DIR *dir = opendir(("/proc/" + to_string(pid) + "/task/" +  to_string(pid) + "/children").c_str());
    string s = "/proc/" + to_string(pid) + "/task/" + to_string(pid) + "/children";
    ifstream file(s);
    if (!file)
    {
      cout << "File not found!" << endl;
      return;
    }
    string word;
    int children = 0;
    while (file >> word)
    {
      children++;
    }
    file.close();
    cout << time << " " << children << endl;
    if (time > 10 || (children >= 5 && children < 10))
    {
      cout << string(indent, ' ') << "SUSPICIOUS PROCESS!" << endl;
    }
  }

  // Recursively traverse the process tree
  int parentPID = atoi(details["PPid"].c_str());
  if (parentPID > 0)
  {
    traverseProcessTree(parentPID, indent, suggest, depth++);
  }
}

// This code defines a function named expandWildcards that takes two parameters: a constant reference to a std::string object named arg and a reference to a std::vector object that holds strings named args.

// The function uses the glob function to expand any wildcard characters in the arg string, such as * or ?, and returns a list of matching file paths. The glob_t data structure is used to hold the matching file paths.

// The memset function is used to initialize the globBuffer data structure to 0. The glob function is then called with three arguments: the arg string converted to a C-style string using the c_str() member function, the GLOB_TILDE flag to expand tilde characters, and NULL to indicate that no callback function is used.

// If the glob function returns 0, indicating success, the function iterates through the resulting gl_pathv array and adds each path to the args vector using the push_back function.

// Finally, the globfree function is called to free the memory used by the globBuffer data structure.
void expandWildcards(const std::string &arg, std::vector<std::string> &args)
{
  glob_t globBuffer;
  memset(&globBuffer, 0, sizeof(globBuffer));
  int globResult = glob(arg.c_str(), GLOB_TILDE, NULL, &globBuffer);
  if (globResult == 0)
  {
    for (size_t i = 0; i < globBuffer.gl_pathc; ++i)
      args.push_back(globBuffer.gl_pathv[i]);
  }
  globfree(&globBuffer);
}

// This function search file in the directory .
// The function uses trie data structure to
// store the file names in the folder

/**
 * @brief
 * The function gets executed when tab is pressed
 * @param cmd: char pointer: command when the tab was pressed
 * @param pos : integer : index where new character should be placed in the array
 * @param isFound : variable to check if any file exists or not
 * @return char*: updated command:
 *               = NULL if no file is found
 *               = command after completion of the file name
 */
class shell_history
{
public:
  int index;
  char **commands;
  int maxSize;
  /**
   * @brief Construct a new shell history object
   *
   */
  shell_history()
  {
    this->index = 0;
    this->maxSize = HISTFILESIZE;
    this->commands = (char **)malloc(sizeof(char *) * HISTFILESIZE);
    int fdRead = open(".history.txt", O_CREAT | O_RDONLY, 0666);
    FILE *file = fdopen(fdRead, "r");
    char *command = NULL;
    size_t len = 0;
    while (getline(&command, &len, file) != -1)
    {
      commands[index] = (char *)malloc(sizeof(char) * strlen(command));
      strcpy(commands[index], command);
      commands[index][(int)strlen(commands[index]) - 1] = '\0';
      index++;
    }
    fclose(file);
  }

  /**
   * @brief
   * The function prints the top 1000 commands in the history
   *
   */
  void print()
  {
    int ind = max(0, index - HISTSIZE);
    for (int i = index - ind - 1, j = 1; i >= 0; i--, j++)
      fprintf(stdout, "%d) %s\n", j, commands[i + ind]);
  }

  /**
   * @brief
   * The command is added in the shell_history
   * @param command
   */
  void push(char *command)
  {
    if (this->index == this->maxSize)
    {
      this->maxSize = this->maxSize + HISTFILESIZE;
      this->commands = (char **)realloc(this->commands, sizeof(char *) * (this->maxSize));
    }
    this->commands[this->index] = (char *)malloc(sizeof(char) * CMD_LEN);
    strcpy(this->commands[this->index], command);
    this->commands[this->index][(int)strlen(command)] = '\0';
    this->index++;
  }
  void pop()
  {
    if (index == 0)
      return;
    // decrement the size of the array
    this->index--;
    // allocate a new array with one less element
    this->commands = (char **)realloc(this->commands, this->index * sizeof(char *));
    // return the new array
  }
  /**
   * @brief
   * THis function write the recent 10000 commands
   * in the file.
   */
  void updateFile()
  {
    FILE *file = fopen(".history.txt", "w");
    int ind = max(0, index - HISTFILESIZE);
    for (int i = ind; i < index; i++)
    {
      fprintf(file, "%s", this->commands[i]);
      fprintf(file, "\n");
    }
    fclose(file);
  }
};

/**
 * @brief Handler to perform a specific function when CTRL+C is
 * pressed.
 *
 * @param dum integer variable that carries the integer value of
 * the signal received. Here it is SIGINT as this is with for a SIGINT Handler
 */
void ctrlCHandler(int dum)
{
  fprintf(stdout, " Ctrl C Detected.\n");
  return;
}

/**
 * @brief Handler to perform a specific function when CTRL+C is
 * pressed.
 *
 * @param dum integer variable that carries the integer value of
 * the signal received. Here it is SIGTSTP as this is with for a SIGTSTP Handler
 */
void ctrlZHandler(int dum)
{
  // try and catch
  fprintf(stdout, " Ctrl Z Detected.\n");
  int x = 0;
  for (int i = 0; i < pipe_cnt + 1 && i < sizeof(pids) / sizeof(pid_t); i++)
  {
    try
    {
      if (kill(pids[i], SIGCONT))
        ;
      else
        throw x;
    }
    catch (int x)
    {
      fprintf(stdout, "No More Processes to Kill\n");
    }
  }
  runInBackgrnd = 1;
}

/**
 * @brief Split the command by spaces into a number of tokens to return
 * a NULL terminated array with these tokens. Also redirects the input and output
 * to specific file descriptors.
 *
 * @param cmd The complete command with various terms with spaces
 * @param ipFile File Descriptor where input should be redirected. Modified if there
 *               is < token in the command
 * @param opFile File Descriptor where output should be redirected. Modified if there
 *               is > token in the command
 * @return char** NULL terminated character array with command terms
 */
char **splitCommand(char *cmd, int &ipFile, int &opFile)
{
  char cmd_copy[CMD_LEN] = {0};
  strcpy(cmd_copy, cmd);
  char **cmds;

  char *p;
  p = strtok(cmd, " ");
  int cnt = 0;
  while (p)
  {
    p = strtok(NULL, " ");
    cnt++;
  }
  cmds = (char **)malloc((cnt + 1) * sizeof(char *));

  p = strtok(cmd_copy, " ");
  int id = 0;

  int isRedirected = 0;

  while (p)
  {
    if (strcmp(p, "<") == 0)
    {
      isRedirected = 1;
      p = strtok(NULL, " ");
      if (p == NULL)
      {
        fprintf(stderr, "ERROR. No file given .\n");
        exit(0);
      }
      int fd = open(p, O_RDONLY);
      if (fd < 0)
      {
        fprintf(stderr, "Error in opening file.\n");
        exit(0);
      }
      ipFile = fd;
      p = strtok(NULL, " ");
    }
    else if (strcmp(p, ">") == 0)
    {
      isRedirected = 1;
      p = strtok(NULL, " ");
      if (p == NULL)
      {
        fprintf(stderr, "ERROR. No file given.\n");
        exit(0);
      }
      int fd = open(p, O_CREAT | O_WRONLY, 0666);
      if (fd < 0)
      {
        fprintf(stderr, "Error in openingfile.\n");
        exit(0);
      }
      opFile = fd;
      p = strtok(NULL, " ");
    }
    else
    {
      if (isRedirected)
      {
        fprintf(stderr,
                "Syntax Error. Cannot have command after input or output "
                "redirection.\n");
        exit(0);
      }
      cmds[id] = (char *)malloc((strlen(p) + 1) * sizeof(char));
      strcpy(cmds[id], p);
      cmds[id][strlen(p)] = '\0';
      id++;
      p = strtok(NULL, " ");
    }
  }

/*
dup2() is a system call in Unix-like operating systems that allows you to duplicate a file descriptor. It is short for "duplicate to", and the function takes two file descriptor arguments: oldfd and newfd.

The dup2() function duplicates the file descriptor oldfd to the file descriptor newfd, closing the file descriptor newfd first if necessary. This allows you to redirect input and output streams to different files or devices, and to control which file descriptors are used for input and output.

dup2() is commonly used in Unix shell scripts to redirect input and output streams, as well as in C and C++ programming for low-level file handling.*/
  if (ipFile != STD_INPUT)
    dup2(ipFile, STD_INPUT);
  if (opFile != STD_OUTPUT)
  {
    dup2(opFile, STD_OUTPUT);
  }
  cmds[id] = NULL;

  return cmds;
}

// Remove the back and last white spaces 
void trim(string &s)
{
  while (s.length() && s.back() == ' ')
    s.pop_back();
  int i = 0;
  while (i < (int)s.length() && s[i] == ' ')
    i++;
  s = s.substr(i);
}

/**
 * @brief Function to execute piped or unpiped commands by tokenising the command
 * and then running it using execvp in a child process. Handles piped commands too
 * by creating different child processes for each command and pipes for inter-command
 * data transfer.
 *
 * @param cmd The entire line of input
 * @param isBackGrnd Flag to signify whether the processes are to be run in the background or not.
 *                  If yes then parent process does not wait for child to complete.
 */

// This is a C function called executeCommand that is designed to execute a shell command specified as a string. The function takes two arguments: cmd (a pointer to the command string) and isBackGrnd (an integer flag that indicates whether the command should be executed in the background).
// The function first initializes some variables, including ipFile (input file descriptor), opFile (output file descriptor), pipe_cnt (a count of the number of pipes in the command), and pipes (an array of pipe file descriptors). It then uses a loop to count the number of pipes in the command and create the necessary number of pipe file descriptors.
// Next, the function uses strtok to split the command string into individual commands (if the command contains pipes). It then creates a child process for each command using fork(), and sets up the necessary file descriptors for input and output redirection. Each child process then executes the corresponding command using execvp(), passing in the command and any necessary arguments as an array of strings. If execvp() fails, an error message is printed and the child process exits.
// Finally, the function waits for each child process to complete (if isBackGrnd is false) using waitpid(). If any child process exits or is interrupted by a signal, the function exits. If isBackGrnd is true, the function returns immediately without waiting for the child processes to complete.
void executeCommand(char *cmd, int isBackGrnd)
{
  int ipFile = STD_INPUT, opFile = STD_OUTPUT;
  fprintf(stdout, "\n");

  pipe_cnt = 0;
  for (int i = 0; cmd[i] != '\0'; i++)
  {
    if (cmd[i] == '|')
      pipe_cnt++;
  }
  int pipes[pipe_cnt + 1][2];
  pid_t wpid[pipe_cnt + 1];
  int status[pipe_cnt + 1];
  for (int i = 0; i < pipe_cnt; i++)
  {
    if (pipe(pipes[i]) < 0)
    {
      fprintf(stderr, "ERROR in creating pipes >. Exitting.\n");
      exit(0);
    }
  }

  pids = (pid_t *)malloc((pipe_cnt + 1) * sizeof(pid_t));
  char *command;
  char **cmds;
  char **commandsPipeSep = (char **)(malloc((pipe_cnt + 2) * sizeof(char *)));
  for (int i = 0; i < pipe_cnt + 1; i++)
  {
    if (i == 0)
    {
      command = strtok(cmd, "|");
    }
    else
      command = strtok(NULL, "|");
    commandsPipeSep[i] = (char *)(malloc((strlen(command) + 1) * sizeof(char)));
    strcpy(commandsPipeSep[i], command);
    commandsPipeSep[i][strlen(command)] = '\0';
  }
  commandsPipeSep[pipe_cnt + 1] = NULL;
  for (int i = 0; i < pipe_cnt + 1; i++)
  {
    pids[i] = fork();
    if (pids[i] == -1)
    {
      fprintf(stderr, "ERROR in forking child >. Exitting.\n");
      exit(0);
    }
    if (pids[i] == 0)
    {
      for (int j = 0; j < pipe_cnt; j++)
      {
        if (i != j)
          close(pipes[j][1]);
        if (i - 1 != j)
          close(pipes[j][0]);
      }
      if (i == 0)
      {
        ipFile = STD_INPUT;
      }
      else
        ipFile = pipes[i - 1][0];
      if (i == pipe_cnt)
      {
        opFile = STD_OUTPUT;
      }
      else
      {
        opFile = pipes[i][1];
      }
      cmds = splitCommand(commandsPipeSep[i], ipFile, opFile);
      char **tem = cmds;

      if (i < pipe_cnt)
        close(pipes[i][1]);
      if (i > 0)
        close(pipes[i - 1][0]);
      if (execvp(cmds[0], cmds) < 0)
      {
        fprintf(stderr, "\nERROR. Could not execute program %s.\n", cmds[0]);
        exit(0);
      }
      return;
    }
  }
  for (int i = 0; i < pipe_cnt; i++)
  {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }
  if (!isBackGrnd)
  {
    do
    {
      int done = 0;
      for (int i = 0; i < pipe_cnt + 1; i++)
        wpid[i] = waitpid(pids[i], &status[i], WUNTRACED | WCONTINUED);
      for (int i = 0; i < pipe_cnt + 1; i++)
      {
        if (WIFEXITED(status[i]) || WIFSIGNALED(status[i]) ||
            WIFCONTINUED(status[i]))
        {
          done = 1;
          break;
        }
      }
      if (done || runInBackgrnd)
      {
        break;
      }
    } while (true);
  }
}

// This is a C++ function named parseFile that takes a file name and two vectors (pids and pid_lock) as input parameters.
// The purpose of the function is to read the contents of a file and extract some specific information from each line of the file. The file is assumed to have a header line, which is skipped by the function.
// The function starts by opening the file using an input file stream (ifstream) and initializing some variables (line, lineNum). Then it reads the file line by line using the getline function from the file stream.
// For each line, the function skips the first line (header) and then splits the line into tokens using a space (' ') as a delimiter. The tokens are stored in a vector named total.
// The function then iterates over each token in total and prints it to the console. If the third token contains the letter 'W', it means that the corresponding process is locked, and the function adds the second token (process ID) to the pid_lock vector.
// Finally, the function prints the size of the pid_lock vector to the console.
// Note that the function does not return anything, but it modifies the pids and pid_lock vectors passed as reference parameters.

// COMMAND     PID      USER   FD   TYPE DEVICE SIZE/OFF    NODE NAME
// bash      14735 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14754 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14756 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// chrome_cr 14773 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14798 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14815 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14826 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14851 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14869 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14880 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14886 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14913 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14936 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// code      14937 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// cpptools  14969 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// bash      15148 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// lsof      15676 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission
// lsof      15677 parallels  cwd    DIR    8,2     4096 3543603 /home/parallels/Downloads/Operating-Systems_Labortary-2023/Assignment 2/Final Submission


// delep function's parsefile function is actually used for the parsing the tmpfile.csv to idenfify the current processes with file open and file lock

void parseFile(const string &fileName, vector<string> &pids, vector<string> &pid_lock)
{
  ifstream file(fileName);
  // vector<string> pids;
  string line;
  int lineNum = 0;
  while (getline(file, line))
  {
    lineNum++;
    if (lineNum == 1)
      continue; // skip the first line (header)
    istringstream ss(line);
    string token;
    int tokenNum = 0;
    vector<string> total;
    while (getline(ss, token, ' '))
    {
      tokenNum++;
      if (token.size() != 0)
      {
        total.push_back(token);
        if (tokenNum == 2)
          pids.push_back(token);
      }
    }
    for (auto it : total)
    {
      cout << it << " ";
    }
    cout << endl;
    if (total[3].find("W") != string::npos)
    {
      pid_lock.push_back(total[1]);
    }
  }
  cout << pid_lock.size() << endl;
  // return pids;
}

// This code defines a function named get_arg0 that takes a single parameter: a pointer to a C-style string cmd. The function returns a pointer to a pointer to a C-style string.
// The function is used to parse the command line arguments passed to a program, and return the first argument as a separate string.
// The function first creates a copy of the cmd string in a local variable cmd_copy. Then, it uses the strtok function to tokenize the cmd string on whitespace characters (i.e., space, tab, and newline) to count the number of arguments.
// The function then allocates memory for an array of pointers to C-style strings, with a size of cnt + 1, where cnt is the number of arguments counted earlier. The +1 is for the null terminator, which is added to the end of the array to mark the end of the list of arguments.
// The function then tokenizes the cmd_copy string again to copy each argument into the array of C-style strings. It uses calloc to allocate memory for each C-style string, and then uses strcpy to copy the argument string into the newly allocated memory.
// Finally, the function sets the last element of the array to NULL to mark the end of the list of arguments, and returns the pointer to the array of C-style strings.
// Note that the function has a limitation in that it assumes that the maximum length of the cmd string is 1000 characters. It also does not handle quoted or escaped arguments.
char **get_arg0(char *cmd)
{
  char cmd_copy[1000] = {0};
  strcpy(cmd_copy, cmd);
  char **arg;

  char *p = strtok(cmd, " ");
  int cnt = 0;
  while (p)
  {
    p = strtok(NULL, " ");
    cnt++;
  }
  arg = (char **)calloc((cnt + 1), sizeof(char *));
  p = strtok(cmd_copy, " ");
  int i = 0;
  for (; p; i++)
  {
    arg[i] = (char *)calloc((strlen(p) + 1), sizeof(char));
    strcpy(arg[i], p);
    arg[i][strlen(p)] = '\0';
    p = strtok(NULL, " ");
  }
  arg[i] = NULL;
  return arg;
}
// This code defines a function delep that takes a char* parameter cmd.
// Inside the function, it declares two vector<string> variables data_open and data_lock, and two char* variables ch and ch1, and a pid_t variable pid.

// Then it forks the process and checks whether the process is a child or a parent using the pid variable. If pid is equal to 0, the code is executing in the child process.

// In the child process, the function first allocates memory for ch and ch1 variables using malloc. Then it copies the string "lsof " to ch1 variable and the cmd parameter to ch variable. After that, it concatenates ch1 and ch variables using strcat and stores the result in ch.

// The function then allocates memory for ch2 variable using malloc and initializes it to "tmpfile.csv". After that, it uses get_arg0 function to get the arguments of the ch command and opens a file descriptor for the tmpfile.csv file using open.

// Then it redirects the standard output to the file descriptor of the tmpfile.csv file using dup2. Finally, it executes the ch command using execvp.

// In the parent process, the function waits for the child process to finish using waitpid and then calls the parseFile function to parse the tmpfile.csv file and populate the data_open and data_lock vectors.

// Then it prints the contents of the data_open and data_lock vectors to the standard output using cout. After that, it prompts the user to enter "YES" or "NO" to kill the processes that have the file open and delete the file. If the user enters "YES", the function uses kill to kill each process in the data_open vector and remove to delete the file.

// Finally, the function frees the memory allocated for ch.
void delep(char *cmd)
{
  vector<string> data_open, data_lock;
  char *ch = (char *)malloc((CMD_LEN + 1000) * sizeof(char));
  char *ch1 = (char *)malloc(100 * sizeof(char));
  pid_t pid = fork();
  if (pid == 0)
  {
    memset(ch, '\0', (CMD_LEN + 1000));
    memset(ch1, '\0', (100));
    strcpy(ch1, "lsof ");

    char *ch2 = (char *)malloc(100 * sizeof(char));
    memset(ch2, '\0', (100));

    strcpy(ch2, "tmpfile.csv");
    strcat(ch, ch1);
    strcat(ch, cmd);
    // runExtCmd0(ch, ch2);
    cout << ch << " " << ch2 << endl;
    char **arg = get_arg0(ch);
    int fd = open(ch2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
      perror("open");
    if (dup2(fd, STDOUT_FILENO) < 0)
      perror("dup2");
    if (execvp(arg[0], arg) < 0)
      fprintf(stderr, "\nERROR. Could not execute program.\n");
    cout << 1 << endl;
    // executeCommand(ch, 0);
    // lsof cmd > tmpfile.csv
  }
  else
  {
    // wait(NULL);
    int status;
    waitpid(pid, &status, 0);
    parseFile("tmpfile.csv", data_open, data_lock);
    fprintf(stdout, "PID's that have the file open\n\n");
    for (auto pid_open : data_open)
      cout << pid_open << "   ";
    cout << endl;
    fprintf(stdout, "PID's that have the file lock\n\n");
    for (auto pid_lock : data_lock)
      cout << pid_lock << "   ";
    cout << endl;
    fprintf(stdout, "\n[?] Enter YES or NO to Kill the Processes:- ");
    string t;
    cin >> t;
    if (t.compare("YES") == 0)
    {
      for (auto pid : data_open)
      {
        if (kill(stoi(pid), SIGKILL) < 0)
          perror("Error with KILL.");
      }
      if (remove(cmd) > 0)
      {
        perror("Error: Removing File.");
      }
    }
    cout << endl;
    if (remove((char *)"tmpfile.csv"))
      ;
    free(ch);
  }
}
/**
 * @brief Function to read input command from command line
 *
 * @param isBackGrnd Flag passed by referrence to check if command has & present in
 *                  in the end and needs to be run in background
 * @param needExecution
 * @param shellHistory Object of the shellHistory class that maintains the history of commands run till now.
 * @return char* The entire command line input
 */
char *readLine(int &isBackGrnd, int &needExecution, shell_history &shellHistory)
{
  char *cmd = (char *)malloc(CMD_LEN * sizeof(char));
  char ch = 'a';
  int pos = 0;
  int inputModeSet = 0;
  set_input_mode();
  char **history = shellHistory.commands;
  int hist_cur = max(0, shellHistory.index);
  int cnt = 0;
  int flag_hist = 0;
  while (1)
  {
    ch = getchar();
    if ((int)ch == 1)
    {
      flag_hist = 0;
      for (int i = 0; i < pos; i++)
      {
        char *pr = (char *)"\033[1D";
        fputs(pr, stdout);
      }
      pos = 0;
      continue;
    }
    else if ((int)ch == 5)
    {
      flag_hist = 0;
      for (int i = 0; i < strlen(cmd) - pos; i++)
      {
        char *pr = (char *)"\033[1C";
        fputs(pr, stdout);
      }
      pos = strlen(cmd);
      continue;
    }
    else if ((int)ch == 127)
    {
      if (pos == 0)
        continue;
      else
      {
        pos--;
        cmd[pos] = '\0';
        fputs("\b \b", stdout);
      }
    }
    else if ((int)ch == 27)
    {
      char ch1 = getchar();
      if ((int)ch1 == 91)
      {
        char ch2 = getchar();
        if ((int)ch2 == 65)
        {
          for (int i = 0; i < strlen(cmd) + strlen(prompt); i++)
            fputs("\b \b", stdout);
          fputs(prompt1, stdout);
          // Up Arrow Key
          if (hist_cur >= 0 && shellHistory.index)
          {
            hist_cur--;
            if (hist_cur == -1)
              hist_cur = 0;
            cmd = history[hist_cur];
            pos = strlen(cmd);
            cnt++;
          }
          fprintf(stdout, cmd);
        }
        // Down Arrow Key
        else if ((int)ch2 == 66 && shellHistory.index)
        {
          for (int i = 0; i < strlen(cmd) + strlen(prompt); i++)
            fputs("\b \b", stdout);
          fputs(prompt1, stdout);
          if (hist_cur < shellHistory.index)
          {
            hist_cur++;
            if (hist_cur == shellHistory.index)
              hist_cur = shellHistory.index - 1;
            cmd = history[hist_cur];
            pos = strlen(cmd);
            cnt++;
          }
          fprintf(stdout, cmd);
        }
        // Left Arrow Key
        else if ((int)ch2 == 68)
        {
          if (pos > 0)
          {
            pos--;
            char *pr = (char *)"\033[1D";
            fputs(pr, stdout);
          }
        }
        // Right Arrow Key
        else if ((int)ch2 == 67)
        {
          if (pos < strlen(cmd))
          {
            pos++;
            char *pr = (char *)"\033[1C";
            fputs(pr, stdout);
          }
        }
      }
    }
    else if ((ch == EOF || ch == '\n' || pos >= CMD_LEN - 1))
      break;
    else
    {
      cmd[pos++] = ch;
      putchar(ch);
    }
  }
  if (pos == 0)
  {
    needExecution = 0;
    printf("\n");
    return cmd;
  }
  if (cmd[pos - 1] == '&')
    isBackGrnd = 1;
  cmd[pos] = '\0';
  reset_input_mode();
  return cmd;
}

/**
 * @brief Function to execute the cd command used to switch between directories
 *
 * @param cmd Entire command line input
 */
void executeCd(char *cmd)
{
  int ipFile = STD_INPUT, opFile = STD_OUTPUT;
  char **cmds = splitCommand(cmd, ipFile, opFile);
  if (cmds[1] == NULL)
  {
    fprintf(stderr, "ERROR. No argument to cd.\n");
  }
  else
  {
    if (chdir(cmds[1]) != 0)
      fprintf(stderr, "ERROR. Directory not found.\n");
  }
}

/**
 * @brief Function to execute the help command used to get help on using the terminal
 *
 * @param cmd Entire command line input
 */
void executeHelp(char *cmd)
{
  fprintf(stdout,
          "SHELL HELP\nThis is a SHELL designed as a part of assignment 2 of "
          "OS Lab.\n");
  fprintf(stdout, "Type man <command> to know about a command\n");
}

int main()
{
  // Look out for CTRL+C and CTRL+Z press events
  signal(SIGINT, ctrlCHandler);
  signal(SIGTSTP, ctrlZHandler);
  // signal(SIGKILL, ctrl9Handler);
  shell_history shellHistory;

  while (true)
  {
    fprintf(stdout, prompt);

    // initialise variables
    int isBackgrnd = 0;
    int needExecution = 1;
    // get command line input
    char *cmd = readLine(isBackgrnd, needExecution, shellHistory);
    fprintf(stdout, "\n");
    if (!needExecution)
      continue;
    char *cmdCopy = (char *)malloc(sizeof(char) * CMD_LEN);
    strcpy(cmdCopy, cmd);
    if (cmd[(int)strlen(cmd) - 1] == '&')
      cmd[(int)strlen(cmd) - 1] = '\0';
    shellHistory.push(cmdCopy); // add command to history
    if (strcmp("exit", cmd) == 0)
    { // exit from shell if exit is written
      printf("\n");
      shellHistory.updateFile();
      break;
    }
    if (strcmp("history", cmd) == 0)
    {
      // run special command history
      printf("\n");
      shellHistory.print();
      continue;
    }
    if (cmd[0] == 'd' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'e' && cmd[4] == 'p' && strlen(cmd) >= 7)
    {
      char *cmd1 = (char *)malloc(sizeof(char) * CMD_LEN);
      for (int i = 6; i < strlen(cmd); i++)
        cmd1[i - 6] = cmd[i];
      // fprintf(stdout,cmd1);
      // std::cout << file_path << std::endl;
      delep(cmd1);
      continue;
    }
    // initialise character array to extract part of input
    char *dest = (char *)malloc((11) * sizeof(char));
    for (int i = 0; i < 11; i++)
    {
      dest[i] = '\0';
    }
    if (strlen(cmd) > 2)
      strncpy(dest, cmd, 2);

    if (strcmp(dest, "cd") == 0)
    { // run special command cd
      executeCd(cmd);
      continue;
    }
    if (!strcmp(cmd, "pwd"))
    {
      char *pwd = (char *)malloc(1000 * sizeof(char));
      getcwd(pwd, 1000);
      pwd[strlen(pwd)] = '\0';
      fprintf(stdout, pwd);
      continue;
    }
    if (strlen(cmd) >= 4)
      strncpy(dest, cmd, 4);

    if (strcmp(dest, "help") == 0)
    { // run special command help
      executeHelp(cmd);
      continue;
    }
    // cout << cmd << endl;
    bool flag_wildcard = false;
    for (int i = 0; i < strlen(cmd); i++)
    {
      if (cmd[i] == '?' || cmd[i] == '*')
      {
        flag_wildcard = true;
        break;
      }
    }

    if (flag_wildcard)
    {
      string arg;
      vector<string> args;
      stringstream ss(cmd);
      string c = "";
      for (int i = 0; cmd[i] != ' '; i++)
        c.push_back(cmd[i]);
      while (ss >> arg)
        expandWildcards(arg, args);
      cout << "Expanded arguments:" << endl;
      string expanded_arg_concatenated = "";
      if (strstr(cmd, "sort") != nullptr)
      {
        expanded_arg_concatenated += "sort ";
        for (const auto &arg : args)
        {
          expanded_arg_concatenated += arg + " ";
        }
        cout << expanded_arg_concatenated << endl;
        vector<char *> expanded_args;
        int length = expanded_arg_concatenated.size();
        char *w = (char *)malloc((length + 1) * sizeof(char));
        int i = 0;
        for (char ch : expanded_arg_concatenated)
          w[i++] = ch;
        w[length] = '\0';
        expanded_args.push_back(w);
        for (const auto &exp_arg : expanded_args)
        {
          cout << exp_arg << endl;
          executeCommand(exp_arg, isBackgrnd);
        }
        continue;
      }
      if (strstr(cmd, "gedit") != nullptr)
      {
        expanded_arg_concatenated += "gedit ";
        for (const auto &arg : args)
          expanded_arg_concatenated += arg + " ";
        cout << expanded_arg_concatenated << endl;
        vector<char *> expanded_args;
        int length = expanded_arg_concatenated.size();
        char *w = (char *)malloc((length + 1) * sizeof(char));
        int i = 0;
        for (char ch : expanded_arg_concatenated)
          w[i++] = ch;
        w[length] = '\0';
        expanded_args.push_back(w);
        for (const auto &exp_arg : expanded_args)
        {
          cout << exp_arg << endl;
          executeCommand(exp_arg, isBackgrnd);
        }
        continue;
      }
      else
      {
        string first = "";
        for (int i = 0; i < strlen(cmd); i++)
        {
          if (cmd[i] != ' ')
            first.push_back((char)cmd[i]);
          else {
            first.push_back((char)cmd[i]);
            break;
          }
        }
        expanded_arg_concatenated += first;
        for (const auto &arg : args)
          expanded_arg_concatenated += arg + " ";
        cout << expanded_arg_concatenated << endl;
        vector<char *> expanded_args;
        int length = expanded_arg_concatenated.size();
        char *w = (char *)malloc((length + 1) * sizeof(char));
        int i = 0;
        for (char ch : expanded_arg_concatenated)
          w[i++] = ch;
        w[length] = '\0';
        expanded_args.push_back(w);
        for (const auto &exp_arg : expanded_args)
        {
          cout << exp_arg << endl;
          executeCommand(exp_arg, isBackgrnd);
        }
        continue;
      }
    }
    // test delep with wild_card;
    // sb command starts
    if (cmd[0] == 's' && cmd[1] == 'b')
    {
      int pos = strcspn(cmd, " ");
      if (pos == strlen(cmd))
      {
        cout << "Incorrect input format for sb command" << endl;
        continue;
      }
      char *command = (char *)malloc(sizeof(char) * pos + 1);
      strncpy(command, cmd, pos);
      command[pos] = '\0';

      int pid = atoi(cmd + pos + 1);

      pos = strcspn(cmd + pos + 1, " ");
      if (pos == strlen(cmd))
      {
        cout << "Incorrect input format for sb command" << endl;
        continue;
      }
      char *flag = cmd + pos + 1;

      // Check if the suggest flag is set
      bool suggest = false;
      cout << "Flag:-" << flag << endl;
      if (strstr(cmd, "-suggest") != nullptr)
      {
        suggest = true;
      }
      cout << suggest << endl;
      traverseProcessTree(pid, 0, suggest, 0);
      continue;
    }
    // execute every other command
    executeCommand(cmd, isBackgrnd);
    // flush the standard input and output.
    fflush(stdout);
    fflush(stdin);
  }
  return 0;
}