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

#ifndef _MONGO_SCHEDULER
#define _MONGO_SCHEDULER


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


using std::string;

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

const int32_t CPUS_PER_TASK = 2;
const int32_t MEM_PER_TASK = 512;

class MongoScheduler : public Scheduler
{
public:
	static const std::string REV;

  MongoScheduler(
      bool implicitAcknowledgements,
      const string& role,
      const string& config)
    : implicitAcknowledgements_(implicitAcknowledgements),
      role_(role),
      config_(config),
      launched_(false),
      pidFilename_("/tmp/mongod.pid") { }

  virtual ~MongoScheduler() {}

  virtual void registered(SchedulerDriver*,
                          const FrameworkID&,
                          const MasterInfo&) override;
  virtual void reregistered(SchedulerDriver* driver,
    const MasterInfo& masterInfo) override;

  virtual void disconnected(SchedulerDriver* driver) override {};

  virtual void resourceOffers(SchedulerDriver* driver,
                              const std::vector<Offer>& offers) override;

  virtual void offerRescinded(SchedulerDriver* driver,
                              const OfferID& offerId) override {}

  virtual void statusUpdate(SchedulerDriver* driver, const TaskStatus& status)
      override;

  virtual void frameworkMessage(SchedulerDriver* driver,
                                const mesos::ExecutorID& executorId,
                                const mesos::SlaveID& slaveId,
                                const string& data) override {}

  virtual void slaveLost(SchedulerDriver* driver, const mesos::SlaveID& sid)
      override {}

  virtual void executorLost(SchedulerDriver* driver,
                            const mesos::ExecutorID& executorID,
                            const mesos::SlaveID& slaveID,
                            int status) override {}

  virtual void error(SchedulerDriver* driver, const string& message) override;

private:
  bool validateConfig();
  long getPid();
  void setMongoCmd(mesos::CommandInfo* pCmd);

  // The resources necessary to run a MongoDb server on a Mesos Slave
  static const Resources TASK_RESOURCES;

  const bool implicitAcknowledgements_;
  string role_;
  string config_;
  bool launched_;
  string pidFilename_;
};


int run_scheduler(const string& uri, const string& role, const string& master);

#endif // _MONGO_SCHEDULER
