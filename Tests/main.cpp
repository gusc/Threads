//
//  main.cpp
//  Threads
//
//  Created by Gusts Kaksis on 21/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#include "ThreadTests.hpp"
#include "TaskQueueTests.hpp"
#include "SignalTests.hpp"

int main(int argc, const char * argv[]) {
    runThreadTests();
    runTaskQueueTests();
    runSignalTests();
    return 0;
}
