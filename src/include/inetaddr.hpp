//
// Created by Marco Massenzio on 4/6/15.
//

#ifndef _INETADDR_H
#define _INETADDR_H

#include <regex>

class InetAddress
{
public:
    InetAddress(const short unsigned int values[4], int port_ = 0);

    InetAddress(const std::string &address, int port = 0) : port(port)
    {
        for (int i = 0; i < 4; ++i)
        {
            octets[i] = 0;
        }
        parse(address);
        buildRepr();
    }

    /**
     * Empty virtual destructor (necessary for subclasses).
     */
    virtual ~InetAddress()
    { }

    static const std::regex IP_REGEX;

protected:
    short unsigned int octets[4];
    int port = 0;
    std::string repr;

    void buildRepr();

    void parse(const std::string& inetaddr);

public:
    operator const char *()
    {
        return repr.c_str();
    }

    static bool is_valid(const std::string &ipaddr)
    {
        return std::regex_match(ipaddr, IP_REGEX);
    }

    const std::string& to_string()
    {
        return repr;
    }

};

#endif //_INETADDR_H
