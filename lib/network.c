#include <avr/io.h>
#include <avr/pgmspace.h>
#include "network.h"
#include "enc28j60.h"

static uint8_t macaddr[6];
static uint8_t ipaddr[4];
static uint16_t wwwport;
static int16_t info_hdr_len = 0x0000;
static int16_t info_data_len = 0x0000;
static uint8_t seqnum = 0x0A;

void init_network(uint8_t *mymac,uint8_t *myip,uint16_t mywwwport){
    uint8_t i;
    wwwport = mywwwport;
    for(i=0;i<4;i++) ipaddr[i]=myip[i];
    for(i=0;i<6;i++) macaddr[i]=mymac[i];
}

uint16_t checksum(uint8_t *buf, uint16_t len,uint8_t type){
    uint32_t sum = 0;
    if(type==1) {
        sum += IP_UDP;
        sum += len-8;
    }
    if(type==2){
        sum+=IP_TCP; 
        sum+=len-8;
    }
    while(len>1){
        sum += 0xFFFF & (((uint32_t)*buf<<8)|*(buf+1));
        buf+=2;
        len-=2;
    }
    if(len) sum += ((uint32_t)(0xFF & *buf))<<8;
    while(sum>>16) sum = (sum & 0xFFFF)+(sum >> 16);
    return( (uint16_t) sum ^ 0xFFFF);
}

uint8_t eth_is_arp(uint8_t *buf,uint16_t len){
    uint8_t i=0;
    if (len<41) return(0);
    if(buf[ETH_TYPE] != ETH_ARP_H) return(0);
    if(buf[ETH_TYPE+1] != ETH_ARP_L) return(0);
    for(i=0;i<4;i++) if(buf[ARP_DST_IP+i] != ipaddr[i]) return(0);
    return(1);
}

uint8_t eth_is_ip(uint8_t *buf,uint16_t len){
    uint8_t i=0;
    if(len<42) return(0);
    if(buf[ETH_TYPE] != ETH_IP_H) return(0);
    if(buf[ETH_TYPE+1] != ETH_IP_L) return(0);
    if(buf[IP_VERSION] != 0x45) return(0);
    for(i=0;i<4;i++) if(buf[IP_DST+i]!=ipaddr[i]) return(0);
    return(1);
}

void make_eth_hdr(uint8_t *buf) {
    uint8_t i=0;
    for(i=0;i<6;i++) {
        buf[ETH_DST_MAC+i]=buf[ETH_SRC_MAC+i];
        buf[ETH_SRC_MAC+i]=macaddr[i];
    }
}

void make_ip_checksum(uint8_t *buf) {
    uint16_t ck;
    buf[IP_CHECKSUM]=0;
    buf[IP_CHECKSUM+1]=0;
    buf[IP_FLAGS]=0x40;
    buf[IP_FLAGS+1]=0;
    buf[IP_TTL]=64;
    ck = checksum(&buf[0x0E],IP_HEADER_LEN,0);
    buf[IP_CHECKSUM] = ck >> 8;
    buf[IP_CHECKSUM+1] = ck & 0xFF;
}

void make_ip_hdr(uint8_t *buf) {
    uint8_t i=0;
    while(i<4){
        buf[IP_DST+i]=buf[IP_SRC+i];
        buf[IP_SRC+i]=ipaddr[i];
        i++;
    }
    make_ip_checksum(buf);
}

void make_tcp_hdr(uint8_t *buf,uint16_t rel_ack_num,uint8_t mss,uint8_t cp_seq) {
    uint8_t i=4;
    uint8_t tseq;
    buf[TCP_DST_PORT] = buf[TCP_SRC_PORT];
    buf[TCP_DST_PORT+1] = buf[TCP_SRC_PORT+1];
    buf[TCP_SRC_PORT] = wwwport >> 8;
    buf[TCP_SRC_PORT+1] = wwwport & 0xFF;
    while(i>0){
        rel_ack_num=buf[TCP_SEQ+i-1]+rel_ack_num;
        tseq=buf[TCP_SEQACK+i-1];
        buf[TCP_SEQACK+i-1]=0xFF&rel_ack_num;
        if (cp_seq) buf[TCP_SEQ+i-1]=tseq;
        else buf[TCP_SEQ+i-1] = 0;
        rel_ack_num=rel_ack_num>>8;
        i--;
    }
    if(cp_seq==0) {
        buf[TCP_SEQ] = 0;
        buf[TCP_SEQ+1] = 0;
        buf[TCP_SEQ+2] = seqnum; 
        buf[TCP_SEQ+3] = 0;
        seqnum += 2;
    }
    buf[TCP_CHECKSUM] = 0;
    buf[TCP_CHECKSUM+1] = 0;
    if(mss) {
        buf[TCP_OPTIONS] = 0x02;
        buf[TCP_OPTIONS+1] = 0x04;
        buf[TCP_OPTIONS+2] = 0x05; 
        buf[TCP_OPTIONS+3] = 0x80;
        buf[TCP_HEADER_LEN] = 0x60;
    }
    else buf[TCP_HEADER_LEN] = 0x50;
}

void arp_reply(uint8_t *buf) {
    uint8_t i=0;
    make_eth_hdr(buf);
    buf[ARP_OPCODE] = ARP_REPLY_H;
    buf[ARP_OPCODE+1] = ARP_REPLY_L;
    for(i=0;i<6;i++) {
        buf[ARP_DST_MAC+i] = buf[ARP_SRC_MAC+i];
        buf[ARP_SRC_MAC+i] = macaddr[i];
    }
    for(i=0;i<4;i++) {
        buf[ARP_DST_IP+i]=buf[ARP_SRC_IP+i];
        buf[ARP_SRC_IP+i]=ipaddr[i];
    }
    ENC28J60_PacketSend(42,buf); 
}

void icmp_reply(uint8_t *buf,uint16_t len) {
    make_eth_hdr(buf);
    make_ip_hdr(buf);
    buf[ICMP_TYPE] = ICMP_REPLY;
    if(buf[ICMP_CHECKSUM] > (0xFF-0x08)) buf[ICMP_CHECKSUM+1]++;
    buf[ICMP_CHECKSUM] += 0x08;
    ENC28J60_PacketSend(len,buf);
}

void tcp_synack(uint8_t *buf) {
    uint16_t ck;
    make_eth_hdr(buf);
    buf[IP_TOTLEN] = 0;
    buf[IP_TOTLEN+1] = IP_HEADER_LEN+TCP_LEN_PLAIN+4;
    make_ip_hdr(buf);
    buf[TCP_FLAGS] = TCP_SYN|TCP_ACK;
    make_tcp_hdr(buf,1,1,0);
    ck=checksum(&buf[IP_SRC],8+TCP_LEN_PLAIN+4,2);
    buf[TCP_CHECKSUM] = ck >> 8;
    buf[TCP_CHECKSUM+1] = ck & 0xFF;
    ENC28J60_PacketSend(IP_HEADER_LEN+TCP_LEN_PLAIN+4+ETH_HEADER_LEN,buf);
}

void init_len_info(uint8_t *buf) {
    info_data_len = (((int16_t)buf[IP_TOTLEN])<<8)|(buf[IP_TOTLEN+1]&0xFF);
    info_data_len -= IP_HEADER_LEN;
    info_hdr_len = (buf[TCP_HEADER_LEN]>>4)*4;
    info_data_len -= info_hdr_len;
    if(info_data_len<=0) info_data_len=0;
}

uint16_t get_tcp_data_ptr(void) {
    if(info_data_len) return((uint16_t)TCP_SRC_PORT+info_hdr_len);
    else return(0);
}

uint16_t make_tcp_data_pos(uint8_t *buf,uint16_t pos, const prog_char *progmem_s) {
    char c;
    while((c = pgm_read_byte(progmem_s++))) {
        buf[TCP_CHECKSUM+4+pos] = c;
        pos++;
    }
    return(pos);
}

uint16_t make_tcp_data(uint8_t *buf,uint16_t pos, const char *s) {
    while(*s) {
        buf[TCP_CHECKSUM+4+pos]=*s;
        pos++;
        s++;
    }
    return(pos);
}

void tcp_ack(uint8_t *buf) {
    uint16_t j;
    make_eth_hdr(buf);
    buf[TCP_FLAGS] = TCP_ACK;
    if(info_data_len==0) make_tcp_hdr(buf,1,0,1);
    else make_tcp_hdr(buf,info_data_len,0,1);
    j = IP_HEADER_LEN+TCP_LEN_PLAIN;
    buf[IP_TOTLEN] = j >> 8;
    buf[IP_TOTLEN+1] = j & 0xFF;
    make_ip_hdr(buf);
    j = checksum(&buf[IP_SRC],8+TCP_LEN_PLAIN,2);
    buf[TCP_CHECKSUM] = j >> 8;
    buf[TCP_CHECKSUM+1] = j & 0xFF;
    ENC28J60_PacketSend(IP_HEADER_LEN+TCP_LEN_PLAIN+ETH_HEADER_LEN,buf);
}

void tcp_ack_with_data(uint8_t *buf,uint16_t dlen) {
    uint16_t j;
    buf[TCP_FLAGS] = TCP_ACK|TCP_PSH|TCP_FIN;
    j = IP_HEADER_LEN+TCP_LEN_PLAIN+dlen;
    buf[IP_TOTLEN] = j >> 8;
    buf[IP_TOTLEN+1] = j & 0xFF;
    make_ip_checksum(buf);
    buf[TCP_CHECKSUM] = 0;
    buf[TCP_CHECKSUM+1] = 0;
    j = checksum(&buf[IP_SRC],8+TCP_LEN_PLAIN+dlen,2);
    buf[TCP_CHECKSUM] = j>>8;
    buf[TCP_CHECKSUM+1] = j& 0xFF;
    ENC28J60_PacketSend(IP_HEADER_LEN+TCP_LEN_PLAIN+dlen+ETH_HEADER_LEN,buf);
}
