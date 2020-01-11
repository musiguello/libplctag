#include <lib/libplctag.h>
#include <stdio.h>



int main(int argc, const char **argv)
{
    int rc = PLCTAG_STATUS_OK;

    // read arguments to set up event chain
    for(int i=0; i < argc; i++) {
        printf("Arg[%d]= \"%s\".\n", i, argv[i]);
    }

    return PLCTAG_STATUS_OK;
}