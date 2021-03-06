/*
 * low_level_op.c
 *
 *  Created on: Apr 2, 2012
 *      Author: lucas.russo
 */

#include "low_level_op.h"
#include "common.h"
#include "xparameters.h"

/* align the buffer to 16-byte boundary. DMA engine requires this for DDC samples */
/* There is an error associated to this construct. Data corruption! */
//u32 dma_buf[MAX_DMA_TRANSFER_SIZE/4] __attribute__((aligned(16)));

/***************** Structures for controlling devices *****************/
/* DMA control structutes */
struct low_level_attr ddc_dma_attr = {
	.sample_size = DDC_SAMPLE_SIZE,
	.mem_start_addr = (char *) DDR3_DMA_DDC_BUFFER,
	.mem_size = MAX_DMA_TRANSFER_SIZE,
	.max_resp_buffer_packets = RESPONSE_PACKET_MAX_DDC_SAMPLES
};

struct low_level_attr fmc150_adc_dma_attr = {
	.sample_size = ADC_SAMPLE_SIZE,
	.mem_start_addr = (char *) DDR3_DMA_ADC_BUFFER,
	.mem_size = MAX_DMA_TRANSFER_SIZE,
	.max_resp_buffer_packets = RESPONSE_PACKET_MAX_ADC_SAMPLES
};

/* samples_count and samples_count_pos should not be visible to the other layer! FIX this*/
struct low_level_handler ddc_low_level_handler = {
	.id = DDC_ID,
	.baseaddr = (char *)BPM_DDC_BASEADDR,				/* capture_ctl register within BPM DDC pcore */
	.attr = &ddc_dma_attr,
	.samples_count = 0,
	.samples_count_pos = 0
};

/* samples_count and samples_count_pos should not be visible to the other layer! FIX this*/
struct low_level_handler fmc150_adc_low_level_handler = {
	.id = ADC_ID,
	.baseaddr = ((char *)FMC150_BASEADDR + 0x020),		/* capture_ctl register within FMC150 pcore */
	.attr = &fmc150_adc_dma_attr,
	.samples_count = 0,
	.samples_count_pos = 0
};

/* Specific command attribute structutes */
const struct command_attr fmc150_comm_attr = {
	.max_samples = MAX_ADC_SAMPLES_COUNT,
	.sample_size = ADC_SAMPLE_SIZE,
};

const struct command_attr ddc_comm_attr = {
	.max_samples = MAX_DDC_SAMPLES_COUNT,
	.sample_size = DDC_SAMPLE_SIZE,
};

const struct command_attr fmc150_ddc_comm_attr = {
	.max_samples = MAX_ADC_DDC_SAMPLES_COUNT,
	.sample_size = ADC_DDC_SAMPLE_SIZE,
};

/* Dummy structure */
const struct command_attr soft_reg_attr = {
	.max_samples = 0,
	.sample_size = 0,
};

/* This structure is not used for the led command. Just fill with zeros */
const struct command_attr led_comm_attr = {
	.max_samples = 0,
	.sample_size = 0,
};

/* Specific command operations */
const struct command_ops fmc150_comm_ops = {
	.capture_samples = capture_samples,
	.get_samples = get_samples,
	.read_reg = read_fmc150_register,
	.write_reg = write_fmc150_register
};

/* Caution with the null pointer function */
const struct command_ops ddc_comm_ops = {
	.capture_samples = capture_samples,
	.get_samples = get_samples,
	.read_reg =  read_reg_dummy,
	.write_reg =  write_reg_dummy
};

const struct command_ops soft_reg_ops = {
	.capture_samples = capture_samples_dummy,
	.get_samples = get_samples_dummy,
	.read_reg =  read_soft_register,
	.write_reg =  write_soft_register
};

const struct command_ops led_comm_ops = {
	.capture_samples = capture_samples_dummy,
	.get_samples = get_samples_dummy,
	.read_reg = read_reg_dummy,
	.write_reg = write_reg_dummy
};
/***************** End of Structures for controlling devices *****************/

/* Lookup for the correct handler to associate */
int get_low_level_handler(unsigned int comm, struct low_level_handler **low_level_handler){
	int error = SUCCESS;

	switch(comm){
		/*case FMC150_ADC_DELAY:*/
		case FMC150_CAPTURE:
		case FMC150_GET_SAMPLES:
		case FMC150_READ:
		case FMC150_WRITE:
			*low_level_handler = &fmc150_adc_low_level_handler;
			break;

		case DDC_CAPTURE:
		case DDC_GET_SAMPLES:
		/*case DDC_READ:
		case DDC_WRITE:*/
			*low_level_handler = &ddc_low_level_handler;
			break;

		/* no need for low_level_handler */
		case LED_READ:
		case LED_WRITE:
		case SOFT_REG_READ:
		case SOFT_REG_WRITE:
			*low_level_handler = NULL;
			break;

		/* No handler available or undefined command */
		default:
			error = ERROR;
	}

	return error;
}

/* Delay of 10000*counts clock cycles = 100us for a 100MHz clock*/
void delay(int counts){
	int i;
	for(i = 0; i < counts*10000; i++){
		asm("nop");
	}
}

int update_fmc150_adc_delay(u8 adc_strobe_delay, u8 adc_cha_delay, u8 adc_chb_delay)
{
    u32 aux_value;

    aux_value = (u32)adc_strobe_delay + (((u32)adc_cha_delay) << 8) + (((u32)adc_chb_delay) << 16);

    XIo_Out32(FMC150_BASEADDR+OFFSET_FMC150_ADC_DELAY*0x4, aux_value);
    XIo_Out32(FMC150_BASEADDR+OFFSET_FMC150_FLAGS_PULSE_0*0x4, 0x1);

    return SUCCESS;
}

int capture_samples(u32 qw_count, struct low_level_handler *low_lev_handler) {

	int error;
	low_lev_handler->samples_count_pos = 0;
	low_lev_handler->samples_count = qw_count;
	/* Workaround. Try to fix this ASAP */
	u32 qw_count_fix = (low_lev_handler->id == ADC_ID) ? qw_count : qw_count-1;

	/* Clear status registers before proceeding. Just a test!!! FIXME */
	if(low_lev_handler->id == ADC_ID)
		write_soft_register(0x0, FMC150_BASEADDR + FMC150_STATUS_REG_OFFSET, 0);
	else if(low_lev_handler->id == DDC_ID)
		write_soft_register(0x0, BPM_DDC_BASEADDR + BPM_DDC_STATUS_REG_OFFSET, 0);

#ifdef LOW_LEV_DEBUG
	xil_printf("capture_samples: samples_count_pos = %d\n", low_lev_handler->samples_count_pos);
	xil_printf("capture_samples: samples_count = %d\n", low_lev_handler->samples_count);
#endif

    //XAxiDma_Reset(&low_lev_handler->attr->AxiDma);

	// Check if DMA has completed its reset
	/*if(XAxiDma_ResetIsDone(&low_lev_handler->attr->AxiDma) == 0){
#ifdef LOW_LEV_DEBUG
	xil_printf("DMA has not completed its reset!\n");
#endif
		return ERROR;
	}

	XAxiDma_IntrEnable(&low_lev_handler->attr->AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);*/

   /* Note the fact that each sample is SAMPLE_SIZE*8-bit (SAMPLE_SIZE bytes) large. Therefore we have (qw_count * SAMPLE_SIZE) bytes */
	//xil_printf("mem start_addr = %08X\n", low_lev_handler->attr->mem_start_addr);
  if((error = XAxiDma_SimpleTransfer(&low_lev_handler->attr->AxiDma, (u32 *)/*dma_buf*/(low_lev_handler->attr->mem_start_addr), qw_count*(low_lev_handler->attr->sample_size), XAXIDMA_DEVICE_TO_DMA)) != XST_SUCCESS){
	  xil_printf("Error transferring data to buffer! Error: %s\n", (error == XST_FAILURE) ? "XST_FAILURE" : "XST_INVALID_PARAM");
	  return ERROR;
  }

  // start the adc capture. 0 -> 1 transition on bit #21
  XIo_Out32(low_lev_handler->baseaddr, 0x00000000);
  //delay(100);
  //xil_printf("capture_ctl register reg: %08X\n", XIo_In32(low_lev_handler->baseaddr));
  /* Here we have # of SAMPLE_SIZE*8-bit samples (= SAMPLE_SIZE samples) */
  /* Check for the qw_count - 1 !!!!*/
  XIo_Out32(low_lev_handler->baseaddr, (0x00200000 | qw_count_fix));
  //delay(100);
  //xil_printf("capture_ctl register reg: %08X\n", XIo_In32(low_lev_handler->baseaddr));

  return SUCCESS;
}

/* Should be called after adc_fmc150_capture() */
int get_samples(u32 *size, u32 *byte_offset, struct low_level_handler *low_lev_handler)
{
	int i;
	s32 tmp_size = 1;

#ifdef LOW_LEV_DEBUG
	xil_printf("get_samples: trying to get %d samples...\n", *size);
#endif

	/*XAxiDma_Reset(&low_lev_handler->attr->AxiDma);
	// Check if DMA has completed its reset
	if(XAxiDma_ResetIsDone(&low_lev_handler->attr->AxiDma) == 0){
#ifdef LOW_LEV_DEBUG
	xil_printf("DMA has not completed its reset!\n");
#endif
		return ERROR;
	}*/

	/* Resume DMA transactions if halted */
	/*if ((error = XAxiDma_Resume(&low_lev_handler->attr->AxiDma)) != XST_SUCCESS){
#ifdef LOW_LEV_DEBUG
	xil_printf("DMA has not resumed! Error: %s\n", (error == XST_NOT_SGDMA) ? "XST_NOT_SGDMA" : "XST_DMA_ERROR");
#endif
		return ERROR;
	}
	else{
#ifdef LOW_LEV_DEBUG
	xil_printf("Enabling DMA interrupts\n");
#endif
		Enable DMA interrupts
	    XAxiDma_IntrEnable(&low_lev_handler->attr->AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	}*/

	/* Trying to read more than available in buffer */
	if(low_lev_handler->samples_count_pos + *size > low_lev_handler->samples_count){
		tmp_size = low_lev_handler->samples_count - low_lev_handler->samples_count_pos;
#ifdef LOW_LEV_DEBUG
		xil_printf("get_samples: samples_count_pos = %d\n", low_lev_handler->samples_count_pos);
		xil_printf("get_samples: samples_count = %d\n", low_lev_handler->samples_count);
		xil_printf("get_samples: Size from adc_fmc150_get_samples trimmed to %d\n", tmp_size);
#endif
	}

	/* No more data to read from buffer! Issue a new capture from adc.
	 * Tricky construct*/
	if(tmp_size <= 0 && (tmp_size = *size) && capture_samples(*size, low_lev_handler) < 0){
		xil_printf("Could not acquire samples\n");
		return ERROR;
	}

	/* FIX this! Decouple modules! and improve clarity*/
	*size = (tmp_size > low_lev_handler->attr->max_resp_buffer_packets) ?
			low_lev_handler->attr->max_resp_buffer_packets : tmp_size;

	for(i = 0; i < MAX_DMA_TRIES; ++i){
		if(XAxiDma_Busy(&low_lev_handler->attr->AxiDma, XAXIDMA_DEVICE_TO_DMA) != TRUE){
#ifdef LOW_LEV_DEBUG
	xil_printf("get_samples: DMA has completed!\n");
#endif
			break;

		}

		delay(1000);
	}

	if(i == MAX_DMA_TRIES){
#ifdef LOW_LEV_DEBUG
	xil_printf("get_samples: DMA has failed to deliver data!\n");
#endif
		//XAxiDma_Reset(&low_lev_handler->attr->AxiDma);
		return ERROR;
	}

	/* Check for DMA errors explicitly */
	//xil_printf("DMA ADC status reg: %08X\n", XIo_In32(BPM_ADC_DMA_BASEADDR + 0x034));

	*byte_offset = low_lev_handler->samples_count_pos * low_lev_handler->attr->sample_size;
	low_lev_handler->samples_count_pos += *size;

#ifdef LOW_LEV_DEBUG
	xil_printf("get_samples: *byte_offset = %d\n", *byte_offset);
	xil_printf("get_samples: samples_count_pos = %d\n", low_lev_handler->samples_count_pos);
	xil_printf("get_samples: *size = %d\n", *size);
#endif

	return SUCCESS;
}

int read_fmc150_register(u32 chipselect, u32 addr, volatile u32* value)
{
    volatile u32 aux_value;

    if ((XIo_In32(FMC150_BASEADDR+
    		OFFSET_FMC150_FLAGS_OUT_0*0x4) & MASK_AND_FLAGSOUT0_SPI_BUSY) != MASK_AND_FLAGSOUT0_SPI_BUSY)
    {
        aux_value = XIo_In32(FMC150_BASEADDR+OFFSET_FMC150_FLAGS_IN_0*0x4);
        XIo_Out32(FMC150_BASEADDR+OFFSET_FMC150_FLAGS_IN_0*0x4, aux_value | MASK_OR_FLAGSIN0_SPI_READ);
        XIo_Out32(FMC150_BASEADDR+OFFSET_FMC150_ADDR*0x4, addr);

        aux_value = XIo_In32(FMC150_BASEADDR+OFFSET_FMC150_CHIPSELECT*0x4) ^ chipselect;
        XIo_Out32(FMC150_BASEADDR+OFFSET_FMC150_CHIPSELECT*0x4, aux_value);

        /* Should test in order to see if this delay could be less than 10 */
        delay(10);

        *value = XIo_In32(FMC150_BASEADDR+OFFSET_FMC150_DATAOUT*0x4);

        return SUCCESS;
    }
    else
        return ERROR;
}

int write_fmc150_register(u32 chipselect, u32 addr, u32 value)
{
    volatile u32 aux_value;

    aux_value = XIo_In32(FMC150_BASEADDR+OFFSET_FMC150_FLAGS_IN_0*0x4);
    XIo_Out32(FMC150_BASEADDR+OFFSET_FMC150_FLAGS_IN_0*0x4, aux_value & MASK_AND_FLAGSIN0_SPI_WRITE);
    XIo_Out32(FMC150_BASEADDR+OFFSET_FMC150_ADDR*0x4, addr);
    XIo_Out32(FMC150_BASEADDR+OFFSET_FMC150_DATAIN*0x4, value);

    aux_value = XIo_In32(FMC150_BASEADDR+OFFSET_FMC150_CHIPSELECT*0x4) ^ chipselect;
    XIo_Out32(FMC150_BASEADDR+OFFSET_FMC150_CHIPSELECT*0x4, aux_value);

    return SUCCESS;
}

int read_soft_register(u32 chipselect, u32 addr, volatile u32 *value)
{
#ifdef LOW_LEV_DEBUG
	xil_printf("read_soft_register: addr = %08X\n", addr);
#endif
	*value = XIo_In32(addr);
#ifdef LOW_LEV_DEBUG
	xil_printf("read_soft_register: value read = %08X\n", *value);
#endif
	return SUCCESS;
}

int write_soft_register(u32 chipselect, u32 addr, u32 value)
{
#ifdef LOW_LEV_DEBUG
	xil_printf("write_soft_register: addr = %08X\n", addr);
	xil_printf("write_soft_register: value to be written = %08X\n", value);
#endif
	XIo_Out32(addr, value);
	return SUCCESS;
}

int led_read(u32 chipselect, u32 addr, volatile u32* value){
	*value = XIo_In32(LED_BASEADDR);
	return SUCCESS;
}

int led_write(u32 chipselect, u32 addr, u32 value){
	XIo_Out32(LED_BASEADDR, value);
	return SUCCESS;
}

/* Dummy functions for placeholder only */
int capture_samples_dummy(u32 qw_count, struct low_level_handler *low_lev_handler)
{
	xil_printf("capture_samples not implemented for this device\n");
	return NOFUNC;
}

int get_samples_dummy(u32 *size, u32 *byte_offset, struct low_level_handler *low_lev_handler)
{
	xil_printf("get_samples not implemented for this device\n");
	return NOFUNC;
}

int read_reg_dummy(u32 chipselect, u32 addr, volatile u32* value)
{
	xil_printf("read_reg not implemented for this device\n");
	return NOFUNC;
}

int write_reg_dummy(u32 chipselect, u32 addr, u32 value)
{
	xil_printf("write_reg not implemented for this device\n");
	return NOFUNC;
}
