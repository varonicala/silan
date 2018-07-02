#ifndef _NETLINK_H
#define _NETLINK_H

void sendnlmsg(char *message);
int netlink_init(void);
void netlink_exit(void);

#endif

