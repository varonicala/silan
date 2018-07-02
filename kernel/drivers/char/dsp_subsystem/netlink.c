#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/proc_fs.h>

#define BUF_SIZE  16384
#define MAX_MSGSIZE 1024
#define NETLINK_DSP		22

static struct sock *netlink_sock;
static int exit_flag = 0;
int err;
int pid;

int stringlength(char *s)
{
	int slen = 0;

	for(;*s;s++){
		slen++;
	}
	return slen;
}

void sendnlmsg(char *message)
{
	struct sk_buff *skb_1;
	struct nlmsghdr *nlh;
	int len = NLMSG_SPACE(MAX_MSGSIZE);
	int slen = 0;
	if(!message || !netlink_sock){
		return ;
	}
	skb_1 = alloc_skb(len, GFP_KERNEL);
	if(!skb_1){
		printk("netlink test: alloc skb_1 error\n");
	}
	slen = stringlength(message);
	nlh = nlmsg_put(skb_1, 0, 0, 0, MAX_MSGSIZE, 0);
	
	NETLINK_CB(skb_1).pid = 0;
	NETLINK_CB(skb_1).dst_group = 0;

	message[slen] = '\0';
	memcpy(NLMSG_DATA(nlh), message, slen+1);
	//printk("netlink test: send message '%s'  \n", (char*)NLMSG_DATA(nlh));
	netlink_unicast(netlink_sock, skb_1, pid, MSG_DONTWAIT);
}

static void recv_handler(struct sk_buff *_skb) 
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	char str[100];
	skb = skb_get(_skb);
	if(skb->len >= NLMSG_SPACE(0)){
		nlh = nlmsg_hdr(skb);
		memcpy(str, NLMSG_DATA(nlh), sizeof(str));
		printk("[Kernel] Message received:%s\n", str);
		pid = nlh->nlmsg_pid;
		kfree_skb(skb);
		sendnlmsg("I am from kernel!");
	}
}

int netlink_init(void)
{
	netlink_sock = netlink_kernel_create(&init_net, NETLINK_DSP, 0, recv_handler, NULL, THIS_MODULE);
	if(!netlink_sock) {
		printk("Failed to create netlink socket.\n");
		return -1;
	}
	//printk("#### %s %d ok####\n", __func__, __LINE__);
	return 0;
}

void netlink_exit(void)
{
	exit_flag = 1;
	if(netlink_sock != NULL){
		sock_release(netlink_sock->sk_socket);
	}
}

