// Copyright (c) Marco Massenzio, 2015.  All rights reserved.


#include <cstdlib>
#include <iostream>

#include <stout/flags/flags.hpp>

#include <gtest/gtest.h>

#include "mongo_scheduler.hpp"
#include "config.h"

using std::cout;
using std::cerr;
using std::endl;

using std::string;


// Program flags, allows user to run the tests (--test) or the Scheduler
// against a Mesos Master at --master IP:PORT; or the Executor, which will
// invoke Mongo using the --config FILE configuration file.
//
// All the flags are optional, but at least ONE (and at most one) MUST be
// present.
class MongoFlags : public flags::FlagsBase {
public:
  MongoFlags();

  Option<string> master;
  Option<string> config;
  string role;
  bool test;
};


inline MongoFlags::MongoFlags() {
  add(&MongoFlags::master, "master", "The IP address of the Mesos Master.");
  add(&MongoFlags::config, "config", "The location of the configuration file,"
      " on the Worker node (MUST exist).");
  add(&MongoFlags::role, "role", "The role for the executor", "*");
  add(&MongoFlags::test, "test", "Will only run unit tests and exit.", false);
}


int test(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


int main(int argc, char **argv) {
  MongoFlags flags;

  Try<Nothing> load = flags.load(None(), argc, argv);


  if (load.isError()) {
    LOG(ERROR) << flags.usage() << "Failed to load flags: "
    << load.error() << std::endl;
    return EXIT_FAILURE;
  }

  LOG(INFO) << "Running Version: " << std::to_string(VERSION_MAJOR)
  << "." << std::to_string(VERSION_MINOR);
  if (flags.test) {
    LOG(INFO) << "Running unit tests for Playground App";
    return test(argc, argv);
  }

  if (flags.config.isNone() || flags.master.isNone()) {
    LOG(ERROR) << "Must define both the master IP and the configuration file to use"
    << flags.usage();
    return EXIT_FAILURE;
  }

  auto masterIp = flags.master.get();
  auto role = flags.role;
  auto config = flags.config.get();
  cout << "MongoScheduler starting - rev. " << MongoScheduler::REV << '\n';

  return run_scheduler(masterIp, config, role);
}
