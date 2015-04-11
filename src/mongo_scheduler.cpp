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

#include "include/mongo_executor.hpp"
#include "include/mongo_scheduler.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

const Resources MongoScheduler::TASK_RESOURCES = Resources::parse(
        "cpus:" + stringify(CPUS_PER_TASK) + ";mem:"
                + stringify(MEM_PER_TASK)).get();


void MongoScheduler::registered(SchedulerDriver*, const FrameworkID&,
        const MasterInfo&)
{
    cout << "Registered!" << endl;
}

void MongoScheduler::reregistered(SchedulerDriver* driver,
        const MasterInfo& masterInfo)
{
    cout << "Worker came back and reregistered: with master ["
            << masterInfo.id() << "] at: " << masterInfo.hostname() << endl;
}

void disconnected(SchedulerDriver* driver)
{
    cout << "Master disconnected" << endl;
}

void MongoScheduler::resourceOffers(SchedulerDriver* driver,
        const vector<Offer>& offers)
{
    if (launched) return;

    vector<TaskInfo> tasks;
    foreach (const Offer& offer, offers) {
        Resources remaining = offer.resources();
        if (remaining.flatten().contains(TASK_RESOURCES)) {
            cout << "Starting MongoDb server, using offer [" << offer.id() <<
                    "] with resources: "<< offer.resources() << endl;

            TaskInfo task;
            task.set_name("MongoServerTask");
            task.mutable_task_id()->set_value(std::to_string(0));
            task.mutable_slave_id()->MergeFrom(offer.slave_id());
            task.mutable_executor()->MergeFrom(executor);

            Option<Resources> resources = remaining.find(
                    TASK_RESOURCES.flatten(role));
            CHECK_SOME(resources);
            task.mutable_resources()->MergeFrom(resources.get());
            remaining -= resources.get();
            tasks.push_back(task);
            driver->launchTasks(offer.id(), tasks);
            launched = true;
        }
        if (launched) {
            cout << "MongoDB now running on Slave IP [TODO] and port [TODO]"
                 << "\nPress Ctrl-C to terminate...\n" << endl;
            break;
        }
    }
}

void MongoScheduler::statusUpdate(SchedulerDriver* driver,
        const TaskStatus& status)
{
    int taskId = std::atoi(status.task_id().value().c_str());

    cout << "MongoServerTask #" << taskId << " is in state " <<
            status.state() << endl;

    if (status.state() == mesos::TASK_FINISHED) {
        cout << "Exiting now\n";
        driver->stop();
    }

    if (status.state() == mesos::TASK_LOST
            || status.state() == mesos::TASK_KILLED
            || status.state() == mesos::TASK_FAILED) {
        cout << "Aborting because task " << taskId << " is in unexpected state "
                << status.state() << " with reason " << status.reason()
                << ", from source " << status.source() << ":: With message: '"
                << status.message() << "'" << endl;
        driver->abort();
    }
    if (!implicitAcknowledgements) {
        driver->acknowledgeStatusUpdate(status);
    }
}

void MongoScheduler::error(SchedulerDriver* driver, const string& message)
{
    cout << message << endl;
}

const std::string MongoScheduler::REV { "0.0.1" };

int run_scheduler(const std::string& uri, const std::string& role,
        const std::string& master)
{

    ExecutorInfo executor;
    executor.mutable_executor_id()->set_value("default");
    executor.mutable_command()->set_value(uri);
    executor.set_name("Test Executor (C++)");
    executor.set_source("MongoExecutor.cpp");

    mesos::FrameworkInfo framework;
    framework.set_user(""); // Have Mesos fill in the current user.
    framework.set_name("MongoDB Framework (C++)");
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
            EXIT(1)
                    << "Expecting authentication principal in the environment";
        }
        if (!os::hasenv("DEFAULT_SECRET")) {
            EXIT(1) << "Expecting authentication secret in the environment";
        }

        Credential credential;
        credential.set_principal(getenv("DEFAULT_PRINCIPAL"));
        credential.set_secret(getenv("DEFAULT_SECRET"));
        framework.set_principal(getenv("DEFAULT_PRINCIPAL"));
        driver = std::make_shared<mesos::MesosSchedulerDriver>(
                mesos::MesosSchedulerDriver(&scheduler, framework, master,
                        implicitAcknowledgements, credential));
    } else {
        framework.set_principal("test-framework-cpp");
        driver = std::make_shared<mesos::MesosSchedulerDriver>(&scheduler,
                framework, master, implicitAcknowledgements);
    }
    int status = driver->run() == mesos::DRIVER_STOPPED ? 0 : 1;
    // Ensure that the driver process terminates.
    driver->stop();
    return status;
}
