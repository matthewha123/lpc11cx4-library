#ifndef _LTC6804_H_
#define _LTC6804_H_

#include "chip.h"
#include <string.h>
#include <stdint.h>

#define LTC6804_COMMAND_LEN 4
#define LTC6804_DATA_LEN 8
#define LTC6804_CALC_BUFFER_LEN(max_modules) (LTC6804_COMMAND_LEN+LTC6804_DATA_LEN*max_modules)



/* =================== TIMING MACROS ================== */

#define T_REFUP 5
#define T_SLEEP 1800
#define T_IDLE 4
#define T_WAKE 1

/* =================== COMMAND CODES ================== */

#define WRCFG 0x001
#define RDCFG 0x002
// [TODO] precalculate Command PECs

#define RDCVA 0x004
#define RDCVB 0x006
#define RDCVC 0x008
#define RDCVD 0x00A
#define RDAUXA 0x00C
#define RDAUXB 0x00E
#define RDSTATA 0x010
#define RDSTATB 0x012

#define CLRCELL 0x711
#define CLRAUX 0x712
#define CLRSTAT 0x713
#define PLADC 0x714
#define DIAGN 0x715
#define WRCOMM 0x721
#define RDCOMM 0x722
#define STCOMM 0x723

/* =================== COMMAND BITS ================== */

#define ADC_MODE_7KHZ 0x10

#define ADC_CHN_ALL 0x0
#define ADC_CHN_1_7 0x1
#define ADC_CHN_2_8 0x2
#define ADC_CHN_3_9 0x3
#define ADC_CHN_4_10 0x4
#define ADC_CHN_5_11 0x5
#define ADC_CHN_6_12 0x6

#define PUP_DOWN 0x0
#define PUP_UP 0x1

#define ST_ONE 0x1
#define ST_TWO 0x2
#define ST_ONE_27KHZ_RES 0x9565
#define ST_ONE_14KHZ_RES 0x9553
#define ST_ONE_7KHZ_RES 0x9555
#define ST_ONE_2KHZ_RES 0x9555
#define ST_ONE_26HZ_RES 0x9555
#define ST_TWO_27KHZ_RES 0x6A9A
#define ST_TWO_14KHZ_RES 0x6AAC
#define ST_TWO_7KHZ_RES 0x6AAA
#define ST_TWO_2KHZ_RES 0x6AAA
#define ST_TWO_26HZ_RES 0x6AAA

#define NUM_CELL_GROUPS 4

/***************************************
		Public Types
****************************************/

typedef enum {
	LTC6804_ADC_MODE_FAST = 1, LTC6804_ADC_MODE_NORMAL = 2, LTC6804_ADC_MODE_SLOW = 3
} LTC6804_ADC_MODE_T;

typedef enum {
	LTC6804_ADC_NONE, LTC6804_ADC_GCV, LTC6804_ADC_OWT, LTC6804_ADC_CVST, LTC6804_ADC_GPIO
} LTC6804_ADC_STATUS_T;

static const uint8_t LTC6804_ADCV_MODE_WAIT_TIMES[] = {
	0, 2, 4, 202
};

static const uint8_t LTC6804_ADAX_MODE_WAIT_TIMES[] = {
	0, 2, 4, 202
};

static const uint16_t LTC6804_SELF_TEST_RES[] = {
	0, 0x9565, 0x9555, 0x9555
};

typedef struct {
	LPC_SSP_T *pSSP;
	uint32_t baud;
	uint8_t cs_gpio;
	uint8_t cs_pin;

	uint8_t num_modules;
	uint8_t *module_cell_count;

	uint32_t min_cell_mV;
	uint32_t max_cell_mV;

	LTC6804_ADC_MODE_T adc_mode;
} LTC6804_CONFIG_T;

typedef struct {
	Chip_SSP_DATA_SETUP_T *xf;
	uint8_t *tx_buf;
	uint8_t *rx_buf;
	uint32_t last_message;
	uint32_t wake_length;
	uint8_t *cfg;

	bool waiting;
	uint32_t wait_time;
	uint32_t flight_time;
	uint32_t last_sleep_wake;

	bool balancing;
	uint16_t *bal_list;

	LTC6804_ADC_STATUS_T adc_status;
} LTC6804_STATE_T;

typedef enum {
	LTC6804_WAITING, LTC6804_PASS, LTC6804_FAIL, LTC6804_PEC_ERROR, LTC6804_WAITING_REFUP
} LTC6804_STATUS_T;

typedef struct {
	uint32_t *cell_voltages_mV; // array size = #modules * cells/module
	uint32_t pack_cell_max_mV;
	uint32_t pack_cell_min_mV;
} LTC6804_ADC_RES_T;

typedef struct {
	uint8_t failed_module;
	uint8_t failed_wire;
} LTC6804_OWT_RES_T;


/***************************************
	Public Function Prototypes
****************************************/
/**
 * Initializes the LTC6804 Driver, SPI, and CS GPIO
 * 
 * @return always returns LTC6804_PASS
 */
LTC6804_STATUS_T LTC6804_Init(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, uint32_t msTicks);

/**
 * Write the current configuration to the LTC6804s. Clears the balance bits
 * 
 * @return always returns LTC6804_PASS
 */
LTC6804_STATUS_T LTC6804_WriteCFG(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, uint32_t msTicks);

/**
 * Verifies that the LTC6804 Configuration matches the expected configuration
 * 
 * @return true if correct, false otherwise
 */
bool LTC6804_VerifyCFG(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, uint32_t msTicks);

/**
 * Performs a Cell Voltage Self-test on the LTC6804s. Waits until the ADC is free to send the command and then continually returns LTC6804_WAITING 
 * until the ADC values are ready. Then it reads the value and checks if the test passed (LTC6804_PASS), failed (LTC6804_FAIL), or generated a 
 * communication error (LTC6804_PEC_ERROR)
 * 
 * @return LTC6804_STATUS_T
 */
LTC6804_STATUS_T LTC6804_CVST(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, uint32_t msTicks);

/**
 * Performs an open wire test on the LTC6804s, res contains the failed module/wire. Uses the ADC so it waits (LTC6804_WAITING) while the ADC is in use 
 * and during conversion time. Then it reads the value and checks if the test passed (LTC6804_PASS), failed (LTC6804_FAIL), or generated a 
 * communication error (LTC6804_PEC_ERROR)
 *
 * @param      config   The configuration
 * @param      state    The state
 * @param      res      The resource
 * @param      msTicks  The milliseconds ticks
 *
 * @return     current status of test
 */
LTC6804_STATUS_T LTC6804_OpenWireTest(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, LTC6804_OWT_RES_T *res, uint32_t msTicks);

/**
 * Updates the LTC6804 balance states to match the input array. The input array should have a bool for the each cell in each module in order.
 * For example, with two modules that have 12 cells each, the array should contain 24 boolean values starting at C0 of the first module and ending
 * at C11 of the second module
 *
 * @param      config       The configuration
 * @param      state        The state
 * @param      balance_req  The balance request
 * @param      msTicks      The milliseconds ticks
 *
 * @return     always returns LTC6804_PASS
 */
LTC6804_STATUS_T LTC6804_UpdateBalanceStates(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, bool *balance_req, uint32_t msTicks);

/**
 * Updates the GPIO state at the given GPIO on all connected LTC6804s (i.e. if you set GPIO1 high, all LTC6804 in the chain will exhibit this)
 *
 * @param      config   The configuration
 * @param      state    The state
 * @param      gpio     The gpio (between 1 and 5)
 * @param      val      The value (1 or 0)
 * @param      msTicks  The milliseconds ticks
 *
 * @return     always returns LTC6804_PASS
 */
LTC6804_STATUS_T LTC6804_SetGPIOState(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, uint8_t gpio, bool val, uint32_t msTicks);

/**
 * Get the current cell voltages. Uses the ADC so it waits (LTC6804_WAITING) while the ADC is in use and during conversion time. 
 * Then it reads the value and checks if the result passes (LTC6804_PASS), fails (LTC6804_FAIL), or generated a 
 * communication error (LTC6804_PEC_ERROR)
 *
 * @param      config   The configuration
 * @param      state    The state
 * @param      res      The result (min, max, voltages)
 * @param      msTicks  The milliseconds ticks
 *
 * @return     current status of voltage read operation
 */
LTC6804_STATUS_T LTC6804_GetCellVoltages(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, LTC6804_ADC_RES_T *res, uint32_t msTicks);

/**
 * Clear the cell voltages stored on the LTC6804s
 *
 * @param      config   The configuration
 * @param      state    The state
 * @param      msTicks  The milliseconds ticks
 *
 * @return     always returns LTC6804_PASS
 */
LTC6804_STATUS_T LTC6804_ClearCellVoltages(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, uint32_t msTicks);

/**
 * Get the current gpio voltages. Uses the ADC so it waits (LTC6804_WAITING) while the ADC is in use and during conversion time. 
 * Then it reads the value and checks if the result passes (LTC6804_PASS), fails (LTC6804_FAIL), or generated a 
 * communication error (LTC6804_PEC_ERROR)
 *
 * @param      config   The configuration
 * @param      state    The state
 * @param      res      The resource
 * @param      msTicks  The milliseconds ticks
 *
 * @return     current status of voltage read operation
 */
LTC6804_STATUS_T LTC6804_GetGPIOVoltages(LTC6804_CONFIG_T *config, LTC6804_STATE_T *state, uint32_t *res, uint32_t msTicks);


#endif
