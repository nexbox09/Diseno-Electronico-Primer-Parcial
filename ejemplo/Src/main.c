#include <stdint.h>

// Define base addresses for various peripherals and registers in the STM32's memory map
#define PERIPHERAL_BASE_ADDRESS     0x40000000U
#define AHB_BASE_ADDRESS            (PERIPHERAL_BASE_ADDRESS + 0x00020000U)
#define RCC_BASE_ADDRESS            (AHB_BASE_ADDRESS + 0x00001000U)
#define RCC_IOPENR_ADDRESS          (RCC_BASE_ADDRESS + 0x0000002CU)
#define IOPORT_ADDRESS              (PERIPHERAL_BASE_ADDRESS + 0x10000000U)
#define GPIOA_BASE_ADDRESS          (IOPORT_ADDRESS + 0x00000000U)
#define GPIOB_BASE_ADDRESS          (IOPORT_ADDRESS + 0x00000400U)
#define GPIOC_BASE_ADDRESS          (IOPORT_ADDRESS + 0x00000800U)

// Define pointers to structures that represent the registers of the GPIOA, GPIOB, GPIOC, and RCC peripherals
#define GPIOA ((GPIO_RegDef_t*)GPIOA_BASE_ADDRESS)
#define GPIOB ((GPIO_RegDef_t*)GPIOB_BASE_ADDRESS)
#define GPIOC ((GPIO_RegDef_t*)GPIOC_BASE_ADDRESS)
#define RCC ((RCC_RegDef_t*)RCC_BASE_ADDRESS)

// Define and initialize global variables
uint8_t watch_state = 0x00; // Variable to store the state of a finite state machine
uint8_t inc_second = 0x00; // Variable to increment seconds for a timer or clock
uint8_t formato_24_horas = 1; // Flag to indicate whether a clock should be displayed in 24-hour format
uint8_t countdown_time = 0; // Countdown time in seconds


// Define the structure representing the GPIO registers
typedef struct
{
    uint32_t MODER;     // GPIO port mode register
    uint32_t OTYPER;    // GPIO port output type register
    uint32_t OSPEEDR;   // GPIO port output speed register
    uint32_t PUPDR;     // GPIO port pull-up/pull-down register
    uint32_t IDR;       // GPIO port input data register
    uint32_t ODR;       // GPIO port output data register
    uint32_t BSRR;      // GPIO port bit set/reset register
    uint32_t LCKR;      // GPIO port configuration lock register
    uint32_t AFR[2];    // GPIO alternate function registers
    uint32_t BRR;       // GPIO port bit reset register
} GPIO_RegDef_t;

typedef struct
{
    uint32_t CR;
    uint32_t ICSCR;
    uint32_t CRRCR;
    uint32_t CFGR;
    uint32_t CIER;
    uint32_t CIFR;
    uint32_t CICR;
    uint32_t IOPRSTR;
    uint32_t AHBRSTR;
    uint32_t APB2RSTR;
    uint32_t APB1RSTR;
    uint32_t IOPENR;
} RCC_RegDef_t;

struct Time_t {
    uint8_t hour_decimal;
    uint8_t hour_unit;
    uint8_t minute_decimal;
    uint8_t minute_unit;
    uint8_t second_decimal;
    uint8_t second_unit;
};

struct Time_t watch = {0}; 


#define cero  0x3F
#define uno   0x06
#define dos   0x5B
#define tres  0x4F
#define cuatro  0x66
#define cinco   0x6D
#define seis   0x7D
#define siete  0x07
#define ocho   0x7F
#define nueve  0x67



void delay_ms(uint16_t n); // Function to delay the program execution for n milliseconds

uint8_t decoder(uint8_t value_to_decode); // Function to decode a value using the decoder_table

void formato_hora(); // Function to format the time

// Array of decoded values for the digits 0-9
const uint8_t decoder_table[] = {cero, uno, dos, tres, cuatro, cinco, seis, siete, ocho, nueve};

// Function to set the time on the watch. The time is broken down into decimal and unit parts for hours, minutes, and seconds.
void set_time(uint8_t hour_decimal, uint8_t hour_unit, uint8_t minute_decimal, uint8_t minute_unit, uint8_t second_decimal, uint8_t second_unit) {
    watch.hour_decimal = hour_decimal; // Set the decimal part of the hour
    watch.hour_unit = hour_unit; // Set the unit part of the hour
    watch.minute_decimal = minute_decimal; // Set the decimal part of the minute
    watch.minute_unit = minute_unit; // Set the unit part of the minute
    watch.second_decimal = second_decimal; // Set the decimal part of the second
    watch.second_unit = second_unit; // Set the unit part of the second
}
// Definir una estructura para los estados del reloj
typedef struct {
    uint32_t segmento; // GPIOC segmento
    uint8_t (*decoderFunction)(uint8_t); // Función para decodificar el dígito
    uint8_t valor; // Valor a decodificar y mostrar, se actualizará en runtime
} EstadoReloj;

// Inicializar los estados del reloj con valores que pueden ser constantes
EstadoReloj estados[6] = {
    {0X01 << 5, decoder, 0}, // Valor se actualizará en runtime
    {0X01 << 6, decoder, 0}, // Valor se actualizará en runtime
    {0X01 << 8, decoder, 0}, // Valor se actualizará en runtime
    {0X01 << 9, decoder, 0}, // Valor se actualizará en runtime
    {0X01 << 7, decoder, 0}, // Valor se actualizará en runtime
    {0X01 << 4, decoder, 0}  // Valor se actualizará en runtime
};

// Función para actualizar los valores en runtime antes de usar el arreglo
void actualizarValoresEstados() {
    estados[0].valor = watch.second_unit;
    estados[1].valor = watch.second_decimal;
    estados[2].valor = watch.minute_unit;
    estados[3].valor = watch.minute_decimal;
    estados[4].valor = watch.hour_unit % 10;
    estados[5].valor = watch.hour_decimal;
}

int main(void)
{
    //Initialize the clock for the GPIOA, GPIOB, and GPIOC peripherals
    RCC->IOPENR |= (1 << 2) | (1 << 1) | (1 << 0);
        
    //create a a¿mask to set the pins as output in the GPIOC 
    uint32_t portC_masks = (0b01 << 8) |(0b01 << 10) | (0b01 << 12) | (0b01 << 14) | (0b01 << 16) | (0b01 << 18);

    //Restart the MODER register to set the pins as output
    GPIOC->MODER &= ~(0b11 << 8 | 0b11 << 10 | 0b11 << 12 | 0b11 << 14 | 0b11 << 16 | 0b11 << 18);
    GPIOC->MODER |= portC_masks;

    //Create a bitmask to set the pins as output in the GPIOB
    uint32_t portB_masks = (0b01 << 0) | (0b01 << 2) | (0b01 << 4) | (0b01 << 6) |
                           (0b01 << 8) | (0b01 << 10) | (0b01 << 12) | (0b01 << 14);
    
    //Restart the MODER register to set the pins as output
    GPIOB->MODER &= ~(0xFFFF); 
    GPIOB->MODER |= portB_masks;

    //Create a bitmask to set the pins as input in the GPIOC for the buttons PC13 and PC3
    uint32_t portC_masks_input = (0b00 << 26) | (0b00 << 6) | (0b00 << 20) | (0b00 << 24); //Pines PC13,PC3, PC10 Y PC12 como entrada

    GPIOC->MODER &= ~(0b11 << 26) ; 
    GPIOC->MODER &= ~(0b11 << 6);
    GPIOC->MODER &= ~(0b11 << 24);
    GPIOC->MODER &= ~(0b11 << 20); 
    GPIOC->MODER |= portC_masks_input;

    //Create a bitmask to set the pins as input in the GPIOA for the buttons PA0 and PA10
    uint32_t portA_masks_input = (0b00 << 20) | (0b00 << 0); // PA10 como entrada y PA0 como entrada

    GPIOA->MODER &= ~(0b11 << 20); 
    GPIOA->MODER &= ~(0b11 << 0);  
    GPIOA->MODER |= portA_masks_input; 

    // Set the time to 12:00:00 as given in the problem statement 
    set_time(0, 0, 0, 0, 0, 0);

    while (1)
    {
        // This function checks if the button connected to pin PCA10 is pressed. If the button is pressed
        // it toggles the time format between 12-hour and 24-hour. It also performs debouncing by adding
        // a delay of 100 milliseconds.
        if ((GPIOA->IDR & (1 << 10)) == 0) 
{
    uint8_t temp_hour = watch.hour_decimal * 10 + watch.hour_unit;
    
    // Convertir directamente entre formatos 24 y 12 horas sin pasos intermedios redundantes
    if (formato_24_horas) // Si actualmente está en formato 24 horas
    {
        if (temp_hour == 0) temp_hour = 12; // Medianoche en formato 12 horas
        else if (temp_hour > 12) temp_hour -= 12; // Convertir a formato PM
    }
    else // Si actualmente está en formato 12 horas
    {
        if (temp_hour == 12) temp_hour = 0; // Mediodía en formato 24 horas
        else temp_hour = (temp_hour % 12) + 12; // Convertir a formato 24 horas, ajustando las horas PM correctamente
    }

    set_time(temp_hour / 10, temp_hour % 10, watch.minute_decimal, watch.minute_unit, watch.second_decimal, watch.second_unit);

    formato_24_horas = !formato_24_horas; // Cambiar el formato

    delay_ms(50); // Retardo para evitar rebotes o cambios rápidos
}


        //This function checks if a button connected to pin PA0 is pressed. If the button is pressed,
        //it increments the minutes in the watch structure and starts a 10-second countdown. It also
        // performs debouncing by adding a delay of 100 milliseconds. If the units of minutes reach 10,
        // it resets the units to 0 and increments the tens of minutes. If the tens of minutes reach 6,
        // it resets the tens to 0.

        if ((GPIOA->IDR & (1 << 0)) == 0) //  If the button connected to PA0 is pressed
        {
            // increase minutes
            watch.minute_unit++;

            
            // Verificar si se necesita llevar a cabo un carry en los minutos
            if (watch.minute_unit == 10)
            {
                watch.minute_unit = 0; // Reiniciar las unidades de minuto a cero
                watch.minute_decimal++; // Incrementar las decenas de minuto

                // Verificar si se necesita llevar a cabo un carry en las decenas de minuto
                if (watch.minute_decimal == 6)
                {
                    watch.minute_decimal = 0; // Reiniciar las decenas de minuto a cero
                }
            }

           // Debounce delay
            delay_ms(50);
        }

        if ((GPIOC->IDR & (1 << 3)) == 0) // If the button connected to PC3 is pressed
        {
            // Decrement the minutes
            if (watch.minute_unit == 0) {
            watch.minute_unit = 9; // If the units of minute is 0, set it to 9
            if (watch.minute_decimal > 0) {
            watch.minute_decimal--; // If the tens of minute is not 0, decrement it
            }
        } else {
        watch.minute_unit--; // If the units of minute is not 0, decrement it
        }

    // Debounce delay
    delay_ms(50);
        }
    if ((GPIOC->IDR & (1 << 10)) == 0) // If the button connected to PC10 is pressed
    {
     // Increment the hours
        watch.hour_unit++;
        if (watch.hour_unit == 10) {
        watch.hour_unit = 0;
        watch.hour_decimal++;
            if (watch.hour_decimal == 2 && watch.hour_unit > 3) { // If the time is 24 hours
            watch.hour_decimal = 0;
            watch.hour_unit = 0;
            } else if (watch.hour_decimal == 10) { // If the time is more than 24 hours
            watch.hour_decimal = 0;
            }
        } else if (watch.hour_decimal == 2 && watch.hour_unit > 3) { // If the time is 24 hours
        watch.hour_decimal = 0;
        watch.hour_unit = 0;
        }

    // Debounce delay
    delay_ms(100);
    }
    if ((GPIOC->IDR & (1 << 12)) == 0) // If the button connected to PC12 is pressed
    {
        // Decrement the hours
        if (watch.hour_unit == 0) {
        watch.hour_unit = 9;
            if (watch.hour_decimal > 0) {
            watch.hour_decimal--;
                } else {
            watch.hour_decimal = 2;
            watch.hour_unit = 3;
                }
            } else {
            watch.hour_unit--;
    }

    // Debounce delay
    delay_ms(100);
}
        // call the function to format the time
        formato_hora();
    }
}
void formato_hora()
{
    // clear all the pins of the GPIOB and GPIOC
    GPIOB->BSRR |= 0xFFFF0000; 
    GPIOC->BSRR |= 0xFFFF0000;
    
    //Call the function to update the values of the states
    actualizarValoresEstados();

    // Update and display the time
    if (watch_state < sizeof(estados) / sizeof(estados[0])) {
            GPIOC->BSRR |= estados[watch_state].segmento; // Encender segmento correspondiente
            GPIOB->BSRR |= estados[watch_state].decoderFunction(estados[watch_state].valor); // Encender el dígito
            watch_state = (watch_state + 1) % (sizeof(estados) / sizeof(estados[0])); // Avanzar al siguiente estado cíclicamente
        } else {
            watch_state = 0; // Reiniciar al estado inicial si se supera el número de estados
        }

    delay_ms(1);
    inc_second++;
    
// Increment the second counter
inc_second++;

// Check if a second has passed
if (inc_second >= 225) { // Assuming 1000 milliseconds for 1 second
    inc_second = 0;

    // Increment the units of seconds
    watch.second_unit++;
    if (watch.second_unit >= 10) {
        watch.second_unit = 0;

        // Increment the tens of seconds
        watch.second_decimal++;
        if (watch.second_decimal >= 6) {
            watch.second_decimal = 0;

            // Increment the units of minutes
            watch.minute_unit++;
            if (watch.minute_unit >= 10) {
                watch.minute_unit = 0;

                // Increment the tens of minutes
                watch.minute_decimal++;
                if (watch.minute_decimal >= 6) {
                    watch.minute_decimal = 0;

                    // Increment the units of hours
                    watch.hour_unit++;
                    if (watch.hour_unit >= 10) {
                        watch.hour_unit = 0;

                        // Increment the tens of hours
                        watch.hour_decimal++;
                    }

                    // Check if the hour format is 24 hours and adjust accordingly
                    if (watch.hour_decimal >= 2 && watch.hour_unit >= 4) {
                        watch.hour_decimal = 0;
                        watch.hour_unit = 0;
                    }
                }
            }
        }
    }
}

}

void delay_ms(uint16_t n) {
    uint16_t i;
    for (; n > 0; n--)
        for (i = 0; i < 250; i++)
            ;
}

uint8_t decoder(uint8_t value_to_decode)
{
    if (value_to_decode < 10)
    {
        return decoder_table[value_to_decode];
    }
    else
    {
        return 0;
    }
}
