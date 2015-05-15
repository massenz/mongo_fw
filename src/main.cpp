// Copyright (c) Marco Massenzio, 2015.  All rights reserved.


#include <cstdlib>
#include <iostream>

#include <stout/flags/flags.hpp>

#include <gtest/gtest.h>

#include "include/mongo_executor.hpp"
#include "include/mongo_scheduler.hpp"

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
class MongoFlags: public flags::FlagsBase
{
public:
  MongoFlags();

  Option<string> master;
  Option<string> config;
  string role;
  bool test;
};

MongoFlags::MongoFlags()
{
  add(&MongoFlags::master, "master", "The host address of the Mesos Master.");
  add(&MongoFlags::config, "config", "The location of the configuration file,"
      " on the Worker node (MUST exist).");
  add(&MongoFlags::role, "role", "The role for the executor", "*");
  add(&MongoFlags::test, "test", "Will only run unit tests and exit.", false);
}


void printUsage(const string& prog, const string& flags)
{
  cout << "Usage: " << os::basename(prog).get() << " [options]\n\n"
      "One (and only one) of the following options MUST be present.\n\n"
      "Options:\n" << flags << endl;
}


int test(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


int main(int argc, char** argv)
{
  MongoFlags flags;

  Try<Nothing> load = flags.load(None(), argc, argv);

  if (load.isError()) {
    flags.printUsage(std::cerr) << "Failed to load flags: "
        << load.error() << std::endl;
    return EXIT_FAILURE;
  }

  if (flags.help) {
    flags.printUsage();
    return EXIT_SUCCESS;
  }

  if (flags.test) {
    cout << "Running unit tests for Playground App\n";
    return test(argc, argv);
  }

  if (flags.config.isSome()) {
    cout << "Invoked by Mesos scheduler to execute binary" << endl;
    return run_executor(flags.config.get());
  }

  if (flags.master.isSome()) {
    string uri = os::realpath(argv[0]).get();
    auto masterIp = flags.master.get();
    auto role = flags.role;
    cout << "MongoExecutor starting - launching Scheduler rev. "
        << MongoScheduler::REV << " starting Executor at: " << uri << '\n';

    return run_scheduler(uri, role, masterIp);
  }
}
