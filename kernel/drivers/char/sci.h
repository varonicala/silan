//#define sci_base_addr 0xbfbca000
//#define sl_w8(reg,val)  sl_writeb(val,reg)

#define sl_w8(reg,val)  sl_writeb(val,pRdrExt->sci_base_addr+reg)
#define sl_w16(reg,val)  sl_writew(val,pRdrExt->sci_base_addr+reg)

#define sl_r8(reg)  sl_readb(pRdrExt->sci_base_addr+reg)
#define sl_r16(reg)  sl_readw(pRdrExt->sci_base_addr+reg)

#define SCIDATA       0x000
#define SCICR0        0x004
#define SCICR1        0x008
#define SCICR2        0x00c
#define SCICLKICC     0x010
#define SCIVALUE      0x014
#define SCIBAUD       0x018
#define SCITIDE       0x01c
#define SCISTABLE     0x020
#define SCIATIME      0x024
#define SCIDTIME      0x028
#define SCIATRSTIME   0x02c
#define SCIATRDTIME   0x030
#define SCIRETRY      0x034
#define SCICHTIMELS   0x038
#define SCICHTIMEMS   0x03c
#define SCIBLKTIMELS  0x040
#define SCIBLKTIMEMS  0x044
#define SCICHGUARD    0x048
#define SCIBLKGUARD   0x04c
#define SCIFIFOSTATUS 0x050
#define SCITXCOUNT    0x054
#define SCIRXCOUNT    0x058
#define SCIIMSC       0x05c
#define SCIRIS        0x060
#define SCIMIS        0x064
#define SCIICR        0x068
