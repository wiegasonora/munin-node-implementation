/*  
    Wiega Sonora            /13514019
    Ari Pratama Zhorifiandi /13514039
    Ahmad Faiq Rahman       /13514081 
*/
    
#define _GNU_SOURCE
#include <stdio.h> 
#include <string.h>  
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>  
#include <arpa/inet.h>    
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/unistd.h>
#include <netinet/in.h> 
#include <sys/time.h>
#include "sys/sysinfo.h"
    
#define TRUE   1 
#define FALSE  0 
#define PORT 4949   //port khusus MuninMaster

int main(int argc , char *argv[])  
{  

    int opt = TRUE;  
    int socket_node , addrlen , new_socket , socket_client[30] , 
          max_clients = 30 , activity, i , valread , sd;  
    int max_sd;  
    struct sockaddr_in address;  
    
    char * strtemp;         //penampung string yang akan dijadikan feedback
    char line[256];         //penampung command
    char * buffer;          //penampung buffer
    char hostname[1024];    //penampung hostname
    
	
    //socket descriptor 
    fd_set readfds;  
        
    //hostname
    gethostname(hostname, sizeof hostname);

    //header
    char header[20] = "# munin node at ";
    strcat(header, hostname);
    strcat(header, "\n");
    
    //inisialisasi socket
    for (i = 0; i < max_clients; i++){  
        socket_client[i] = 0;  
    }  
        
    //membuat socket node 
    if( (socket_node = socket(AF_INET , SOCK_STREAM , 0)) == 0){  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
    
    //set socket node untuk multiple koneksi
    if( setsockopt(socket_node, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
    
    //setingan koneksi
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  //4949
        
    //bind
    if (bind(socket_node, (struct sockaddr *)&address, sizeof(address))<0){  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", PORT);  
        
    //validasi listen
    if (listen(socket_node, 3) < 0){  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
        
    //menunggu koneksi yang masuk
    addrlen = sizeof(address);  
    puts("Waiting for connections ...");  
        
    while(TRUE){  
        FD_ZERO(&readfds);    
        FD_SET(socket_node, &readfds);  
        max_sd = socket_node;  
            
        //atribut multiclient 
        for ( i = 0 ; i < max_clients ; i++){  
            sd = socket_client[i];  
            if(sd > 0) {
                FD_SET( sd , &readfds);  
            }
            
            if(sd > max_sd) {
                max_sd = sd; 
            } 
        }  
    
        //set null
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
      
        if ((activity < 0) && (errno!=EINTR)){  
            printf("select error");  
        }  
            
        //menerima koneksi
        if (FD_ISSET(socket_node, &readfds)){  
            if ((new_socket = accept(socket_node, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
            
            //cetak atribut client ke layar
            printf("New connection , socket fd: %d , ip: %s , port: %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs
                  (address.sin_port));  
          
            //greeting msg
            if( send(new_socket, header, strlen(header), 0) != strlen(header) ){  
                perror("send");  
            }  
                
            //multi
            for (i = 0; i < max_clients; i++){  
                if( socket_client[i] == 0 ){  
                    socket_client[i] = new_socket;  
                    printf("Added to list of socket, index: %d\n" , i);  
                    break;  
                }  
            }  
        }  
            
        //processes
        for (i = 0; i < max_clients; i++){  
            sd = socket_client[i];  
                
            if (FD_ISSET( sd , &readfds)){  
                //client disconnected
                if ((valread = read( sd , &line, 1024)) == 0){  
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                    close( sd );  
                    socket_client[i] = 0;  
                } else {  

                    buffer = strtok(line, "\t\n\r");
                    printf("%s\n",line);
					int temp = 0;
					while (buffer[temp] != ' ' && temp < strlen(buffer)) {
						temp += 1;
					}
					buffer[temp] = '\0';
					
                
					printf("Command from client: %s,\n",line);

                    //COMMAND FEEDBACK
                    if (strcmp(buffer, "cap") == 0) {                                   //feedback untuk command: cap
                        write(socket_client[i],"cap multigraph dirtyconfig\n",27);
                    } else if (strcmp(buffer, "nodes") == 0) {                          //feedback untuk command: nodes
                        printf("%s\n", hostname);
                        write(socket_client[i], hostname , strlen(hostname));
                        write(socket_client[i],"\n.\n",3);
                    } else if (strcmp(buffer, "list") == 0) {                           //feedback untuk command: list 'hostname'
                        if (line[temp+1] == '\0' || line[temp+1] == '\n') {
                            write(socket_client[i], "memory\n", 7);
                        } else {
                            int temp2 = 0;
                            char *buffer2 = &buffer[temp+1];
                            while (buffer2[temp2] != ' ' && buffer2[temp2]) {
                                temp2 += 1;
                            }
                            buffer2[temp2] = '\0';
                            if (strcmp(buffer2,hostname) == 0) {
                                write(socket_client[i], "memory\n", 7);
                            } else {
                                write(socket_client[i], "\n", 1);
                            }
                        }
                    } else if (strcmp(buffer,"config") == 0) {                          //feedback untuk command: config memory
                        
                        int temp2 = 0;
                        char *buffer2 = &buffer[temp+1];
                        while (buffer2[temp2] != ' ' && temp2 < strlen(buffer2)) {
                            temp2 += 1;
                        }
                        buffer2[temp2] = '\0';
                        if (strcmp(buffer2,"memory") != 0) {
                            asprintf(&strtemp, "# Unknown command. Try cap, list, nodes, config, fetch, version or quit\n");
                            write(socket_client[i], strtemp, strlen(strtemp));
                            asprintf(&strtemp, ".\n");
                            write(socket_client[i], strtemp, strlen(strtemp));
                        } else {    
                            //retrieve memory info
                            FILE *fp;
                            char path[512];
                            fp = popen("free", "r");

                            if (fp == NULL) {
                                printf("Failed\n" );
                            } else {
                                char totalMem[256];
                                fgets(path, sizeof(path)-1, fp);
                                fgets(path, sizeof(path)-1, fp);
                                int i = 0;
                                while (i < strlen(path)) {
                                    if (path[i] >= '0' && path[i] <= '9') {
                                        int j = i;
                                        while (path[j] >= '0' && path[j] <= '9') {
                                            totalMem[j-i] = path[j];
                                            j += 1;
                                        }
                                        break;
                                    }
                                    i += 1;
                                }
                                pclose(fp);
                                long long totalPhysMem = atoi(totalMem);
                                totalPhysMem *= 1024;
                                asprintf(&strtemp, "graph_args --base 1024 -l 0 --upper-limit %llu\n", totalPhysMem);
                                write(socket_client[i], strtemp, strlen(strtemp));
                            }

                            write(socket_client[i], "graph_vlabel Bytes\n", 19);
                            write(socket_client[i], "graph_title Memory usage\n", 25);
                            write(socket_client[i], "graph_category system\n", 22);
                            write(socket_client[i], "graph_info This graph shows this machine memory.\n", 49);
                            write(socket_client[i], "graph_order used free\n", 22);
                            write(socket_client[i], "used.label used\n", 16);
                            write(socket_client[i], "used.draw STACK\n", 16);
                            write(socket_client[i], "used.info Used memory.\n", 23);
                            write(socket_client[i], "free.label free\n", 16);
                            write(socket_client[i], "free.draw STACK\n", 16);
                            write(socket_client[i], "free.info Free memory.\n", 23);
                            write(socket_client[i], ".\n", 2);
                        }
                    } else if (strcmp(buffer,"fetch") == 0) {                               //feedback untuk command: fetch memory 
                        int temp2 = 0;
                        char *buffer2 = &buffer[temp+1];
                        while (buffer2[temp2] != ' ' && temp2 < strlen(buffer2)) {
                            temp2 += 1;
                        }
                        buffer2[temp2] = '\0';

                        
                        if (strcmp(buffer2,"memory") != 0) {    //bukan fetch memory
                            write(socket_client[i], "# Unknown command. Try cap, list, nodes, config, fetch, version or quit\n",72);
                        } else {    
                            //retrieve memory usage
                            FILE *fp;
                            char path[512];
                            fp = popen("free", "r");
                            if (fp == NULL) {
                                printf("Failed\n" );
                            } else {
                                char usedMem[256];
                                char freeMem[256];
                                fgets(path, sizeof(path)-1, fp);
                                fgets(path, sizeof(path)-1, fp);
                                int i;
                                i = 0;
                                int idx = 0;
                                while (i < strlen(path)) {
                                    if (path[i] >= '0' && path[i] <= '9') {
                                        int j = i;
                                        if (idx == 0) {
                                            while (path[j] >= '0' && path[j] <= '9') {
                                                j += 1;
                                            }
                                            idx += 1;
                                        } else if (idx == 1) {
                                            while (path[j] >= '0' && path[j] <= '9') {
                                                usedMem[j-i] = path[j];
                                                j += 1;
                                            }
                                            idx += 1;
                                        } else if (idx == 2) {
                                            while (path[j] >= '0' && path[j] <= '9') {
                                                freeMem[j-i] = path[j];
                                                j += 1;
                                            }
                                            idx += 1;
                                        } else if (idx == 3) {
                                            break;
                                        }
                                        i = j;
                                    }
                                    i += 1;
                                }
                                pclose(fp);

                                //used memory
                                long long usedPhysMem = atoi(usedMem);
                                usedPhysMem *= 1024;
                                asprintf(&strtemp, "used.value %lld\n", usedPhysMem); 
                                write(socket_client[i], strtemp, strlen(strtemp));

                                //free memory
                                long long nfreeMem = atoi(freeMem);
                                nfreeMem *= 1024;
                                asprintf(&strtemp, "free.value %lld\n", nfreeMem);
                                write(socket_client[i], strtemp, strlen(strtemp));

                                asprintf(&strtemp, ".\n");
                                write(socket_client[i], ".\n", 2);
                            }
                        }
					} else if (strcmp(buffer, "version") == 0) {                       //feedback untuk command: version
						write(socket_client[i], "wiega faiq arizho on ",21);
						write(socket_client[i], hostname, strlen(hostname));
						write(socket_client[i], " version: 7.69\n", 15);
					}
                    else if (strcmp(buffer,"quit") == 0){                             //feedback untuk command: quit
                        //tutup koneksi
                        close(socket_client[i]);
                        
                    } else {    //selain command yg dikenal
                        write(socket_client[i],"# Unknown command. Try cap, list, nodes, config, fetch, version or quit\n",72);
                    }
                }  
            }  
        }  
    }  
        
    return 0;  
}  
