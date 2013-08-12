#include "timer.h"
//#include "coop.h"

struct ack_timer *timer;

/**
 * ack_timer_func - timer handler
 * Executed when no ack is received for the buffered data frame
 */
static int
ack_timer_func(struct hrtimer *atimer) 
{
	struct ack_timer *timer = container_of(atimer, struct ack_timer, atimer);
	struct ieee80211_local *local = timer->local;
	struct sk_buff *skb;
	unsigned long flags;
	
	spin_lock_irqsave(&timer->qlock, flags);

	while ((skb = skb_dequeue(&timer->coop_queue))) {
		//rcu_read_lock();
		/* retransmit buffered data frame */
		//ieee80211_coop_tx(skb, local);
		//rcu_read_unlock();
	}
	spin_unlock_irqrestore(&timer->qlock, flags);
	return HRTIMER_NORESTART;
}

/**
 * timer_setup - sets up the timer
 *
 * Calculates ack timeout based on @rate of the previous data frame
 * and sets up the timer
 */
void
timer_setup(struct ack_timer *timer, struct ieee80211_rx_status *status,
	    struct ieee80211_rate *rate)
{
	unsigned int ack_time; /* in usecs */
	ktime_t ack_timeout;

	ack_timeout = ktime_set(0, 0);

	if (status->band == IEEE80211_BAND_2GHZ)
		ack_time = GET_ACK_TIMEOUT(SIFS_BG, rate->bitrate);
	else
		/* 5GHz band */
		ack_time = GET_ACK_TIMEOUT(SIFS_A, rate->bitrate);

	/* advance the timer if it is enqueued or callback function is running */
	if (hrtimer_active(&timer->atimer)) {
		ack_timeout = ktime_add_ns(ack_timeout, ack_time*NSEC_PER_USEC);
		hrtimer_forward_now(&timer->atimer, ack_timeout);
		return;
	}

	ack_timeout = ktime_add_ns(ktime_get(), ack_time*NSEC_PER_USEC);

	/* start the timer */
	hrtimer_start(&timer->atimer, ack_timeout, HRTIMER_MODE_ABS);
}

int
timer_init(void)
{
	timer = kmalloc(sizeof(struct ack_timer), GFP_KERNEL);

	if (!timer)
		return -ENOMEM;

        spin_lock_init(&timer->qlock);
	
	/* init rx/tx queue */
	skb_queue_head_init(&timer->coop_queue);

	/* init hrtimer */
	hrtimer_init(&timer->atimer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	timer->atimer.function = (void *)ack_timer_func;

	return 0;
}

void
timer_destroy(void)
{
	kfree(timer);
}
