#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include "rlib.h"

#include  <signal.h>

typedef int bool;
#define TRUE 1
#define FALSE 0


int min(int x, int y) {
	if (x <= y) return x;
	return y;
}

enum packet_type {
  DATA_PACKET, ACK_PACKET, INVALID_PACKET
};


static const int ACK_PACKET_LEN = 8;
static const int DATA_PACKET_HEADER_LEN = 12;
static const int DATA_PACKET_MAX_PAYLOAD_LEN = 500;
static const int DATA_PACKET_MAX_LEN = 512;

struct reliable_state {
  rel_t *next;		             /* Linked list for traversing all connections */
  rel_t **prev;
  conn_t *c;		             /* This is the connection object */
  struct sockaddr_storage ss;        /* Network peer */
  int window_size;                   /* The size of the window */ 
  packet_t *pkts_sent;               /* An array of packets sent with size = window size */
  packet_t *pkts_recvd;              /* An array of packets received with size = window size */
  bool *has_recvd_pkt;               /* An array of booleans indicating which packets in the receive window have been received */
  uint64_t *pkt_send_time_millis;    /* An array of timestamps representing when each packet in the window was sent */
  uint32_t last_ackno_sent;          /* The last ackno sent to the other side of the connection */
  uint32_t last_ackno_recvd;         /* The last ackno received from the other side of the connection */
  uint32_t last_seqno_sent;          /* The sequence number of the last sent packet */
  uint32_t last_seqno_recvd;         /* The sequence number of the last received packet */
  uint16_t last_pkt_bytes_outputted; /* The number of bytes of the last packet that have been sent to the output connection already */
  bool last_pkt_recvd_eof;           /* Signifies whether the last received data packet was an EOF */
  bool read_eof;                     /* Whether or not the last read input was EOF */
  int timeout_millis;                /* The maximum timeout before attempting to re-send a packet */
};
rel_t *rel_list;

rel_t *get_session(const struct sockaddr_storage *ss);

void process_first_pkt(packet_t *pkt, enum packet_type pkt_type, const struct sockaddr_storage *ss, const struct config_common *cc);
void process_pkt(rel_t *r, packet_t *pkt, enum packet_type pkt_type);
void process_ack_pkt(rel_t *r, packet_t *pkt);
void process_data_pkt(rel_t *r, packet_t *pkt);

bool pkt_out_of_bounds(rel_t *r, packet_t *pkt);
bool has_recvd_pkt(rel_t *r, packet_t *pkt);
uint32_t get_pkt_idx(rel_t *r, uint32_t seqno);
bool has_buffered_pkts(rel_t *r);
void process_eof_pkt(rel_t *r, packet_t *pkt);

void add_pkt_to_send_window(rel_t *r, packet_t *pkt);
void add_pkt_to_recv_window(rel_t *r, packet_t *pkt);

void output_pkts(rel_t *r);
uint16_t output_pkt(rel_t *r, packet_t *pkt, uint16_t start, uint16_t payload_len);

int send_ack_pkt(rel_t *r, uint32_t ackno);

uint32_t avail_send_window_slots(rel_t *r);
bool has_unackd_pkts(rel_t *r);

int send_new_data_pkt(rel_t *r, char *data, uint16_t payload_len);
int send_data_pkt(rel_t *r, packet_t *pkt);

void pkt_retranmission(rel_t *r);
bool should_close_conn(rel_t *r);

void init_ack_pkt(packet_t *pkt, uint32_t ackno);
void init_data_pkt(packet_t *pkt, uint32_t ackno, uint32_t seqno, char *data, uint16_t payload_len);

void pkt_ntoh(packet_t *pkt);
void pkt_hton(packet_t *pkt);

enum packet_type get_type_of_pkt(packet_t *pkt, size_t n);
bool match_chksum(packet_t *pkt);

uint64_t get_timestamp();

/* Creates a new reliable protocol session, returns NULL on failure.
 * Exactly one of c and ss should be NULL.  (ss is NULL when called
 * from rlib.c, while c is NULL when this function is called from
 * rel_demux.) */
rel_t * rel_create(conn_t *c, const struct sockaddr_storage *ss,
	    const struct config_common *cc) {

  printf("rel_create is called\n");
  rel_t *r;

  r = xmalloc (sizeof (*r));
  memset (r, 0, sizeof (*r));

  if (!c) {
    c = conn_create (r, ss);
    if (!c) {
      free (r);
      return NULL;
    }
  }

  r->c = c;

  if (ss != NULL)r->ss = *ss;

  r->next = rel_list;
  r->prev = &rel_list;
  if (rel_list) rel_list->prev = &r->next;
  rel_list = r;

  //my implementation
  r->window_size = cc->window;
  r->pkts_sent = xmalloc(r->window_size * sizeof(packet_t));
  r->pkts_recvd = xmalloc(r->window_size * sizeof(packet_t));
  r->pkt_send_time_millis = xmalloc(r->window_size * sizeof(uint64_t));
  r->has_recvd_pkt = xmalloc(r->window_size * sizeof(bool));
  r->timeout_millis = cc->timeout;
  for (int i = 0; i < r->window_size; i++) {
    r->pkt_send_time_millis[i] = 0;
    r->has_recvd_pkt[i] = FALSE;
  }
  r->last_seqno_sent = 0;
  r->last_pkt_bytes_outputted = 0;
  r->read_eof = FALSE;
  r->last_pkt_recvd_eof = FALSE;
  r->last_seqno_recvd = 1;
  r->last_ackno_sent = 1;
  r->last_ackno_recvd = 1;
  return r;
}

void rel_destroy(rel_t *r) {
  printf("rel_destroy is called\n");
  if (r->next)
    r->next->prev = r->prev;
  *r->prev = r->next;
  conn_destroy (r->c);

  free(r->pkts_sent);
  free(r->pkts_recvd);
  free(r->has_recvd_pkt);
  free(r->pkt_send_time_millis);

  free(r);
}


/* This function only gets called when the process is running as a
 * server and must handle connections from multiple clients.  You have
 * to look up the rel_t structure based on the address in the
 * sockaddr_storage passed in.  If this is a new connection (sequence
 * number 1), you will need to allocate a new conn_t using rel_create
 * ().  (Pass rel_create NULL for the conn_t, so it will know to
 * allocate a new connection.)
 */
void rel_demux(const struct config_common *cc,
	   const struct sockaddr_storage *ss,
	   packet_t *pkt, size_t len) {
}

void rel_recvpkt(rel_t *r, packet_t *pkt, size_t n) {
  enum packet_type pkt_type = get_type_of_pkt(pkt, n);
  // printf("Packet  received %d\n",pkt_type);
  pkt_ntoh(pkt);

  char *res;
  res = "receive data ";

  if(pkt_type == 1) res = "receive ACK ";
  else if(pkt_type == 0) res = "receive data ";
  else res = "receive INVALID";

  print_pkt(pkt,res,pkt->len);
  process_pkt(r, pkt, pkt_type);
}

void rel_read(rel_t *r) {
  // printf("read called...\n");
  if (r->read_eof == TRUE) {
    fprintf(stderr, "%d: rel_read: already read EOF\n", getpid());
    return;
  }

  uint32_t pkts_to_send = avail_send_window_slots(r);

  while (pkts_to_send > 0) {
    char buf[DATA_PACKET_MAX_PAYLOAD_LEN];
    int bytes_read = conn_input(r->c, buf, DATA_PACKET_MAX_PAYLOAD_LEN);
    printf("%d, READ Len %d\n",getpid(),bytes_read);

    if (bytes_read < 0) {
      fprintf(stderr, "%d: rel_read: EOF\n", getpid());
      r->read_eof = TRUE;
      send_new_data_pkt(r, NULL, 0); /* send EOF to other side */
      return;
    }

    if (bytes_read == 0) {
      return;
    }
    pkts_to_send--;
    send_new_data_pkt(r, buf, bytes_read);
    
  }
}



void rel_output(rel_t *r) {output_pkts(r);}

void rel_timer() {
  rel_t *r = rel_list;
  while (r != NULL) {
    pkt_retranmission(r);
    if (should_close_conn(r) == TRUE) {
      fprintf(stderr, "%d: closing connection\n", getpid());
      rel_t *next = r->next;
      rel_destroy(r);
      r = next;
    }
    else r = r->next;
  }
}

uint32_t avail_send_window_slots(rel_t *r){return r->window_size - (r->last_seqno_sent - r->last_ackno_recvd + 1);
}

bool has_unackd_pkts(rel_t *r){
  if (r->last_seqno_sent + 1 != r->last_ackno_recvd)return FALSE;
  return TRUE;
}

rel_t *get_session(const struct sockaddr_storage *ss) {
  rel_t *r = rel_list;

  while (r != NULL) {
    if (addreq(&r->ss, ss))return r;
    r = r->next;
  }
  return NULL; 
}

void process_first_pkt(packet_t *pkt, enum packet_type pkt_type, const struct sockaddr_storage *ss, const struct config_common *cc) {
  if (pkt_type != DATA_PACKET){
    fprintf(stderr, "%d: rel_demux: ignoring non-data pkt\n", getpid());
    return;
  }

  if(pkt->seqno != 1){
    fprintf(stderr, "%d: rel_demux: ignoring pkt with seqno != 1\n", getpid());
    return;
  }
  rel_t *r = rel_create(NULL, ss, cc);
  process_pkt(r, pkt, pkt_type);
}

void process_pkt(rel_t *r, packet_t *pkt, enum packet_type pkt_type) {
  if(pkt_type == DATA_PACKET) process_data_pkt(r, pkt);
  else if(pkt_type == ACK_PACKET) process_ack_pkt(r, pkt);
}

void process_ack_pkt(rel_t *r, packet_t *pkt) {
  if (pkt->ackno <= r->last_ackno_recvd){
    fprintf(stderr, "%d: ignoring already received ack\n", getpid());
    return;
  }

  if (pkt->ackno > r->last_seqno_sent + 1) {
    fprintf(stderr, "%d: invalid ackno %u\n", getpid(), pkt->ackno);
    return;
  }
  r->last_ackno_recvd = pkt->ackno;
  rel_read(r);
}

void process_data_pkt(rel_t *r, packet_t *pkt) {
  if (pkt_out_of_bounds(r, pkt) || has_recvd_pkt(r, pkt)) {
    if(pkt_out_of_bounds(r, pkt) ){
      fprintf(stderr, "%d: dropping out-of-bounds sequence number %u\n", getpid(), pkt->seqno);
    }
    else fprintf(stderr, "%d: ignoring duplicate data packet %u\n", getpid(), pkt->seqno);
    send_ack_pkt(r, r->last_ackno_sent); 
    return;
  }


  if (r->last_pkt_recvd_eof == TRUE) {
    fprintf(stderr, "%d: ignoring data packet - already received EOF\n", getpid());
    send_ack_pkt(r, r->last_ackno_sent);
    return;
  }

  if (pkt->len == DATA_PACKET_HEADER_LEN) {
    if (has_buffered_pkts(r)) {
      fprintf(stderr, "%d: ignoring EOF - waiting on pkt %u\n", getpid(), r->last_ackno_sent);
      send_ack_pkt(r, r->last_ackno_sent);
    } 
    else process_eof_pkt(r, pkt);
    return;
  }

  add_pkt_to_recv_window(r, pkt);
  output_pkts(r);
}

bool pkt_out_of_bounds(rel_t *r, packet_t *pkt) {
  if (pkt->seqno < r->last_ackno_sent || pkt->seqno >= r->last_ackno_sent + r->window_size) return TRUE;
  return FALSE;
}

bool has_recvd_pkt(rel_t *r, packet_t *pkt) {
  uint32_t idx = get_pkt_idx(r, pkt->seqno);
  return r->has_recvd_pkt[idx];
}

uint32_t get_pkt_idx(rel_t *r, uint32_t seqno) {
  return (seqno - 1) % r->window_size;
}


void add_pkt_to_send_window(rel_t *r, packet_t *pkt) {
  uint32_t idx = get_pkt_idx(r, pkt->seqno);
  r->pkts_sent[idx] = *pkt;
  r->pkt_send_time_millis[idx] = get_timestamp();
  if (pkt->seqno > r->last_seqno_sent) r->last_seqno_sent = pkt->seqno;
}

void add_pkt_to_recv_window(rel_t *r, packet_t *pkt) {
  uint32_t idx = get_pkt_idx(r, pkt->seqno);
  r->pkts_recvd[idx] = *pkt;
  r->has_recvd_pkt[idx] = TRUE;
  if (pkt->seqno > r->last_seqno_recvd)r->last_seqno_recvd = pkt->seqno;
}

bool has_buffered_pkts(rel_t *r) {
  if (r->last_ackno_sent >= r->last_seqno_recvd) return FALSE;
  else return TRUE;
}

void process_eof_pkt(rel_t *r, packet_t *pkt) {
  fprintf(stderr, "%d: received EOF\n", getpid());
  r->last_pkt_recvd_eof = TRUE;
  conn_output(r->c, NULL, 0);
  send_ack_pkt(r, r->last_ackno_sent + 1);
}


uint16_t output_pkt(rel_t *r, packet_t *pkt, uint16_t start, uint16_t payload_len) {
  uint16_t bufspace = conn_bufspace(r->c);
  uint16_t bytes_to_output = min(bufspace, payload_len - start);

  if (bufspace <= 0) {
    fprintf(stderr, "%d: no bufspace available\n", getpid());
    return 0;
  }

  char buf[bytes_to_output];
  memcpy(buf, pkt->data + start, bytes_to_output);

  int bytes_outputted = conn_output(r->c, buf, bytes_to_output); 
  assert(bytes_outputted != 0); /* guaranteed not to be 0 because we checked bufspace */

  return bytes_outputted;
}


void output_pkts(rel_t *r) {
  uint32_t idx = get_pkt_idx(r, r->last_ackno_sent);
  uint16_t num_pkts = 0;

  while (r->has_recvd_pkt[idx] == TRUE) {
    packet_t *pkt = &r->pkts_recvd[idx];
    uint16_t payload_len = pkt->len - DATA_PACKET_HEADER_LEN;
    uint16_t bytes_outputted = output_pkt(r, pkt, r->last_pkt_bytes_outputted, payload_len);
    if (bytes_outputted < 0) {
      perror("error calling conn_output");
      rel_destroy(r);
      return;
    }

    uint16_t bytes_left = payload_len - bytes_outputted - r->last_pkt_bytes_outputted;
    if (bytes_left > 0){
      r->last_pkt_bytes_outputted += bytes_outputted;
      return;
    }
  
    r->has_recvd_pkt[idx] = FALSE;
    r->last_pkt_bytes_outputted = 0;
    
    idx = get_pkt_idx(r, r->last_ackno_sent + num_pkts+1);
    num_pkts += 1;
  }

  if (num_pkts > 0)send_ack_pkt(r, r->last_ackno_sent + num_pkts);
}


int send_ack_pkt(rel_t *r, uint32_t ackno) {
  packet_t pkt;
  init_ack_pkt(&pkt, ackno);
  pkt_hton(&pkt);
  pkt.cksum = 0;
  pkt.cksum = cksum((void *)&pkt, ACK_PACKET_LEN);
  int sent_bytes = conn_sendpkt(r->c, &pkt, ACK_PACKET_LEN);

  if (sent_bytes > 0) {
    r->last_ackno_sent = ackno;
    print_pkt(&pkt,"send ack ",sent_bytes);
  }
  else if (sent_bytes == 0)fprintf(stderr, "%d: no bytes sent calling conn_sendpkt", getpid());
  else perror("error occured calling conn_sendpkt");
  rel_read(r);
  return sent_bytes;
}

int send_data_pkt(rel_t *r, packet_t *pkt) {
  uint16_t pkt_len = pkt->len;
  pkt_hton(pkt);
  pkt->cksum = 0;
  pkt->cksum = cksum((void *)pkt, pkt_len);
  char *res;
  res = "send data";
  if(pkt->len == 1)res = "send EOF ";
  print_pkt(pkt,res,pkt_len);
  int sent_bytes = conn_sendpkt(r->c, pkt, pkt_len);
  pkt_ntoh(pkt);
  if (sent_bytes == 0)fprintf(stderr, "no bytes sent calling conn_sendpkt\n");
  else if (sent_bytes < 0)perror("error occured calling conn_sendpkt");
  return sent_bytes;
}


int send_new_data_pkt(rel_t *r, char *data, uint16_t payload_len) {
  packet_t pkt;
  init_data_pkt(&pkt, r->last_ackno_sent, r->last_seqno_sent + 1, data, payload_len);
  add_pkt_to_send_window(r, &pkt);
  return send_data_pkt(r, &pkt);
}




void init_ack_pkt(packet_t *pkt, uint32_t ackno) {
  pkt->cksum = 0;
  pkt->len = ACK_PACKET_LEN;
  pkt->ackno = ackno;
}

void init_data_pkt(packet_t *pkt, uint32_t ackno, uint32_t seqno, char *data, uint16_t payload_len) {
  pkt->cksum = 0;
  pkt->len = DATA_PACKET_HEADER_LEN + payload_len;
  pkt->ackno = ackno;
  pkt->seqno = seqno;
  memcpy(pkt->data, data, payload_len);
}

void pkt_retranmission(rel_t *r) {
  if (r->last_seqno_sent < r->last_ackno_recvd)return;
  int idx = get_pkt_idx(r, r->last_ackno_recvd);
  packet_t *pkt = &r->pkts_sent[idx];
  unsigned int ellapsed_time = get_timestamp() - r->pkt_send_time_millis[idx];
  if (ellapsed_time > r->timeout_millis){
    fprintf(stderr, "%d: re-transmitting seqno=%u\n", getpid(), pkt->seqno);
    send_data_pkt(r, pkt);
  }
}

bool should_close_conn(rel_t *r) {
  if(!r->read_eof || !r->last_pkt_recvd_eof || !has_unackd_pkts(r) || !has_buffered_pkts(r)) return FALSE;
  return TRUE;
}

void pkt_ntoh(packet_t *pkt) {
  pkt->ackno = ntohl(pkt->ackno);
  pkt->len = ntohs(pkt->len);
  if (pkt->len >= DATA_PACKET_HEADER_LEN) pkt->seqno = ntohl(pkt->seqno);
}

void pkt_hton(packet_t *pkt) {
  if (pkt->len >= DATA_PACKET_HEADER_LEN)pkt->seqno = htonl(pkt->seqno);
  pkt->ackno = htonl(pkt->ackno);
  pkt->len = htons(pkt->len);
}


uint64_t get_timestamp() {
  struct timespec tp;
  int ret = clock_gettime(CLOCK_MONOTONIC, &tp);
  if (ret < 0)perror("Error calling clock_gettime");
  return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}



enum packet_type get_type_of_pkt(packet_t *pkt, size_t n) {
  if (n < ACK_PACKET_LEN){
    fprintf(stderr, "%d: invalid packet length: %zu\n", getpid(), n);
    return INVALID_PACKET;
  }
  if (match_chksum(pkt) == FALSE){
    fprintf(stderr, "%d: invalid checksum: %04x\n", getpid(), pkt->cksum);
    return INVALID_PACKET;
  }
  int pkt_len = ntohs(pkt->len);
  if (pkt_len == ACK_PACKET_LEN)return ACK_PACKET;
  if (pkt_len >= DATA_PACKET_HEADER_LEN && pkt_len <= DATA_PACKET_MAX_LEN)return DATA_PACKET;
  fprintf(stderr, "%d: invalid packet length: %u", getpid(), pkt_len);
  return INVALID_PACKET;
}




bool match_chksum(packet_t *pkt) {
  uint16_t len = ntohs(pkt->len);
  if(!len || len > sizeof(*pkt)) return FALSE;
  uint16_t cksum_val = pkt->cksum;
  pkt->cksum = 0;
  uint16_t comp_chksm = cksum((void *)pkt, len);
  pkt->cksum = cksum_val;
  if (cksum_val == comp_chksm)return TRUE;
  else return FALSE;
}


