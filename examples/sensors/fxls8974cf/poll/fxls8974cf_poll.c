/*
 * Copyright 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fxls8974cf_poll.c
 * @brief The fxls8974cf_poll.c file implements the ISSDK FXLS8974 sensor driver
 *        example demonstration with polling mode.
 */

/* C Library Includes */
#include <stdio.h>

/* CMSIS Includes */
#include "Driver_I2C.h"

/* ISSDK Includes */
#include "issdk_hal.h"
#include "fxls8974_drv_i2c.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define FXLS8974_DATA_SIZE 6

/*******************************************************************************
 * Constants
 ******************************************************************************/
/*! @brief Register settings for Normal (non buffered) mode. */
const registerwritelist_t cFxls8974ConfigNormal[] = {
    /* Set Full-scale range as 2G. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_2G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    /* Set Wake Mode ODR Rate as 6.25Hz. */
    {FXLS8974_SENS_CONFIG3, FXLS8974_SENS_CONFIG3_WAKE_ODR_6_25HZ, FXLS8974_SENS_CONFIG3_WAKE_ODR_MASK},
    __END_WRITE_DATA__};

/*! @brief Address of DATA Ready Status Register. */
const registerreadlist_t cFxls8974DRDYEvent[] = {{.readFrom = FXLS8974_INT_STATUS, .numBytes = 1}, __END_READ_DATA__};

/*! @brief Address of Raw Accel Data in Normal Mode. */
const registerreadlist_t cFxls8974OutputNormal[] = {{.readFrom = FXLS8974_OUT_X_LSB, .numBytes = FXLS8974_DATA_SIZE},
                                                    __END_READ_DATA__};

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
int app_main(void)
{
    int32_t status;
    uint8_t whoami;
    uint8_t dataReady;
    uint8_t data[FXLS8974_DATA_SIZE];
    fxls8974_acceldata_t rawData;

    ARM_DRIVER_I2C *I2Cdrv = &FXLS8974_I2C_DRIVER;
    fxls8974_i2c_sensorhandle_t fxls8974Driver;

    PRINTF("\r\n ISSDK FXLS8974 sensor driver example demonstration with poll mode\r\n");

    /*! Initialize the I2C driver. */
    status = I2Cdrv->Initialize(I2C_SignalEvent(FXLS8974_I2C_INDEX));
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Initialization Failed\r\n");
        return -1;
    }

    /*! Set the I2C Power mode. */
    status = I2Cdrv->PowerControl(ARM_POWER_FULL);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Power Mode setting Failed\r\n");
        return -1;
    }

    /*! Set the I2C bus speed. */
    status = I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Control Mode setting Failed\r\n");
        return -1;
    }

    /*! Initialize FXLS8974 sensor driver. */
    status = FXLS8974_I2C_Initialize(&fxls8974Driver, &FXLS8974_I2C_DRIVER, FXLS8974_I2C_INDEX, FXLS8974_I2C_ADDR,
                                     &whoami);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n Sensor Initialization Failed\r\n");
        return -1;
    }
    if ((FXLS8964_WHOAMI_VALUE == whoami) || (FXLS8967_WHOAMI_VALUE == whoami))
    {
    	PRINTF("\r\n Successfully Initialized Gemini with WHO_AM_I = 0x%X\r\n", whoami);
    }
    else if ((FXLS8974_WHOAMI_VALUE == whoami) || (FXLS8968_WHOAMI_VALUE == whoami))
    {
    	PRINTF("\r\n Successfully Initialized Timandra with WHO_AM_I = 0x%X\r\n", whoami);
    }
    else if (FXLS8962_WHOAMI_VALUE == whoami)
    {
    	PRINTF("\r\n Successfully Initialized Newstein with WHO_AM_I = 0x%X\r\n", whoami);
    }
    else
    {
    	PRINTF("\r\n Bad WHO_AM_I = 0x%X\r\n", whoami);
        return -1;
    }

    /*!  Set the task to be executed while waiting for I2C transactions to complete. */
    FXLS8974_I2C_SetIdleTask(&fxls8974Driver, (registeridlefunction_t)COMM_IDLE_FUNC, COMM_IDLE_ARG);

    /*! Configure the FXLS8974 sensor. */
    status = FXLS8974_I2C_Configure(&fxls8974Driver, cFxls8974ConfigNormal);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n FXLS8974 Sensor Configuration Failed, Err = %d\r\n", status);
        return -1;
    }
    PRINTF("\r\n Successfully Applied FXLS8974 Sensor Configuration\r\n");

    for (;;) /* Forever loop */
    {
        /*! Wait for data ready from the FXLS8974. */
        status = FXLS8974_I2C_ReadData(&fxls8974Driver, cFxls8974DRDYEvent, &dataReady);
        if (0 == (dataReady & FXLS8974_INT_STATUS_SRC_DRDY_MASK))
        {
            continue;
        }

        /*! Read new raw sensor data from the FXLS8974. */
        status = FXLS8974_I2C_ReadData(&fxls8974Driver, cFxls8974OutputNormal, data);
        if (ARM_DRIVER_OK != status)
        {
            PRINTF("\r\n Read Failed. \r\n");
            return -1;
        }

        /*! Convert the raw sensor data to signed 16-bit container for display to the debug port. */
        rawData.accel[0] = ((int16_t)data[1] << 8) | data[0];
        rawData.accel[1] = ((int16_t)data[3] << 8) | data[2];
        rawData.accel[2] = ((int16_t)data[5] << 8) | data[4];

        /* NOTE: PRINTF is relatively expensive in terms of CPU time, specially when used with-in execution loop. */
        PRINTF("\r\nX=%5d Y=%5d Z=%5d\r\n", rawData.accel[0], rawData.accel[1], rawData.accel[2]);
        ASK_USER_TO_RESUME(50); /* Ask for user input after processing 50 samples. */
    }
}