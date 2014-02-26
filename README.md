## avr-enc28j60

### Description
Library files for building embedded network system using Atmel AVR Microcontroller and Microchip ENC28J60 Ethernet Controller.

### Configuration
Configure your pin settings based on your microcontroller type. All settings are defined in `conf/config.h` file. Example :
```c
#define SPI_DDR  DDRB   // Use Data Direction Register of Port B
#define SPI_PORT PORTB  // Use Port B for Serial Peripheral Interface
#define SPI_CS   2      // Use Pin 2 for Chip Select
#define SPI_MOSI 3      // Use Pin 3 for Master Out Slave In
#define SPI_MISO 4      // Use Pin 4 for Master In Slave out
#define SPI_SCK  5      // Use Pin 5 for Serial Clock
```

Configure your network settings in your aplication file. Example :
```c
static uint8_t mymac[6]   = {0x62,0x5F,0x70,0x72,0x61,0x79};  // MAC Address
static uint8_t myip[4]    = {192,168,0,254};                  // IP Address
static uint16_t mywwwport = 80;                               // HTTP Port
```

### Compiling
Use `avr-gcc` together with `avr-binutils` and `avr-libc` to compile the source. Visit [AVR Libc Home Page](http://www.nongnu.org/avr-libc/) for more information.

### Datasheet
 - [Microchip ENC28J60](http://ww1.microchip.com/downloads/en/DeviceDoc/39662a.pdf)
 - [Atmel ATMega168](http://www.atmel.com/pt/br/Images/doc2545.pdf) (Microcontroller used in example)
