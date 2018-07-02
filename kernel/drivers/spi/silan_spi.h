/********SPI CTL*********/
#define RESET			(1<<0)
#define RISC_ACCESS 	(1<<4)
#define PROGRAM_END	(1<<5)
#define SEL_BANK2		(1<<10)

/******SPI CMD***********/
#define CMD_TYPE(x)		(x<<8)

/*****SPI STATUS*********/
#define SPI_IDLE		(1<<0)
#define DATA_READY		(1<<1)
#define READY_FOR_NEXT	(1<<2)

/****REGISTER ADDR******/ 
#define	SPI_CTRL 0x00
#define SPI_CMD 0x100
#define SPI_ADDR 0x200
#define SPI_DATA 0x300
#define SPI_STATUS 0x400

#define SPI_MAP_ADDR 0xbfbb0000

#define DOUBLE_READ (0x1<<11)
#define QUAL_READ	(0x1<<12)	
struct silan_spi_info {
	int			 pin_cs;	/* simple gpio cs */
	unsigned int		 num_cs;	/* total chipselects */
	int			 bus_num;       /* bus number to use. */

	void (*gpio_setup)(struct silan_spi_info *spi, int enable);
	void (*set_cs)(struct silan_spi_info *spi, int cs, int pol);
};

