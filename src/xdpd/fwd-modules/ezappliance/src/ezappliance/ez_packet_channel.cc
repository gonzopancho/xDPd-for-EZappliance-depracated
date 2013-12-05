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
#include <time.h>
#include <rofl/datapath/pipeline/switch_port.h>
#include <rofl/common/utils/c_logger.h>
#include <rofl/datapath/pipeline/openflow/openflow1x/pipeline/of1x_pipeline.h>
#include <rofl/datapath/afa/openflow/openflow1x/of1x_cmm.h>
#include "../io/datapacket_storage.h"
#include "../io/bufferpool.h" 
#include "../ls_internal_state.h"
#include "../config.h"

using namespace xdpd::gnu_linux;


//Local static variable for ez_packet_channel thread
static pthread_t ez_thread;
static bool ez_continue_execution = true;
static ez_packet_channel* ez_packet_channel_instance = NULL;

//Constructor and destructor
ez_packet_channel::ez_packet_channel() {
}

ez_packet_channel::~ez_packet_channel() {
        
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

        if (sockfd < 0) {
                ROFL_ERR("[EZ-packet-channel] Couldn't open SOCK_STREAM socket, errno(%d): %s\n", errno, strerror(errno));
                return -1;
        }

        dest_addr.sin_family = AF_INET; // host byte order
        dest_addr.sin_port = htons(EZ_PORT); // destination port
        dest_addr.sin_addr.s_addr = inet_addr(EZ_IP); // destination address
        memset(&(dest_addr.sin_zero), '\0', 8); 

        /* Now connect to the server */
        if (connect(sockfd,(struct sockaddr*) & dest_addr, sizeof(struct sockaddr)) < 0) {
                ROFL_ERR("[EZ-packet-channel] Couldn't connect to %s:%d, errno(%d): %s\n", EZ_IP, EZ_PORT, errno, strerror(errno));
                return -1;
        }   
        ROFL_INFO("[EZ-packet-channel] Connected to %s:%d\n", EZ_IP, EZ_PORT);

        char msg[] = "Testing message to server!";

        if(::write(sockfd, msg, strlen(msg)) < 0) { //returns bytes sent
                ROFL_ERR("[EZ-packet-channel] ERROR writing to socket\n");
                
        } 
        return sockfd;
}

datapacket_t* ez_packet_channel::read() {
        
        datapacket_t* pkt; 
        datapacketx86* pkt_x86;
        uint8_t packet_buffer[4086];
        uint8_t input_port;
        uint32_t frame_size;	    

        //Allocate free buffer	
        pkt = bufferpool::get_free_buffer();
        pkt_x86 = ((datapacketx86*)pkt->platform_state);

        //Copy something from TCP socket to buffer
        int n=0;
        if((n = ::read(ez_packets_socket,packet_buffer,4086)) < 0) { 
                //ROFL_DEBUG_VERBOSE("[EZ-packet-channel] Error: %s\n", strerror(errno));
                bufferpool::release_buffer(pkt);
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                       // ROFL_DEBUG_VERBOSE("[EZ-packet-channel] socket read timeout\n");
                }
                return NULL;
        }
        if (n==0) {
                ROFL_ERR("[EZ-packet-channel] EZ-Proxy Server no longer online!\n");
                bufferpool::release_buffer(pkt);
                sleep(10);
                throw 20; // handled upper will break the read loop
                return NULL;
        }

        //Parsing metedata header
        input_port = packet_buffer[0];
        frame_size = ntohs(((uint16_t*)(packet_buffer+1))[0]);

        //Init in user space
        pkt_x86->init(packet_buffer+3, frame_size, NULL, (uint32_t)input_port);
        
        ROFL_DEBUG("[EZ-packet-channel] Received packet from port: %d with length: %d\n", input_port, frame_size);
        
        return pkt;	
}


rofl_result_t ez_packet_channel::write(datapacket_t* pkt, uint8_t output_port) {

        datapacketx86* pkt_x86;
        uint8_t packet_buffer[4086];

        pkt_x86 = (datapacketx86*)pkt->platform_state;

        //Creating metedata header
        packet_buffer[0] = output_port;
        ((uint16_t*)(packet_buffer+1))[0] = htons(pkt_x86->get_buffer_length());
        memcpy(packet_buffer+3, pkt_x86->get_buffer(), pkt_x86->get_buffer_length());

        //Send packet by TCP connection to EZ
        if(::write(ez_packets_socket, packet_buffer, pkt_x86->get_buffer_length()) < 0) {
                ROFL_ERR("[EZ-packet-channel] ERROR writing to socket\n");
                return ROFL_FAILURE;
                
        } 
        ROFL_DEBUG("Packet [%p] was sent to EZ for output port: %d \n", pkt, output_port);
        
        //Free buffer
        bufferpool::release_buffer(pkt);
        return ROFL_SUCCESS;
}

void ez_packet_channel::put_packet_to_pipeline(datapacket_t* pkt) {
        
        datapacketx86* pkt_x86 = (datapacketx86*)pkt->platform_state;
        datapacket_storage* storage =( (logical_switch_internals*)logical_switch->platform_state)->storage;
        
        // TODO: check if port enabled (port->platform_port_state != NULL)
        
        storeid storage_id = storage->store_packet(pkt);

        __of1x_init_packet_matches(pkt); 

        if(cmm_process_of1x_packet_in(
                        (of1x_switch*)logical_switch,
                        pkt_x86->pktin_table_id,
                        pkt_x86->pktin_reason,
                        pkt_x86->in_port,
                        storage_id,
                        pkt_x86->get_buffer(),
                        pkt_x86->get_buffer_length(),
                        pkt_x86->get_buffer_length(), 
                        pkt->matches.of1x) == AFA_FAILURE)
                ROFL_ERR("[EZ-packet-channel] Sending a frame to pipeline unsuccessful\n");
        else
                ROFL_DEBUG("[EZ-packet-channel] Sending a frame to pipeline successful\n");
}


void ez_packet_channel::start() {
        
        datapacket_t* pkt;
        while (ez_continue_execution) {
                if((ez_packets_socket = connect_to_ezproxy_packet_interface()) < 0) {
                        close(ez_packets_socket);
                        sleep(10); // retry connect in 10 seconds
                        continue;
                }
               
                while (ez_continue_execution) {
                        try {
                                if ((pkt = read()) != NULL)
                                        put_packet_to_pipeline(pkt);
                        }
                        catch (int e) {
                                if (e == 20)
                                        break; //back to connecting state
                                else
                                        ROFL_ERR("Exception code is %d\n", e);
                        }
                }
        }
}

static void* ez_packet_channel_routine(void* param) {
        
        ez_packet_channel* channel = (ez_packet_channel*) param;
        channel->start();
        
        delete channel;
        pthread_exit(NULL);
}

/**
* launches the thread
*/
rofl_result_t launch_ez_packet_channel() {
        
        //Set flag
        ez_continue_execution = true;

        ez_packet_channel_instance = new ez_packet_channel();
        if(pthread_create(&ez_thread, NULL, ez_packet_channel_routine, ez_packet_channel_instance)<0){
                ROFL_ERR("[EZ-packet-channel] pthread_create failed, errno(%d): %s\n", errno, strerror(errno));
                return ROFL_FAILURE;
        }
        return ROFL_SUCCESS;
}

/**
* stops the thread
*/
rofl_result_t stop_ez_packet_channel() {
        
        ez_continue_execution = false;
        pthread_join(ez_thread,NULL);
        return ROFL_SUCCESS;
}

void* get_ez_packet_channel() {
        
        return (void*)ez_packet_channel_instance;
}

void set_lsw_for_ez_packet_channel(of_switch_t* sw) {
        
	ez_packet_channel_instance->logical_switch = sw;
}

rofl_result_t send_packet_via_ez_packet_channel(datapacket_t* pkt, uint32_t output_port) {
        
        return ez_packet_channel_instance->write(pkt, (uint8_t)output_port);
}
