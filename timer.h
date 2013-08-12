#ifndef __TIMER_H
#define __TIMER_H

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/skbuff.h>
#include <net/mac80211.h>

/* 
 * ACK Timeout = SIFS
 *		+ ACK transmission time in _us (14/rate in Msymbols/s)
 *		+ preamble_time (192us or 96us)
 *		+ propagation_delay (RRT) = 2us
 * XXX:		+ overheads
 *
 * rate is in units of 100 Kbps
 */
#define GET_ACK_TIMEOUT(sifs, rate) DIV_ROUND_UP(1120, rate)+192+2+100

#define SIFS_A  16 /* 802.11a */
#define SIFS_BG 10 /* 802.11b/g */

extern struct ack_timer *timer;

extern int timer_init(void);
extern void timer_destroy(void);
extern void timer_setup(struct ack_timer*, struct ieee80211_rx_status*, 
			struct ieee80211_rate*);

struct ack_timer {
#define IEEE80211_COOP_QUEUE_LIMIT 128
	struct sk_buff_head coop_queue;
	struct ieee80211_local *local;
	struct hrtimer atimer;
	spinlock_t qlock;
};

#endif
