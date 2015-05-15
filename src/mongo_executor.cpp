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

#include "mongo_executor.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <thread>


using std::cout;
using std::endl;


const std::string MongoRunner::CONFIG = "/etc/mongodb/mongod.conf";

void MongoRunner::operator ()()
{
  running_ = true;
  if (!validateConfig()) {
    LOG(ERROR) << "Invalid configuration file " << config_ << endl;
    return;
  }
  string cmd_line = "/usr/local/bin/mongod --config " + config_;
  auto retValue = system(cmd_line.c_str());
  if (retValue < 0) {
    LOG(ERROR) << "Could not start a shell to run mongod (" << retValue << ")\n";
  } else {
    LOG(INFO) << "MongoDB Server exited\n";
  }
  running_ = false;
}

long MongoRunner::getPid()
{
  long pid;

  std::ifstream pidFile(pidFilename_);
  if (!pidFile) {
    LOG(ERROR) << "Cannot open PID File " << pidFilename_ << endl;
    return -1;
  }
  pidFile >> pid;
  return pid;
}

bool MongoRunner::validateConfig()
{
  std::ifstream cfgFile(config_);
  if (!cfgFile) {
    LOG(ERROR) << "Configuration " << config_ << " does not exist" << endl;
    return false;
  }
  // TODO(marco): validate contents, perhaps?
  return true;
}

const std::string MongoExecutor::REV = "0.0.1";
const std::string MongoExecutor::MONGOD = "mongod";

void MongoExecutor::registered(ExecutorDriver* driver,
    const ExecutorInfo& executorInfo, const FrameworkInfo& frameworkInfo,
    const SlaveInfo& slaveInfo)
{
  LOG(INFO) << "Registered MongoDB executor Rev. " << MongoExecutor::REV
      << " on " << slaveInfo.hostname() << endl;
}

void MongoExecutor::reregistered(ExecutorDriver* driver,
    const SlaveInfo& slaveInfo)
{
  LOG(INFO) << "Re-registered executor on " << slaveInfo.hostname() << endl;
}

void MongoExecutor::launchTask(ExecutorDriver* driver, const TaskInfo& task)
{
  // TODO(marco): here we can look at the name and take different actions:
  //    - install mongod
  //    - run mongod
  //    - add to ReplicaSet
  LOG(INFO) << "Running Task: " << task.name() << endl;

  // start MongoDB server (mongod) in a separate thread
  // TODO(marco): extract the configuration file from Task::data()
  MongoRunner runner;
  std::thread mongod(runner);
  // TODO: parse config file & emit location of logs

  TaskStatus status;
  status.mutable_task_id()->CopyFrom(task.task_id());

    // wait a bit, giving a chance to mongod start and save the PID:
  std::this_thread::sleep_for(std::chrono::seconds(1));
  int pid = -1;
  if (runner.isRunning()) {
    pid = runner.getPid();
    LOG(INFO) << "Mongo Server started with PID: " << pid << '\n';
    status.set_state(TASK_RUNNING);
    driver->sendStatusUpdate(status);

    mongod.join();
    LOG(INFO) << "Terminating Mongod (" << pid << ") with TaskId: "
              << task.task_id().value() << endl;
    status.set_state(TASK_FINISHED);
    driver->sendStatusUpdate(status);
  } else {
    status.set_state(TASK_FAILED);
    driver->sendStatusUpdate(status);
  }
}


int run_executor(const string& configFile)
{
  MongoExecutor executor(configFile);
  MesosExecutorDriver driver(&executor);
  return driver.run() == DRIVER_STOPPED ? 0 : 1;
}
