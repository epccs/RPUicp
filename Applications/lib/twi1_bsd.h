#ifndef twi1_h
#define twi1_h

#define TWI1_BUFFER_LENGTH 32

// Some SMBus devices (Raspberry Pi Zero) can not handle clock stretching.
// An interleaving receive buffer allwows the callback to save a pointer
// to the buffer and swap to another buffer in ISR thread.
// The main thread loop has to notice and process the twi buffer befor 
// the next transaction is done.  
//#define TWI1_SLAVE_RX_BUFFER_INTERLEAVING

typedef enum TWI1_PINS_enum {
    TWI1_PINS_FLOATING,
    TWI1_PINS_PULLUP
} TWI1_PINS_t;

// TWI master write attempted.
typedef enum TWI1_WRT_enum {
    TWI1_WRT_TRANSACTION_STARTED, // Transaction started
    TWI1_WRT_TO_MUCH_DATA, // to much data
    TWI1_WRT_NOT_READY // TWI state machine not ready for use
} TWI1_WRT_t;

// TWI master write transaction status.
typedef enum TWI1_WRT_STAT_enum {
    TWI1_WRT_STAT_SUCCESS, // success
    TWI1_WRT_STAT_BUSY, // twi busy, write operation not complete
    TWI1_WRT_STAT_ADDR_NACK, // address send, NACK received
    TWI1_WRT_STAT_DATA_NACK, // data send, NACK received
    TWI1_WRT_STAT_ILLEGAL // illegal start or stop condition
} TWI1_WRT_STAT_t;

// TWI master read attempted.
typedef enum TWI1_RD_enum {
    TWI1_RD_TRANSACTION_STARTED, // Transaction started
    TWI1_RD_TO_MUCH_DATA, // to much data
    TWI1_RD_NOT_READY // TWI state machine not ready for use
} TWI1_RD_t;

void twi1_init(uint32_t bitrate, TWI1_PINS_t pull_up);

TWI1_WRT_t twi1_masterAsyncWrite(uint8_t slave_address, uint8_t *write_data, uint8_t bytes_to_write, uint8_t send_stop);
TWI1_WRT_STAT_t twi1_masterAsyncWrite_status(void);
uint8_t twi1_masterBlockingWrite(uint8_t slave_address, uint8_t* write_data, uint8_t bytes_to_write, uint8_t send_stop);

TWI1_RD_t twi1_masterAsyncRead(uint8_t slave_address, uint8_t bytes_to_read, uint8_t send_stop);
uint8_t twi1_masterAsyncRead_bytesRead(uint8_t *read_data);
uint8_t twi1_masterBlockingRead(uint8_t slave_address, uint8_t* read_data, uint8_t bytes_to_read, uint8_t send_stop);

uint8_t twi1_slaveAddress(uint8_t slave);
uint8_t twi1_fillSlaveTxBuffer(const uint8_t* slave_data, uint8_t bytes_to_send);
void twi1_registerSlaveRxCallback( void (*function)(uint8_t*, uint8_t) );
void twi1_registerSlaveTxCallback( void (*function)(void) );

#endif // twi1_h

