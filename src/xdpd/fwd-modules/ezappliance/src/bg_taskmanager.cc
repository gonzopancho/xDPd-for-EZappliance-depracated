#include "bg_taskmanager.h"

#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/rtnetlink.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <algorithm>
#include <rofl/common/utils/c_logger.h>
#include <rofl/datapath/pipeline/physical_switch.h>
#include <rofl/datapath/afa/fwd_module.h>
#include <rofl/datapath/afa/cmm.h>
#include "ls_internal_state.h"
#include "io/bufferpool.h"
#include "io/datapacket_storage.h"
#include "io/pktin_dispatcher.h"
#include "io/iface_utils.h"
#include "util/time_utils.h"
#include "ezappliance/ez_corba_structures.h"

using namespace xdpd::gnu_linux;

//Local static variable for background manager thread
static pthread_t bg_thread;
static bool bg_continue_execution = true;

/**
 * This piece of code is meant to manage a thread that does:
 * 
 * - the expiration of the flow entries.
 * - the update the status of the ports
 * - purge old buffers in the buffer storage of a logical switch(pkt-in) 
 * - more?
 */

/**
 * @name process_timeouts
 * @brief checks if its time to process timeouts (flow entries and pool of buffers)
 * @param psw physical switch (where all the logical switches are)
 */
int process_timeouts()
{
    datapacket_t* pkt;
    unsigned int i, max_switches;
    struct timeval now;
    of_switch_t** logical_switches;
    static struct timeval last_time_entries_checked={0,0}, last_time_pool_checked={0,0};
    gettimeofday(&now,NULL);

    //Retrieve the logical switches list
    logical_switches = physical_switch_get_logical_switches(&max_switches);
    
    if(get_time_difference_ms(&now, &last_time_entries_checked)>=LSW_TIMER_SLOT_MS)
    {
#ifdef DEBUG
        static int dummy = 0;
#endif

        //TIMERS FLOW ENTRIES
        for(i=0; i<max_switches; i++)
        {

            if(logical_switches[i] != NULL){
                of_process_pipeline_tables_timeout_expirations(logical_switches[i]);
                
#ifdef DEBUG
                if(dummy%20 == 0)
                    of1x_full_dump_switch((of1x_switch_t*)logical_switches[i]);
#endif
            }
        }
            
#ifdef DEBUG
        dummy++;
        //ROFL_DEBUG_VERBOSE("Checking flow entries expirations %lu:%lu\n",now.tv_sec,now.tv_usec);
#endif
        last_time_entries_checked = now;
    }
    
    if(get_time_difference_ms(&now, &last_time_pool_checked)>=LSW_TIMER_BUFFER_POOL_MS){
        uint32_t buffer_id;
        datapacket_storage* dps=NULL;
        
        for(i=0; i<max_switches; i++){

            if(logical_switches[i] != NULL){

                //Recover storage pointer
                dps =( (struct logical_switch_internals*) logical_switches[i]->platform_state)->storage;
                //Loop until the oldest expired packet is taken out
                while(dps->oldest_packet_needs_expiration(&buffer_id)){

                    ROFL_DEBUG_VERBOSE("Trying to erase a datapacket from storage: %u\n", buffer_id);

                    if( (pkt = dps->get_packet(buffer_id) ) == NULL ){
                        ROFL_DEBUG_VERBOSE("Error in get_packet_wrapper %u\n", buffer_id);
                    }else{
                        ROFL_DEBUG_VERBOSE("Datapacket expired correctly %u\n", buffer_id);
                        //Return buffer to bufferpool
                        bufferpool::release_buffer(pkt);
                    }
                }
            }
        }
        
#ifdef DEBUG
        //ROFL_ERR("Checking pool buffers expirations %lu:%lu\n",now.tv_sec,now.tv_usec);
#endif
        last_time_pool_checked = now;
    }
    
    return ROFL_SUCCESS;
}

static void check_ports_statuses()
{
    struct timeval now;
    static struct timeval last_time_checked={0,0};
    gettimeofday(&now,NULL);
    
    if(get_time_difference_ms(&now, &last_time_checked) >= 60000) {
        
        last_time_checked = now;
        //update_ports_statuses(); // TODO: uncomment
    }

}


static void check_np3_flowtable()
{
    struct timeval now;
    static struct timeval last_time_checked={0,0};
    gettimeofday(&now,NULL);
    
    if(get_time_difference_ms(&now, &last_time_checked) >= 180000) {
        
        last_time_checked = now;
        show_ez_flow_entries();
    }

}

/**
 * @name x86_background_tasks_thread
 * @brief contents the infinite loop checking for ports and timeouts
 */
void* x86_background_tasks_routine(void* param)
{
    int i, efd,nfds;
    struct epoll_event event_list[MAX_EPOLL_EVENTS], epe_port;

    // program an epoll that listents to the file descriptors of the ports with a
    // timeout that makes us check 
    
    memset(event_list,0,sizeof(event_list));
    memset(&epe_port,0,sizeof(epe_port));
    
    efd = epoll_create1(0);

    if(efd == -1){
        ROFL_ERR("Error in epoll_create1, errno(%d): %s\n", errno, strerror(errno) );
        return NULL;
    }
    
    //Add PKT_IN
    init_packetin_pipe();

    epe_port.data.fd = get_packet_in_read_fd();
    epe_port.events = EPOLLIN | EPOLLET;
    
    if(epoll_ctl(efd,EPOLL_CTL_ADD, epe_port.data.fd, &epe_port)==-1){
        ROFL_ERR("Error in epoll_ctl, errno(%d): %s\n", errno, strerror(errno));
        return NULL;
    }
    
    while(bg_continue_execution){
        
        //Throttle
        nfds = epoll_wait(efd, event_list, MAX_EPOLL_EVENTS, LSW_TIMER_SLOT_MS/*timeout needs TBD somewhere else*/);
        

        if(nfds==-1){
            //ROFL_DEBUG("Epoll Failed\n");
            continue;
        }

        //Check for events
        for(i=0;i<nfds;i++){

            if( (event_list[i].events & EPOLLERR) || (event_list[i].events & EPOLLHUP)/*||(event_list[i].events & EPOLLIN)*/){
                //error on this fd
                //ROFL_ERR("Error in file descriptor\n");
                close(event_list[i].data.fd); //fd gets removed automatically from efd's
                continue;
            }else{
                //Is netlink or packet-in subsystem
                if(get_packet_in_read_fd() == event_list[i].data.fd){
                    //PKT_IN
                    process_packet_ins();
                }
            }
        }
        
        //check timers expiration 
        process_timeouts();
     
        check_ports_statuses();

        check_np3_flowtable();
    }

    //Cleanup packet-in
    destroy_packetin_pipe();
    
    //Cleanup epoll fd
    close(efd);
    
    //Printing some information
    ROFL_DEBUG("[bg] Finishing thread execution\n"); 

    //Exit
    pthread_exit(NULL); 
}

/**
 * launches the main thread
 */
rofl_result_t launch_background_tasks_manager()
{
    //Set flag
    bg_continue_execution = true;

    if(pthread_create(&bg_thread, NULL, x86_background_tasks_routine,NULL)<0){
        ROFL_ERR("pthread_create failed, errno(%d): %s\n", errno, strerror(errno));
        return ROFL_FAILURE;
    }
    return ROFL_SUCCESS;
}

rofl_result_t stop_background_tasks_manager()
{
    bg_continue_execution = false;
    pthread_join(bg_thread,NULL);
    return ROFL_SUCCESS;
}
