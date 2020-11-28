//
//  main.cpp
//  ThreadSafeSignals
//
//  Created by Gusts Kaksis on 21/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#include "ThreadTests.hpp"
#include "SignalTests.hpp"

int main(int argc, const char * argv[]) {
    runThreadTests();
    runSignalTests();
    return 0;
}
