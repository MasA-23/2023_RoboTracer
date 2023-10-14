//################################################
// ラズピコキット(Raspberry Pi Pico Kit) C-Version
// mcp3208自作関数 (2023.1.14～)
//################################################

//************************************************
// ピン定義
//************************************************
#define PIN_MISO 0
#define PIN_CS   1
#define PIN_SCK  6
#define PIN_MOSI 7
#define SPI_PORT spi0

//************************************************
// 
//************************************************
static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);
    asm volatile("nop \n nop \n nop");
}

void setup_SPI(){
    // This example will use SPI0 at 0.5MHz.
    spi_init(SPI_PORT, 500 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
}

int readADC(uint8_t ch){
    uint8_t writeData[] = {0b00000110, 0x00, 0xff};
    switch(ch){
      case 0:
        writeData[0] = 0b00000110;
        writeData[1] = 0b00000000; 
      break;
      case 1:
        writeData[0] = 0b00000110;
        writeData[1] = 0b01000000; 
      break;
      case 2:
        writeData[0] = 0b00000110;
        writeData[1] = 0b10000000; 
      break;
      case 3:
        writeData[0] = 0b00000110;
        writeData[1] = 0b11000000; 
      break;
      case 4:
        writeData[0] = 0b00000111;
        writeData[1] = 0b00000000; 
      break;
      case 5:
        writeData[0] = 0b00000111;
        writeData[1] = 0b01000000; 
      break;
      case 6:
        writeData[0] = 0b00000111;
        writeData[1] = 0b10000000; 
      break;
      case 7:
        writeData[0] = 0b00000111;
        writeData[1] = 0b11000000; 
    }
    // printf("\n %0b %0b %0b\n",writeData[0],writeData[1],writeData[2]);
    uint8_t buffer[3];
    cs_select();
    sleep_us(4);
    spi_write_read_blocking(SPI_PORT, writeData, buffer, 3);
    sleep_us(4);
    cs_deselect();

    return (buffer[1] & 0x0f) << 8 | buffer[2];
}