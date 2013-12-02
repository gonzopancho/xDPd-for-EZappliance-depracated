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
#include<time.h>
#include <rofl/datapath/pipeline/switch_port.h>
#include <rofl/common/utils/c_logger.h>
#include <rofl/datapath/pipeline/openflow/openflow1x/pipeline/of1x_pipeline.h>
#include <rofl/datapath/afa/openflow/openflow1x/of1x_cmm.h>
#include "../io/datapacket_storage.h"
#include "../io/bufferpool.h" 
#include "../ls_internal_state.h"

using namespace xdpd::gnu_linux;


//Local static variable for ez_packet_channel thread
static pthread_t ez_thread;
static bool ez_continue_execution = true;
static ez_packet_channel* ez_packket_channel_instance = NULL;

//Constructor and destructor
ez_packet_channel::ez_packet_channel(){
        ez_packket_channel_instance = this;
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
        //fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK); //set socket as non-blocking
        
        struct timeval tv;
        tv.tv_sec = 5;  /*5 secs Timeout */
        tv.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval)) < 0) {
                ROFL_ERR("[EZ-packet-channel] Setting socket timeout failed \n");
        }

        if (sockfd < 0) 
        {
                ROFL_ERR("[EZ-packet-channel] Couldn't open SOCK_STREAM socket, errno(%d): %s\n", errno, strerror(errno));
                return -1;
        }

        dest_addr.sin_family = AF_INET; // host byte order
        dest_addr.sin_port = htons(8080); // destination port
        dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // destination address
        memset(&(dest_addr.sin_zero), '\0', 8); 

        /* Now connect to the server */
        if (connect(sockfd,(struct sockaddr*) & dest_addr, sizeof(struct sockaddr)) < 0) 
        {
                ROFL_ERR("[EZ-packet-channel] Couldn't connect to %s:%d, errno(%d): %s\n", "127.0.0.1", 8080, errno, strerror(errno));
                return -1;
        }   
        ROFL_INFO("[EZ-packet-channel] Connected to %s:%d\n", "127.0.0.1", 8080);

        char msg[] = "Testing message to server!";

        if(::write(sockfd, msg, strlen(msg)) < 0) { //returns bytes sent
                ROFL_ERR("[EZ-packet-channel] ERROR writing to socket\n");
                
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
                ROFL_DEBUG_VERBOSE("[EZ-packet-channel] Error: %s\n", strerror(errno));
                bufferpool::release_buffer(pkt);
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                        ROFL_DEBUG_VERBOSE("[EZ-packet-channel] socket read timeout\n");
                return NULL;
        }
        if (n==0) {
                ROFL_ERR("[EZ-packet-channel] EZ-Proxy Server no longer online!\n");
                sleep(10);
                throw 20; // handled upper will break the read loop
                return NULL;
        }

        //Parsing metedata header
        ROFL_DEBUG("[EZ-packet-channel] Received message from EZ with length: %d\n", n);
        input_port = packet_buffer[0];
        frame_size = ntohs(((uint16_t*)(packet_buffer+1))[0]);

        //Init in user space
        pkt_x86->init(packet_buffer+3, frame_size, NULL, (uint32_t) input_port, (uint32_t) input_port, true, true);
        
        ROFL_DEBUG("[EZ-packet-channel] Received packet from port: %d with length: %d\n", input_port, frame_size);
        ROFL_DEBUG("[EZ-packet-channel] Filled buffer with id:%d. Sending to process.\n", pkt_x86->buffer_id);
        
        return pkt;	
}


unsigned int ez_packet_channel::write(datapacket_t* pkt){

        datapacketx86* pkt_x86;
        uint8_t packet_buffer[4086];
        uint8_t output_port = 1;

        pkt_x86 = (datapacketx86*)pkt->platform_state;

        //Creating metedata header
        packet_buffer[0] = output_port;
        ((uint16_t*)(packet_buffer+1))[0] = htons(pkt_x86->get_buffer_length());
        memcpy(packet_buffer+3, pkt_x86->get_buffer(), pkt_x86->get_buffer_length());

        //Send packet by TCP connection to EZ
        if(::write(ez_packets_socket, packet_buffer, 4086) < 0) {
                ROFL_ERR("[EZ-packet-channel] ERROR writing to socket\n");
        } 

        //Free buffer
        bufferpool::release_buffer(pkt);
        return 0;
}

void ez_packet_channel::put_packet_to_pipeline(datapacket_t* pkt) {
        
        datapacketx86* pkt_x86 = (datapacketx86*)pkt->platform_state;
        datapacket_storage* storage;
        
        storage = ((logical_switch_internals*)logical_switch->platform_state)->storage;
        storeid storage_id = storage->store_packet(pkt);

        __of1x_init_packet_matches(pkt);// tranform packet->matches into of12 matches
        of1x_packet_matches_t* matches = &pkt->matches.of1x;  

        afa_result result = cmm_process_of1x_packet_in(
                (of1x_switch*)logical_switch,
                pkt_x86->pktin_table_id,
                pkt_x86->pktin_reason,
                pkt_x86->in_port,
                storage_id,
                pkt_x86->get_buffer(),
                pkt_x86->get_buffer_length(),
                pkt_x86->get_buffer_length(), 
                *matches);

        if (result == AFA_SUCCESS) 
                ROFL_DEBUG("[EZ-packet-channel] packet_io.cc cmm packet_in successful\n");
        else
                ROFL_DEBUG("[EZ-packet-channel] packet_io.cc cmm packet_in unsuccessful\n");
}


void ez_packet_channel::start() {
        
        datapacket_t* pkt;
        while (ez_continue_execution) {
                if((ez_packets_socket = connect_to_ezproxy_packet_interface()) < 0){
                        close(ez_packets_socket);
                        sleep(10); // retry connect in 10 secunds
                        continue;
                }
               
                while (ez_continue_execution) {
                        try {
                                if ((pkt = read()) != NULL)
                                        put_packet_to_pipeline(pkt);
                        }
                        catch (int e)   {
                                break; //back to connecting state
                        }
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
                ROFL_ERR("[EZ-packet-channel] pthread_create failed, errno(%d): %s\n", errno, strerror(errno));
                return ROFL_FAILURE;
        }
        return ROFL_SUCCESS;
}

rofl_result_t stop_ez_packet_channel() {
        ez_continue_execution = false;
        pthread_join(ez_thread,NULL);
        return ROFL_SUCCESS;
}

void* get_ez_packet_channel() {
        return (void*)ez_packket_channel_instance;
}

void set_lsw_for_ez_packet_channel(of_switch_t* sw) {
        ez_packket_channel_instance->logical_switch = sw;
}