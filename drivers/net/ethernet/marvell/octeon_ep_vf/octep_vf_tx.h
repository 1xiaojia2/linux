/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell Octeon EP (EndPoint) VF Ethernet Driver
 *
 * Copyright (C) 2020 Marvell.
 *
 */

#ifndef _OCTEP_VF_TX_H_
#define _OCTEP_VF_TX_H_

#define IQ_SEND_OK          0
#define IQ_SEND_STOP        1
#define IQ_SEND_FAILED     -1

#define TX_BUFTYPE_NONE          0
#define TX_BUFTYPE_NET           1
#define TX_BUFTYPE_NET_SG        2
#define NUM_TX_BUFTYPES          3

/* Hardware format for Scatter/Gather list
 *
 * 63      48|47     32|31     16|15       0
 * -----------------------------------------
 * |  Len 0  |  Len 1  |  Len 2  |  Len 3  |
 * -----------------------------------------
 * |                Ptr 0                  |
 * -----------------------------------------
 * |                Ptr 1                  |
 * -----------------------------------------
 * |                Ptr 2                  |
 * -----------------------------------------
 * |                Ptr 3                  |
 * -----------------------------------------
 */
struct octep_vf_tx_sglist_desc {
	u16 len[4];
	dma_addr_t dma_ptr[4];
};

static_assert(sizeof(struct octep_vf_tx_sglist_desc) == 40);

/* Each Scatter/Gather entry sent to hardwar hold four pointers.
 * So, number of entries required is (MAX_SKB_FRAGS + 1)/4, where '+1'
 * is for main skb which also goes as a gather buffer to Octeon hardware.
 * To allocate sufficient SGLIST entries for a packet with max fragments,
 * align by adding 3 before calcuating max SGLIST entries per packet.
 */
#define OCTEP_VF_SGLIST_ENTRIES_PER_PKT ((MAX_SKB_FRAGS + 1 + 3) / 4)
#define OCTEP_VF_SGLIST_SIZE_PER_PKT \
	(OCTEP_VF_SGLIST_ENTRIES_PER_PKT * sizeof(struct octep_vf_tx_sglist_desc))

struct octep_vf_tx_buffer {
	struct sk_buff *skb;
	dma_addr_t dma;
	struct octep_vf_tx_sglist_desc *sglist;
	dma_addr_t sglist_dma;
	u8 gather;
};

#define OCTEP_VF_IQ_TXBUFF_INFO_SIZE (sizeof(struct octep_vf_tx_buffer))

/* VF Hardware interface Tx statistics */
struct octep_vf_iface_tx_stats {
	/* Total frames sent on the interface */
	u64 pkts;

	/* Total octets sent on the interface */
	u64 octs;

	/* Packets sent to a broadcast DMAC */
	u64 bcst;

	/* Packets sent to the multicast DMAC */
	u64 mcst;

	/* Packets dropped */
	u64 dropped;

	/* Reserved */
	u64 reserved[13];
};

/* VF Input Queue statistics */
struct octep_vf_iq_stats {
	/* Instructions posted to this queue. */
	u64 instr_posted;

	/* Instructions copied by hardware for processing. */
	u64 instr_completed;

	/* Instructions that could not be processed. */
	u64 instr_dropped;

	/* Bytes sent through this queue. */
	u64 bytes_sent;

	/* Gather entries sent through this queue. */
	u64 sgentry_sent;

	/* Number of transmit failures due to TX_BUSY */
	u64 tx_busy;

	/* Number of times the queue is restarted */
	u64 restart_cnt;
};

/* The instruction (input) queue.
 * The input queue is used to post raw (instruction) mode data or packet
 * data to Octeon device from the host. Each input queue (up to 4) for
 * a Octeon device has one such structure to represent it.
 */
struct octep_vf_iq {
	u32 q_no;

	struct octep_vf_device *octep_vf_dev;
	struct net_device *netdev;
	struct device *dev;
	struct netdev_queue *netdev_q;

	/* Index in input ring where driver should write the next packet */
	u16 host_write_index;

	/* Index in input ring where Octeon is expected to read next packet */
	u16 octep_vf_read_index;

	/* This index aids in finding the window in the queue where Octeon
	 * has read the commands.
	 */
	u16 flush_index;

	/* Statistics for this input queue. */
	struct octep_vf_iq_stats *stats;

	/* Pointer to the Virtual Base addr of the input ring. */
	struct octep_vf_tx_desc_hw *desc_ring;

	/* DMA mapped base address of the input descriptor ring. */
	dma_addr_t desc_ring_dma;

	/* Info of Tx buffers pending completion. */
	struct octep_vf_tx_buffer *buff_info;

	/* Base pointer to Scatter/Gather lists for all ring descriptors. */
	struct octep_vf_tx_sglist_desc *sglist;

	/* DMA mapped addr of Scatter Gather Lists */
	dma_addr_t sglist_dma;

	/* Octeon doorbell register for the ring. */
	u8 __iomem *doorbell_reg;

	/* Octeon instruction count register for this ring. */
	u8 __iomem *inst_cnt_reg;

	/* interrupt level register for this ring */
	u8 __iomem *intr_lvl_reg;

	/* Maximum no. of instructions in this queue. */
	u32 max_count;
	u32 ring_size_mask;

	u32 pkt_in_done;
	u32 pkts_processed;

	u32 status;

	/* Number of instructions pending to be posted to Octeon. */
	u32 fill_cnt;

	/* The max. number of instructions that can be held pending by the
	 * driver before ringing doorbell.
	 */
	u32 fill_threshold;
};

/* Hardware Tx Instruction Header */
struct octep_vf_instr_hdr {
	/* Data Len */
	u64 tlen:16;

	/* Reserved */
	u64 rsvd:20;

	/* PKIND for SDP */
	u64 pkind:6;

	/* Front Data size */
	u64 fsz:6;

	/* No. of entries in gather list */
	u64 gsz:14;

	/* Gather indicator 1=gather*/
	u64 gather:1;

	/* Reserved3 */
	u64 reserved3:1;
};

static_assert(sizeof(struct octep_vf_instr_hdr) == 8);

/* Tx offload flags */
#define OCTEP_VF_TX_OFFLOAD_VLAN_INSERT	BIT(0)
#define OCTEP_VF_TX_OFFLOAD_IPV4_CKSUM	BIT(1)
#define OCTEP_VF_TX_OFFLOAD_UDP_CKSUM	BIT(2)
#define OCTEP_VF_TX_OFFLOAD_TCP_CKSUM	BIT(3)
#define OCTEP_VF_TX_OFFLOAD_SCTP_CKSUM	BIT(4)
#define OCTEP_VF_TX_OFFLOAD_TCP_TSO	BIT(5)
#define OCTEP_VF_TX_OFFLOAD_UDP_TSO	BIT(6)

#define OCTEP_VF_TX_OFFLOAD_CKSUM	(OCTEP_VF_TX_OFFLOAD_IPV4_CKSUM | \
					 OCTEP_VF_TX_OFFLOAD_UDP_CKSUM | \
					 OCTEP_VF_TX_OFFLOAD_TCP_CKSUM)

#define OCTEP_VF_TX_OFFLOAD_TSO		(OCTEP_VF_TX_OFFLOAD_TCP_TSO | \
					 OCTEP_VF_TX_OFFLOAD_UDP_TSO)

#define OCTEP_VF_TX_IP_CSUM(flags)	((flags) & \
					 (OCTEP_VF_TX_OFFLOAD_IPV4_CKSUM | \
					  OCTEP_VF_TX_OFFLOAD_TCP_CKSUM | \
					  OCTEP_VF_TX_OFFLOAD_UDP_CKSUM))

#define OCTEP_VF_TX_TSO(flags)		((flags) & \
					 (OCTEP_VF_TX_OFFLOAD_TCP_TSO | \
					  OCTEP_VF_TX_OFFLOAD_UDP_TSO))

struct tx_mdata {
	/* offload flags */
	u16 ol_flags;

	/* gso size */
	u16 gso_size;

	/* gso flags */
	u16 gso_segs;

	/* reserved */
	u16 rsvd1;

	/* reserved */
	u64 rsvd2;
};

static_assert(sizeof(struct tx_mdata) == 16);

/* 64-byte Tx instruction format.
 * Format of instruction for a 64-byte mode input queue.
 *
 * only first 16-bytes (dptr and ih) are mandatory; rest are optional
 * and filled by the driver based on firmware/hardware capabilities.
 * These optional headers together called Front Data and its size is
 * described by ih->fsz.
 */
struct octep_vf_tx_desc_hw {
	/* Pointer where the input data is available. */
	u64 dptr;

	/* Instruction Header. */
	union {
		struct octep_vf_instr_hdr ih;
		u64 ih64;
	};

	union  {
		u64 txm64[2];
		struct tx_mdata txm;
	};

	/* Additional headers available in a 64-byte instruction. */
	u64 exhdr[4];
};

static_assert(sizeof(struct octep_vf_tx_desc_hw) == 64);

#define OCTEP_VF_IQ_DESC_SIZE (sizeof(struct octep_vf_tx_desc_hw))
#endif /* _OCTEP_VF_TX_H_ */
