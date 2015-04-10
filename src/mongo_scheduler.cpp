//
// Created by Marco Massenzio on 4/6/15.
//

#include "include/mongo_executor.hpp"

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

#include <libgen.h>

#include <iostream>
#include <string>


#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>
#include <mesos/type_utils.hpp>

#include <stout/check.hpp>
#include <stout/exit.hpp>
#include <stout/flags.hpp>
#include <stout/numify.hpp>
#include <stout/os.hpp>
#include <stout/stringify.hpp>

// TODO: parse command-line arguments
//#include "logging/flags.hpp"
//#include "logging/logging.hpp"



using std::cerr;
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

using mesos::Credential;
using mesos::ExecutorInfo;
using mesos::FrameworkID;
using mesos::MasterInfo;
using mesos::Offer;
using mesos::OfferID;
using mesos::Resources;
using mesos::Scheduler;
using mesos::SchedulerDriver;
using mesos::TaskInfo;
using mesos::TaskState;
using mesos::TaskStatus;

const int32_t CPUS_PER_TASK = 1;
const int32_t MEM_PER_TASK = 128;

class MongoScheduler : public Scheduler
{
public:
	static const std::string REV;

  MongoScheduler(
      bool _implicitAcknowledgements,
      const ExecutorInfo& _executor,
      const string& _role)
    : implicitAcknowledgements(_implicitAcknowledgements),
      executor(_executor),
      role(_role),
      tasksLaunched(0),
      tasksFinished(0),
      totalTasks(5) {}

  virtual ~MongoScheduler() {}

  virtual void registered(SchedulerDriver*,
                          const FrameworkID&,
                          const MasterInfo&)
  {
    cout << "Registered!" << endl;
  }

  virtual void reregistered(SchedulerDriver* driver,
		  const MasterInfo& masterInfo) override
  {
	  cout << "Worker came back and reregistered: with master [" << masterInfo.id()
			  << "] at: " << masterInfo.hostname() << endl;
  }

  virtual void disconnected(SchedulerDriver* driver) override
  {
	  cout << "Master disconnected" << endl;
  }

  virtual void resourceOffers(SchedulerDriver* driver,
                              const vector<Offer>& offers)
  {
    foreach (const Offer& offer, offers) {
      cout << "Received offer " << offer.id() << " with " << offer.resources()
           << endl;

      static const Resources TASK_RESOURCES = Resources::parse(
          "cpus:" + stringify(CPUS_PER_TASK) +
          ";mem:" + stringify(MEM_PER_TASK)).get();

      Resources remaining = offer.resources();

      // Launch tasks.
      vector<TaskInfo> tasks;
      while (tasksLaunched < totalTasks &&
             remaining.flatten().contains(TASK_RESOURCES)) {
        int taskId = tasksLaunched++;

        cout << "Launching task " << taskId << " using offer "
             << offer.id() << endl;

        TaskInfo task;
        task.set_name("Task " + std::to_string(taskId));
        task.mutable_task_id()->set_value(std::to_string(taskId));
        task.mutable_slave_id()->MergeFrom(offer.slave_id());
        task.mutable_executor()->MergeFrom(executor);

        Option<Resources> resources =
          remaining.find(TASK_RESOURCES.flatten(role));

        CHECK_SOME(resources);
        task.mutable_resources()->MergeFrom(resources.get());
        remaining -= resources.get();

        tasks.push_back(task);
      }

      driver->launchTasks(offer.id(), tasks);
    }
  }

  virtual void offerRescinded(SchedulerDriver* driver,
                              const OfferID& offerId) override {}

  virtual void statusUpdate(SchedulerDriver* driver, const TaskStatus& status) override
  {
    int taskId = std::atoi(status.task_id().value().c_str());

    cout << "Task " << taskId << " is in state " << status.state() << endl;

    if (status.state() == mesos::TASK_FINISHED) {
      tasksFinished++;
    }

    if (status.state() == mesos::TASK_LOST ||
        status.state() == mesos::TASK_KILLED ||
        status.state() == mesos::TASK_FAILED) {
      cout << "Aborting because task " << taskId
           << " is in unexpected state " << status.state()
           << " with reason " << status.reason()
           << ", from source " << status.source()
           << ":: With message: '" << status.message() << "'"
           << endl;
      driver->abort();
    }

    if (!implicitAcknowledgements) {
      driver->acknowledgeStatusUpdate(status);
    }

    if (tasksFinished == totalTasks) {
      driver->stop();
    }
  }

  virtual void frameworkMessage(SchedulerDriver* driver,
                                const mesos::ExecutorID& executorId,
                                const mesos::SlaveID& slaveId,
                                const string& data) {}

  virtual void slaveLost(SchedulerDriver* driver, const mesos::SlaveID& sid) {}

  virtual void executorLost(SchedulerDriver* driver,
                            const mesos::ExecutorID& executorID,
                            const mesos::SlaveID& slaveID,
                            int status) {}

  virtual void error(SchedulerDriver* driver, const string& message)
  {
    cout << message << endl;
  }

private:
  const bool implicitAcknowledgements;
  const ExecutorInfo executor;
  string role;
  int tasksLaunched;
  int tasksFinished;
  int totalTasks;
};


void usage(const char* prog)
{
  cerr << "Usage: " << os::basename(prog).get() << " master-ip\n\n" <<
          "master-ip\tThe Mesos Master IP address (and optionally port), eg: 10.10.2.15:5050" << endl;
}

const std::string MongoScheduler::REV{"0.0.1"};

int main(int argc, char** argv)
{
	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}
  // Find this executable's directory to locate executor.
  string path = os::realpath(dirname(argv[0])).get();
  string uri = path + "/do_work";

  cout << "MongoExecutor rev. " << MongoScheduler::REV << " starting at: " << uri << '\n';

  // TODO: parse command args flags
  auto role = "*";
  auto master = argv[1];

  ExecutorInfo executor;
  executor.mutable_executor_id()->set_value("default");
  executor.mutable_command()->set_value(uri);
  executor.set_name("Test Executor (C++)");
  executor.set_source("MongoExecutor.cpp");

  mesos::FrameworkInfo framework;
  framework.set_user(""); // Have Mesos fill in the current user.
  framework.set_name("Marco's First Framework (C++)");
  framework.set_role(role);

  if (os::hasenv("MESOS_CHECKPOINT")) {
    framework.set_checkpoint(
        numify<bool>(os::getenv("MESOS_CHECKPOINT")).get());
  }

  bool implicitAcknowledgements = true;
  if (os::hasenv("MESOS_EXPLICIT_ACKNOWLEDGEMENTS")) {
    cout << "Enabling explicit acknowledgments for status updates" << endl;

    implicitAcknowledgements = false;
  }

  std::shared_ptr<mesos::MesosSchedulerDriver> driver;
  MongoScheduler scheduler(implicitAcknowledgements, executor, role);

  if (os::hasenv("MESOS_AUTHENTICATE")) {
    cout << "Enabling authentication for the framework" << endl;

    if (!os::hasenv("DEFAULT_PRINCIPAL")) {
      EXIT(1) << "Expecting authentication principal in the environment";
    }

    if (!os::hasenv("DEFAULT_SECRET")) {
      EXIT(1) << "Expecting authentication secret in the environment";
    }

    Credential credential;
    credential.set_principal(getenv("DEFAULT_PRINCIPAL"));
    credential.set_secret(getenv("DEFAULT_SECRET"));

    framework.set_principal(getenv("DEFAULT_PRINCIPAL"));

    driver = std::make_shared<mesos::MesosSchedulerDriver>(
        &scheduler,
        framework,
        master,
        implicitAcknowledgements,
        credential);
  } else {
    framework.set_principal("test-framework-cpp");

    driver = std::make_shared<mesos::MesosSchedulerDriver>(
        &scheduler,
        framework,
        master,
        implicitAcknowledgements);
  }

  int status = driver->run() == mesos::DRIVER_STOPPED ? 0 : 1;

  // Ensure that the driver process terminates.
  driver->stop();

  return status;
}
