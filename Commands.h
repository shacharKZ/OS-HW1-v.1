#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <cctype>
//#include <map>

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

enum Status{STOPPED, RUNNING};
enum CMD_TYP{NORMAL, TAKEOVER};


class Command {
// TODO: Add your data members
protected:
    const char* cmd_line;
public:
    Command(const char* cmd_line, CMD_TYP type = NORMAL);
    virtual ~Command() = default;
    CMD_TYP type;
    virtual void execute() = 0;
    const char* getCmd();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
protected:
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() = default;
};

class TimeoutCommand : public Command {
public:
    TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand()  = default;
    void execute() override;
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char* cmd_line);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
    int out_or_err;
    string first_cmd;
    string second_cmd;
    bool is_bg;
    Command* first_command;
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
    char** set_dir;
public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() = default;
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
    string dir_to_print;
    bool flag_print;
public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
    pid_t pid;
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() = default;
    void execute() override;
};

class LSCommand : public BuiltInCommand { // i added this SH
public:
    LSCommand(const char* cmd_line);
    virtual ~LSCommand() = default;
    void execute() override;
};


class JobsList;
class QuitCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

class CommandsHistory { // TODO what what?
protected:
    class CommandHistoryEntry {
        // TODO: Add your data members
    };
    // TODO: Add your data members
public:
    CommandsHistory();
    ~CommandsHistory() {}
    void addRecord(const char* cmd_line);
    void printHistory();
};

class HistoryCommand : public BuiltInCommand { // TODO what what
    // TODO: Add your data members
public:
    HistoryCommand(const char* cmd_line, CommandsHistory* history);
    virtual ~HistoryCommand() {}
    void execute() override;
};

class JobsList {
public:
    class JobEntry {
    private:
        // TODO: Add your data members
        pid_t pid;
        time_t start_time;
        Status status;
        int id;
        string cmd_line;
    public:
        JobEntry(Command*, time_t, Status, int id, int pid);
        JobEntry(const char*, time_t, Status, int id, int pid);
        JobEntry () = delete;
        pid_t getPid();
        time_t getStartTime();
        Status getStatus();
        int getId ();
        string getCmd ();
        void setStatus(Status);
    };

private:
    vector<JobEntry> jobs;
//    int max_id;
    int last_stooped;

public:
    JobsList();
    ~JobsList() = default;
    void addJob(Command&, Status, pid_t);
    void addJob(const char*, Status, pid_t);
    void printJobsList();
    void removeFinishedJobs();
    void removeJobById(int jobId);
    void killAllJobs();
    int getNumOfJobs();
    JobEntry *getLastStoppedJob();
    JobEntry *getLastJob();
    JobEntry * getJobById(int jobId);
    JobEntry * getJobByPid(pid_t pid);

};


class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char* cmd_line);
    virtual ~JobsCommand() = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    bool flag_good_input;
    int sig_num;
    int proc_id;
public:
    KillCommand(const char* cmd_line);
    virtual ~KillCommand() = default;
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class CpCommand : public BuiltInCommand {
    JobsList* jobs;

public:
    CpCommand(const char* cmd_line, JobsList* jobs);
    virtual ~CpCommand() {}
    void execute() override;
};



/* -------------------------------------------------
 *            Small Shell
--------------------------------------------------- */
class SmallShell {
private:
    SmallShell();
    std::string name;
    pid_t currentPid;
    char currentCmd[COMMAND_ARGS_MAX_LENGTH];
    bool currentFg;
public:
    // fields
    JobsList jb;
    vector<pair<int, pair<string, pid_t>>> time_jb;
//    map<pid_t , pair<pid_t , pid_t>> pipe_extra_jb; // TODO if we ever want to set signals for pipes
    string last_pwd;

    // Constructors
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        instance.setcurrentPid(-1);
        instance.setcurrentFg(false);

        return instance;
    }
    ~SmallShell();

    // methods
    void executeCommand(const char* cmd_line);
    string getName();
    void setName(std::string set_name);
    pid_t getcurrentPid();
    void setcurrentPid(pid_t currentPid);
    bool getcurrentFg();
    void setcurrentFg(bool);
    const char* getcurrentCmd();
    void setcurrentCmd(const char*);
};


class ChpromptCommand : public BuiltInCommand {
    std::string name_to_set;
public:
    ChpromptCommand(const char* cmd_line);
    virtual ~ChpromptCommand() = default;
    void execute() override;
};

#endif //SMASH_COMMAND_H_


/* --------------------------------------------------------
 * ----------------------UTILS-----------------------------
 */

class Utils {
public:
    static int strToInt (string str) {
        int n = -1;
        try {
            n = std::stoi(str);
        }
        catch (exception &e) {
            return -1;
        }

        int tmp = n / 10;
        int digits = 1;
        while (tmp != 0) {
            tmp = tmp / 10;
            digits++;
        }
        if (digits != str.length()) {
            return -1;
        }
        return n;
    }

};
