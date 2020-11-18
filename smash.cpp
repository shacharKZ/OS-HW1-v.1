#include <iostream>
#include <unistd.h>
#include <sys/wait.h> // TODO SH
#include <signal.h>
#include "Commands.h"
#include "signals.h"

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
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}