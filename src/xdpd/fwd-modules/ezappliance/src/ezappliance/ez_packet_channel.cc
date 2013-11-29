#include "ez_packet_channel.h"
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <rofl.h>
#include <rofl/datapath/pipeline/switch_port.h>
#include <rofl/common/utils/c_logger.h>
#include "../io/datapacketx86.h" 
#include "../io/bufferpool.h" 

using namespace xdpd::gnu_linux;

//Local static variable for ez_packet_channel thread
static pthread_t ez_thread;
static bool ez_continue_execution = true;

//Constructor and destructor
ez_packet_channel::ez_packet_channel(){
}

ez_packet_channel::~ez_packet_channel(){
        close(ez_packets_socket);
}

/**
* @name connect_to_ezproxy_packet_interface
* @brief creates TCP connection for packet-in and packet-out operations between NP-3 and xdpd
*/
int ez_packet_channel::connect_to_ezproxy_packet_interface() {
        int sockfd;
        struct sockaddr_in dest_addr;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        //fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);

        if (sockfd < 0) 
        {
                ROFL_ERR("Couldn't open SOCK_STREAM socket, errno(%d): %s\n", errno, strerror(errno));
                return -1;
        }

        dest_addr.sin_family = AF_INET; // host byte order
        dest_addr.sin_port = htons(8080); // destination port
        dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // destination address
        memset(&(dest_addr.sin_zero), '\0', 8); 

        /* Now connect to the server */
        if (connect(sockfd,(struct sockaddr*) & dest_addr, sizeof(struct sockaddr)) < 0) 
        {
                ROFL_ERR("Couldn't connect to %s:%d, errno(%d): %s\n", "127.0.0.1", 8080, errno, strerror(errno));
                return sockfd;
        }   
        ROFL_INFO("Connected to %s:%d\n", "127.0.0.1", 8080);

        char msg[] = "Testing message to server!";

        if(::write(sockfd, msg, strlen(msg)) < 0) { //returns bytes sent
                ROFL_ERR("ERROR writing to socket");
                
        } 
        return sockfd;
}

datapacket_t* ez_packet_channel::read(){
        
        datapacket_t* pkt; 
        datapacketx86* pkt_x86;
        uint8_t packet_buffer[4086];
        uint8_t input_port;
        uint32_t frame_size;	    

        //Allocate free buffer	
        pkt = bufferpool::get_free_buffer();
        pkt_x86 = ((datapacketx86*)pkt->platform_state);

        
        
        //Copy something from TCP socket to buffer, TODO: read packet metatada header containing input port and packet size
        int n=0;
        if((n = ::read(ez_packets_socket,packet_buffer,4086)) < 0){    
                bufferpool::release_buffer(pkt);
                return NULL;
        }

        //Parsing metedata header
        ROFL_DEBUG("Received message from EZ with length: %d\n", n);
        input_port = packet_buffer[0];
        frame_size = ntohs(((uint16_t*)(packet_buffer+1))[0]);

        //Init in user space
        pkt_x86->init(packet_buffer+3, frame_size, NULL, (uint32_t) input_port, (uint32_t) input_port, true, true);
        
        ROFL_DEBUG("Received packet from port: %d with length: %d\n", input_port, frame_size);
        ROFL_DEBUG("Filled buffer with id:%d. Sending to process.\n", pkt_x86->buffer_id);
        
        return pkt;	
}


unsigned int ez_packet_channel::write(unsigned int q_id, unsigned int num_of_buckets){

        datapacket_t* pkt;
        datapacketx86* pkt_x86;
        uint8_t packet_buffer[4086];
        uint8_t output_port = 1;
        
        (void)pkt_x86;

        // from where get pkt?
        pkt=NULL;

        pkt_x86 = (datapacketx86*)pkt->platform_state;

        //Creating metedata header
        packet_buffer[0] = output_port;
        ((uint16_t*)(packet_buffer+1))[0] = htons(pkt_x86->get_buffer_length());
        memcpy(packet_buffer+3, pkt_x86->get_buffer(), pkt_x86->get_buffer_length());

        //Send packet by TCP connection to EZ
        if(::write(ez_packets_socket, packet_buffer, 4086) < 0) {
                ROFL_ERR("ERROR writing to socket");
        } 

        //Free buffer
        bufferpool::release_buffer(pkt);
        return 0;
}


void ez_packet_channel::start() {
        
        datapacket_t* pkt;
        while (ez_continue_execution) {
                ez_packets_socket = connect_to_ezproxy_packet_interface();
                
                while (ez_continue_execution) {
                        if ((pkt = read()) == NULL) {
                                ROFL_INFO("EZ-Proxy TCP server not online");
                                //sleep (10)
                        }
                        //put pkt to pipeline
                }
        }
}

static void* ez_packet_channel_routine(void* param) {
        ez_packet_channel* channel = new ez_packet_channel();
        channel->start();
        
        delete channel;
        pthread_exit(NULL);
}

/**
* launches the main thread
*/
rofl_result_t launch_ez_packet_channel() {
        //Set flag
        ez_continue_execution = true;

        if(pthread_create(&ez_thread, NULL, ez_packet_channel_routine, NULL)<0){
                ROFL_ERR("pthread_create failed, errno(%d): %s\n", errno, strerror(errno));
                return ROFL_FAILURE;
        }
        return ROFL_SUCCESS;
}

rofl_result_t stop_ez_packet_channel() {
        ez_continue_execution = false;
        pthread_join(ez_thread,NULL);
        return ROFL_SUCCESS;
}
