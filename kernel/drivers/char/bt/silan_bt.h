#ifndef __BT_H__
#define __BT_H__

#define BT_DEV_NAME ("silan-bt")

#define BT_IOCTL_MAGIC  'T'

#define BT_IOCTL_INIT_UART         _IO(BT_IOCTL_MAGIC, 0)
#define BT_IOCTL_DEINIT            _IO(BT_IOCTL_MAGIC, 1)
#define BT_IOCTL_GET_RCV_LEN       _IO(BT_IOCTL_MAGIC, 2)
#define BT_IOCTL_GET_RCV_LEN_CON   _IO(BT_IOCTL_MAGIC, 3)
#define BT_IOCTL_GET_RCV_BUF_LOCK  _IO(BT_IOCTL_MAGIC, 4)
#define BT_IOCTL_FREE_RCV_BUF_LOCK _IO(BT_IOCTL_MAGIC, 5)
#define BT_IOCTL_INIT_BT_UART      _IO(BT_IOCTL_MAGIC, 6)

#endif
