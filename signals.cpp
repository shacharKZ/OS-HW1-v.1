#include <iostream>
#include "signals.h"
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
//#include <iomanip>
#include "Commands.h"
#include <sys/stat.h>
#include <algorithm>


using namespace std;

extern SmallShell& smash;

void ctrlZHandler(int sig_num) {
    cout<<"smash: got ctrl-Z" <<endl;
    const char* cmd = smash.getcurrentCmd();
    pid_t currPid = smash.getcurrentPid();

    // no process on back ground
    if (currPid == -1)
        return;

    if(kill(currPid ,SIGSTOP)==-1){
        perror("smash error: kill failed");
        return;
    }

    cout << "smash: process " << currPid << " was stopped" << endl;
    if (smash.getcurrentFg() == false)
        smash.jb.addJob(cmd, STOPPED, currPid);
    else
        smash.jb.getJobByPid(currPid)->setStatus(STOPPED);
}

void ctrlCHandler(int sig_num) {
    cout<<"smash: got ctrl-C" <<endl;
    const char* cmd = smash.getcurrentCmd();
    pid_t currPid = smash.getcurrentPid();

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
    cout << "smash: got an alarm" << endl;
    time_t now = time(nullptr);
    smash.jb.removeFinishedJobs();

    auto it = smash.time_jb.begin();
    while (it < smash.time_jb.end()) {
        if(it->first <= now) {
            pid_t pid = it->second.second;
            if (smash.jb.getJobByPid(pid) != nullptr) {
                cout << "smash: " << it->second.first << " timed out!" << endl;
                kill(pid,SIGKILL);
            }
            smash.time_jb.erase(it++);
        }
        else {
            ++it;
        }
    }

    if (!smash.time_jb.empty()) {
        sort(smash.time_jb.begin(), smash.time_jb.end());
        alarm(smash.time_jb.begin()->first - now);
    }

}
