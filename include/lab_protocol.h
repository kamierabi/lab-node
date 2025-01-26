#ifndef _LAB_PROTOCOL_H
#define _LAB_PROTOCOL_H 1

// general view of protocol:
// request = [u8 version][u8 transport][u8 opcode][u8 buffer_size][any data]
// response = [u8 version][u8 transport][u8 opcode][u8 status][any data]


// General settings of the protocol
#define MAX_PACKET_SIZE                 4096
#define BUFFER_OUTPUT_SIZE              4096
#define BUFFER_ERROR_SIZE               4096

// Protocol version

#define PROTOCOL_V1                     0x01

// Transports


// Operations

#define OP_READ                         0x01
#define OP_WRITE                        0x02


// General status codes 

#define SUCCESS                         0x00
#define ERROR_INVALID_FORMAT            0x01
#define ERROR_INVALID_PROTOCOL          0x02
#define ERROR_INVALID_TRANSPORT         0x03
#define ERROR_INVALID_OPERATION         0x04
#define ERROR_MODULE_INTERNAL           0x05
#define ERROR_NO_OPERATION              0x06
#define NO_OP                           0xFF

#define LAB_STATUS uint8_t

#endif // _LAB_PROTOCOL_H