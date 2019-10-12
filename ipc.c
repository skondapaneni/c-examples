#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/poll.h>

#include "ipc.h"
#include "logger.h"

typedef struct msg_handle_st {
    uint8_t version;
    msg_handler_func f;
    void *client_data;
} msg_handle_t;

msg_handle_t handlers[MSG_TYPE_MAX-1] = {};
char version[10];

void
msg_printf (uint8_t *msg, int size) {
    size_t cx = 0, max_buf = MAX_MSG_SIZE*4;
    char buf[max_buf];

    uint8_t *p = msg;
    for (; p != msg + size; p++) {
        cx  += snprintf(buf + cx, max_buf-cx, "%d ", *p); 
    }
    snprintf(buf + cx, max_buf-cx, "\n");
    log_info(buf);
}

int 
write_msg (int fd, msg_type_e type, void *buf, size_t nbytes)
{
    msg_hdr_t   msg_hdr;
    int         nwritten;
    struct iovec iov[2];

    // fill in the write_header at the beginning
    msg_hdr.major_version = VER_MAJOR;
    msg_hdr.minor_version = VER_MINOR;
    msg_hdr.type = type + '0'; 

    msg_hdr.nbytes = nbytes;
    log_info("hdr size %ld, msg size is %ld\n", sizeof(msg_hdr_t), 
               nbytes);

    iov[0].iov_base = &msg_hdr;
    iov[0].iov_len = sizeof(msg_hdr_t);

    iov[1].iov_base = buf;
    iov[1].iov_len = nbytes;

    // send the message to the server
    nwritten = writev (fd, iov, 2);
    log_info("nwriiten size is %d\n", nwritten);

    return (nwritten);
}

int 
read_msg (int fd, void *buf, size_t max_size)
{
    msg_hdr_t msg;
    int    num_bytes1 = 0, num_bytes2 = 0;
    struct iovec iov[2];

    memset(&msg, 0, sizeof(msg_hdr_t));

    iov[0].iov_base = &msg;
    iov[0].iov_len = sizeof(msg);

    iov[1].iov_base = buf;
    iov[1].iov_len = max_size;

    if ((num_bytes1 = readv(fd, iov, 1)) > 0) {
        msg_hdr_t *msg_p = (msg_hdr_t*) iov[0].iov_base;
        iov[1].iov_len = msg_p->nbytes;
        int type = (int)msg_p->type - '0';
        log_info("Received msg:- ver:%c.%c type %d, len %u\n",
                      msg_p->major_version,
                      msg_p->minor_version, 
                      type, 
                      msg_p->nbytes);
        msg_printf((uint8_t *)msg_p, sizeof(msg_hdr_t));

        if ((num_bytes2 = readv(fd, &iov[1], 1)) > 0) {
            msg_printf((uint8_t *)buf, msg_p->nbytes);
            if (type < MSG_TYPE_MAX) {
                if (handlers[type].f != NULL) {
                   (handlers[type]).f(msg_p, (unsigned char *)iov[1].iov_base, 
                            handlers[type].client_data);
                }
            }
        }
    }

    return num_bytes1 + num_bytes2;
}

int 
ipc_register (msg_type_e mt, uint8_t version, msg_handler_func f, void *client_data)
{
    if (mt < MSG_TYPE_MAX) {
        handlers[mt].f = f;
        handlers[mt].version = version;
        handlers[mt].client_data = client_data;
        return 0;
    }

    return -1;
}

int 
ipc_unregister (msg_type_e mt, uint8_t version) 
{
    /* Todo: Add version support */
    if (mt < MSG_TYPE_MAX && handlers[mt].f != NULL) {
        handlers[mt].f = NULL;
        handlers[mt].client_data = NULL;
        return 0;
    }
    return -1;
}

const char*
ipc_get_version ()
{
    snprintf(version, sizeof(version), "%c.%c", VER_MAJOR, VER_MINOR);
    return version; 
}

ipc_conn_params_t *
ipc_conn_params_new () 
{
     ipc_conn_params_t *params = 
           (ipc_conn_params_t *) malloc(sizeof(ipc_conn_params_t));
     if (!params) {
         memset(params, 0, sizeof(ipc_conn_params_t));
     }
     return params;
}

int 
ipc_conn_enable_nonblocking (ipc_conn_params_t *ipc_params)
{
    ipc_params->flags |= IPC_OPT_SOCKETS_NONBLOCKING; 
    return 0;
}

int 
ipc_conn_enable_sock_reuse (ipc_conn_params_t *ipc_params)
{
    ipc_params->flags |=  IPC_OPT_SOCKETS_REUSEABLE;    
    return 0;
}

int 
ipc_conn_set_callbacks (ipc_conn_params_t *ipc_params, 
                  disconnect_func sdf,
                  error_func ef)
{
    ipc_params->disconnect_f = sdf;
    ipc_params->error_f = ef;
    return 0;
}

inline short 
ipc_is_valid (ipc_context_t *ctxt) 
{
    return !(ctxt->sock_descriptor == -1);
}

static void 
close_sock (struct pollfd *pfds, int idx) 
{
    log_info("Closing socket %d\n", pfds[idx].fd);
    shutdown(pfds[idx].fd, SHUT_RDWR);
    pfds[idx].fd = -1;
    pfds[idx].revents = 0;
}

static void 
readjust_array (struct pollfd *pfds, int *nfds) 
{
    int i, j;
    for (i = 0; i < *nfds; i++) {
        if (pfds[i].fd == -1) {
            for (j = i; j < *nfds; j++) {
                pfds[j].fd = pfds[j+1].fd;
              }
              i--;
              *nfds = *nfds - 1;
        }
    }
}

int 
accept_new_conn (int sock_descriptor, struct pollfd *pfds, int *nfds) {
    int on = 1;
    int new_fd;

    /* Rate limit number of connections */
    if (*nfds >= 10) {
        perror("Maximum connections reached,"
                "cannot accept new connections\n");
        return -1;
    }

    do {
        new_fd = accept(sock_descriptor, NULL, NULL);
        if (new_fd < 0 && errno != EWOULDBLOCK) {
            perror("Accept failed\n");
            return -1;
        }

        if (new_fd > 0) {
            if (ioctl(new_fd, FIONBIO, (char *)&on) < 0) {
                perror("Failed to set socket to non-blocking mode\n");
                close(sock_descriptor);
                return -1;
            }
            log_info("Accepted incoming connection %d\n", new_fd);
            pfds[*nfds].fd = new_fd;
            pfds[*nfds].events = POLLIN;
            *nfds = *nfds + 1;
        }
    } while (new_fd != -1);
    return 0;
}

void 
ipc_poll(ipc_context_t  *ctxt) 
{
    struct pollfd pfds[10];
    int timeout_msecs = 50;
    int nrepeated = 0;
    int nfds = 0, i;
    int rc;

    memset(pfds, 0, sizeof(pfds));

    /* Add listen socket to poll fds to
     * accept any new incoming connections
     */
    pfds[0].fd = ctxt->sock_descriptor;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    nfds++;

    do {
        rc = poll(pfds, nfds, timeout_msecs);

        if (rc < 0) {
            perror("poll failed");
            if (ctxt->ipc_params->error_f != NULL) {
                (ctxt->ipc_params->error_f)(ctxt);
            }
            break;
        }

        if (rc == 0) {
            //printf("  poll() timed out.\n");
            nrepeated++;
            if (nrepeated == 100000) {
                if (ctxt->ipc_params->error_f != NULL) {
                    (ctxt->ipc_params->error_f)(ctxt);
                }
                break; // give up
            }
            continue;
        }

        nrepeated = 0;
        /* Handle unix socket connections*/
        for (i = 0; i < nfds; i++) {
            if (pfds[i].revents & POLLHUP) {
                log_info("connection reset by remote host\n");
                close_sock(pfds, i);
                readjust_array(pfds, &nfds);
                continue;
            }

            if ((pfds[i].fd != -1) &&  (pfds[i].revents)) {
                 /* If the fd is unix listen fd, add new fd's */
                if (pfds[i].fd == ctxt->sock_descriptor) {
                    if (accept_new_conn(ctxt->sock_descriptor, 
                         pfds, &nfds) < 0) {
                        perror("Accept failed\n");
                    } else {
                        continue; 
                    }
                } else {
                    /* Read on accepted connection fd's*/
                    char buff[MAX_MSG_SIZE];
                    int num_bytes = read_msg(pfds[i].fd, buff, MAX_MSG_SIZE);
                    log_info("nread = %d\n", num_bytes); 
                    if (num_bytes == 0) {  // connection was closed by client
                        close_sock(pfds, i);
                        readjust_array(pfds, &nfds);
                    }
                }
            } //end of if
        }// end of for nfds loop
    } while (1);
}

void 
ipc_select (ipc_context_t *ctxt)
{
    int   i, j, k, numready;
    int   max_sd, new_sd, maxi;
    int   desc_ready, end_server = 0;
    fd_set  master_set, working_set;

    struct sockaddr_in  client_addr;
    int client[FD_SETSIZE]; // array of client descriptors
 
    /*************************************************************/
    /* Initialize the master fd_set                              */
    /*************************************************************/
    FD_ZERO(&master_set);
    max_sd = ctxt->sock_descriptor;
    FD_SET(ctxt->sock_descriptor, &master_set);

    maxi = -1; // index in the client connected descriptor array
    for (i=0; i<FD_SETSIZE; i++)
        client[i] = -1;  // this indicates the entry is available. 

    while(1) { // main server loop
        /*******************************************************************/
        /* Copy the master fd_set over to the working fd_set.              */
        /* This is needed because select will overwrite this set           */
        /* and we will lose track of what we originally wanted to monitor. */
        /*******************************************************************/
        memcpy(&working_set, &master_set, sizeof(master_set));

        log_info("Waiting on select()...\n");
        // pass max descriptor and wait indefinitely until data arrives
        numready = select(max_sd+1, &working_set, NULL, NULL, NULL); 

        /* Check to see if the select call failed.*/
        if (numready < 0) {
            perror("select() failed");
            break;
        }

	if (numready == 0) {
            log_info("  select() timed out.  End program.\n");
            break;
        }

        /**********************************************************/
        /* One or more descriptors are readable.  Need to         */
        /* determine which ones they are.                         */
        /**********************************************************/
        desc_ready = numready;
        for (i=0; i <= max_sd  &&  desc_ready > 0; ++i) {
            if (FD_ISSET(i, &working_set)) { // new client connection
                if (i == ctxt->sock_descriptor) {
                    desc_ready -=1;
                    do {
                        log_info("new client connection\n");
                        socklen_t size = sizeof(client_addr);
                        new_sd = accept(ctxt->sock_descriptor, (struct sockaddr *)&client_addr, 
                                   &size);

                        if (new_sd < 0) {
                            if (errno != EWOULDBLOCK) {
                                perror("   accept() failed");
                                end_server = 1;
                            }
                            break;
                        }

                        for (j=0; j < FD_SETSIZE; j++) {
                            if (client[j] < 0) {
                                client[j] = new_sd; // save the descriptor
                                break;
                            }
                        }
                        if (j > maxi) {
                            maxi = j;   // max used index in client array
                        }

                        FD_SET(new_sd, &master_set); // add new descriptor to set of monitored ones
                        if (new_sd > max_sd) {
                            max_sd = new_sd; // max for select
                        }

                    } while (new_sd != -1);
                } 
            }
        }

        if (desc_ready > 0) {
            // check all clients if any received data
            for (k=0; k <= maxi; k++) {
                if (client[k] > 0) {
                    if (FD_ISSET(client[k], &working_set)) {
                        char buff[MAX_MSG_SIZE];
                        int num_bytes = read_msg(client[k], buff, MAX_MSG_SIZE);
                        log_info("nread = %d\n", num_bytes); 
                        if (num_bytes == 0) {  // connection was closed by client
                            close(client[k]);
                            FD_CLR(client[k], &master_set);
                            if (k == max_sd) {
                                while (FD_ISSET(max_sd, &master_set) == 0) {
                                      max_sd -= 1;
                                }
                            }
                            client[k] = -1;
                        }
                        if (--desc_ready <=0) { // num of monitored descriptors returned by select call
                            break; 
                        }
                    }
                }
            }
        }
    } // End main listening loop
}

static int
set_socket_reusable (int fd, ipc_conn_params_t *ipc_params) 
{
    struct stat st;
    int rc;
    /************************************************************************/
    /* Allow socket descriptor to be reuseable,                             */
    /* when the server is restarted before the required wait time expires.  */
    /* Note `SO_REUSEADDR` does not work with `AF_UNIX` sockets,            */
    /* so we will have to unlink the socket node if it already exists,      */
    /* before we bind. For safety, we won't unlink an already existing node */
    /* which is not a socket node.                                          */
    /************************************************************************/
    rc = stat (ipc_params->unix_path, &st);
    if (rc == 0) {
        /* A file already exists. Check if this file is a socket node.
         * If yes: unlink it.
         * If no: treat it as an error condition.
         */
        if ((st.st_mode & S_IFMT) == S_IFSOCK) {
            rc = unlink (ipc_params->unix_path);
            if (rc != 0) {
               log_err ("Error unlinking the socket node");
               return -1; 
            }
        } else {
            /* We won't unlink to create a socket in place of who-know-what.
             */
            log_err("The path already exists and is not a socket node.\n");
            return -1;
        }
    } else {
        if (errno == ENOENT) {
            /* No file of the same path: do nothing. */
        } else {
            log_err ("Error stating the socket node path");
            return -1; 
        }
    }
    return 0;
}

ipc_context_t * 
ipc_open (ipc_conn_params_t *ipc_params) 
{
    int fd;
    int on = 1;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Error while creating unix socket");
        return NULL;
    }

    if ((ipc_params->flags & IPC_OPT_SOCKETS_REUSEABLE) == 
           IPC_OPT_SOCKETS_REUSEABLE) {
        set_socket_reusable(fd, ipc_params);
    }

    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for      */
    /* the incoming connections will also be nonblocking since   */
    /* they will inherit that state from the listening socket.   */
   /**************************************************************/
    if ((ipc_params->flags & IPC_OPT_SOCKETS_NONBLOCKING) == 
           IPC_OPT_SOCKETS_NONBLOCKING) {
        if (ioctl(fd, FIONBIO, (char *)&on) < 0) {
            perror("Failed to set socket to non-blocking mode\n");
            close(fd);
            return NULL;
        }
    }

    ipc_context_t *ctxt = (ipc_context_t *) 
              malloc(sizeof(ipc_context_t));

    if (ctxt == NULL) {
        close(fd);
        return NULL;
    }

    ctxt->sock_descriptor = fd;
    ctxt->ipc_params = ipc_params;
    return ctxt;
}

int
ipc_bind (ipc_context_t *ctxt)
{
    int len;
    if (ctxt == NULL || !ipc_is_valid(ctxt)) {
        return -1;
    }

    /* Bind the socket */
    struct sockaddr_un  *sun = &ctxt->serv_addr;
    sun->sun_family = AF_UNIX;
    memset(sun->sun_path, 0, sizeof(sun->sun_path));
    strncpy(sun->sun_path, ctxt->ipc_params->unix_path, sizeof(sun->sun_path) - 1);
    len = sizeof(sun->sun_family) + sizeof(sun->sun_path) - 1;

    if (bind(ctxt->sock_descriptor, (struct sockaddr *)sun, len) < 0) {
        perror("Failed to bind\n");
        close(ctxt->sock_descriptor);
        ctxt->sock_descriptor = -1;
        return -1;
    }

    /* Set the listen back log */
    if (listen(ctxt->sock_descriptor, 5) == -1) {
        perror("listen error");
        close(ctxt->sock_descriptor);
        ctxt->sock_descriptor = -1;
        return -1;
    }

    return 0;
}

int
ipc_connect (ipc_context_t *ctxt)
{
    int len;
    if (ctxt == NULL || !ipc_is_valid(ctxt)) {
        return -1;
    }

    struct sockaddr_un  *sun = &ctxt->serv_addr;

    /* connect to the socket */
    sun->sun_family = AF_UNIX;
    memset(sun->sun_path, 0, sizeof(sun->sun_path));
    strncpy(sun->sun_path, ctxt->ipc_params->unix_path, 
             sizeof(sun->sun_path) - 1);
    len = sizeof(sun->sun_family) + sizeof(sun->sun_path) - 1;

    if (connect(ctxt->sock_descriptor, (struct sockaddr *)sun, len) < 0) {
        perror("Failed to connect to server\n");
        return -1;
    } else {
        log_info("Connected successfully\n");
    }
    return 0;
}

ipc_context_t * 
ipc_client_init (ipc_conn_params_t *ipc_params)
{
    ipc_context_t *ctxt = ipc_open(ipc_params);
    if (ctxt == NULL || !ipc_is_valid(ctxt)) {
        return NULL;
    }
    ipc_connect(ctxt);
    return ctxt;
}

ipc_context_t * 
ipc_server_init (ipc_conn_params_t *ipc_params)
{
    ipc_context_t *ctxt = ipc_open(ipc_params);
    if (ctxt == NULL || !ipc_is_valid(ctxt)) {
        return NULL;
    }
    ipc_bind(ctxt);
    return ctxt;
}
