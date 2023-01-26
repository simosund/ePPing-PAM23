/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PPING_H
#define PPING_H

#include <linux/types.h>
#include <linux/in6.h>
#include <stdbool.h>

#define NS_PER_SECOND 1000000000UL
#define NS_PER_MS 1000000UL
#define MS_PER_S 1000UL
#define S_PER_DAY (24 * 3600UL)

typedef __u64 fixpoint64;
#define FIXPOINT_SHIFT 16
#define DOUBLE_TO_FIXPOINT(X) ((fixpoint64)((X) * (1UL << FIXPOINT_SHIFT)))
#define FIXPOINT_TO_UINT(X) ((X) >> FIXPOINT_SHIFT)

/* For the event_type members of rtt_event and flow_event */
#define EVENT_TYPE_FLOW 1
#define EVENT_TYPE_RTT 2
#define EVENT_TYPE_MAP_FULL 3
#define EVENT_TYPE_MAP_CLEAN 4

#define RTT_AGG_NR_BINS 1000UL
#define RTT_AGG_BIN_WIDTH 100000UL // 0.1 ms (100,000 ns)

enum __attribute__((__packed__)) flow_event_type {
	FLOW_EVENT_NONE,
	FLOW_EVENT_OPENING,
	FLOW_EVENT_CLOSING,
	FLOW_EVENT_CLOSING_BOTH
};

enum __attribute__((__packed__)) flow_event_reason {
	EVENT_REASON_NONE,
	EVENT_REASON_SYN,
	EVENT_REASON_SYN_ACK,
	EVENT_REASON_FIRST_OBS_PCKT,
	EVENT_REASON_FIN,
	EVENT_REASON_RST,
	EVENT_REASON_FLOW_TIMEOUT
};

enum __attribute__((__packed__)) flow_event_source {
	EVENT_SOURCE_PKT_SRC,
	EVENT_SOURCE_PKT_DEST,
	EVENT_SOURCE_GC
};

enum __attribute__((__packed__)) pping_map {
	PPING_MAP_FLOWSTATE = 0,
	PPING_MAP_PACKETTS
};

enum __attribute__((__packed__)) connection_state {
        CONNECTION_STATE_EMPTY,
        CONNECTION_STATE_WAITOPEN,
        CONNECTION_STATE_OPEN,
        CONNECTION_STATE_CLOSED
};

struct bpf_config {
	__u64 rate_limit;
	fixpoint64 rtt_rate;
	bool use_srtt;
	bool track_tcp;
	bool track_icmp;
	bool localfilt;
	bool push_individual_events;
	bool agg_rtts;
	__u16 reserved;
};

/*
 * Struct that can hold the source or destination address for a flow (l3+l4).
 * Works for both IPv4 and IPv6, as IPv4 addresses can be mapped to IPv6 ones
 * based on RFC 4291 Section 2.5.5.2.
 */
struct flow_address {
	struct in6_addr ip;
	__u16 port;
	__u16 reserved;
};

/*
 * Struct to hold a full network tuple
 * The ipv member is technically not necessary, but makes it easier to 
 * determine if saddr/daddr are IPv4 or IPv6 address (don't need to look at the
 * first 12 bytes of address). The proto memeber is not currently used, but 
 * could be useful once pping is extended to work for other protocols than TCP.
 */
struct network_tuple {
	struct flow_address saddr;
	struct flow_address daddr;
	__u16 proto; //IPPROTO_TCP, IPPROTO_ICMP, QUIC etc
	__u8 ipv; //AF_INET or AF_INET6
	__u8 reserved;
};

struct flow_state {
	__u64 min_rtt;
	__u64 srtt;
	__u64 last_timestamp;
	__u64 sent_pkts;
	__u64 sent_bytes;
	__u64 rec_pkts;
	__u64 rec_bytes;
	__u32 last_id;
	__u32 outstanding_timestamps;
	enum connection_state conn_state;
	enum flow_event_reason opening_reason;
	__u8 reserved[6];
};

/*
 * Stores flowstate for both direction (src -> dst and dst -> src) of a flow
 *
 * Uses two named members instead of array of size 2 to avoid hassels with
 * convincing verifier that member access is not out of bounds
 */
struct dual_flow_state {
	struct flow_state dir1;
	struct flow_state dir2;
};

struct packet_id {
	struct network_tuple flow;
	__u32 identifier; //tsval for TCP packets
};


/*
 * Events that can be passed from the BPF-programs to the user space
 * application.
 * The initial event_type memeber is used to allow multiplexing between
 * different event types in a single perf buffer. Memebers event_type and
 * timestamp are common among all event types, and flow is common for
 * rtt_event, flow_event and map_full_event.
 */

/*
 * An RTT event message passed when an RTT has been calculated
 * Uses explicit padding instead of packing based on recommendations in cilium's
 * BPF reference documentation at https://docs.cilium.io/en/stable/bpf/#llvm.
 */
struct rtt_event {
	__u64 event_type;
	__u64 timestamp;
	struct network_tuple flow;
	__u32 padding;
	__u64 rtt;
	__u64 min_rtt;
	__u64 sent_pkts;
	__u64 sent_bytes;
	__u64 rec_pkts;
	__u64 rec_bytes;
	__u32 tsecr;
	__u32 ack;
	bool match_on_egress;
	__u8 reserved[7];
};

/*
 * A flow event message passed when a flow has changed state (opened/closed)
 */
struct flow_event {
	__u64 event_type;
	__u64 timestamp;
	struct network_tuple flow;
	enum flow_event_type flow_event_type;
	enum flow_event_reason reason;
	enum flow_event_source source;
	__u8 reserved;
};

/*
 * An event indicating that a new entry could not be created the map due to the
 * map being full.
 */
struct map_full_event {
	__u64 event_type;
	__u64 timestamp;
	struct network_tuple flow;
	enum pping_map map;
	__u8 reserved[3];
};

/*
 * Struct for storing various debug-information about the map cleaning process.
 * The last_* members contain information from the last clean-cycle, whereas the
 * tot_* entires contain cumulative stats from all clean cycles.
 */
struct map_clean_event {
	__u64 event_type;
	__u64 timestamp;
	__u64 tot_runtime;
	__u64 tot_processed_entries;
	__u64 tot_timeout_del;
	__u64 tot_auto_del;
	__u64 last_runtime;
	__u32 last_processed_entries;
	__u32 last_timeout_del;
	__u32 last_auto_del;
	__u32 clean_cycles;
	enum pping_map map;
	__u8 reserved[7];
};

union pping_event {
	__u64 event_type;
	struct rtt_event rtt_event;
	struct flow_event flow_event;
	struct map_full_event map_event;
	struct map_clean_event map_clean_event;
};

struct aggregated_rtt_stats {
	__u64 min;
	__u64 max;
	__u32 bins[RTT_AGG_NR_BINS];
};

#endif
