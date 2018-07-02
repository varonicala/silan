#ifndef __LCD_H__
#define __LCD_H__

#define LCD_IOCTL_MAGIC  'L'

#define LCD_IOCTL_REFRESH_SCR  _IO(LCD_IOCTL_MAGIC, 0)
#define LCD_IOCTL_REFRESH_LINE _IO(LCD_IOCTL_MAGIC, 1)

#define LCD_WIDTH  128
#define LCD_HEIGHT 64

#endif
