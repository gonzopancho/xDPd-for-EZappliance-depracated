#include "ioport_ez_packet_channel.h"
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rofl/common/utils/c_logger.h>
#include "../../bufferpool.h" 
#include <fcntl.h>
#include <string.h>

using namespace xdpd::gnu_linux;

//Constructor and destructor
ioport_ez_packet_channel::ioport_ez_packet_channel(switch_port_t* of_ps, unsigned int num_queues):ioport(of_ps,num_queues){
	
	int ret,flags,i;
    
    ez_packets_socket = connect_to_ezproxy_packet_interface();
		
	//Open pipe for output signaling on enqueue	
	ret = pipe(notify_pipe);
	(void)ret; // todo use the value

	//Set non-blocking read/write in the pipe
	for(i=0;i<2;i++){
		flags = fcntl(notify_pipe[i], F_GETFL, 0);	///get current file status flags
		flags |= O_NONBLOCK;				//turn off blocking flag
		fcntl(notify_pipe[i], F_SETFL, flags);		//set up non-blocking read
	}

}

ioport_ez_packet_channel::~ioport_ez_packet_channel(){
	close(ez_packets_socket);
	close(notify_pipe[READ]);
	close(notify_pipe[WRITE]);
}

/**
 * @name connect_to_ezproxy_packet_interface
 * @brief creates TCP connection for packet-in and packet-out operations between NP-3 and xdpd
 */
int ioport_ez_packet_channel::connect_to_ezproxy_packet_interface() {
    int sockfd;
    struct sockaddr_in dest_addr;
    //char buffer[4096];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
    
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

//Read and write methods over port
void ioport_ez_packet_channel::enqueue_packet(datapacket_t* pkt, unsigned int q_id){

	size_t ret;
	//Whatever
	const char c='a';

	//Put in the queue
	output_queues[q_id].blocking_write(pkt);

	//TODO: make it happen only if thread is really sleeping...
	ret = ::write(notify_pipe[WRITE],&c,sizeof(c));
	(void)ret; // todo use the value

}

datapacket_t* ioport_ez_packet_channel::read(){
	
	datapacket_t* pkt; 
	datapacketx86* pkt_x86;
    uint8_t packet_buffer[4086];
    uint8_t input_port;
    uint32_t frame_size;


	//First attempt drain local buffers from previous reads that failed to push 
	pkt = input_queue.non_blocking_read();	
	if(pkt)
		return pkt;		

	//Allocate free buffer	
	pkt = bufferpool::get_free_buffer();
	pkt_x86 = ((datapacketx86*)pkt->platform_state);

	//Init in user space
	pkt_x86->init(NULL, SIMULATED_PKT_SIZE, of_port_state->attached_sw, of_port_state->of_port_num);
	
	//Copy something from TCP socket to buffer, TODO: read packet metatada header containing input port and packet size
    int n=0;
	if((n = ::read(ez_packets_socket,packet_buffer,SIMULATED_PKT_SIZE)) < 0){    
		bufferpool::release_buffer(pkt);
		return NULL;
	}
    
     //Parsing metedata header
	ROFL_DEBUG("Received message from EZ with length: %d\n", n);
	input_port = packet_buffer[0];
	frame_size = ntohs(((uint16_t*)(packet_buffer+1))[0]);
    memcpy(pkt_x86->get_buffer(), packet_buffer+3, frame_size);
    
    ROFL_DEBUG("Received packet from port: %d with length: %d\n", input_port, frame_size);

	ROFL_DEBUG("Filled buffer with id:%d. Sending to process.\n", pkt_x86->buffer_id);
	
	return pkt;	
}


unsigned int ioport_ez_packet_channel::write(unsigned int q_id, unsigned int num_of_buckets){

	size_t ret;
	uint8_t dummy[SIMULATED_PKT_SIZE];
	unsigned int i;
	datapacket_t* pkt;
	datapacketx86* pkt_x86;
    uint8_t packet_buffer[4086];
    uint8_t output_port = 1;
	
	(void)pkt_x86;

	//Free the pipe
	ret = ::read(notify_pipe[READ],&dummy,SIMULATED_PKT_SIZE);
	(void)ret; // todo use the value

	//Go and do stuff
	for(i=0;i<num_of_buckets;i++){	
		//Pick buffer	
		pkt = output_queues[q_id].non_blocking_read();
		
		if(!pkt)
			break;
		
		pkt_x86 = (datapacketx86*)pkt->platform_state;
        
        //Creating metedata header
        packet_buffer[0] = output_port;
        ((uint16_t*)(packet_buffer+1))[0] = htons(pkt_x86->get_buffer_length());
        memcpy(packet_buffer+3, pkt_x86->get_buffer(), pkt_x86->get_buffer_length());
        
        //Send packet by TCP connection to EZ
        if(::write(ez_packets_socket, packet_buffer, SIMULATED_PKT_SIZE) < 0) {
            ROFL_ERR("ERROR writing to socket");
        } 
		
		//Free buffer
		bufferpool::release_buffer(pkt);
	}
	i++;
	return num_of_buckets-i;

}

rofl_result_t ioport_ez_packet_channel::disable(){
	return ROFL_SUCCESS;
}

rofl_result_t ioport_ez_packet_channel::enable(){
	return ROFL_SUCCESS;
}
