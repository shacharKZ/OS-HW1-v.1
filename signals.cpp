#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
	exit(1); // TODO i added this SH
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
    exit(1); // TODO i added this SH
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
    exit(1); // TODO i added this SH
}

