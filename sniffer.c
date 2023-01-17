//
// Created by alon on 1/12/23.
//

#include <pcap.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ether.h>
#include "arpa/inet.h"

typedef struct application_header {
    uint32_t unix_time;
    uint16_t total_length;
    union {
        uint16_t flags ;
        uint16_t reserved: 3;
        uint16_t cache: 1;
        uint16_t steps: 1;
        uint16_t type: 1;
        uint16_t status_code: 10;
    };

    uint16_t cache_control;
    uint16_t padding;
} app_header;

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
    FILE *pfile;
    pfile = fopen("211344015_208007351.txt", "a+");
    if (pfile == NULL) {
        printf("Error opening file!\n");
        return;
    }

    struct ether_header *eth_header = (struct ether_header *) packet;
    const struct ip *ip_header = (struct ip *) (packet + sizeof(struct ether_header));
    struct tcphdr *tcp_header = (struct tcphdr *) (packet + sizeof(struct ether_header) + (ip_header->ip_hl) * 4);
    app_header *appH = (app_header *) (packet + sizeof(struct ether_header) + (ip_header->ip_hl) * 4 +
                                       tcp_header->doff * 4);


    char source_ip[INET_ADDRSTRLEN], dest_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_header->ip_src, source_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_header->ip_dst, dest_ip, INET_ADDRSTRLEN);
    //
    unsigned char *data = (unsigned char *) (packet + sizeof(struct ether_header) + (ip_header->ip_hl) * 4 + tcp_header->doff * 4 + sizeof(app_header));
    unsigned int data_size = header->len - sizeof(struct ether_header) - (ip_header->ip_hl) * 4 - tcp_header->doff * 4 ;
    if(tcp_header->psh) {
        fprintf(pfile,"source ip: %s, dest_ip: %s\n",source_ip, dest_ip);
        fprintf(pfile,"source_port: %u, dest_port: %hu\n",  ntohs(tcp_header->th_sport), ntohs(tcp_header->th_dport));
        fprintf(pfile,"timestamp: %u\ntotal_length: %hu\n", ntohl(appH->unix_time), ntohs(appH->total_length));
        fprintf(pfile, "cache Flag: %u\n", (ntohs(appH->flags) >> 12)  & 0x1) ;
        fprintf(pfile, "steps Flag: %u\n", (ntohs(appH->flags) >> 11) & 0x1);
        fprintf(pfile, "type Flag: %u\n", (ntohs(appH->flags) >> 10)  & 0x1);
        fprintf(pfile, "status Code: %u\n", ntohs(appH->flags) & ((1 << 10) - 1));
        fprintf(pfile, "cache_control: %hu\ndata:", ntohs(appH->cache_control));

        for (int i = 0; i < data_size; i++) {
            if (!(i & 15)) fprintf(pfile, "\n%04X:  ", i);
            fprintf(pfile, "%02X ", ((unsigned char *) data)[i]);
        }
    }
    fprintf(pfile,"\n");

    fclose(pfile);
}


int main() {
    pcap_t *handle;
    char error_buf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;
    char filter_exp[] = "tcp";
    bpf_u_int32 net;

    // Step 1: Open live pcap session on NIC with name eth3
    handle = pcap_open_live("lo", BUFSIZ, 1, 1000, error_buf);
    // Step 2: Compile filter_exp into BPF psuedo-code
    pcap_compile(handle, &fp, filter_exp, 0, net);
    pcap_setfilter(handle, &fp);
    // Step 3: Capture packets
    pcap_loop(handle, -1, got_packet, NULL);

    pcap_close(handle);   //Close the handle
    return 0;
}

//    uint16_t source_port = tcp_header->th_sport
//    uint16_t dest_port = tcp_header->th_dport
//    uint32_t timestamp = ntohl(appH->unix_time);
//    uint16_t total_length = ntohl((header->caplen));
//    uint16_t cache_flag = appH->cache;
//    uint16_t steps_flag = appH->steps;
//    uint16_t type_flag = appH->type;
//    uint16_t status_code = appH->status_code;
//    uint16_t cache_control = appH->cache_control;