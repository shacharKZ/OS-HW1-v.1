#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <cctype>

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

enum Status{STOPPED, RUNNING};





class Command {
// TODO: Add your data members

protected:
    const char* cmd_line;

 public:
  Command(const char* cmd_line);
  virtual ~Command() = default;
  virtual void execute() = 0;
  const char* getCmd();
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
protected:
//    const char* cmd_line; // TODO WTF?? SH
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
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
  //void prepare() override;
  //void cleanup() override;
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
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() = default;
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() = default;
  void execute() override;
};




class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
  std::vector<Command> vec; // TODO this is my suggestion SH

  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class CommandsHistory {
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

class HistoryCommand : public BuiltInCommand {
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
//      JobEntry(JobEntry&) = delete ; // TODO in // for dibugging SH
        pid_t getPid();
        time_t getStartTime();
        Status getStatus();
        int getId ();
        string getCmd ();
        void setStatus(Status);
    };

private:
    vector<JobEntry> jobs;
    int max_id;
    int last_stooped;

public:
    JobsList();
    ~JobsList() = default;
    void addJob(Command&, Status, pid_t);
    void printJobsList();
    void removeFinishedJobs();
    JobEntry *getLastStoppedJob();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    //TODO continue from here - ofir
    JobEntry * getLastJob(int* lastJobId);
    void killAllJobs();

    // TODO: Add extra methods or modify exisitng ones as needed
};


class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
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
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

// TODO: add more classes if needed 
// maybe ls, timeout ?

class SmallShell {
 private:
  // TODO: Add your data members
  std::string name;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell(); // TODO probably future valgrind problems.... SH
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
  string getName();
  JobsList jb;
  string last_pwd;
  void setName(std::string set_name);
  Command* createCommand(const char *cmd_line);
};


//class ChpromptCommand : public BuiltInCommand {
//    SmallShell* smash;
//    std::string name_to_set;
//public:
//    ChpromptCommand(SmallShell* smash, std::string set_name, const char* cmd_line);
//    virtual ~ChpromptCommand() {}
//    void execute() override;
//};

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
