// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)

/*
 * tcpstates    Trace TCP session state changes with durations.
 * Copyright (c) 2021 Hengqi Chen
 *
 * Based on tcpstates(8) from BCC by Brendan Gregg.
 * 18-Dec-2021   Hengqi Chen   Created this.
 */
#include <argp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <stdlib.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "btf_helpers.h"
#include "tcpstates.h"
#include "tcpstates.skel.h"
#include "trace_helpers.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define PERF_BUFFER_PAGES	16
#define PERF_POLL_TIMEOUT_MS	100
#define warn(...) fprintf(stderr, __VA_ARGS__)

static volatile sig_atomic_t exiting = 0;

#define SHM_KEY 12345

static bool emit_timestamp = false;
static short target_family = 0;
static char *target_sports = NULL;
static char *target_dports = NULL;
static bool wide_output = false;
static bool verbose = false;
static const char *tcp_states[] = {
	[1] = "ESTABLISHED",
	[2] = "SYN_SENT",
	[3] = "SYN_RECV",
	[4] = "FIN_WAIT1",
	[5] = "FIN_WAIT2",
	[6] = "TIME_WAIT",
	[7] = "CLOSE",
	[8] = "CLOSE_WAIT",
	[9] = "LAST_ACK",
	[10] = "LISTEN",
	[11] = "CLOSING",
	[12] = "NEW_SYN_RECV",
	[13] = "UNKNOWN",
};

enum {
        TCP_ESTABLISHED = 1,
        TCP_SYN_SENT,
        TCP_SYN_RECV,
        TCP_FIN_WAIT1,
        TCP_FIN_WAIT2,
        TCP_TIME_WAIT,
        TCP_CLOSE,
        TCP_CLOSE_WAIT,
        TCP_LAST_ACK,
        TCP_LISTEN,
        TCP_CLOSING,    /* Now a valid state */
        TCP_NEW_SYN_RECV,

        TCP_MAX_STATES  /* Leave at the end! */
};

const char *argp_program_version = "tcpstates 1.0";
const char *argp_program_bug_address =
	"https://github.com/iovisor/bcc/tree/master/libbpf-tools";
const char argp_program_doc[] =
"Trace TCP session state changes and durations.\n"
"\n"
"USAGE: tcpstates [-4] [-6] [-T] [-L lport] [-D dport]\n"
"\n"
"EXAMPLES:\n"
"    tcpstates                  # trace all TCP state changes\n"
"    tcpstates -T               # include timestamps\n"
"    tcpstates -L 80            # only trace local port 80\n"
"    tcpstates -D 80            # only trace remote port 80\n";

static const struct argp_option opts[] = {
	{ "verbose", 'v', NULL, 0, "Verbose debug output" },
	{ "timestamp", 'T', NULL, 0, "Include timestamp on output" },
	{ "ipv4", '4', NULL, 0, "Trace IPv4 family only" },
	{ "ipv6", '6', NULL, 0, "Trace IPv6 family only" },
	{ "wide", 'w', NULL, 0, "Wide column output (fits IPv6 addresses)" },
	{ "localport", 'L', "LPORT", 0, "Comma-separated list of local ports to trace." },
	{ "remoteport", 'D', "DPORT", 0, "Comma-separated list of remote ports to trace." },
	{ NULL, 'h', NULL, OPTION_HIDDEN, "Show the full help" },
	{},
};

static error_t parse_arg(int key, char *arg, struct argp_state *state)
{
	long port_num;
	char *port;

	switch (key) {
	case 'v':
		verbose = true;
		break;
	case 'T':
		emit_timestamp = true;
		break;
	case '4':
		target_family = AF_INET;
		break;
	case '6':
		target_family = AF_INET6;
		break;
	case 'w':
		wide_output = true;
		break;
	case 'L':
		if (!arg) {
			warn("No ports specified\n");
			argp_usage(state);
		}
		target_sports = strdup(arg);
		port = strtok(arg, ",");
		while (port) {
			port_num = strtol(port, NULL, 10);
			if (errno || port_num <= 0 || port_num > 65536) {
				warn("Invalid ports: %s\n", arg);
				argp_usage(state);
			}
			port = strtok(NULL, ",");
		}
		break;
	case 'D':
		if (!arg) {
			warn("No ports specified\n");
			argp_usage(state);
		}
		target_dports = strdup(arg);
		port = strtok(arg, ",");
		while (port) {
			port_num = strtol(port, NULL, 10);
			if (errno || port_num <= 0 || port_num > 65536) {
				warn("Invalid ports: %s\n", arg);
				argp_usage(state);
			}
			port = strtok(NULL, ",");
		}
		break;
	case 'h':
		argp_state_help(state, stderr, ARGP_HELP_STD_HELP);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
	if (level == LIBBPF_DEBUG && !verbose)
		return 0;

	return vfprintf(stderr, format, args);
}

static void sig_int(int signo)
{
	exiting = 1;
}

FILE *file;

void write_tuples(struct event *e)
{
    char ts[32], saddr[26], daddr[26];

    inet_ntop(e->family, &e->saddr, saddr, sizeof(saddr));
    inet_ntop(e->family, &e->daddr, daddr, sizeof(daddr));
    file = fopen("unique.txt", "ab");
    if (file == NULL) {
        perror("Error opening file");
        return false;
    }
    fprintf(file, "Process Name: %s    S_IP: %s    SPort: %d   D_IP: %s   DPort: %d    ThreadID: %d    PID: %d     Latency: %.3f\n ", e->task, saddr, e->sport, daddr, e->dport, e->tid, e->pid, (double)e->delta_us / 1000 );
    // Close the file
    fclose(file);
}
void insert_socket(struct list **headref, struct event *e, bool tuple_on)
{
	if(e->newstate != TCP_ESTABLISHED)
		return 0;

	char ts[32], saddr[26], daddr[26];
        
	struct list *node = (struct list*) malloc(sizeof(struct list));
	node->socket_details.skaddr = e->skaddr;
        node->socket_details.newstate = e->newstate;
	node->socket_details.saddr = e->saddr;
	node->socket_details.daddr = e->daddr;
	node->socket_details.pid = e->pid;
        strcpy(node->socket_details.task, e->task);
	node->socket_details.sport = e->sport;
	node->socket_details.dport = e->dport;
	node->socket_details.protocol = e->protocol;
	

	node->next = NULL;

        if(tuple_on){
	   write_tuples(e);
	}

	if(*headref == NULL){
		*headref = node;
		
	}else{	
           struct list *head = *headref;
      	   while(head->next != NULL){
		head = head->next;
	   }
          head->next = node;
	}
}

int search_socket(struct list **headref, struct event *e)
{
	 struct list *head = *headref;
         if(head == NULL){
	//	 printf("list is empty \n");
                return 0;
	 }
	 while(head != NULL){
//		 printf(" head socket %llx e socket %llx \n", head->socket_details.skaddr, e->skaddr);
		if(head->socket_details.skaddr == e->skaddr){
			printf("list socket state %d \n",head->socket_details.newstate);
			return head->socket_details.newstate;
		}
		head = head->next;
	  } 
         printf("retuing false \n");
	 return false;
}

bool search_5_tuples(struct list **headref, struct event *e)
{

	struct list *head = *headref;

	if(head == NULL)
		return false;

	while(head != NULL){
		if(head->socket_details.daddr == e->daddr &&   
		head->socket_details.dport == e->dport &&
		head->socket_details.protocol == e->protocol)
		{
			return true;
		}

		head = head->next;
       	}

	return false;
}

int delete_socket(struct list **headref, struct event *e)
{

          struct list *head = *headref;
	  struct list *prev = NULL;


	  if(head != NULL && head->socket_details.skaddr == e->skaddr){
		  *headref = head->next;
		  
		  free(head);
		  return 0; 
	   }

	  while(head != NULL && head->socket_details.skaddr != e->skaddr){
		  prev= head;
		  head=head->next;
	  }

	  if(head == NULL)
              return 0;

	if (prev != NULL) {
        prev->next = head->next;
        
        free(head);
        } else {
        // If prev is NULL, it means we're deleting the first node, so we don't need to free prev.
        *headref = head->next;
        
        free(head);
        }

   /*       prev->next = head->next;
	  printf("second time list freed %p \n ", prev);
          free(prev);*/
	 
}

void print_details(struct list **node){
     
        
	struct list *headd = *node;
	char daddr[26];
//	inet_ntop(2, &headd->socket_details.daddr, daddr, sizeof(daddr));

	printf("Starting... \n");	
	while(headd != NULL){
		inet_ntop(2, &headd->socket_details.daddr, daddr, sizeof(daddr));
		printf("Active Connections  %s  task %s\n", daddr, headd->socket_details.task);
                headd = headd->next;
	}
	printf("End.. \n\n");
	
}

int socket_handle(struct event *e)
{
        int state;
	state = search_socket(&head,e);
	int close, open;

	if(state == TCP_ESTABLISHED && e->newstate == TCP_CLOSE)
	{	
	   close = 1;
           delete_socket(&head, e);
	  // print_details(&head);
	}

        if(e->newstate == TCP_ESTABLISHED)
	{
           open = 1;		
           insert_socket(&head, e, false);
	   printf("checking 5 tuples \n");
	   if(!search_5_tuples(&head_conn, e)){
		   printf("5 tuples not detected \n");
		   insert_socket(&head_conn, e, true);
	   }
         //  print_details(&head);	
	   printf("5 Tuples printing \n");
	   print_details(&head_conn);
	}
       
/*      if(open == 1 && close ==1){	
           memcpy(sharedList, head, sizeof(struct list));

           // Wait for a while (simulate ongoing updates)
           sleep(1);

           // Detach shared memory
           shmdt(sharedList);
      }*/
      

}

//struct list *node;
static void handle_event(void *ctx, int cpu, void *data, __u32 data_sz)
{
	char ts[32], saddr[26], daddr[26];
	struct event *e = data;
	struct tm *tm;
	int family;
	time_t t;
	if (emit_timestamp) {
		time(&t);
		tm = localtime(&t);
		strftime(ts, sizeof(ts), "%H:%M:%S", tm);
		printf("%8s ", ts);
	}
	inet_ntop(e->family, &e->saddr, saddr, sizeof(saddr));
	inet_ntop(e->family, &e->daddr, daddr, sizeof(daddr));
	if (wide_output) {
		family = e->family == AF_INET ? 4 : 6;
		printf("%-16llx %-7d %-16s %-2d %-26s %-5d %-26s %-5d %-11s -> %-11s %.3f\n",
		       e->skaddr, e->pid, e->task, family, saddr, e->sport, daddr, e->dport,
		       tcp_states[e->oldstate], tcp_states[e->newstate], (double)e->delta_us / 1000);
	} else {
		printf("%-16llx %-7d %-10.10s %-15s %-5d %-15s %-5d %-11s -> %-11s %.3f\n",
		       e->skaddr, e->pid, e->task, saddr, e->sport, daddr, e->dport,
		       tcp_states[e->oldstate], tcp_states[e->newstate], (double)e->delta_us / 1000);
	}

	 socket_handle(e);
             

}

static void handle_lost_events(void *ctx, int cpu, __u64 lost_cnt)
{
	warn("lost %llu events on CPU #%d\n", lost_cnt, cpu);
}

int shmid;
//struct list *sharedList;
int main(int argc, char **argv)
{
	LIBBPF_OPTS(bpf_object_open_opts, open_opts);
	static const struct argp argp = {
		.options = opts,
		.parser = parse_arg,
		.doc = argp_program_doc,
	};
	struct perf_buffer *pb = NULL;
	struct tcpstates_bpf *obj;
	int err, port_map_fd;
	short port_num;
	char *port;
        shmid = shmget(SHM_KEY, sizeof(struct list), 0666 | IPC_CREAT);

        if (shmid == -1) {
           perror("shmget");
           exit(1);
        } 

       sharedList = (struct Node*)shmat(shmid, NULL, 0);
       if (sharedList == (void*)-1) {
        perror("shmat");
        exit(1);
       }

	err = argp_parse(&argp, argc, argv, 0, NULL, NULL);
	if (err)
		return err;

	libbpf_set_print(libbpf_print_fn);

/*	err = ensure_core_btf(&open_opts);
	if (err) {
		warn("failed to fetch necessary BTF for CO-RE: %s\n", strerror(-err));
		return 1;
	}*/

	obj = tcpstates_bpf__open_opts(&open_opts);
	if (!obj) {
		warn("failed to open BPF object\n");
		return 1;
	}

	obj->rodata->filter_by_sport = target_sports != NULL;
	obj->rodata->filter_by_dport = target_dports != NULL;
	obj->rodata->target_family = target_family;

	err = tcpstates_bpf__load(obj);
	if (err) {
		warn("failed to load BPF object: %d\n", err);
		goto cleanup;
	}

	if (target_sports) {
		port_map_fd = bpf_map__fd(obj->maps.sports);
		port = strtok(target_sports, ",");
		while (port) {
			port_num = strtol(port, NULL, 10);
			bpf_map_update_elem(port_map_fd, &port_num, &port_num, BPF_ANY);
			port = strtok(NULL, ",");
		}
	}
	if (target_dports) {
		port_map_fd = bpf_map__fd(obj->maps.dports);
		port = strtok(target_dports, ",");
		while (port) {
			port_num = strtol(port, NULL, 10);
			bpf_map_update_elem(port_map_fd, &port_num, &port_num, BPF_ANY);
			port = strtok(NULL, ",");
		}
	}

	err = tcpstates_bpf__attach(obj);
	if (err) {
		warn("failed to attach BPF programs: %d\n", err);
		goto cleanup;
	}

	pb = perf_buffer__new(bpf_map__fd(obj->maps.events), PERF_BUFFER_PAGES,
			      handle_event, handle_lost_events, NULL, NULL);
	if (!pb) {
		err = - errno;
		warn("failed to open perf buffer: %d\n", err);
		goto cleanup;
	}

	if (signal(SIGINT, sig_int) == SIG_ERR) {
		warn("can't set signal handler: %s\n", strerror(errno));
		err = 1;
		goto cleanup;
	}

	if (emit_timestamp)
		printf("%-8s ", "TIME(s)");
	if (wide_output)
		printf("%-16s %-7s %-16s %-2s %-26s %-5s %-26s %-5s %-11s -> %-11s %s\n",
		       "SKADDR", "PID", "COMM", "IP", "LADDR", "LPORT",
		       "RADDR", "RPORT", "OLDSTATE", "NEWSTATE", "MS");
	else
		printf("%-16s %-7s %-10s %-15s %-5s %-15s %-5s %-11s -> %-11s %s\n",
		       "SKADDR", "PID", "COMM", "LADDR", "LPORT",
		       "RADDR", "RPORT", "OLDSTATE", "NEWSTATE", "MS");

	while (!exiting) {
		err = perf_buffer__poll(pb, PERF_POLL_TIMEOUT_MS);
		if (err < 0 && err != -EINTR) {
			warn("error polling perf buffer: %s\n", strerror(-err));
			goto cleanup;
		}
		/* reset err to return 0 if exiting */
		err = 0;
	}

cleanup:
	perf_buffer__free(pb);
	tcpstates_bpf__destroy(obj);
//	cleanup_core_btf(&open_opts);

	return err != 0;
}
