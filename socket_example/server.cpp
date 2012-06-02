/**
 * @brief: Internet domain, connection-oriented SERVER
 *         converts lower case to uppercase 
 * @author: John Shapley Gray, Interprocess Communications in Linux
 *          The Nooks and Crannies 
 */

#include "local_sock.h"
void signal_catcher(int);
int
main(  ) {
    int             orig_sock,          // Original socket in server
                    new_sock;           // New socket from connect
    socklen_t       clnt_len;           // Length of client address
    struct sockaddr_in                  // Internet addr client & server
                    clnt_adr, serv_adr;
    int             len, i;             // Misc counters, etc.

    // Catch when child terminates
    if (signal(SIGCHLD , signal_catcher) == SIG_ERR) {
        perror("SIGCHLD");
        return 1;
    }

    // SOCKET creation 
    if ((orig_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("generate error");
        return 2;
    }
    memset( &serv_adr, 0, sizeof(serv_adr) );      // Clear structure
    serv_adr.sin_family      = AF_INET;            // Set address type
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);  // Any interface
    serv_adr.sin_port        = htons(PORT);        // Use our fake port

    // BIND
    if (bind( orig_sock, (struct sockaddr *) &serv_adr,
                sizeof(serv_adr)) < 0){
        perror("bind error");
        close(orig_sock);
        return 3;
    }

    // LISTEN
    if (listen(orig_sock, 5) < 0 ) {
        perror("listen error");
        close (orig_sock);
        return 4;
    }

    // ACCEPT a connection
    do {
        clnt_len = sizeof(clnt_adr); 
        if ((new_sock = accept( orig_sock, (struct sockaddr *) &clnt_adr,
                        &clnt_len)) < 0) {
            perror("accept error");
            close(orig_sock);
            return 5;
        }
        if ( fork( ) == 0 ) {                        // Generate a CHILD
            while ( (len=read(new_sock, buf, BUFSIZ)) > 0 ){
                for (i=0; i < len; ++i)              // Change the case
                    buf[i] = toupper(buf[i]);
                write(new_sock, buf, len);           // Write back to socket
                if ( buf[0] == '.' ) break;          // Are we done yet?
            }
            close(new_sock);                         // In CHILD process
            return 0;
        } else
            close(new_sock);                 // In PARENT process
    } while( true );                         // FOREVER
    return 0;
}

void
signal_catcher(int the_sig){
    signal(the_sig, signal_catcher);         // reset
    wait(0);                                 // keep the zombies at bay
}
