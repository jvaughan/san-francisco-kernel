/* ip6tables module for matching the Hop Limit value
 * Maciej Soltysiak <solt@dns.toxicfilms.tv>
 * Based on HW's ttl module */

#ifndef _IP6T_HL_H
#define _IP6T_HL_H

enum {
	IP6T_HL_EQ = 0,	 
	IP6T_HL_NE,	 
	IP6T_HL_LT,		 
	IP6T_HL_GT,	 
};


struct ip6t_hl_info {
	u_int8_t	mode;
	u_int8_t	hop_limit;
};


#endif
