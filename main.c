// Tempurature and ADC module
// Grant Sparks for EE 588
// Lab 4 Code Example

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

static uint32_t uADCvalue, samples;

// ADC0_SEQ3_Handler
// reads into register as ADC data comes in
void ADC0IntHandler(void) {

	samples++;
	ROM_ADCSequenceDataGet(ADC0_BASE, 3, &uADCvalue);
	ROM_ADCIntClear(ADC0_BASE, 3);
}

int setupUART(void) {
	// enable UART 0 module on GPIOA
	ROM_SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOA );
	ROM_GPIOPinConfigure( GPIO_PA0_U0RX );
	ROM_GPIOPinConfigure( GPIO_PA1_U0TX );
	
	ROM_SysCtlPeripheralEnable( SYSCTL_PERIPH_UART0 );
	ROM_GPIOPinTypeUART( GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
	
	ROM_UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
	UARTStdioConfig(0, 115200, 16000000);
	
	UARTprintf("UART Initialized\n");
	return 0;
}
int printTemperatures(uint32_t raw) {
	uint32_t f, c;
	UARTprintf("RAW: %u  ", raw);
	// http://www.ti.com/lit/ug/spmu357b/spmu357b.pdf (pp. 11)
	// from the TI manual and the ADC code for tempurature
	// 2**12 == 4096 steps, voltage falls with higher temps
	c = (1475 - (2250*raw)/4095) / 10;
	f = (( c * 9 ) / 5) + 32;
	UARTprintf("%dF %dC", f, c);
	//UARTprintf("  %ds", samples);
	UARTprintf("\n");
	return 0;
}

int main(void) {
	uint32_t uADClast;
	
	setupUART();
	
	// enable the ADC 0 module (slide 6)
	ROM_SysCtlPeripheralEnable( SYSCTL_PERIPH_ADC0 );
	
	// wait for the ADC 0 module to initialize (slide 6)
	while( !ROM_SysCtlPeripheralReady( SYSCTL_PERIPH_ADC0 ) ) {
		ROM_SysCtlDelay(1); // do nothing
	}
	
	// configure sequence #3 (slide 8)
	// sequence #3 because it has only 1 sample
	// ADC_TRIGGER_ALWAYS will cause it to constantly read in
	// priority level 3 because this reads all the time
	ROM_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 3);
	
	// configure step #0 in sequence #3 (slide 9)
	// step #0 is the only step available (slide 8)
	// step should read from the temperature sensor (ADC_CTL_TS)
	//   and should cause an interrupt (ADC_CTL_IE)
	ROM_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);
	
	// enable interrupts prior to starting sequencer
	// connect ADC interrput handler to the interrupt table
	ROM_ADCIntEnable(ADC0_BASE, 3);
	ROM_ADCIntClear(ADC0_BASE, 3);
	ADCIntRegister(ADC0_BASE, 3, &ADC0IntHandler);
	
	// Tune ADC clock down
	ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PIOSC, ADC_CLOCK_RATE_EIGHTH);
	ROM_ADCHardwareOversampleConfigure(ADC0_BASE, 64);
	
	// enable sequencer (slide 10)
	ROM_ADCSequenceEnable(ADC0_BASE, 3);
	ROM_ADCProcessorTrigger(ADC0_BASE, 3);
	
	while(true) {
		if( uADClast != uADCvalue ) {
			printTemperatures(uADCvalue);
			uADClast = uADCvalue;
		}
		ROM_SysCtlDelay(1000000);
		ROM_ADCProcessorTrigger(ADC0_BASE, 3);
	}
	return(0);
}
