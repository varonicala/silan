#ifndef _QUEUE_H_
#define _QUEUE_H_
#include "mailbox.h"

int silan_mbox_init(struct silan_mbox *mbox);
void silan_mbox_fini(struct silan_mbox *mbox);

int silan_mbox_msg_send(struct silan_mbox *mbox, mbox_msg_t *msg);
#endif

