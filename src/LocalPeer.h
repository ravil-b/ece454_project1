#ifndef LOCALPEER_H
#define LOCALPEER_H



#include <vector>



struct RawRequest
{
    size_t messageLength;
    char data[];
};


struct RawResponse
{
    size_t messageLength;
    char data[];
};

#endif
