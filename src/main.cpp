#include <iostream>

#include "include/mongo_executor.hpp"
#include "include/mongo_scheduler.hpp"

using std::cout;
using std::cerr;
using std::endl;

void usage(const char* prog)
{
    cerr << "Usage: " << os::basename(prog).get() << " master-ip\n\n"
            << "master-ip\tThe Mesos Master IP address (and optionally port), "
               "eg: 10.10.2.15:5050" << endl;
}

int main(int argc, char** argv)
{
    // FIXME: this is a hack to work around some Eclipse stupidity,
    //        what we really want are two distinct binaries
    if (argc < 2) {
        cout << "Invoked by Mesos scheduler to execute binary" << endl;
        return run_executor("/Users/marco/dev/mongodb/mongod.conf");
    }

    if (argc == 2 && std::strcmp(argv[1], "-h") == 0) {
        usage(argv[0]);
        exit(0);
    }

    // Find this executable's directory to locate executor.
    // TODO: parse command args flags
    string uri = os::realpath(argv[0]).get();
    auto master = argv[1];
    auto role = "*";
    if (argc == 3) {
        role = argv[2];
    }
    cout << "MongoExecutor starting - launching Scheduler rev. "
         << MongoScheduler::REV << " starting Executor at: " << uri << '\n';

    return run_scheduler(uri, role, master);
}
