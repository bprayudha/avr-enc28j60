#include <avr/io.h>
#include <avr/iom168.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/network.h"
#include "../lib/enc28j60.h"
#include "../conf/config.h"

static uint8_t mymac[6] = {0x62,0x5F,0x70,0x72,0x61,0x79};
static uint8_t myip[4] = {192,168,0,254};
static uint16_t mywwwport = 80;

#define BUFFER_SIZE 900
uint8_t buf[BUFFER_SIZE+1],browser;
uint16_t plen; 

void testpage(void) {
    plen=make_tcp_data_pos(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
    plen=make_tcp_data_pos(buf,plen,PSTR("<html><body><h1>It Works!</h1></body></html>"));
}

void test404(void) {
    plen=make_tcp_data_pos(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
    plen=make_tcp_data_pos(buf,plen,PSTR("<html><body><h1>404</h1></body></html>"));
}

void sendpage(void) {
    tcp_ack(buf);
    tcp_ack_with_data(buf,plen);
}

int main(void) {
    uint16_t dat_p;
    CLKPR = (1<<CLKPCE);
    CLKPR = 0;
    _delay_loop_1(50); 
    ENC28J60_Init(mymac);
    ENC28J60_ClkOut(2);
    _delay_loop_1(50);
    ENC28J60_PhyWrite(PHLCON,0x0476);
    _delay_loop_1(50);
    init_network(mymac,myip,mywwwport);
    while(1) {
        plen = ENC28J60_PacketReceive(BUFFER_SIZE,buf);
        if(plen==0) continue;
        if(eth_is_arp(buf,plen)) {
            arp_reply(buf);
            continue;
        }
        if(eth_is_ip(buf,plen)==0) continue;
        if(buf[IP_PROTO]==IP_ICMP && buf[ICMP_TYPE]==ICMP_REQUEST) {
            icmp_reply(buf,plen);
            continue;
        }
        if(buf[IP_PROTO]==IP_TCP && buf[TCP_DST_PORT]==0 && buf[TCP_DST_PORT+1]==mywwwport) {
            if(buf[TCP_FLAGS] & TCP_SYN) {
                tcp_synack(buf);
                continue;
            }
            if(buf[TCP_FLAGS] & TCP_ACK) {
                init_len_info(buf);
                dat_p = get_tcp_data_ptr();
                if(dat_p==0) {
                    if(buf[TCP_FLAGS] & TCP_FIN) tcp_ack(buf);
                    continue;
                }
				
                if(strstr((char*)&(buf[dat_p]),"User Agent")) browser=0;
                else if(strstr((char*)&(buf[dat_p]),"MSIE")) browser=1;
                else browser=2;
				
                if(strncmp("/ ",(char*)&(buf[dat_p+4]),2)==0){
                    testpage();
                    sendpage();
                    continue;
                }
                else {
                    test404();
                    sendpage();
                    continue;
                }
            }
        }    
    }
    return 0;
}
