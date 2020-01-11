#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/libplctag.h>
#include <tests/regression/packet.h>
#include <tests/regression/socket.h>
#include <tests/regression/util.h>
#ifdef WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <netdb.h>
    #include <sys/socket.h>
    #include <sys/types.h>
#endif

#define BUF_SIZE (550)

typedef struct {
    int listen_sock;
    int client_sock;
} context_s;



typedef int (*step_func)(context_s*);

static int accept_client(context_s *context);
static int expect_session_open_req(context_s *context);

static slice_s get_eip_pkt(int sock, slice_s buf);

static void usage(void);
static step_func find_step(const char *step_name);
static void check_steps(int num_steps, const char **step_names);
static int run_steps(context_s *context, int num_steps, const char **step_names);


struct {
    const char* name;
    step_func func;
} steps[] = {
    { .name="accept_client", .func=accept_client },
    { .name="expect_session_open_req", .func=expect_session_open_req },

    /* terminating entry. */
    { .name = NULL, .func = NULL }
};





int main(int argc, const char **argv)
{
    int rc = PLCTAG_STATUS_OK;
    context_s context = {0,};

    if(argc < 2) {
        usage();
        exit(PLCTAG_ERR_BAD_PARAM);
    }

    /* Check that all steps are supported. Does not return on failure. */
    check_steps(argc-1 , &argv[1]); /* skip the first argument as that is the program name. */

    /* open the socket. Does not return on failure. */
    context.listen_sock = socket_open(NULL, "44818");

    /* run the steps. */
    rc = run_steps(&context, argc-1, &argv[1]);

    return rc;
}


void usage(void)
{
    printf("ab <step>+\n");
    printf("   where <step> is one of the supported protocol steps:\n");
    printf("       accept_client - Listen on the listener socket for a client connection and open it.\n");
    printf("       expect_session_open_req - Expect and match a session open request.\n");
    printf("       return_session_open_resp - Return a valid session open request response.\n");
}


step_func find_step(const char *step_name)
{
    for(int i=0; steps[i].name; i++) {
        if(strcmp(step_name, steps[i].name) == 0) {
            return steps[i].func;
        }
    }

    return NULL;
}



void check_steps(int num_steps, const char **step_names)
{
    do {
        // read arguments to set up event chain
        for(int i=0; i < num_steps; i++) {
            step_func tmp_func = find_step(step_names[i]);

            if(!tmp_func) {
                printf("Step (%d) \"%s\" NOT FOUND!.\n", i, step_names[i]);
                usage();
                exit(PLCTAG_ERR_BAD_PARAM);
            } else {
                printf("Step (%d) \"%s\" found.\n", i, step_names[i]);
            }
        }
    } while(0);
}


int run_steps(context_s *context, int num_steps, const char **step_names)
{
    int rc = PLCTAG_STATUS_OK;

    for(int i=0; rc == PLCTAG_STATUS_OK && i < num_steps; i++) {
        step_func f = find_step(step_names[i]);

        rc = f(context);
    }

    return rc;
}






/*
 * get_eip_pkt
 *
 * Get an EIP packet.  This is a 24-byte header with zero or more bytes as a payload.
 * The header is not part of the length in the packet.
 */

slice_s get_eip_pkt(int sock, slice_s buf)
{
    slice_s res_buf = slice_make(buf.data, 0);
    int bytes_left_to_read = 24;

    /* get at least 24 bytes. */
    do {
        slice_s tmp_in_buf = slice_remainder(buf, res_buf.len);
        slice_s tmp_buf = socket_read(sock, tmp_in_buf);

        if(slice_err(tmp_buf)) {
            info("ERROR: Unable to read socket: %s.", plc_tag_decode_error(slice_err(tmp_buf)));
            return tmp_buf;
        }

        /* bump out the result buffer. */
        res_buf.len += tmp_buf.len;

        /* Do we have enough bytes for the payload length?  If so, process it. */
        if(res_buf.len >= 4) {
            /* get the packet length */
            uint16_t packet_len = (uint16_t)(slice_at(res_buf, 2) + (slice_at(res_buf, 3) << 8));
            bytes_left_to_read = (24 + packet_len) - res_buf.len;
            info("get_eip_pkt() got sufficient bytes, %d, to calculate real packet length, %d.", res_buf.len, packet_len);
        }
    } while(bytes_left_to_read > 0);

    return res_buf;
}




/****** Steps ******/



/* wait for a client to connect. This blocks. */

int accept_client(context_s *context)
{
    context->client_sock = accept(context->listen_sock, NULL, NULL);
    if (context->client_sock < 0) {
        printf("ERROR: accept() call failed: %s\n", gai_strerror(context->client_sock));
        return PLCTAG_ERR_BAD_CONNECTION;
    }

    return PLCTAG_STATUS_OK;
}



int expect_session_open_req(context_s *context)
{
    int rc = PLCTAG_STATUS_OK;
    uint8_t buf[BUF_SIZE];
    slice_s inslice = slice_make(buf, BUF_SIZE);
    slice_s inpkt;

    do {
        slice_s match_slice;

        /* get the input packet */
        inpkt = get_eip_pkt(context->client_sock, inslice);
        rc = slice_err(inpkt);
        if(rc != PLCTAG_STATUS_OK) {
            printf("ERROR: Unable to get EIP packet: %s.\n", plc_tag_decode_error(rc));
            break;
        }

        /* decode it and make sure that it is a Session Open request. */
        match_slice = unpack_slice(inpkt,
                                  "=65 =00 =04 =00 =00 =00 =00 =00 =00 =00 "
                                  "=00 =00 =00 =00 =00 =00 =00 =00 =00 =00 "
                                  "=00 =00 =00 =00 =01 =00 =00 =00 "
        );

        rc = slice_err(match_slice);
        if(rc != PLCTAG_STATUS_OK) {
            printf("ERROR: did NOT MATCH Session Open Request packet: %s.\n", plc_tag_decode_error(rc));
            break;
        }
    } while(0);

    return rc;
}


