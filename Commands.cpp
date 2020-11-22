#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <sys/types.h>
#include <dirent.h>
#include <algorithm>

extern SmallShell& smash;

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));


string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

/// --------------------------- From here down is here our code---------------------------
// TODO: Add your implementation for classes in Commands.h

// command class
Command::Command(const char *cmd_line) : cmd_line(cmd_line) {};

const char *Command::getCmd() {
  return cmd_line;
}


BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {};

LSCommand::LSCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

void LSCommand::execute() {

    char path[COMMAND_ARGS_MAX_LENGTH];
    if (!getcwd(path, sizeof(path))) {
        perror("");
    }

    struct dirent **names;
    vector<string> vec;

    int len = scandir(".", &names, 0, alphasort);
    if (len < 0) {
        perror("");
        return;
    }
    for (int i = 0; i < len; ++i) {
        vec.push_back(names[i]->d_name);
        free(names[i]);
    }
    free(names);

    sort(vec.begin(), vec.end());
    for (auto name : vec) {
        cout << name << endl;
    }

//    DIR* dir = opendir(path);
//    dirent * file_name = readdir(dir);
//    while (file_name) {
//        cout << file_name->d_name << endl;
//        file_name = readdir(dir);
//    }
//    closedir(dir);

}


ShowPidCommand::ShowPidCommand(const char* cmd_line) :
        BuiltInCommand(cmd_line) {};

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl;
};


//ChpromptCommand::ChpromptCommand(SmallShell* smash, string set_name, const char* cmd_line) :
//        smash(smash), name_to_set(set_name), BuiltInCommand(cmd_line) {};
//
//void ChpromptCommand::execute() {
//    if (name_to_set == "") smash->setName("smash");
//    else smash->setName(name_to_set);
//};

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : set_dir(plastPwd), BuiltInCommand(cmd_line) {};

void ChangeDirCommand::execute() {
    if (!set_dir || !set_dir[1] || set_dir[2]) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }

    string go_to_adress = string(set_dir[1]); // easier to work with and solves strange bug... SH

    if (go_to_adress == "-") {
        if (smash.last_pwd == "") {
            cerr<< "smash error: cd: OLDPWD not set" << endl;
            return;
        }
        go_to_adress = smash.last_pwd;
    }

    char dir[COMMAND_ARGS_MAX_LENGTH]; // get current pwd to store in smash.last_pwd (OLDPWD)
    if (!getcwd(dir, sizeof(dir))) {
        perror("");
        return;
    }

    if (chdir(go_to_adress.c_str()) == 0) { // in this case 0 symbols success
        smash.last_pwd = string(dir);
    }
    else {
        perror("");
    }
}


GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void GetCurrDirCommand::execute() {
    char dir[COMMAND_ARGS_MAX_LENGTH];
    if (getcwd(dir, sizeof(dir))) {
        std::cout << string(dir) << std::endl;
    }
    else perror(""); // should not get here SH
}




ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {};


void ExternalCommand::execute() {

    bool is_bg = _isBackgroundComamnd(cmd_line);

    pid_t pid = fork();
    if (pid == -1) {
        perror(""); // or cout << "smash.... fork fail..."...
    }
    else if (pid == 0) {// son. the new forked proc
        char cmd_cpy[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd_cpy,cmd_line);
        _removeBackgroundSign(cmd_cpy);
        const char *proc_args[] = {"/bin/bash", "-c" , cmd_cpy, nullptr};
        if (execv("/bin/bash", (char * const*) proc_args) == -1) {
            perror("");
            exit(0);
        }
    }
    else { // father. original proc
//        smash.jb.addJob(*this, RUNNING, pid); // TODO should also non "&" be added to jb?
        if (is_bg) {
            smash.jb.addJob(*this, RUNNING, pid);
//            cout<< "BG command was created" << endl; // TODO for debug
        }
        else {
            smash.setcurrentPid(pid);
            smash.setcurrentCmd(cmd_line);
            if (waitpid(pid,NULL,WUNTRACED) == -1) {
                perror("");
            }
            smash.setcurrentPid(-1);
        }
    }
}


// BUILT IN NUMBER #6
KillCommand::KillCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void KillCommand::execute() {

    string s_tmp = string(cmd_line);
    s_tmp = s_tmp.erase(s_tmp.find_first_of("kill"), s_tmp.find_first_of(" ") + 1);
    s_tmp = _ltrim(s_tmp);
    s_tmp = _rtrim(s_tmp);
    if (s_tmp[0] != '-') {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int tmp_index = s_tmp.find_first_of(' ');
    int proc_id = Utils::strToInt(s_tmp.substr(1, tmp_index-1));
    int sig_num = Utils::strToInt(s_tmp.substr(tmp_index+1, s_tmp.length()));

    if (proc_id == -1 || sig_num == -1) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    JobsList::JobEntry* je = smash.jb.getJobById(proc_id);
    if (je == nullptr) {
        cerr << "smash error: kill: job-id " << proc_id << " does not exist" << endl;
        return;
    }

    pid_t pid = je->getPid();
//    assert(waitpid(pid,NULL,WNOHANG)==0); // TODO if pass this section so pid is a real proc in prog SH
    int flag = kill(pid, sig_num); // TODO the usless arg use for debug. before finish just put it inside the if check SH
    if (flag == -1) {
        perror("");
        return;
    }
    cout << "signal number " << sig_num << " was sent to pid " << pid << endl;
}


// BUILT IN NUMBER #7
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line), jobs(jobs) {}

void ForegroundCommand::execute() {
  char* args[COMMAND_MAX_ARGS + 1];
  int argsNum = _parseCommandLine(cmd_line, args);
  pid_t jobPid;
  int jobId;
  JobsList::JobEntry* lastJob;

  if (argsNum == 1) {
    lastJob = jobs->getLastJob();
    if (lastJob) {
      jobPid = lastJob->getPid();
    } else {
      cout << "smash error: fg: jobs list is empty" << endl;
      return;
    }
  }

  else if (argsNum == 2) {
    jobId = Utils::strToInt(args[1]);
    if (jobId == -1) {
      cout << "smash error: fg: invalid arguments" << endl;
      return;
    } else if (jobId < 0) {
      cout << "smash error: fg: invalid arguments" << endl;
      return;
    } else {
      lastJob = jobs->getJobById(jobId);
      if (!lastJob) {
        cout << "smash error: fg: job-id " << jobId << " does not exist" << endl;
        return;
      }
      else
        jobPid = lastJob->getPid();
    }
  }

  else {
    cout << "smash error: fg: invalid arguments";
    return;
  }

  // removing from job list
  jobs->removeJobById(lastJob->getId());


  // continue the job if it stopped
  if(lastJob->getStatus() == STOPPED) {
    if (kill(jobPid, SIGCONT) == -1) {
      perror("smash error: kill failed");
      return;
    }
  }

  cout << lastJob->getCmd() << " :" << jobPid <<endl;

  smash.setcurrentPid(jobPid);
  smash.setcurrentCmd(lastJob->getCmd().c_str());

  if (waitpid(jobPid, nullptr, WUNTRACED) == -1) {
    perror("smash error: waitpid failed");
    return;
  }
  smash.setcurrentPid(-1);
}


// BUILT IN NUMBER #8

BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line), jobs(jobs) {}

void BackgroundCommand::execute() {
  char* args[COMMAND_MAX_ARGS + 1];
  int argsNum = _parseCommandLine(cmd_line, args);
  pid_t jobPid;
  int jobId;
  JobsList::JobEntry* lastJob;

  if (argsNum == 1) {
    lastJob = jobs->getLastStoppedJob();
    if (lastJob) {
      jobPid = lastJob->getPid();
    } else {
      cout << "smash error: bg: there is no stopped jobs to resume" << endl;
      return;
    }
  }

  else if (argsNum == 2) {
    jobId = Utils::strToInt(args[1]);
    if (jobId == -1) {
      cout << "smash error: bg: invalid arguments" << endl;
      return;
    } else if (jobId < 0) {
      cout << "smash error: bg: invalid arguments" << endl;
      return;
    } else {
      lastJob = jobs->getJobById(jobId);
      if (!lastJob) {
        cout << "smash error: bg: job-id " << jobId << " does not exist" << endl;
        return;
      } else if (lastJob->getStatus() == RUNNING) {
        cout << "smash error: bg: job-id " << jobId << " is already running in the background" << endl;
        return;
      } else {
        jobPid = lastJob->getPid();
      }
    }
  }

  else {
    cout << "smash error: bg: invalid arguments";
    return;
  }


  // continue the job
  cout << lastJob->getCmd() << " :" << jobPid <<endl;
  if (kill(jobPid, SIGCONT) == -1) {
    perror("smash error: kill failed");
    return;
  }

  // changing status of job
  lastJob->setStatus(RUNNING);
}



// BUILT IN NUMBER #9


QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),  jobs(jobs) {}

void QuitCommand::execute() {
  char* args[COMMAND_MAX_ARGS + 1];
  int argsNum = _parseCommandLine(cmd_line, args);

  // TODO check quit blabla kill
  for (int i=1; i < argsNum; i++) {
    if (strcmp(args[i], "kill") == 0) {
      cout << "sending SIGKILL signal to " << jobs->getNumOfJobs() << " jobs:" << endl;
      jobs->killAllJobs();
      break;
    }
  }
  exit(0);
}




RedirectionCommand::RedirectionCommand (const char* cmd_line) : Command(cmd_line) {};

void RedirectionCommand::execute() {
    string cmd_cpy = string(cmd_line);
    int split_index = cmd_cpy.find_first_of(">");
    if (split_index - 1 <= 0) {
        cout << "smash error: invalid arguments" << endl;
    }

    bool flag_overwrite = true;
    string file_name = "";

    if (cmd_cpy.length() >= split_index+1 && cmd_line[split_index+1] == '>') {
        flag_overwrite = false;
        file_name = _trim(cmd_cpy).substr(split_index+2);
    }
    else {
        file_name = cmd_cpy.substr(split_index+1);
    }
    file_name = _trim(file_name);
    string real_cmd = cmd_cpy.substr(0, split_index - 1);

    int dup_org = -1;
    try {
        dup_org = dup(1);
        close(1);
    }
    catch (exception &e) {
        perror("");
        return;
    }
    if (dup_org == -1) {
        cout << "smash error:....." << endl; // TODO
        return;
    }

    int file = -1;
    try {
        if (flag_overwrite) {
            file = open(file_name.c_str(),O_CREAT | O_WRONLY | O_TRUNC, 0666);
        }
        else {
            file = open(file_name.c_str(),O_CREAT | O_WRONLY | O_APPEND, 0666);
        }
    }
    catch (exception &e) {
        perror("");
        return;
    }

    if (file == -1) {
        cout << "smash error:....." << endl; // TODO
        return;
    }

    smash.executeCommand(real_cmd.c_str());

    try {
        if (close(1) == -1) {
            cout << "smash error:....." << endl; // TODO
            return;
        }

        if (dup(dup_org) == -1) {
            cout << "smash error:....." << endl; // TODO
            return;
        }
    }
    catch (exception &e) {
        perror("");
    }
}

PipeCommand::PipeCommand(const char* cmd_line) : Command(cmd_line) {};

void PipeCommand::execute() {
    //// smash fork first son and  fork second son
    //// first - reader, second - writer

    bool out_or_err = STDOUT_FILENO; // STDOUT_FILENO=1 // TODO implement

    string cmd_cpy = string(cmd_line);
    string first_cmd; // writer
    int split_index = cmd_cpy.find_first_of("|");
    if (cmd_cpy.length() > split_index && cmd_cpy[split_index + 1] == '&') {
        out_or_err = STDERR_FILENO; // STDERR_FILENO=2
        first_cmd = _trim(cmd_cpy.substr(split_index + 1));
    }
    else {
        first_cmd = _trim(cmd_cpy.substr(split_index));
    }
    string second_cmd = _trim(cmd_cpy.substr(0, split_index)); // reader

    bool is_bg = _isBackgroundComamnd(second_cmd.c_str()); // TODO implemnet

    int fd[2];
    if (pipe(fd) == -1) { // TODO check if really need to be check
        perror(""); // or cout << "smash.... pipe fail..."...
        exit(0);
    }

    int pid_first = fork();
    if (pid_first == -1) {
        perror(""); // or cout << "smash.... fork fail..."...
        return;
    }
    else if (pid_first > 0) {
        cout << "father got fork for first" << endl; //TODO
        if (is_bg) {
            smash.jb.addJob(*this, RUNNING, pid_first);
//            cout<< "BG command was created" << endl; // TODO for debug
        }
    }
    else if (pid_first == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);
//        SmallShell &sm_tmp = SmallShell::getInstance();
//        sm_tmp.executeCommand(first_cmd.c_str());
        smash.executeCommand(first_cmd.c_str());
        exit(0);
//        return;
    }

//    if (waitpid(pid_first,NULL,WUNTRACED) == -1) {
//        perror("");
//    }

    int pid_second = fork();
    if (pid_second == -1) {
        perror(""); // or cout << "smash.... fork fail..."...
        return;
    }
    else if (pid_second > 0) {
        cout << "father got fork for second" << endl; //TODO
    }
    else if (pid_second == 0) {
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);
//        SmallShell &sm_tmp = SmallShell::getInstance();
//        sm_tmp.executeCommand(second_cmd.c_str());
        smash.executeCommand(second_cmd.c_str());
        exit(0);
//        return;
    }
    if (close(fd[0]) == -1 || close(fd[1])) {
        perror("");
//        exit(0); // no exit. need to take care of waitpid VVV
    }

//    kill(pid_first, SIGEV_SIGNAL);
//    kill(pid_second, SIGEV_SIGNAL);

    if (waitpid(pid_first,NULL,WUNTRACED) == -1 || waitpid(pid_second,NULL,WUNTRACED) == -1) {
        perror("");
        exit(0);
    }
}


//void PipeCommand::execute2() {
//    //// smash fork son fork grandson
//    //// son - reader, grandson - writer
//    bool out_or_error = STDOUT_FILENO;
//    string cmd_cpy = string(cmd_line);
//    string grandson_cmd; // writer
//    int split_index = cmd_cpy.find_first_of("|");
//    if (cmd_cpy.length() > split_index && cmd_cpy[split_index + 1] == '&') {
//        out_or_error = STDERR_FILENO;
//        grandson_cmd = _trim(cmd_cpy.substr(split_index + 1));
//    }
//    else {
//        grandson_cmd = _trim(cmd_cpy.substr(split_index));
//    }
//    string son_cmd = _trim(cmd_cpy.substr(0, split_index)); // reader
//
//    bool is_bg = _isBackgroundComamnd(son_cmd.c_str());
//
//    int pid_son = fork();
//    if (pid_son == -1) {
//        perror(""); // or cout << "smash.... fork fail..."...
//        return;
//    }
//    else if (pid_son == 0) { // son. reader
//        int fd[2];
//        if (pipe(fd) == -1) { // TODO
//            perror(""); // or cout << "smash.... pipe fail..."...
//            exit(0);
//        }
//
//
//        int pid_grandson = fork();
//
//        if (pid_grandson == -1) {
//            perror(""); // or cout << "smash.... fork fail..."...
//            exit(0);
//        }
//        else if (pid_grandson == 0) { // grandson. writer
//            dup2(fd[0],STDIN_FILENO);
//            close(fd[0]);
//            close(fd[1]);
//            smash.executeCommand(grandson_cmd.c_str());
//        }
//        else { // son. reader
//            dup2(fd[1],out_or_error);
//            close(fd[0]);
//            close(fd[1]);
//            smash.executeCommand(son_cmd.c_str());
//        }
//        close(fd[0]);
//        close(fd[1]);
//        exit(0);
//
//    }
//    else { // father. smash
//        if (is_bg) { // TODO
//            smash.jb.addJob(*this, RUNNING, pid_son);
//        }
//        else {
//            smash.setcurrentPid(pid_son);
//            smash.setcurrentCmd(cmd_line);
//            if (waitpid(pid_son,NULL,WUNTRACED) == -1) {
//                perror("");
//            }
//            smash.setcurrentPid(-1);
//        }
//    }
//
//}

 /// --------------------------- smash V, Command ^ ---------------------------

SmallShell::SmallShell() : name("smash"), last_pwd(""), jb(){
// TODO: add your implementation
}

SmallShell::~SmallShell() {};

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char *cmd_line) {
	// For example:
/*
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */


    if (string(cmd_line).find_first_of(">") != -1) {
        return new RedirectionCommand(cmd_line);
    }
    else if (string(cmd_line).find_first_of("|") != -1) {
        return new PipeCommand(cmd_line);
    }

    char* args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(cmd_line, args);
    string cmd_s = string(args[0]);

    // TODO redirect creation of command for different proposes, like pipe\io SH

    if (cmd_s == "chprompt" || cmd_s == "chprompt&") {
        string name_to_set;
        if (args_num>1) name_to_set = string(args[1]);
        else name_to_set = "smash";
        name = name_to_set;
        return nullptr;

        /* // i find this VV useless so the func is ^^ SH
        string name_to_set;
        if (args_num>1) name_to_set = string(args[1]);
        else name_to_set = "";
        return new ChpromptCommand(this, name_to_set, cmd_line);
         */
    }
    else if (cmd_s == "ls" || cmd_s == "ls&") {
        return new LSCommand(cmd_line);
    }
    else if (cmd_s == "showpid" || cmd_s == "showpid&") {
      return new ShowPidCommand(cmd_line);
    }
    else if (cmd_s == "pwd" || cmd_s == "pwd&") {
      return new GetCurrDirCommand(cmd_line);
    }
    else if (cmd_s == "cd" || cmd_s == "cd&") {
      return new ChangeDirCommand(cmd_line, args);
    }
    else if (cmd_s == "jobs" || cmd_s == "jobs&") {
        jb.printJobsList();
    }

    else if (cmd_s == "fg" || cmd_s == "fg&") {
        return new ForegroundCommand(cmd_line, &jb) ;
    }

    else if (cmd_s == "bg" || cmd_s == "bg&") {
      return new BackgroundCommand(cmd_line, &jb) ;
    }

    else if (cmd_s == "quit" || cmd_s == "quit&") {
      return new QuitCommand(cmd_line, &jb) ;
    }

    else if (cmd_s == "kill" || cmd_s == "kill&") {
        if (args_num != 3 || args[1][0] != '-') {
            cerr << "smash error: kill: invalid arguments" << endl;
        }
        else {
            return new KillCommand(cmd_line);
        }
    }
    else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr; // TODO should not reach here SH
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)

  Command* command = SmallShell::CreateCommand(cmd_line);
  if (!command) {
      return;
  }
  command->execute();

  delete(command); // TODO ??

}

std::string SmallShell::getName() {
    return name;
}

void SmallShell::setName(string to_set) {
    name = to_set;
}


pid_t SmallShell::getcurrentPid() {
  return currentPid;
}


void SmallShell::setcurrentPid(pid_t pid) {
   currentPid = pid;
}

const char* SmallShell::getcurrentCmd() {
  return currentCmd;
}


void SmallShell::setcurrentCmd(const char* cmd) {
  strcpy(currentCmd, cmd);
}



/**
 this section is the implantation of JobEntry class
**/
JobsList::JobEntry::JobEntry(Command* command, time_t time, Status status, int id, int pid)
        : cmd_line(command->getCmd()), start_time(time), status(status), id(id), pid(pid) {}



JobsList::JobEntry::JobEntry(const char* cmd_line, time_t time, Status status, int id, int pid)
        : cmd_line(cmd_line), start_time(time), status(status), id(id), pid(pid) {}


pid_t JobsList::JobEntry::getPid() {
  return pid;
}

int JobsList::JobEntry::getId(){
  return id;
}

time_t JobsList::JobEntry::getStartTime(){
  return start_time;
}

Status JobsList::JobEntry::getStatus() {
//    int st;
//    int res = waitpid(pid, &st, WNOHANG);
  return status;
}


string JobsList::JobEntry::getCmd(){
  return cmd_line;
}

void JobsList::JobEntry::setStatus(Status new_status) {
  status = new_status;
}


/**
 * this section is the implantation of JobsList class
 */
JobsList::JobsList(): jobs(){
  last_stooped = -1;
};

void JobsList::addJob(Command& cmd, Status status, pid_t pid){
  int id = 1;
  if (!jobs.empty()) {
      id = jobs.back().getId() + 1;
  }
  jobs.push_back(JobEntry(&cmd, time(0), status,  id, pid));
}


void JobsList::addJob(const char* cmd, Status status, pid_t pid){
  int id = 1;
  if (!jobs.empty()) {
      id = jobs.back().getId() + 1;
  }
  jobs.push_back(JobEntry(cmd, time(0), status, id, pid));
}


//void JobsList::removeFinishedJobs() {
//    pid_t tmp_pid = 0;
//    while (tmp_pid = waitpid(-1,NULL,WNOHANG) > 0) {
//        assert(tmp_pid!=1); // TODO BUG remove
//        auto it = jobs.begin();
//        while (it->getPid() != tmp_pid) {
//            it++;
//        }
//        cout << "erase from jobs proc " << it->getPid() << endl;
//        jobs.erase(it);
//    }
//}

void JobsList::removeFinishedJobs(){

    auto it = jobs.begin();
    while (it < jobs.end()) {

        int status;
        pid_t res = waitpid(it->getPid(), &status, WNOHANG);
        if (res > 0) {
//            cout << "remove proc " << it->getPid() << endl; // TODO for debug
            jobs.erase(it++);
        }
        else if (res == -1) { // TODO for debug. when finish union this with prev "if" section
//            cout << "some error.... " << (*it).getPid() << "... anyway, i deleted it"<< endl; // TODO for debug
//            perror(""); // TODO for debug. mostly happens when race with short process like sleep 1&
            jobs.erase(it++);
        }
        else {
//            cout << "this proc is still running!!!!!! " << (*it).getPid() << endl; // TODO for debug
            ++it;
        }
    }
}

//void JobsList::removeFinishedJobs(){
//  int current_max = 0;
//  int result;
//  int status;
//  vector<JobEntry> new_jobs;
//
//  vector <JobEntry> :: iterator job = jobs.begin();
//
//
//  for(auto& job: jobs){
//    result = waitpid(job.getPid(), &status, WNOHANG);
//    if (result < 0){
//      // TODO : delete this shit
//      cout << "result weird " << job.getId() << endl;
//    }
//    if (result == 0) {
//      if(job.getId() > current_max)
//        current_max = job.getId();
//      new_jobs.push_back(JobEntry(job));
//
//    }// not finished yet
//  }
//  jobs = new_jobs;
//  max_id = current_max;
//}


void JobsList:: printJobsList(){
  for(auto job = jobs.begin(); job != jobs.end(); ++job){
    cout << "[" << job->getId()<< "] " << job->getCmd() << " : " << job->getPid() << " " << difftime(time(0), job->getStartTime()) << endl;

    //TODO DEBUG
    cout << "status is: " << job->getStatus() << endl;
  }
}

JobsList::JobEntry * JobsList::getJobById(int jobId) {

    for(auto job = jobs.begin(); job != jobs.end(); ++job) {
        if (job->getId() == jobId) {
            JobEntry* tmp = &(*job);
            return tmp;
        }
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob() {
  for(auto job = jobs.rbegin(); job != jobs.rend(); ++job){
      if (job->getStatus() == STOPPED)
        return &(*job);
    }
  return nullptr;
}


JobsList::JobEntry *JobsList::getLastJob() {
  if (jobs.empty())
    return nullptr;
  return &jobs.back();
}


void JobsList::removeJobById(int jobId) {
  for(auto job = jobs.begin(); job != jobs.end(); ++job){
    if (job->getId() == jobId) {
      jobs.erase(job);
      return;
    }
  }
  //TODO remove it before done, for debug
  cout << "no job with id: " << jobId << "found" << endl;
}

void JobsList::killAllJobs() {
  for(auto job = jobs.begin(); job != jobs.end(); ++job){
    cout <<  job->getPid() << ": " << job->getCmd() <<  endl;
    kill(job->getPid(), SIGKILL);
  }
}

int JobsList::getNumOfJobs() {
  return jobs.size();
}
