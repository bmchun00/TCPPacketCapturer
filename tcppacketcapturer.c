#include <stdlib.h>
#include <stdio.h>
#include <pcap.h>
#include <arpa/inet.h>

const int data_length = 100; //표현할 데이터 길이입니다.

/* Ethernet header */
struct ethheader {
  u_char  ether_dhost[6]; /* destination host address */
  u_char  ether_shost[6]; /* source host address */
  u_short ether_type;     /* protocol type (IP, ARP, RARP, etc) */
};

/* IP Header */
struct ipheader {
  unsigned char      iph_ihl:4, //IP version
                     iph_ver:4; //IP header length
  unsigned char      iph_tos; //Type of service
  unsigned short int iph_len; //IP Packet length (data + header)
  unsigned short int iph_ident; //Identification
  unsigned short int iph_flag:3, //Fragmentation flags
                     iph_offset:13; //Flags offset
  unsigned char      iph_ttl; //Time to Live
  unsigned char      iph_protocol; //Protocol type
  unsigned short int iph_chksum; //IP datagram checksum
  struct  in_addr    iph_sourceip; //Source IP address
  struct  in_addr    iph_destip;   //Destination IP address
};

/* TCP Header */
struct tcpheader{
  unsigned short     tcp_sport; //TCP source port
  unsigned short     tcp_dport; //TCP destination port
  unsigned int       tcp_seqnum; //TCP sequence number
  unsigned int       tcp_acknum; //TCP acknowledgement number
  unsigned char      tcp_flagc:1, //TCP flag bit
                     tcp_reserved:3, // TCP reserved
                     tcp_dataoffset:4; //TCP data offset
  unsigned char      flags; //TCP flags
  unsigned short     tcp_winsize; //TCP window size
  unsigned short     tcp_chksum; //TCP checksum
  unsigned short     tcp_urgpointer; //TCP urgent pointer
};

struct httpheader{
  u_char data[data_length];
};

void printMacString(u_char* u){ //u_char 형식의 배열을 mac주소 형태의 string으로 반환합니다. (버퍼 필요)
  printf("%02X:%02X:%02X:%02X:%02X:%02X", u[0],u[1],u[2],u[3],u[4],u[5]);
}

void printDataHex(u_char* data){
  int line = 20;
  for(int i = 0;i<data_length;i+=line){
    for(int j = 0; j<line; j++){
      printf("%02X ", data[i+j]);
    }
    printf("\n");
  }
}

void printDataString(u_char* data){
  for(int i = 0;i<data_length;i++){
    printf("%c", data[i]);
  }
  printf("\n");
}


void got_packet(u_char *args, const struct pcap_pkthdr *header,
                              const u_char *packet)
{
  char buffer[20];
  struct ethheader *eth = (struct ethheader *)packet;

  if (ntohs(eth->ether_type) == 0x0800) {

    printf("=====================got tcp packet======================\n");

    printf("Ethernet Header : ");
    printMacString(eth->ether_dhost);
    printf(" / ");
    printMacString(eth->ether_shost);
    printf("\n");

    struct ipheader * ip = (struct ipheader *) (packet + sizeof(struct ethheader));
    
    printf("IP Header : %s / ", inet_ntoa(ip->iph_sourceip));
    printf("%s\n", inet_ntoa(ip->iph_destip));
    unsigned short int iplen = (ip->iph_ihl) * 4; //ip header의 값에 4를 곱하여 바이트로 표현합니다.

    struct tcpheader * tcp = (struct tcpheader *) ((char*)ip + iplen);

    unsigned short int tcplen = (tcp->tcp_dataoffset)*4; //tcp header의 값에 4를 곱하여 바이트로 표현합니다.

    printf("TCP Header : %d / %d\n" , ntohs(tcp->tcp_sport), ntohs(tcp->tcp_dport));

    printf("IP Header len : %d\n", iplen);

    printf("TCP Header len : %d\n", tcplen);

    struct httpheader * http = (struct httpheader *) (packet + sizeof(struct ethheader) + iplen + tcplen);

    if(ntohs(ip->iph_len)-(iplen)-(tcplen) > 0){ //데이터그램의 길이에서 ip헤더의 길이와 tcp헤더의 길이를 뺐을 때 0 이상이라면 데이터가 있음을 알 수 있습니다.
      printf("Data Found : \n");
      printf("hex data (%dbyte) : \n",data_length);
      printDataHex(http->data);
      printf("ASCII string data (%dbyte) : \n",data_length);
      printDataString(http->data);
    }
    else {
      printf("Data Not Found\n");
    }
    printf("=========================================================\n");
  }
}

int main()
{
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  struct bpf_program fp;
  char filter_exp[] = "tcp";
  bpf_u_int32 net;

  handle = pcap_open_live("en0", BUFSIZ, 1, 1000, errbuf);

  pcap_compile(handle, &fp, filter_exp, 0, net);
  if (pcap_setfilter(handle, &fp) !=0) {
      pcap_perror(handle, "Error:");
      exit(EXIT_FAILURE);
  }

  pcap_loop(handle, -1, got_packet, NULL);

  pcap_close(handle);
  return 0;
}


