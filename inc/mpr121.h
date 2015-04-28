#ifndef MPR121_H
#define MPR121_H

/* Setup SCL and SDA pins on I2C3
 * SDA on PC9 and SCL on PA8
 */

#define SLAVE_ADDR						0x5A
#define I2Cx                           	I2C3
#define I2Cx_CLK                      	RCC_APB1Periph_I2C3
#define I2Cx_CLK_INIT               	RCC_APB1PeriphClockCmd

#define I2Cx_SDA_PIN  		           	GPIO_Pin_9
#define I2Cx_SDA_GPIO_PORT              GPIOC                       
#define I2Cx_SDA_GPIO_CLK               RCC_AHB1Periph_GPIOC
#define I2Cx_SDA_SOURCE                 GPIO_PinSource9
#define I2Cx_SDA_AF                     GPIO_AF_I2C3

#define I2Cx_SCL_PIN                    GPIO_Pin_8
#define I2Cx_SCL_GPIO_PORT              GPIOA                    
#define I2Cx_SCL_GPIO_CLK               RCC_AHB1Periph_GPIOA
#define I2Cx_SCL_SOURCE                 GPIO_PinSource8
#define I2Cx_SCL_AF                     GPIO_AF_I2C3

#define MPR121_IRQ_PIN                  GPIO_Pin_6                
#define MPR121_IRQ_GPIO_PORT            GPIOB                    
#define MPR121_IRQ_GPIO_CLK             RCC_AHB1Periph_GPIOB

void init_mpr121(void);
void init_I2C(void);
void init_mpr121_gpio(void);
void I2C_start(I2C_TypeDef* I2C_, uint8_t address, uint8_t direction);
void I2C_write(I2C_TypeDef* I2C_, uint8_t data);
uint8_t I2C_read_ack(I2C_TypeDef* I2C_);
uint8_t I2C_read_nack(I2C_TypeDef* I2C_);
void I2C_stop(I2C_TypeDef* I2C_);

void set_register(uint8_t r, uint8_t v);
uint8_t mpr121_capStatusChanged(void);
uint8_t mpr121_getStatus(void);
uint8_t mpr121_readButton1(uint8_t status);
uint8_t mpr121_readButton2(uint8_t status);
void mpr121_debug_task(void *param);


// CAP THRESHOLD
#define TOU_THRESH			0x06
#define	REL_THRESH			0x0A

// ELECTRODE and CALIBRATION
#define	ELE_CFG				0x5E 
#define CAL_REG		        0xC0
#define ELE_EN         		0x02

// ELECTRODE THREASHOLD REGISTERS
#define	ELE0_T	0x41
#define	ELE0_R	0x42
#define	ELE1_T	0x43
#define	ELE1_R	0x44

// FILTER CONFIGURATION
#define FIL_CDC 	0x5C
#define	FIL_CDT 	0x5D
#define ESI_INTERVAL	0x04

// BASELINE FILTERING CONTROL
// Maximum Half Delta, Noise Half Delta, Noise Count Limit, Filter Delay Count Limit
#define MHD_R	0x2B
#define	MHD_F	0x2F
#define NHD_R	0x2C
#define	NHD_F	0x30
#define	NCL_R 	0x2D
#define	NCL_F	0x31
#define	FDL_R	0x2E
#define	FDL_F	0x32

// AUTO CONFIGURATION
#define	ATO_CFG0	0x7B
#define	ATO_CFGU	0x7D
#define	ATO_CFGL	0x7E
#define	ATO_CFGT	0x7F


#endif

