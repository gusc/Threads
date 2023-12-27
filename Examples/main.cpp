//
//  main.cpp
//  Threads
//
//  Created by Gusts Kaksis on 11/10/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

#include "SignalExamples.hpp"
#include "ThreadExamples.hpp"
#include "TaskQueueExamples.hpp"

int main(int argc, const char * argv[]) {
    runThreadExamples();
    runTaskQueueExamples();
    runSignalExamples();
    return 0;
}
