#ifndef OPP_MPTCP
#define OPP_MPTCP

int opp_mptcp_is_process_frame(struct ieee80211_hw *, struct sk_buff *);
void opp_mptcp_relay(struct ieee80211_hw *, struct sk_buff *);
int opp_mptcp_init(void);

#endif
