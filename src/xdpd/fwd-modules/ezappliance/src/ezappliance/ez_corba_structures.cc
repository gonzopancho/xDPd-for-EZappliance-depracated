#include <assert.h>

#include <rofl/common/utils/c_logger.h>
#include "ez_corba_structures.h"
#include "ez_corba_client.h"


static uint8_t set_bit_u8(uint8_t bit, uint8_t value=0) {
        
        assert(bit < 8);
        return value | 1 << bit;
}

static uint16_t set_bit_u16(uint8_t bit, uint16_t value=0) {
        
        assert(bit < 16);
        return value | 1 << bit;
}

static void set_mac(uint8_t* mac, uint64_t _mac) {
    
        memcpy(mac, &_mac, 6);
}

//         OF1X_MATCH_IN_PORT,             /* Switch input port. */                //required
//         OF1X_MATCH_IN_PHY_PORT,         /* Switch physical input port. */
//         
//         OF1X_MATCH_METADATA,            /* Metadata passed between tables. */
//         
//         OF1X_MATCH_MPLS_LABEL,          /* MPLS label. */
//         OF1X_MATCH_MPLS_TC,             /* MPLS TC. */
//         OF1X_MATCH_MPLS_BOS,            /* MPLS BoS flag. */
// 
//         OF1X_MATCH_ARP_OP,              /* ARP opcode. */
//         OF1X_MATCH_ARP_SHA,             /* ARP source hardware address. */
//         OF1X_MATCH_ARP_THA,             /* ARP target hardware address. */
//         
//         OF1X_MATCH_IP_DSCP,             /* IP DSCP (6 bits in ToS field). */
//         OF1X_MATCH_IP_ECN,              /* IP ECN (2 bits in ToS field). */
// 
//         OF1X_MATCH_NW_PROTO,            /* OF-1.0 only */
//         OF1X_MATCH_IPV6_SRC,            /* IPv6 source address. */              //required
//         OF1X_MATCH_IPV6_DST,            /* IPv6 destination address. */         //required
//         OF1X_MATCH_IPV6_FLABEL,         /* IPv6 Flow Label */
//         OF1X_MATCH_ICMPV6_TYPE,         /* ICMPv6 type. */
//         OF1X_MATCH_ICMPV6_CODE,         /* ICMPv6 code. */
//         OF1X_MATCH_IPV6_ND_TARGET,      /* Target address for ND. */
//         OF1X_MATCH_IPV6_ND_SLL,         /* Source link-layer for ND. */
//         OF1X_MATCH_IPV6_ND_TLL,         /* Target link-layer for ND. */
//         OF1X_MATCH_IPV6_EXTHDR,         /* Extension header */
// 
//         OF1X_MATCH_SCTP_SRC,            /* SCTP source port. */
//         OF1X_MATCH_SCTP_DST,            /* SCTP destination port. */
//         OF1X_MATCH_ICMPV4_TYPE,         /* ICMP type. */
//         OF1X_MATCH_ICMPV4_CODE,         /* ICMP code. */

rofl_result_t set_ez_struct_key(of1x_flow_entry_t* entry, Proxy_Adapter::EZvalue& _key, Proxy_Adapter::EZvalue& _mask) {
        
        EZFlowTableKey_t key, mask;
        memset(&key, 0, sizeof(key));
        memset(&mask, 0, sizeof(mask));
        
        rofl_result_t key_generated = ROFL_SUCCESS;
        key.priority = (uint16_t)entry->priority;
        
        of1x_match_t* curr_match = entry->matches.head;

        //if (!curr_match) {
        //        ROFL_ERR("No matches present in flowmod\n");
        //       return ROFL_FAILURE;
        //}

        while (curr_match != NULL) {
                ROFL_DEBUG_VERBOSE("\t\t Match type is %d\n", curr_match->type);
                switch (curr_match->type) {
                        //case OF1X_MATCH_IN_PORT:
                        //        // EZ port numbering starts from 0, OF numbering starts from 1
                        //        break;
                        //case OF1X_MATCH_IN_PHY_PORT:
                        //        break;
                        case OF1X_MATCH_ETH_DST:
                                //ROFL_DEBUG("Match: eth dst is present\n");
                                set_mac(key.dst_mac, curr_match->value->value.u64);
                                set_mac(mask.dst_mac, curr_match->value->mask.u64); 
                                key_generated = ROFL_SUCCESS;
                                break;
                        case OF1X_MATCH_ETH_SRC:
                                //ROFL_DEBUG("Match: eth src is present\n");
                                set_mac(key.src_mac, curr_match->value->value.u64);
                                set_mac(mask.src_mac, curr_match->value->mask.u64); 
                                key_generated = ROFL_SUCCESS;
                                break;
                        case OF1X_MATCH_ETH_TYPE:
                                //ROFL_DEBUG("Match: eth type is %d\n", curr_match->value->value.u16);
                                key.ether_type = curr_match->value->value.u16;
                                mask.ether_type = curr_match->value->mask.u16;
                                key_generated = ROFL_SUCCESS;
                                break;
                        case OF1X_MATCH_VLAN_VID:
                                if ((curr_match->value->value.u16 & set_bit_u16(EZ_BIT_VLAN_TAG_FLAG)) == 0) { //handling the second use-case from OF1.3 Table 3 (page 45)
                                        mask.vlan_tag |= set_bit_u16(EZ_BIT_VLAN_TAG_FLAG);
                                } 
                                else { //handling the fourth use-case from OF1.3 Table 3 (page 45)
                                        ROFL_DEBUG("\t\t Match: VLAN flag is set (VLAN present)\n");
                                        key.vlan_tag |= set_bit_u16(EZ_BIT_VLAN_TAG_FLAG);
                                        mask.vlan_tag |= set_bit_u16(EZ_BIT_VLAN_TAG_FLAG);
                                        key.vlan_tag |= curr_match->value->value.u16 & 0xFFF; //FIXME: this should not happen for the third use-case
                                        mask.vlan_tag |= curr_match->value->mask.u16 & 0xFFF; //FIXME: this should not happen for the third use-case
                                }
                                //ROFL_DEBUG("Match: VLAN ID is %d with mask 0x%x\n", curr_match->value->value.u16 & 0xFFF, curr_match->value->mask.u16 & 0xFFF);
                                key_generated = ROFL_SUCCESS;
                                break;
                        case OF1X_MATCH_VLAN_PCP:
                                //ROFL_DEBUG("Match: VLAN PCP is %d with mask 0x%x\n", curr_match->value->value.u8, curr_match->value->mask.u8);
                                key.vlan_tag = curr_match->value->value.u8 << (EZ_BIT_VLAN_TAG_FLAG+1);
                                mask.vlan_tag = curr_match->value->mask.u8 << (EZ_BIT_VLAN_TAG_FLAG+1);
                                key_generated = ROFL_SUCCESS;
                                break;
                         //case OF1X_MATCH_NW_PROTO:
                         //     ROFL_DEBUG("MATCH_NW_PROTO is not supported. This field is ignored - cause not failure in flowmod acceptance\n");
                         //     key_generated = ROFL_SUCCESS;
                         //     break;
                         case OF1X_MATCH_IP_DSCP:
                                ROFL_DEBUG("\t\t MATCH_IP_DSCP is not supported however flowmod is accepted\n");
                                key_generated = ROFL_SUCCESS;
                                break;
                         case OF1X_MATCH_NW_PROTO:  //problematich in case of ARP match
                         case OF1X_MATCH_IP_PROTO:
                                //ROFL_DEBUG("Match: IP protocol is %d with mask 0x%x\n", curr_match->value->value.u8, curr_match->value->mask.u8);
                                key.ip_protocol = curr_match->value->value.u8;
                                mask.ip_protocol = curr_match->value->mask.u8;
                                key_generated = ROFL_SUCCESS;
                                break;  
                         case OF1X_MATCH_IPV4_SRC:
                         case OF1X_MATCH_NW_SRC:
                         case OF1X_MATCH_ARP_SPA:
                                //ROFL_DEBUG("Match: IPv4 source address is %d with mask 0x%x\n", curr_match->value->value.u32, curr_match->value->mask.u32);
                                key.src_ipv4 = curr_match->value->value.u32;
                                mask.src_ipv4 = curr_match->value->mask.u32;
                                key_generated = ROFL_SUCCESS;
                                break;  
                         case OF1X_MATCH_IPV4_DST:
                         case OF1X_MATCH_NW_DST:
                         case OF1X_MATCH_ARP_TPA:
                                //ROFL_DEBUG("Match: IPv4 destination address is %d with mask 0x%x\n", curr_match->value->value.u32, curr_match->value->mask.u32);
                                key.dst_ipv4 = curr_match->value->value.u32;
                                mask.dst_ipv4 = curr_match->value->mask.u32;
                                key_generated = ROFL_SUCCESS;
                                break;     
                         case OF1X_MATCH_TP_SRC:
                         case OF1X_MATCH_TCP_SRC:
                         case OF1X_MATCH_UDP_SRC:
                                //ROFL_DEBUG("Match: TCP/UDP source port is %d with mask 0x%x\n", curr_match->value->value.u16, curr_match->value->mask.u16);
                                key.tp_src_port = curr_match->value->value.u16;
                                mask.tp_src_port = curr_match->value->mask.u16;
                                key_generated = ROFL_SUCCESS;
                                break;     
                         case OF1X_MATCH_TP_DST:
                         case OF1X_MATCH_TCP_DST:
                         case OF1X_MATCH_UDP_DST:
                                //ROFL_DEBUG("Match: TCP/UDP destination port is %d with mask 0x%x\n", curr_match->value->value.u16, curr_match->value->mask.u16);
                                key.tp_dst_port = curr_match->value->value.u16;
                                mask.tp_dst_port = curr_match->value->mask.u16;
                                key_generated = ROFL_SUCCESS;
                                break;                                                 
                        default:
                                ROFL_INFO("Skipping hardware flowmod installation. Some matches are not supported by the EZappliance NP-3 implementation\n");
                                return ROFL_FAILURE;
                };
                curr_match = curr_match->next;
        }
        
        memcpy(_key.get_buffer(), &key, sizeof(key));
        memcpy(_mask.get_buffer(), &mask, sizeof(mask));
        return key_generated;
}

rofl_result_t check_if_match_list_empty(of1x_flow_entry_t* entry) {
        
        of1x_match_t* curr_match = entry->matches.head;

        if (!curr_match) {
                ROFL_DEBUG("\t\t No matches present in flowmod\n");
                return ROFL_SUCCESS;
        }
        ROFL_DEBUG("\t\t Some match entries present in flowmod\n");
        
        while (curr_match) {
                //ROFL_DEBUG("Match type is %d\n", curr_match->type);
                if(curr_match->value->type == UTERN8_T && curr_match->value->value.u8 != 0)
                        return ROFL_FAILURE;
                if(curr_match->value->type == UTERN16_T && curr_match->value->value.u16 != 0)
                        return ROFL_FAILURE;
                if(curr_match->value->type == UTERN32_T && curr_match->value->value.u32 != 0)
                        return ROFL_FAILURE;
                curr_match = curr_match->next;
        }

        return ROFL_SUCCESS;
}

//         //No action. This MUST always exist and the value MUST be 0     
//         OF1X_AT_NO_ACTION= 0,                   /* NO action/EMPTY action. */
// 
//         //Copy TTL inwards
//         OF1X_AT_COPY_TTL_IN,                    /* Copy TTL "inwards" -- from outermost to next-to-outermost */
// 
//         /*
//         * Pop: first VLAN, MPLS, PPPoE, PPP, PBB
//         */
//         OF1X_AT_POP_VLAN,                       /* Pop the outer VLAN tag */
//         OF1X_AT_POP_MPLS,                       /* Pop the outer MPLS tag */
//         OF1X_AT_POP_GTP,                        /* Pop the GTP header */
//         OF1X_AT_POP_PPPOE,                      /* Pop the PPPoE header */
//         OF1X_AT_POP_PBB,                        /* Pop the PBB header */
// 
//         /*
//         * Push: first PBB, PPP, PPPoE, MPLS, VLAN
//         */
//         OF1X_AT_PUSH_PBB,                       /* Push a new PBB header */
//         OF1X_AT_PUSH_PPPOE,                     /* Push a new PPPoE header */
//         OF1X_AT_PUSH_GTP,                       /* Push a new GTP header */
//         OF1X_AT_PUSH_MPLS,                      /* Push a new MPLS tag */
//         OF1X_AT_PUSH_VLAN,                      /* Push a new VLAN tag */
// 
//         //Copy ttl outwards
//         OF1X_AT_COPY_TTL_OUT,                   /* Copy TTL "outwards" -- from next-to-outermost to outermost */
// 
//         //Decrement TTL
//         OF1X_AT_DEC_NW_TTL,                     /* Decrement IP TTL. */
//         OF1X_AT_DEC_MPLS_TTL,                   /* Decrement MPLS TTL */
// 
//         //Set fields
//         OF1X_AT_SET_MPLS_TTL,                   /* MPLS TTL */
//         OF1X_AT_SET_NW_TTL,                     /* IP TTL. */
//         OF1X_AT_SET_QUEUE,                      /* Set queue id when outputting to a port */
// 
//         OF1X_AT_SET_FIELD_ETH_DST,              /* Ethernet destination address. */
//         OF1X_AT_SET_FIELD_ETH_SRC,              /* Ethernet source address. */
//         OF1X_AT_SET_FIELD_ETH_TYPE,             /* Ethernet frame type. */
//                 
//         OF1X_AT_SET_FIELD_MPLS_LABEL,           /* MPLS label. */
//         OF1X_AT_SET_FIELD_MPLS_TC,              /* MPLS TC. */
//         OF1X_AT_SET_FIELD_MPLS_BOS,             /* MPLS BoS flag */
// 
//         OF1X_AT_SET_FIELD_VLAN_VID,             /* VLAN id. */
//         OF1X_AT_SET_FIELD_VLAN_PCP,             /* VLAN priority. */
//         OF1X_AT_SET_FIELD_ARP_OPCODE,           /* ARP opcode */
//         
//         OF1X_AT_SET_FIELD_ARP_SHA,              /* ARP source hardware address */
//         OF1X_AT_SET_FIELD_ARP_SPA,              /* ARP source protocol address */
//         OF1X_AT_SET_FIELD_ARP_THA,              /* ARP target hardware address */
//         OF1X_AT_SET_FIELD_ARP_TPA,              /* ARP target protocol address */
//         
//         /* OF10 only */
//         OF1X_AT_SET_FIELD_NW_PROTO,             /* Network layer proto/arp code */
//         OF1X_AT_SET_FIELD_NW_SRC,               /* Source network address */
//         OF1X_AT_SET_FIELD_NW_DST,               /* Destination network address */
//         /* End of OF10 only */
// 
// 
//         OF1X_AT_SET_FIELD_IP_DSCP,              /* IP DSCP (6 bits in ToS field). */
//         OF1X_AT_SET_FIELD_IP_ECN,               /* IP ECN (2 bits in ToS field). */
//         OF1X_AT_SET_FIELD_IP_PROTO,             /* IP protocol. */
//         
//         OF1X_AT_SET_FIELD_IPV4_SRC,             /* IPv4 source address. */
//         OF1X_AT_SET_FIELD_IPV4_DST,             /* IPv4 destination address. */
//         OF1X_AT_SET_FIELD_IPV6_SRC,             /* IPv6 source address */
//         
//         OF1X_AT_SET_FIELD_IPV6_DST,             /* IPv6 destination address */
//         OF1X_AT_SET_FIELD_IPV6_FLABEL,          /* IPv6 flow label */
//         OF1X_AT_SET_FIELD_IPV6_ND_TARGET,       /* IPv6 Neighbour Discovery target */
//         OF1X_AT_SET_FIELD_IPV6_ND_SLL,          /* IPv6 ND source link level */
//         OF1X_AT_SET_FIELD_IPV6_ND_TLL,          /* IPv6 ND target link level */
//         OF1X_AT_SET_FIELD_IPV6_EXTHDR,          /* IPv6 Extension pseudo header */
//         
//         OF1X_AT_SET_FIELD_TCP_SRC,              /* TCP source port. */
//         OF1X_AT_SET_FIELD_TCP_DST,              /* TCP destination port. */
//         
//         OF1X_AT_SET_FIELD_UDP_SRC,              /* UDP source port. */
//         OF1X_AT_SET_FIELD_UDP_DST,              /* UDP destination port. */
//         
//         OF1X_AT_SET_FIELD_SCTP_SRC,             /* SCTP source port. */
//         OF1X_AT_SET_FIELD_SCTP_DST,             /* SCTP destination port. */
//         
//         /* OF10 only */
//         OF1X_AT_SET_FIELD_TP_SRC,               /* Trans. protocol (TCP/UDP) src port */
//         OF1X_AT_SET_FIELD_TP_DST,               /* Trans. protocol (TCP/UDP) dst port */
//         /* End of OF10 only */
// 
//         OF1X_AT_SET_FIELD_ICMPV4_TYPE,          /* ICMP type. */
//         OF1X_AT_SET_FIELD_ICMPV4_CODE,          /* ICMP code. */
//         
//         OF1X_AT_SET_FIELD_ICMPV6_TYPE,          /* ICMPv6 type */
//         OF1X_AT_SET_FIELD_ICMPV6_CODE,          /* ICMPv6 code */
//         
//         OF1X_AT_SET_FIELD_PBB_ISID,             /* PBB ISID field */
//         OF1X_AT_SET_FIELD_TUNNEL_ID,            /* Tunnel id */
//         
//         /*
//         * Extensions
//         */
// 
//         OF1X_AT_SET_FIELD_PPPOE_CODE,           /* PPPoE code */
//         OF1X_AT_SET_FIELD_PPPOE_TYPE,           /* PPPoE type */
//         OF1X_AT_SET_FIELD_PPPOE_SID,            /* PPPoE session id */
//         OF1X_AT_SET_FIELD_PPP_PROT,             /* PPP protocol */
//     
//         OF1X_AT_SET_FIELD_GTP_MSG_TYPE,         /* GTP message type */
//         OF1X_AT_SET_FIELD_GTP_TEID,             /* GTP TEID */
// 
//         /* Add more set fields here... */
// 
//         //Groups                
//         OF1X_AT_GROUP,                          /* Apply group. */
// 
//         //Experimenter
//         OF1X_AT_EXPERIMENTER,   
// 
//         OF1X_AT_OUTPUT,                         /* Output to switch port. */

rofl_result_t set_ez_struct_result(of1x_flow_entry_t* entry, Proxy_Adapter::EZvalue& _result) {
        
        EZFlowTableResult_t result;
        memset(&result, 0, sizeof(result));
        rofl_result_t result_generated = ROFL_FAILURE;
        
        result.control = set_bit_u8(EZ_BIT_CONTROL_VALID) | set_bit_u8(EZ_BIT_CONTROL_MATCH);
        
        of1x_packet_action_t* action;
  
        //If entry has not actions we are done (should we really install it down there?)
        if(!entry->inst_grp.instructions[OF1X_IT_APPLY_ACTIONS-1].apply_actions)
                return ROFL_FAILURE;

        action = entry->inst_grp.instructions[OF1X_IT_APPLY_ACTIONS-1].apply_actions->head;
        
        if(!action){
                ROFL_DEBUG("\t\t EZ action: action list empty\n");
                result.actions = set_bit_u8(EZ_BIT_DROP);
                result_generated = ROFL_SUCCESS;
        }

        //Loop over apply actions only
        for(; action; action = action->next) {
         
                switch(action->type){

                        case OF1X_AT_SET_FIELD_ETH_DST: 
                                break;
                        case OF1X_AT_SET_FIELD_ETH_SRC: 
                                break;
                        case OF1X_AT_SET_FIELD_VLAN_VID:      
                                break;
                        case OF1X_AT_SET_FIELD_VLAN_PCP:      
                                break;
                        case OF1X_AT_SET_FIELD_IP_DSCP:
                                break;
                        case OF1X_AT_SET_FIELD_IP_ECN:  
                                break;
                        case OF1X_AT_SET_FIELD_IPV4_SRC: 
                                break;
                        case OF1X_AT_SET_FIELD_IPV4_DST:
                                break;
                        case OF1X_AT_SET_FIELD_TCP_SRC: 
                                break;
                        case OF1X_AT_SET_FIELD_TCP_DST:
                                break;
                        case OF1X_AT_SET_FIELD_UDP_SRC:
                                break;
                        case OF1X_AT_SET_FIELD_UDP_DST:
                                break;
                        case OF1X_AT_OUTPUT:
                                if (action->field.u64 == (uint64_t)OF1X_PORT_ALL) {
                                        result.actions = set_bit_u8(EZ_BIT_FORWARD_ALL_PORTS);
                                        ROFL_DEBUG("\t\t EZ action: forward to all ports\n");
                                }
                                else {
                                        result.actions = set_bit_u8(EZ_BIT_FORWARD_SINGLE_PORT);
                                        result.port_number = (uint8_t)action->field.u64-1; // EZ port numbering starts from 0, OF numbering starts from 1
                                        ROFL_DEBUG("\t\t EZ action: forward to port %d\n", result.port_number);
                                }
                                result_generated = ROFL_SUCCESS;
                                break;
                        case OF1X_AT_NO_ACTION:
                                ROFL_DEBUG("\t\t EZ action: drop\n");
                                result.actions = set_bit_u8(EZ_BIT_DROP);
                                result_generated = ROFL_SUCCESS;
                                break;
                        default:
                                break;
                }
        }
        memcpy(_result.get_buffer(), &result, sizeof(result));
        return result_generated;
}


void set_ez_flow_entry(of1x_flow_entry_t* entry) {
        
        CORBA::Octet temp1[EZ_FLOWTABLE_KEY_SIZE], temp2[EZ_FLOWTABLE_RESULT_SIZE], temp3[EZ_FLOWTABLE_KEY_SIZE];
        Proxy_Adapter::EZvalue key(EZ_FLOWTABLE_KEY_SIZE, EZ_FLOWTABLE_KEY_SIZE, temp1), 
                               result(EZ_FLOWTABLE_RESULT_SIZE, EZ_FLOWTABLE_RESULT_SIZE, temp2),
                               mask(EZ_FLOWTABLE_KEY_SIZE, EZ_FLOWTABLE_KEY_SIZE, temp3); 
        
        if (set_ez_struct_key(entry, key, mask) == ROFL_SUCCESS 
           && set_ez_struct_result(entry, result) == ROFL_SUCCESS) {

                set_ez_struct(Proxy_Adapter::EzapiSearch1, EZ_FLOWTABLE_STRUCTURE_NUM, 
                              EZ_FLOWTABLE_KEY_SIZE, EZ_FLOWTABLE_RESULT_SIZE, key, result, mask);
        }
}


void del_ez_flow_entry(of1x_flow_entry_t* entry) {

        CORBA::Octet temp1[EZ_FLOWTABLE_KEY_SIZE], temp2[EZ_FLOWTABLE_RESULT_SIZE], temp3[EZ_FLOWTABLE_KEY_SIZE];
        Proxy_Adapter::EZvalue key(EZ_FLOWTABLE_KEY_SIZE, EZ_FLOWTABLE_KEY_SIZE, temp1), 
                               result(EZ_FLOWTABLE_RESULT_SIZE, EZ_FLOWTABLE_RESULT_SIZE, temp2),
                               mask(EZ_FLOWTABLE_KEY_SIZE, EZ_FLOWTABLE_KEY_SIZE, temp3); 
        
        if (set_ez_struct_key(entry, key, mask) == ROFL_SUCCESS)
                del_ez_struct(Proxy_Adapter::EzapiSearch1, EZ_FLOWTABLE_STRUCTURE_NUM, 
                              EZ_FLOWTABLE_KEY_SIZE, EZ_FLOWTABLE_RESULT_SIZE, key, result, mask);
}
       


void show_ez_flow_entries() {
        
        CORBA::Octet temp1[EZ_FLOWTABLE_KEY_SIZE], temp2[EZ_FLOWTABLE_RESULT_SIZE], temp3[EZ_FLOWTABLE_KEY_SIZE];
        Proxy_Adapter::EZvalue key(EZ_FLOWTABLE_KEY_SIZE, EZ_FLOWTABLE_KEY_SIZE, temp1), 
                               result(EZ_FLOWTABLE_RESULT_SIZE, EZ_FLOWTABLE_RESULT_SIZE, temp2),
                               mask(EZ_FLOWTABLE_KEY_SIZE, EZ_FLOWTABLE_KEY_SIZE, temp3);
        uint32_t k_length, r_length; 
        
        uint32_t length = get_ez_struct_length(Proxy_Adapter::EzapiSearch1, EZ_FLOWTABLE_STRUCTURE_NUM);
       
        for (uint32_t index=0; index<length; index++) {
              get_ez_struct(Proxy_Adapter::EzapiSearch1, EZ_FLOWTABLE_STRUCTURE_NUM, index, 
                            k_length, r_length, key, result, mask);
        }

}

void del_all_ez_flow_entries() {
        ROFL_DEBUG("\t\t Deleting all flow entries in EZ\n");
        del_all_ez_struct_entries(Proxy_Adapter::EzapiSearch1, EZ_FLOWTABLE_STRUCTURE_NUM);

}