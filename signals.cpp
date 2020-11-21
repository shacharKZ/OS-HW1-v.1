#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

extern SmallShell& smash;

void ctrlZHandler(int sig_num) {
  cout<<"smash: got ctrl-Z" <<endl;
  pid_t currPid = smash.getcurrentPid();
  const char* cmd_line = smash.getcurrentCmd();

  // no process on back ground
  if (currPid == -1)
    return;

  if(kill(currPid ,SIGSTOP)==-1){
    perror("smash error: kill failed");
    return;
  }

  cout << "smash: process " << currPid << " was stopped" << endl;
  smash.jb.addJob(cmd_line, STOPPED, currPid);
}

void ctrlCHandler(int sig_num) {
  cout<<"smash: got ctrl-C" <<endl;
  pid_t currPid = smash.getcurrentPid();
  const char* cmd_line = smash.getcurrentCmd();

  // no process on back ground
  if (currPid == -1)
    return;

  if(kill(currPid ,SIGKILL)==-1){
    perror("smash error: kill failed");
    return;
  }
  cout << "smash: process " << currPid << " was killed" << endl;
}


void alarmHandler(int sig_num) {
  // TODO: Add your implementation
    exit(1); // TODO i added this SH
}

