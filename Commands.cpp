#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <assert.h>

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

    assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
    bool is_bg = _isBackgroundComamnd(cmd_line);

    pid_t pid = fork();
    assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
    if (pid == -1) {
        perror(""); // or cout << "smash.... fork fail..."...
    }
    else if (pid == 0) {// son. the new forked proc
        // TODO mybe
        assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
        setpgrp(); // TODO HOLLY BUG? i dont see any different... SH

        char *proc_args[] = {"/bin/bash", "-c" , (char *)cmd_line, NULL};
        if (execv("/bin/bash", proc_args) == -1) {
            perror("");
            exit(0);
        }

    }
    else { // father. original proc
        assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
        cout << "my pid is " << getpid << endl; // TODO
        assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
        smash.jb.addJob(*this, RUNNING, pid);
        if (is_bg) {
            cout<< "BG command was created" << endl;
            assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
            // TODO do something
        }
        else {
            if (waitpid(pid,NULL,WUNTRACED) == -1) {
                assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
                perror("");
            }
        }
        // TODO
    }
    assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
}


KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : jobs(jobs), BuiltInCommand(cmd_line) {};

void KillCommand::execute() {
//    string s_tmp = string(cmd_line);
//    s_tmp = s_tmp.erase(s_tmp.find_first_of("kill"), s_tmp.find_first_of("kill") + 4);
//    s_tmp = _ltrim(s_tmp);
//    if (s_tmp)
//    if (!cmd_line[5] || cmd_line[2][0] != "-") // TODO

}




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

    char* args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(cmd_line, args);
    string cmd_s = string(args[0]);

    // TODO redirect creation of command for different proposes, like pipe\io SH

    if (cmd_s == "chprompt") {
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
    else if (cmd_s == "ls") {

    }
    else if (cmd_s == "showpid") {
      return new ShowPidCommand(cmd_line);
    }
    else if (cmd_s == "pwd") {
      return new GetCurrDirCommand(cmd_line);
    }
    else if (cmd_s == "cd") {
      return new ChangeDirCommand(cmd_line, args);
    }
    else if (cmd_s == "kill") {
        if (args_num != 3) {
            cerr << "smash error: kill: invalid arguments" << endl;
        }
        else {
            return new KillCommand(cmd_line, &smash.jb); // TODO not implment yet
        }

    }
    else if (cmd_s == "jobs") {
        jb.printJobsList();
    }
    else {
        return new ExternalCommand(cmd_line);
    }
    assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH

    return nullptr; // TODO should not reach here SH
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)

  Command* command = SmallShell::CreateCommand(cmd_line);
    assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
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
JobsList::JobsList(): max_id(0), jobs(){
  last_stooped = -1;
};

void JobsList::addJob(Command& cmd, Status status, pid_t pid){
  cout << "my pid is " << getpid << endl;
  assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
  removeFinishedJobs(); // TODO

  max_id+=1;
  jobs.push_back(JobEntry(&cmd, time(0), status,  max_id, pid));
  //TODO: check if needed
  //cmd->setJobID(max_job_id);
  //cmd->setJobID(max_job_id);
}

void JobsList::removeFinishedJobs() {
    cout << "my pid is(removeFinishedJobs) " << getpid << endl; // TODO SH
    assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
    pid_t tmp_pid = 0;
    while (tmp_pid = waitpid(-1,NULL,WNOHANG) > 0) {
        assert(tmp_pid!=1); // TODO BUG remove
        auto it = jobs.begin();
        while (it->getPid() != tmp_pid) {
            it++;
        }
        cout << "erase from jobs proc " << it->getPid() << endl;
        jobs.erase(it);
    }
}

//void JobsList::removeFinishedJobs(){
//
//    auto it = jobs.begin();
//    while (it < jobs.end()) {
//
//        int status;
//        pid_t res = waitpid(it->getPid(), &status, WNOHANG);
//        if (res > 0) {
//            cout << "remove proc " << it->getPid() << endl;
//            jobs.erase(it++);
//        }
//        else if (res == -1) {
//            cout << "some error.... " << (*it).getPid() << endl;
////            perror("");
//            ++it;
//        }
//        else {
//            cout << "this proc is still running!!!!!! " << (*it).getPid() << endl;
//            ++it;
//        }
//    }
//}

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

  assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH

  for(auto job = jobs.begin(); job != jobs.end(); ++job){
    cout << "[" << job->getId()<< "] " << job->getCmd() << " : " << job->getId() << " " << difftime(time(0), job->getStartTime()) << endl;
//    cout << "[" << job->getId()<< "] " << job->getCmd() << " : " << job->getId() << " " << difftime(time(0), job->getStartTime()) << job->getStatus() << endl;

  }
}
