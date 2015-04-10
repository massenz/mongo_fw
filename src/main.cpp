
#include <iostream>

#include "include/mongo_executor.hpp"
#include "include/inetaddr.hpp"

using std::cout;
using std::endl;

int main1()
{
    using std::cout;
    using std::endl;

    InetAddress router = InetAddress("10.10.1.1", 8088);

    // TODO: manipulate the IP address
    cout << "InetAddr demo code\n";
    cout << "Router at IP: " << router << endl;

    return 0;
}
