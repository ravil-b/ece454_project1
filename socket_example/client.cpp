/*
 * @brief: Internet domain, connection-oriented CLIENT
           send user command line input to server.
           quit when "." is typed at the prompt.
 * @author: John Shapley Gray, Interprocess Communications in Linux
 *          The Nooks and Crannies 
 */

#include "local_sock.h"
int
main( int argc, char *argv[] ) {
    int             orig_sock,           // Original socket in client
                    len;                 // Misc. counter
    struct sockaddr_in
        serv_adr;            // Internet addr of server
    struct hostent  *host;               // The host (server) info
    if ( argc != 2 ) {                   // Check cmd line for host name
        cerr << "usage: " << argv[0] << " server" << endl;
        return 1;
    }
    host = gethostbyname(argv[1]);       // Obtain host (server) info
    if (host == (struct hostent *) NULL ) {
        perror("gethostbyname ");
        return 2;
    }
    memset(&serv_adr, 0, sizeof( serv_adr));       // Clear structure
    serv_adr.sin_family = AF_INET;                 // Set address type
    memcpy(&serv_adr.sin_addr, host->h_addr, host->h_length);
    serv_adr.sin_port   = htons( PORT );           // Use our fake port
    // SOCKET
    if ((orig_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("generate error");
        return 3;
    }                                    // CONNECT
    if (connect( orig_sock,(struct sockaddr *)&serv_adr,
                sizeof(serv_adr)) < 0) {
        perror("connect error");
        return 4;
    }
    do {                                 // Process
        write(fileno(stdout),"> ", 3);
        if ((len=read(fileno(stdin), buf, BUFSIZ)) > 0) {
            write(orig_sock, buf, len);
            if ((len=read(orig_sock, buf, len)) > 0 )
                write(fileno(stdout), buf, len);
        }
    } while( buf[0] != '.' );            // until end of input
    close(orig_sock);
    return 0;
}
