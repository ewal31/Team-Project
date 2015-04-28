#include <stm32f4xx.h>
#include <stm32f4xx_i2c.h>
#include <stm32f4xx_gpio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"
#include "mpr121.h"

void init_mpr121(void){

    init_mpr121_gpio();
    init_I2C();

    // Disable electrodes, set operation registers
    set_register(ELE_CFG, 0x00); 

    // Controls filtering when data is > baseline.
    set_register(MHD_R, 0x01);
    set_register(NHD_R, 0x01);
    set_register(NCL_R, 0x00);
    set_register(FDL_R, 0x00);

    // Controls filtering when data is < baseline.
    set_register(MHD_F, 0x01);
    set_register(NHD_F, 0x01);
    set_register(NCL_F, 0xFF);
    set_register(FDL_F, 0x02);

    // Sets touch and release thresholds for each electrode
    set_register(ELE0_T, TOU_THRESH);
    set_register(ELE0_R, REL_THRESH);
    set_register(ELE1_T, TOU_THRESH);
    set_register(ELE1_R, REL_THRESH);
    
    // Set the Filter Configuration
    set_register(FIL_CDT, ESI_INTERVAL);

    // Enable Auto Config and auto Reconfig
    // Target = 0.9*USL = 0xB5 @3.3V
    //set_register(ATO_CFG0, 0x0B);
    //set_register(ATO_CFGU, 0xC9);          // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   
    //set_register(ATO_CFGL, 0x82);          // LSL = 0.65*USL = 0x82 @3.3V
    //set_register(ATO_CFGT, 0xB5);

    // Enable register, begin operation
    set_register(ELE_CFG, CAL_REG | ELE_EN);
}

void init_I2C(void){

    GPIO_InitTypeDef GPIO_InitStruct;
    I2C_InitTypeDef I2C_InitStruct;

    // enable clock for SCL and SDA pins
    RCC_AHB1PeriphClockCmd(I2Cx_SDA_GPIO_CLK | I2Cx_SCL_GPIO_CLK, ENABLE);
            
    GPIO_InitStruct.GPIO_Mode     = GPIO_Mode_AF;            
    GPIO_InitStruct.GPIO_Speed     = GPIO_Speed_50MHz;        
    GPIO_InitStruct.GPIO_OType     = GPIO_OType_OD;            
    GPIO_InitStruct.GPIO_PuPd     = GPIO_PuPd_NOPULL;            
            
    GPIO_InitStruct.GPIO_Pin     = I2Cx_SDA_PIN;
    GPIO_Init(I2Cx_SDA_GPIO_PORT, &GPIO_InitStruct);                    

    GPIO_InitStruct.GPIO_Pin     = I2Cx_SCL_PIN;
    GPIO_Init(I2Cx_SCL_GPIO_PORT, &GPIO_InitStruct);                                

    // Connect I2Cx pins to AF  
    GPIO_PinAFConfig(I2Cx_SDA_GPIO_PORT, I2Cx_SDA_SOURCE, I2Cx_SDA_AF);    
    GPIO_PinAFConfig(I2Cx_SCL_GPIO_PORT, I2Cx_SCL_SOURCE, I2Cx_SCL_AF);    

    // enable I2Cx
    //I2C_StretchClockCmd(I2Cx, ENABLE);
    //
    
    // enable I2Cx clock
    I2Cx_CLK_INIT(I2Cx_CLK, ENABLE);

    // Reset I2C to avoid lockups, not sure if needed
    I2C_Cmd(I2Cx, DISABLE);
    I2C_DeInit(I2Cx); 

    I2C_Cmd(I2Cx, ENABLE);

    // configure I2Cx
    I2C_InitStruct.I2C_ClockSpeed = 100000;         // 100kHz
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;            // I2C mode
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;    // 50% duty cycle --> standard
    I2C_InitStruct.I2C_OwnAddress1 = 0x0E;            // own address, not relevant in master mode
    I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;        // disable acknowledge when reading (can be changed later on)
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // set address length to 7 bit addresses
    I2C_Init(I2Cx, &I2C_InitStruct);                // init I2Cx

    
}

void init_mpr121_gpio(void){
    GPIO_InitTypeDef GPIOInit;

    RCC_AHB1PeriphClockCmd(MPR121_IRQ_GPIO_CLK, ENABLE);

    /* Initialise the irq input pin */
    GPIOInit.GPIO_Mode = GPIO_Mode_IN;
    GPIOInit.GPIO_OType = GPIO_OType_PP;
    //GPIOInit.GPIO_Speed = GPIO_Fast_Speed;
    GPIOInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIOInit.GPIO_PuPd = GPIO_PuPd_UP;

    GPIOInit.GPIO_Pin = MPR121_IRQ_PIN;
    GPIO_Init(MPR121_IRQ_GPIO_PORT, &GPIOInit);
}

/* This function issues a start condition and 
 * transmits the slave address + R/W bit
 * 
 * Parameters:
 *         I2C_         --> the I2C peripheral e.g. I2C1
 *         address     --> the 7 bit slave address
 *         direction     --> the tranmission direction can be:
 *                         I2C_Direction_Tranmitter for Master transmitter mode
 *                         I2C_Direction_Receiver for Master receiver
 */
void I2C_start(I2C_TypeDef* I2C_, uint8_t address, uint8_t direction){

    // wait until I2C_ is not busy anymore
    while(I2C_GetFlagStatus(I2C_, I2C_FLAG_BUSY));

    // Send I2C_ START condition 
    I2C_GenerateSTART(I2C_, ENABLE);
    
    // wait for I2C_ EV5 --> Slave has acknowledged start condition
    while(!I2C_CheckEvent(I2C_, I2C_EVENT_MASTER_MODE_SELECT));
    
    // Send slave Address for write 
    I2C_Send7bitAddress(I2C_, address, direction);
      
    /* wait for I2C_ EV6, check if 
     * either Slave has acknowledged Master transmitter or
     * Master receiver mode, depending on the transmission
     * direction
     */ 
    if(direction == I2C_Direction_Transmitter){
        while(!I2C_CheckEvent(I2C_, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    }
    else if(direction == I2C_Direction_Receiver){
        while(!I2C_CheckEvent(I2C_, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
    }
    
    return;
}

/* This function transmits one byte to the slave device
 * Parameters:
 *        I2C_ --> the I2C peripheral e.g. I2C1 
 *        data --> the data byte to be transmitted
 */
void I2C_write(I2C_TypeDef* I2C_, uint8_t data){
    I2C_SendData(I2C_, data);
    // wait for I2C_ EV8_2 --> byte has been transmitted
    while(!I2C_CheckEvent(I2C_, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
}

/* This function reads one byte from the slave device 
 * and acknowledges the byte (requests another byte)
 * does NOT generate stop conditiong
 */
uint8_t I2C_read_ack(I2C_TypeDef* I2C_){
    I2C_AcknowledgeConfig(I2C_, ENABLE);
    while( !I2C_CheckEvent(I2C_, I2C_EVENT_MASTER_BYTE_RECEIVED) );
    uint8_t data = I2C_ReceiveData(I2C_);
    return data;
}

/* This function reads one byte from the slave device
 * and doesn't acknowledge the recieved data 
 * nack generates stop condition after last byte received
 */
uint8_t I2C_read_nack(I2C_TypeDef* I2C_){
    I2C_AcknowledgeConfig(I2C_, DISABLE);
    I2C_GenerateSTOP(I2C_, ENABLE);
    while( !I2C_CheckEvent(I2C_, I2C_EVENT_MASTER_BYTE_RECEIVED) );
    uint8_t data = I2C_ReceiveData(I2C_);
    return data;
}

/* This funtion issues a stop condition and therefore
 * releases the bus
 */
void I2C_stop(I2C_TypeDef* I2C_){
    I2C_GenerateSTOP(I2C_, ENABLE);
}

void set_register(uint8_t r, uint8_t v){
    I2C_start(I2Cx, SLAVE_ADDR<<1, I2C_Direction_Transmitter); 
    I2C_write(I2Cx, r); 
    I2C_write(I2Cx, v); 
    I2C_stop(I2Cx); 
}

uint8_t mpr121_capStatusChanged(void){
	return !(GPIO_ReadInputDataBit(MPR121_IRQ_GPIO_PORT, MPR121_IRQ_PIN));
}

uint8_t mpr121_getStatus(void){
	uint8_t status;
	I2C_start(I2Cx, SLAVE_ADDR<<1, I2C_Direction_Receiver); 
    status = I2C_read_nack(I2Cx); 
	return status;
}

uint8_t mpr121_readButton1(uint8_t status){
    return (status & (1<<0)) ? Bit_SET : Bit_RESET;
}

uint8_t mpr121_readButton2(uint8_t status){
    return (status & (1<<1)) ? Bit_SET : Bit_RESET;
}


void mpr121_debug_task(void *param){
    while(1){
        if(mpr121_capStatusChanged()){
            uint8_t status = mpr121_getStatus();
            if(mpr121_readButton1(status)){
                usart_send_char('1');
            }

            if(mpr121_readButton2(status)){
                usart_send_char('2');
            }
            usart_send_char('M');
        }
		usart_send_char('C');
        vTaskDelay(100);
    }
}

