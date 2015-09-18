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

#include "mongo_scheduler.hpp"

#include <fstream>
#include <stout/ip.hpp>

#include "config.h"


using std::cerr;
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

const Resources MongoScheduler::TASK_RESOURCES =
    Resources::parse("cpus:" + stringify(CPUS_PER_TASK) +
                     ";mem:" + stringify(MEM_PER_TASK)).get();

const std::string MongoScheduler::REV{
    std::to_string(VERSION_MAJOR) + "." +
    std::to_string(VERSION_MINOR)
};


bool MongoScheduler::validateConfig()
{
  std::ifstream cfgFile(config_);
  if (!cfgFile) {
    LOG(ERROR) << "Configuration " << config_ << " does not exist" << endl;
    return false;
  }
  // TODO(marco): validate contents, perhaps?
  return true;
}


long MongoScheduler::getPid()
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


void MongoScheduler::registered(SchedulerDriver *driver,
                                const FrameworkID &frameworkId,
                                const MasterInfo &masterInfo)
{
  cout << "Registered on Master node:  " << masterInfo.hostname()
  // TODO: net::IP() seems to emit the IP backwards?
  << " (" << net::IP(masterInfo.ip()) << ':' << masterInfo.port() << ")\n";
}


void MongoScheduler::reregistered(SchedulerDriver *driver,
                                  const MasterInfo &masterInfo)
{
  cout << "Worker came back and re-registered: with master [" << masterInfo.id()
  << "] at: " << masterInfo.hostname() << endl;
}


void disconnected(SchedulerDriver *driver)
{
  cout << "Master disconnected" << endl;
}


void MongoScheduler::setMongoCmd(mesos::CommandInfo *pCmd)
{
  pCmd->set_shell(false);
  pCmd->set_value("mongod");
  pCmd->add_arguments("mongod");
  pCmd->add_arguments("--config");
  pCmd->add_arguments(config_);
}


void MongoScheduler::resourceOffers(SchedulerDriver *driver,
                                    const vector<Offer> &offers)
{
  if (launched_) {
    LOG(INFO) << "MongoDB already launched";
    return;
  }

  vector<TaskInfo> tasks;
      foreach (const Offer &offer, offers) {
          Resources remaining = offer.resources();
          if (remaining.flatten().contains(TASK_RESOURCES)) {
            LOG(INFO) << "Starting MongoDb server, using offer [" << offer.id()
            << "] with resources: " << offer.resources() << endl;

            TaskInfo task;
            task.set_name("MongoServerTask");
            task.mutable_task_id()->set_value("mongodb_task");
            task.mutable_slave_id()->CopyFrom(offer.slave_id());

            mesos::CommandInfo *pCmd = task.mutable_command();
            setMongoCmd(pCmd);
            Option<Resources> resources = remaining.find(
                TASK_RESOURCES.flatten(role_));
            CHECK_SOME(resources);
            task.mutable_resources()->MergeFrom(resources.get());
            remaining -= resources.get();
            tasks.push_back(task);
            driver->launchTasks(offer.id(), tasks);
            launched_ = true;
          }
          if (launched_) {
            cout << "MongoDB now running on Slave IP [TODO] and port [TODO]"
            << "\nPress Ctrl-C to terminate...\n" << endl;
            break;
          }
        }
}


void MongoScheduler::statusUpdate(SchedulerDriver *driver,
                                  const TaskStatus &status)
{
  string taskId = status.task_id().value();
  if (status.state() == mesos::TASK_FINISHED) {
    LOG(INFO) << "Task: " << taskId << " finished\n";
    launched_ = false;
  } else if (status.state() == mesos::TASK_LOST ||
      status.state() == mesos::TASK_KILLED ||
      status.state() == mesos::TASK_FAILED) {
    LOG(WARNING) << "Aborting because task " << taskId
                 << " is in unexpected state "
                 << status.state() << " with reason " << status.reason()
                 << ", from source " << status.source() << '\n'
                 << status.message();
    driver->abort();
  } else {
    LOG(INFO) << status.message() << "(" << std::to_string(status.state())
              << ")";
  }
  if (!implicitAcknowledgements_) {
    driver->acknowledgeStatusUpdate(status);
  }
}


void MongoScheduler::error(SchedulerDriver *driver, const string &message)
{
  cout << message << endl;
}

namespace os {
bool hasenv(const string &var)
{
  return os::getenv(var).isSome();
}
}  // namespace os

int run_scheduler(const std::string &masterIp,
                  const std::string &config,
                  const std::string &role)
{

  mesos::FrameworkInfo framework;
  std::unique_ptr<mesos::MesosSchedulerDriver> driver;
  std::ostringstream name;

  name << "MongoDB Framework (rev. " << MongoScheduler::REV << ")";

  framework.set_user(""); // Have Mesos fill in the current user.
  framework.set_name(name.str());
  framework.set_role(role);


  if (os::hasenv("MESOS_CHECKPOINT")) {
    framework.set_checkpoint(
        numify<bool>(os::getenv("MESOS_CHECKPOINT")).get());
  }

  bool implicitAcknowledgements = !os::hasenv(
      "MESOS_EXPLICIT_ACKNOWLEDGEMENTS");
  cout << "Enabling " << (implicitAcknowledgements ? "implicit" : "explicit")
  << " acknowledgments for status updates" << endl;

  MongoScheduler scheduler(implicitAcknowledgements, role, config);

  if (os::hasenv("MESOS_AUTHENTICATE")) {
    cout << "Enabling authentication for the framework" << endl;
    if (!os::hasenv("DEFAULT_PRINCIPAL") || !os::hasenv("DEFAULT_SECRET")) {
      EXIT(1) << "Expecting authentication credentials in the environment";
    }

    Credential credential;
    credential.set_principal(getenv("DEFAULT_PRINCIPAL"));
    credential.set_secret(getenv("DEFAULT_SECRET"));
    framework.set_principal(getenv("DEFAULT_PRINCIPAL"));
    driver.reset(new mesos::MesosSchedulerDriver(
        &scheduler,
        framework,
        masterIp,
        implicitAcknowledgements,
        credential));
  } else {
    framework.set_principal("mongodb-framework-cpp");
    driver.reset(new mesos::MesosSchedulerDriver(
        &scheduler,
        framework,
        masterIp,
        implicitAcknowledgements));
  }
  int status = driver->run() == mesos::DRIVER_STOPPED ? 0 : 1;
  // Ensure that the driver process terminates.
  driver->stop();
  return status;
}
