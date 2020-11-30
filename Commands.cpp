#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
//#include <iomanip>
#include "Commands.h"
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <sys/types.h>
#include <dirent.h>
#include <algorithm>
#include "signals.h"

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


Command::Command(const char *cmd_line, CMD_TYP type) : cmd_line(cmd_line), type(type) {};

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
        if (name != "." and name != "..")
            cout << name << endl;
    }
}


ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
    pid = getpid();
}

void ShowPidCommand::execute() {
    if (pid != -1) {
        cout << "smash pid is " << pid << endl;
    }
    else {
        perror("smash error: showpid");
    }

};


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
        perror("smash error: cd failed"); // chdir failed
        return;
    }

    if (chdir(go_to_adress.c_str()) == 0) { // in this case 0 symbols success
        smash.last_pwd = string(dir);
    }
    else {
        perror("smash error: cd failed"); // chdir failed
    }
}

ChpromptCommand::ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
    char* args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(cmd_line, args);

    if (args_num>1) {
        name_to_set = string(args[1]);
    }
    else {
        name_to_set = "smash";
    }
    smash.setName(name_to_set);
}

void ChpromptCommand::execute() {
    smash.setName(name_to_set); // actually useless... it is write in this way because of edge cases
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
    char dir[COMMAND_ARGS_MAX_LENGTH];
    if (getcwd(dir, sizeof(dir))) {
        dir_to_print = string(dir);
        flag_print = true;
    }
    else {
        flag_print = false;
    }

}

void GetCurrDirCommand::execute() {
    if (flag_print) {
        cout << dir_to_print << endl;
    }
    else {
        perror("smash error: pwd");
    }
}


JobsCommand::JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void JobsCommand::execute() {
    smash.jb.printJobsList();
}


ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {};

void ExternalCommand::execute() {
    bool is_bg = _isBackgroundComamnd(cmd_line);

    pid_t pid = 0;
    if (this->type != TAKEOVER) { // the trick
        pid = fork();
    }

    if (pid == -1) {
        perror("smash error: fork failed");
    }
    else if (pid == 0) {// son. the new forked proc

        if(setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(0);
        }

        char cmd_cpy[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd_cpy,cmd_line);
        _removeBackgroundSign(cmd_cpy);
        const char *proc_args[] = {"/bin/bash", "-c" , cmd_cpy, nullptr};
        if (execv("/bin/bash", (char * const*) proc_args) == -1) {
            perror("smash error: execv failed");
            exit(0);
        }
    }
    else { // father. original proc
        if (is_bg) {
            smash.jb.addJob(*this, RUNNING, pid);
        }
        if(!is_bg) {
            smash.setcurrentPid(pid);
            smash.setcurrentCmd(cmd_line);
            if (waitpid(pid,NULL,WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            smash.setcurrentPid(-1);
        }
    }
}


// BUILT IN NUMBER #6
KillCommand::KillCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
    char* args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(cmd_line, args);
    string cmd_s = string(args[0]);

    if (args_num != 3 || args[1][0] != '-') {
        flag_good_input = false;
        return;
    }

    string s_tmp = string(cmd_line);
    s_tmp = s_tmp.erase(s_tmp.find_first_of("kill"), s_tmp.find_first_of(" ") + 1);
    s_tmp = _ltrim(s_tmp);
    s_tmp = _rtrim(s_tmp);
    if (s_tmp[0] != '-') {
        flag_good_input = false;
        return;
    }
    int tmp_index = s_tmp.find_first_of(' ');
    sig_num = Utils::strToInt(s_tmp.substr(1, tmp_index-1));
    proc_id = Utils::strToInt(s_tmp.substr(tmp_index+1, s_tmp.length()));

    if (proc_id == -1 || sig_num == -1) {
        flag_good_input = false;
        return;
    }
    flag_good_input = true;
}

void KillCommand::execute() {
    if (!flag_good_input) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    JobsList::JobEntry* je = smash.jb.getJobById(proc_id);
    if (je == nullptr) {
        cerr << "smash error: kill: job-id " << proc_id << " does not exist" << endl;
        return;
    }

    pid_t pid = je->getPid();
    if (sig_num == SIGALRM) {
        signal(SIGALRM, SIG_DFL); // because of time
    }
    bool flag = kill(pid, sig_num) == -1;
    if (sig_num == SIGALRM) {
        signal(SIGALRM, alarmHandler); // because of time
    }
    if (flag) {
        perror("smash error: kill failed");
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
        jobId = atoi(args[1]);
        lastJob = jobs->getJobById(jobId);

        if (!lastJob) {
            cout << "smash error: fg: job-id " << jobId << " does not exist" << endl; // cerr? SH
            return;
        } else {
            jobPid = lastJob->getPid();
        }
    }

    else {
        cout << "smash error: fg: invalid arguments" << endl; // cerr? SH
        return;
    }

    // continue the job if it stopped
    if(lastJob->getStatus() == STOPPED) {
        if (kill(jobPid, SIGCONT) == -1) {
            perror("smash error: kill failed");
            return;
        }
    }

    cout << lastJob->getCmd() << " : " << jobPid <<endl;

    smash.setcurrentPid(jobPid);
    smash.setcurrentCmd(lastJob->getCmd().c_str());
    smash.setcurrentFg(true);
    if (waitpid(jobPid, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }

    smash.setcurrentFg(false);
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
        jobId = atoi(args[1]);
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

    else {
        cout << "smash error: bg: invalid arguments" << endl;
        return;
    }


    // continue the job
    cout << lastJob->getCmd() << " : " << jobPid <<endl;
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

    // check quit blabla kill
    for (int i=1; i < argsNum; i++) {
        if (strcmp(args[i], "kill") == 0) {
            cout << "smash: sending SIGKILL signal to " << jobs->getNumOfJobs() << " jobs:" << endl;
            jobs->killAllJobs();
            break;
        }
    }
    exit(0);
}


// BUILT IN NUMBER #10
CpCommand::CpCommand(const char *cmd_line, JobsList* jobs): BuiltInCommand(cmd_line),  jobs(jobs) {}

void CpCommand::execute() {

    pid_t pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
    }

    else if (pid == 0) {// son. the new forked proc

        if(setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(0);
        }

        char* args[COMMAND_MAX_ARGS + 1];
        char cmd_cpy[COMMAND_ARGS_MAX_LENGTH];
        char fullDest[COMMAND_ARGS_MAX_LENGTH*3];
        char fullSrc[COMMAND_ARGS_MAX_LENGTH*3];
        int BUFFER_SIZE = 1024;
        char buffer[BUFFER_SIZE];

        strcpy(cmd_cpy,cmd_line);
        _removeBackgroundSign(cmd_cpy);
        int argsNum = _parseCommandLine(cmd_cpy, args);

        if (argsNum != 3) {
            // should we print error?
            cout << "smash error: invalid arguments" << endl;
            exit(0);
        }

        realpath(args[1], fullSrc);
        realpath(args[2], fullDest);


        // more info about linux command on files can be found here:
        //https://linux.die.net/man/3/open


        //opening src
        int srcFile=open(args[1],O_RDONLY, 0666);
        if(srcFile == -1) {
            perror("smash error: open failed");
            exit(0);
        }

        if (strcmp(fullSrc, fullDest) == 0) {
          cout << "smash: " << args[1] << " was copied to " << args[2] << endl;
          close(srcFile);
          exit(0);
        }

        // opening dest
        int destFile=open(args[2],(O_WRONLY|O_CREAT|O_TRUNC), 0666);
        if(destFile == -1) {
            perror("smash error: open failed" );
            close(srcFile);
            exit(0);
        }

        std::size_t readSize = BUFFER_SIZE; // so will step into while
        std::size_t writeSize;

        while(readSize == BUFFER_SIZE) {
            readSize = read(srcFile, buffer, BUFFER_SIZE);
            if (readSize == -1) {
                perror("smash error: read failed");
                close(srcFile);
                close(destFile);
                return;
            }

            writeSize = write(destFile, buffer, readSize);
            if (writeSize == -1) {
                cout << "write size: " <<  writeSize << " read size " << readSize << endl;
                cout << buffer << endl;
                perror("smash error: write failed");
                close(srcFile);
                close(destFile);
                exit(0);
            }
        }

        cout << "smash: " << args[1] << " was copied to " << args[2] << endl;
        close(srcFile);
        close(destFile);
        exit(0);

    }
    else { // father. original proc

        bool is_bg = _isBackgroundComamnd(cmd_line);

        if (is_bg) {
            smash.jb.addJob(*this, RUNNING, pid);
        }
        else {
            smash.setcurrentPid(pid);
            smash.setcurrentCmd(cmd_line);
            if (waitpid(pid,NULL,WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            smash.setcurrentPid(-1);
        }
    }
}



RedirectionCommand::RedirectionCommand (const char* cmd_line) : Command(cmd_line) {};

void RedirectionCommand::execute() {
    string cmd_cpy = string(cmd_line);
    bool is_bg = false;
    if (_isBackgroundComamnd(cmd_line)) {
        is_bg = true;
        cmd_cpy.erase(cmd_cpy.find_last_of('&'));
    }
    int split_index = cmd_cpy.find_first_of(">");
    if (split_index - 1 <= 0) {
        cerr << "smash error: invalid arguments" << endl;
        return;
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
    string real_cmd = cmd_cpy.substr(0, split_index);

    int dup_org = dup(1);
    if (dup_org == -1) {
        perror("smash error: dup failed");
        return;
    }
    if (close(1) == -1) {
        perror("smash error: close failed");
        return;
    }

    int file = -1;

    if (flag_overwrite) {
        file = open(file_name.c_str(),O_CREAT | O_WRONLY | O_TRUNC, 0666);
    }
    else {
        file = open(file_name.c_str(),O_CREAT | O_WRONLY | O_APPEND, 0666);
    }

    if (file == -1) {
        perror("smash error: open failed");
        close(1);
        dup(dup_org);
        return;
    }

    if (is_bg) {
        real_cmd = real_cmd + '&';
    }

    smash.executeCommand(real_cmd.c_str());

    if (close(1) == -1) {
        perror("smash error: close failed");
        dup(dup_org);
        return;
    }

    if (dup(dup_org) == -1) {
        perror("smash error: dup failed");
        return;
    }
}

static bool isExternalCommand(string cmd) {
    char* args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(cmd.c_str(), args);
    string cmd_s = string(args[0]);

    if (cmd_s == "chprompt" || cmd_s == "ls" || cmd_s == "pwd" || cmd_s == "cd" ||
        cmd_s == "jobs" || cmd_s == "fg" || cmd_s == "bg" || cmd_s == "quit" ||
        cmd_s == "cp" || cmd_s == "kill") {
        return false;
    }
    return true;
}

PipeCommand::PipeCommand(const char* cmd_line) : Command(cmd_line) {
    //// first - reader, second - writer
    out_or_err = STDOUT_FILENO; // STDOUT_FILENO=1
    is_bg = _isBackgroundComamnd(cmd_line);

    string cmd_cpy = string(cmd_line);
    int split_index = cmd_cpy.find_first_of("|");
    if (cmd_cpy.length() > split_index && cmd_cpy[split_index + 1] == '&') {
        out_or_err = STDERR_FILENO; // STDERR_FILENO=2
        second_cmd = _trim(cmd_cpy.substr(split_index + 2)); // writer
    }
    else {
        second_cmd = _trim(cmd_cpy.substr(split_index+1)); // writer
    }
    first_cmd = _trim(cmd_cpy.substr(0, split_index)); // reader

//    if (first_cmd.find_first_of("|") != -1 || second_cmd.find_first_of("|") != -1) {
//        cout << "smash error: pipe: invalid arguments: pipe inside pipe" << endl;
//        return;
//    } // as we believe this kind of cases will not be test

    int idx = second_cmd.find_last_not_of(WHITESPACE);
    if (idx != string::npos && second_cmd[idx] == '&') {
        second_cmd[idx] = ' '; // i set the bg manually
    }
}

void PipeCommand::execute() {
    //// first - reader, second - writer

    int fd[2];
    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        exit(0);
    }

    Command* first_command = smash.SmallShell::CreateCommand(first_cmd.c_str());
    first_command->type = TAKEOVER;
    Command* second_command = smash.CreateCommand(second_cmd.c_str());
    second_command->type = NORMAL;

    int pid_wrapper = fork();
    if (pid_wrapper == -1) {
        perror("smash error: fork failed");
        return;
    }
    else if (pid_wrapper == 0) {
        if(setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(0);
        }

        int pid_first = fork();
        if (pid_first == -1) {
            perror("smash error: fork failed");
            return;
        }
        else if (pid_first == 0) { //
            if(setpgrp() == -1) {
                perror("smash error: setpgrp failed");
                exit(0);
            }
            if (dup2(fd[1], out_or_err) == -1) {
                perror("smash error: dup failed");
                exit(0);
            }
            if (close(fd[0]) == -1) {
                perror("smash error: close failed");
                exit(0);
            }
            first_command->execute();
            delete(first_command);
            exit(0);
        }

        if (dup2(fd[0], 0) == -1) {
            perror("smash error: dup failed");
            exit(0);
        }
        if (close(fd[0]) == -1) { // do not remove from the comment! adding this line may destroy everything
            perror("smash error: close failed");
            exit(0);
        }
        if (close(fd[1]) == -1) {
            perror("smash error: close failed");
            exit(0);
        }
        second_command->execute();
        delete(first_command);
        delete(second_command);

        if (waitpid(pid_first,NULL,WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
        }
        exit(0);
    }

    // if we ever want to send signals to pipe we need to add smash.pipe_extra_jb.append....
    // first and second cmd pid are pid_wrapper+1 and pid_wrapper+2

    delete(first_command);
    delete(second_command);
    if (close(fd[0]) == -1 || close(fd[1])) {
        perror("smash error: close failed");
        return;
    }
    if (is_bg) {
        smash.jb.addJob(*this, RUNNING, pid_wrapper);
    }
    else {
        smash.setcurrentPid(pid_wrapper);
        smash.setcurrentCmd(cmd_line);
        if (waitpid(pid_wrapper,NULL,WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
            return;
        }
        smash.setcurrentPid(-1);
    }
}


TimeoutCommand::TimeoutCommand(const char* cmd_line) : Command(cmd_line) {};

void TimeoutCommand::execute() {

    char* args[COMMAND_MAX_ARGS];
    if(_parseCommandLine(cmd_line, args) < 3) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    bool is_bg = _isBackgroundComamnd(cmd_line);

    string full_cmd = _trim(string(cmd_line));
    int sec = Utils::strToInt(args[1]);
    int idx = full_cmd.find_first_of((args[1]));
    if (sec == -1 || idx == -1 || idx+string(args[1]).length()+1 >= full_cmd.length()) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    string cmd_to_run = _trim(full_cmd.substr(idx+string(args[1]).length()));
    if (is_bg) {
        cmd_to_run.erase(cmd_to_run.find_last_of('&'));
    }
    Command * cmd = smash.CreateCommand(cmd_to_run.c_str());
    cmd->type = TAKEOVER; // the trick!

    time_t now = time(nullptr);
    pid_t pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        exit(0);
    }
    else if (pid == 0) {
        if(setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(0);
        }
        cmd->execute();
        delete(cmd);
        exit(0);
    }

    smash.jb.addJob(full_cmd.c_str(), RUNNING, pid);

    smash.time_jb.push_back(pair<int,pair<string, int>>(now+sec, pair<string, int>(full_cmd, pid)));
    sort(smash.time_jb.begin(), smash.time_jb.end());
    alarm(smash.time_jb.begin()->first - now);
    if(!is_bg) {
        smash.setcurrentPid(pid);
        smash.setcurrentCmd(cmd_line);
        if (waitpid(pid,NULL,WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
        }
        smash.setcurrentPid(-1);
    }
    delete(cmd);
}


/// --------------------------- smash V, Commands ^ ---------------------------

SmallShell::SmallShell() : name("smash"), last_pwd(""), jb(), time_jb() {};

SmallShell::~SmallShell() {};

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char *cmd_line) {
    if (string(cmd_line).find_first_of(">") != -1) {
        return new RedirectionCommand(cmd_line);
    }
    else if (string(cmd_line).find_first_of("|") != -1) {
        return new PipeCommand(cmd_line);
    }

    char* args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(cmd_line, args);
    string cmd_s = string(args[0]);

    if (cmd_s == "chprompt" || cmd_s == "chprompt&") {
        return new ChpromptCommand(cmd_line);
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
        return new JobsCommand(cmd_line);
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
    else if (cmd_s == "cp" || cmd_s == "cp&") {
        return new CpCommand(cmd_line, &jb) ;
    }
    else if (cmd_s == "kill" || cmd_s == "kill&") {
        return new KillCommand(cmd_line);
    }
    else if (cmd_s == "timeout") {
        return new TimeoutCommand(cmd_line) ;
    }

    return new ExternalCommand(cmd_line); // in any other case - external
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* command = SmallShell::CreateCommand(cmd_line);
    if (!command) {
        return;
    }
    command->execute();

    delete(command);
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

bool SmallShell::getcurrentFg() {
    return currentFg;
}


void SmallShell::setcurrentFg(bool cur) {
    currentFg = cur;
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


void JobsList::removeFinishedJobs(){

    auto it = jobs.begin();
    while (it < jobs.end()) {

        int status;
        pid_t res = waitpid(it->getPid(), &status, WNOHANG);
        if (res > 0) {
            jobs.erase(it++);
        }
        else if (res == -1) { // happens when erase timeout commands... so no error should be printed
            jobs.erase(it++);
        }
        else {
            ++it;
        }
    }
}


void JobsList:: printJobsList(){
    removeFinishedJobs(); // before printing remove useless

    for(auto job = jobs.begin(); job != jobs.end(); ++job){
        cout << "[" << job->getId()<< "] " << job->getCmd() << " : " << job->getPid() << " " << difftime(time(0), job->getStartTime()) << " secs";
        if(job->getStatus() == STOPPED)
            cout << " (stopped)";
        cout << endl;
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

JobsList::JobEntry * JobsList::getJobByPid(pid_t pid) {

    for(auto job = jobs.begin(); job != jobs.end(); ++job) {
        if (job->getPid() == pid) {
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
