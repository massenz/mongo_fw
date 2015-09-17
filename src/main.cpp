// Copyright (c) Marco Massenzio, 2015.  All rights reserved.


#include <cstdlib>
#include <iostream>

#include <stout/flags/flags.hpp>

#include <gtest/gtest.h>
#include <mongo.pb.h>

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
  string config;
  string role;
  bool test;
  Option<string> user;
  Option<string> password;
};


MongoFlags::MongoFlags() {
  add(&MongoFlags::master,
      "master",
      "The IP address and port of the Mesos Master.");
  add(&MongoFlags::config,
      "config",
      "The location of the configuration file, on the Worker node (MUST exist).",
      ""
  );
  add(&MongoFlags::role,
      "role",
      "The role for the executor; by default '*'",
      "*"
  );
  add(&MongoFlags::test,
      "test",
      "If set, it will only run unit tests and exit.",
      false
  );
  add(&MongoFlags::user,
      "user",
      "The username to authenticate access"
  );
  add(&MongoFlags::password,
      "passwd",
      "The password to authenticate --user"
  );
}


int test(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


std::string stringify(const mongo::VersionInfo& info)
{
  // TODO(marco): add SHA to version string
  return std::to_string(info.major()) + "." + std::to_string(info.minor()) +
      (info.has_patch() ? "." + info.patch() : "") +
      (info.has_build() ? "-" + info.build() : "");
}


int main(int argc, char **argv) {
  MongoFlags flags;

  mongo::VersionInfo info;
  info.set_major(VERSION_MAJOR);
  info.set_minor(VERSION_MINOR);
  info.set_build(BUILD_ID);

  Try<Nothing> load = flags.load(None(), argc, argv);


  if (load.isError()) {
    LOG(ERROR) << flags.usage() << "Failed to load flags: "
    << load.error() << std::endl;
    return EXIT_FAILURE;
  }

  LOG(INFO) << "Running Version: " << stringify(info);
  if (flags.test) {
    LOG(INFO) << "Running unit tests for Playground App";
    return test(argc, argv);
  }

  if (flags.master.isNone()) {
    LOG(ERROR) << "Must define the Master's location" << flags.usage();
    return EXIT_FAILURE;
  }

  mongo::Credentials credentials;
  credentials.set_crypto("DES-3");
  if (flags.user.isSome()) {
    credentials.set_principal(flags.user.get());
    if (flags.password.isSome()) {
      credentials.set_password(flags.user.get());
    }
  }
  LOG(INFO) << "Authenticating for " << credentials.principal();

  auto masterIp = flags.master.get();
  auto role = flags.role;
  auto config = flags.config;
  cout << "MongoScheduler starting - rev. " << MongoScheduler::REV << endl;

  return run_scheduler(masterIp, config, role);
}
