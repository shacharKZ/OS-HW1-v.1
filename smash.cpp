#include <iostream>
#include <unistd.h>
#include <sys/wait.h> // TODO SH
#include <signal.h>
#include "Commands.h"
#include "signals.h"
#include <assert.h>

SmallShell& smash = SmallShell::getInstance(); // TODO trick to be check in future SH

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) { // TODO
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler


    while(true) {
        std::cout << smash.getName() << "> "; // TODO: change this (why?)
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
//        assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
//        cout << "my pid is(smash main) " << getpid << endl;
        smash.jb.removeFinishedJobs();
        smash.executeCommand(cmd_line.c_str());
        assert(waitpid(-1,NULL,WNOHANG)!=1); // TODO help me find the holly bug SH
    }
    return 0;
}