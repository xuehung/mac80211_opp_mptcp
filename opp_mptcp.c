#include <linux/kernel.h>
#include <linux/etherdevice.h>
#include <linux/spinlock.h>
#include <net/mac80211.h>

#include "ieee80211_i.h"
#include "timer.h"

// the mac addr of the AP that we help to relay
u_char bss_id[] = {0x28, 0x93, 0xfe, 0xc8, 0xa9, 0x11};
// the mac addr of the source
u_char src_addr[] = {0x64, 0x70, 0x02, 0xb5, 0x3a, 0x7b};
// the mac addr of the destination
u_char dst_addr[] = {0x84, 0x18, 0x88, 0x99, 0x54, 0x01};


int opp_mptcp_is_process_frame(struct ieee80211_hw *hw, struct sk_buff *skb)
{
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
    
    if(is_broadcast_ether_addr(hdr->addr1) || is_multicast_ether_addr(hdr->addr1))
        return 0;
    /*
    printk(KERN_INFO "opp_mptcp: addr1-> %.2X-%2.x-%2.x-%2.x-%2.x-%2.x\n", hdr->addr1[0],
            hdr->addr1[1],hdr->addr1[2],hdr->addr1[3],hdr->addr1[4],hdr->addr1[5]);
    printk(KERN_INFO "opp_mptcp: addr2-> %.2X-%2.x-%2.x-%2.x-%2.x-%2.x\n", hdr->addr2[0],
            hdr->addr2[1],hdr->addr2[2],hdr->addr2[3],hdr->addr2[4],hdr->addr2[5]);
    printk(KERN_INFO "opp_mptcp: addr3-> %.2X-%2.x-%2.x-%2.x-%2.x-%2.x\n", hdr->addr3[0],
            hdr->addr3[1],hdr->addr3[2],hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]);
    */

    // pick the packets with specific mac addrs
    if(ieee80211_is_data(hdr->frame_control)){
        if(!memcmp(hdr->addr1, src_addr, ETH_ALEN) && 
                !memcmp(hdr->addr2, bss_id, ETH_ALEN) &&
                !memcmp(hdr->addr3, dst_addr, ETH_ALEN)){
            return 1;
        }
    // pick the acks
    } else if(ieee80211_is_ack(hdr->frame_control)){
        if(!memcmp(hdr->addr2, src_addr, ETH_ALEN))
            return 1;
    }
    return 0;
}

void opp_mptcp_relay(struct ieee80211_hw *hw, struct sk_buff *skb)
{
    //printk(KERN_INFO "opp_mptcp relay!\n");
    
    struct sk_buff *saved_skb;
    struct ieee80211_local *local = hw_to_local(hw);
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
    struct ieee80211_rx_status *status = IEEE80211_SKB_RXCB(skb);
    struct ieee80211_supported_band *sband;
    int rate_idx;
    struct ieee80211_rate *rate = NULL;
    unsigned long flags;

    sband = local->hw.wiphy->bands[status->band];

    // I am not very clear what therse lines do!
    if(status->flag & RX_FLAG_HT) {
        if(WARN((status->rate_idx < 0 ||
            status->rate_idx > 76),
            "Rate marked as an HT rate but passed "
            "status->rate_idx is not "
            "an MCS index [0-76]: %d (0x%02x)\n",
            status->rate_idx,
            status->rate_idx))
            goto drop;
    } else {
        if (WARN_ON(status->rate_idx < 0 ||
               status->rate_idx >= sband->n_bitrates))
            goto drop;
        rate = &sband->bitrates[status->rate_idx];

    }

    // data frame
    if(ieee80211_is_data(hdr->frame_control)) {
        // make a copy of the original frame
        saved_skb = skb_copy(skb, GFP_ATOMIC);

        spin_lock_irqsave(&timer->qlock, flags);
        // the queue is empty
        if(likely(skb_queue_empty(&timer->coop_queue))) {
            //queue a buffer at the list head, need a lock before calling
            //this function
            __skb_queue_head(&timer->coop_queue, saved_skb);

            spin_unlock_irqrestore(&timer->qlock, flags);
        }
        // should not get here too often, which means another data frame
        // before the timer expires
        else {

            spin_unlock_irqrestore(&timer->qlock, flags);
        }
        timer_setup(timer, status, rate);
    }
    // ack frame
    else if(ieee80211_is_ack(hdr->frame_control)) {
    }
    // should not get here
    else {
    }


drop:
    dev_kfree_skb(skb);
    return;
}

int opp_mptcp_init(void)
{
    int errval;
    if((errval = timer_init()))
        return errval;
    return 0;
}
