//
// Created by Marco Massenzio on 4/6/15.
//

#include <iostream>
#include <regex>

#include "include/inetaddr.hpp"


InetAddress::InetAddress(const short unsigned int values[4], int port_) :
        port(0)
{
    for (int i = 0; i < 4; ++i)
    {
        short unsigned int octet = values[i];
        if (octet <= 255)
        {
            octets[i] = octet;
        } else
        {
            octets[i] = 0;
        }
    }
    if (port_ > 0)
    {
        port = port_;
    }
    buildRepr();
}

void InetAddress::buildRepr()
{
    repr = "";
    for (int i = 0; i < 4; ++i)
    {
        repr.append(std::to_string(octets[i]) + ".");
    }
    repr.pop_back();
    if (port > 0)
    {
        repr.append(":" + std::to_string(port));
    }
}

void InetAddress::parse(const std::string &inetaddr)
{
    try
    {
        std::smatch match;
        if (std::regex_search(inetaddr, match, IP_REGEX) &&
            match.size() == 7)
        {
            for (unsigned long i = 0; i < 4; ++i)
            {
                // match(0) is the full string matched by the regex
                int n = std::atoi(match.str(i + 1).c_str());
                if ((n >= 0) && (n < 256))
                {
                    octets[i] = static_cast<unsigned short> (n);
                }
            }
            // TODO: add port parsing
        } else
        {
            std::cerr << "[ERROR] Parsing " << inetaddr <<
                    ": Not enough matches (" << match.size() << ")\n";
        }
    } catch (std::regex_error &e)
    {
        // Syntax error in the regular expression
        // TODO: remove output to console and flag instead invalid IP
        std::cerr << "[ERROR] parsing " << inetaddr << "; caused by:" <<
        e.what() << '\n';
    }
}

const std::regex InetAddress::IP_REGEX = std::regex(
        "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})(:(\\d+))?");
