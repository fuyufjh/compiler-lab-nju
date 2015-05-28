#ifndef _ERROR_TYPE_HEADER
#define _ERROR_TYPE_HEADER

int error_no;

enum error_type {
    ERROR_TYPE_UNKNOWN,
    ERROR_TYPE_A,
    ERROR_TYPE_B
};

const char* const error_description[] = {
    "<Unknwn Type>"
    "type A",
    "type B"
};

#endif
