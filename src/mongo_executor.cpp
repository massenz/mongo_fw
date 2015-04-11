/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <thread>

#include "include/mongo_executor.hpp"

using std::cout;
using std::endl;

const std::string MongoExecutor::REV = "0.0.1";
const std::string MongoExecutor::MONGOD = "mongod";


void MongoExecutor::registered(ExecutorDriver* driver,
        const ExecutorInfo& executorInfo, const FrameworkInfo& frameworkInfo,
        const SlaveInfo& slaveInfo)
{
    cout << "Registered MongoDB executor Rev. " << MongoExecutor::REV <<
            " on " << slaveInfo.hostname() << endl;
}

void MongoExecutor::reregistered(ExecutorDriver* driver,
        const SlaveInfo& slaveInfo)
{
    cout << "Re-registered executor on " << slaveInfo.hostname() << endl;
}

void MongoExecutor::launchTask(ExecutorDriver* driver, const TaskInfo& task)
{
    cout << "Starting MongoDB Server - Task #" << task.task_id().value() << '\n';

    TaskStatus status;
    status.mutable_task_id()->MergeFrom(task.task_id());
    status.set_state(TASK_RUNNING);

    driver->sendStatusUpdate(status);

    // start MongoDB server (mongod) in a separate thread
    std::thread mongod(runMongoDb, configFile);
    // TODO: parse config file & emit location of logs

    // wait a bit, giving a chance to mongod start and save the PID:
    std::this_thread::sleep_for (std::chrono::seconds(1));
    cout << "Mongo Server started with PID: " << getPid(pidFileLocation) << endl;

    mongod.join();
    cout << "Terminating task " << task.task_id().value() << endl;

    status.mutable_task_id()->MergeFrom(task.task_id());
    status.set_state(TASK_FINISHED);

    driver->sendStatusUpdate(status);
}

long MongoExecutor::getPid(const string& pidFilename)
{
    long pid;

    std::ifstream pidFile(pidFilename);
    if (! pidFile) {
        std::cerr << "Cannot open PID File " << pidFilename << endl;
        return -1;
    }
    pidFile >> pid;
    return pid;
}


// Executes the mongod in-process
//
// Note for this to work, mongod's configuration must have set
// `processsManagement.fork: false`
void runMongoDb(const string& configFile)
{
    string cmd = MongoExecutor::MONGOD + " --config " + configFile;
    cout << "Launching mongod server with: " << cmd << endl;
    system(cmd.c_str());
}

int run_executor(const string& configFile)
{
    MongoExecutor executor(configFile);
    MesosExecutorDriver driver(&executor);
    return driver.run() == DRIVER_STOPPED ? 0 : 1;
}
