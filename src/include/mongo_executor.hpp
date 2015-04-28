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

#ifndef _MONGOEXECUTOR_H
#define _MONGOEXECUTOR_H

#include <iostream>
#include <thread>

#include <mesos/executor.hpp>
#include <stout/duration.hpp>
#include <stout/os.hpp>


using namespace mesos;
using std::string;


class MongoRunner {
public:
  static const string CONFIG;

public:
  MongoRunner(const string& config = CONFIG) : config_(config),
      pidFilename_("/tmp/mongod.pid"), running_(false) { }
  virtual ~MongoRunner() {}

  void operator ()();

  // TODO: guard against races for running_
  bool isRunning() { return running_; }

  // Retrieves the PID for the running mongod, saved to a file
  // This takes advantage of mongod's `processManagement.pidFilePath`
  // configuration variable.
  long getPid();

private:
  bool validateConfig();

private:
  string config_;
  string pidFilename_;
  bool running_;
};

class MongoExecutor: public mesos::Executor
{

public:
    static const string REV;
    // TODO: should this be the full path instead?
    static const string MONGOD;

public:
    MongoExecutor(const string& _configFile) :
        // TODO: read pidFileLocation instead from the config file:
        //      pidFilePath: "/path/to/mongod.pid"
        pidFileLocation{"/tmp/mongod.pid"}, configFile{_configFile} {}

    virtual ~MongoExecutor() {}

    virtual void registered(ExecutorDriver* driver,
            const ExecutorInfo& executorInfo,
            const FrameworkInfo& frameworkInfo, const SlaveInfo& slaveInfo)
                    override;

    virtual void reregistered(ExecutorDriver* driver,
            const SlaveInfo& slaveInfo) override;
    virtual void disconnected(ExecutorDriver* driver) override { }
    virtual void launchTask(ExecutorDriver* driver, const TaskInfo& task)
            override;

    virtual void killTask(ExecutorDriver* driver, const TaskID& taskId)
            override { }

    virtual void frameworkMessage(ExecutorDriver* driver, const string& data)
            override { }
    virtual void shutdown(ExecutorDriver* driver) override { }
    virtual void error(ExecutorDriver* driver, const string& message)
            override { }

private:
    // This is where mongod will store its PID
    // TODO: this is a statically defined place in the config file, must find
    //       a way to make it dynamic (or maybe not?)
    // TODO: right now, we just hard-code it, should read it instead from
    //      the config files `pidFilePath` entry
    string pidFileLocation;

    // The location of the configuration file
    string configFile;
};

int run_executor(const string&);

#endif // _MONGOEXECUTOR_H
