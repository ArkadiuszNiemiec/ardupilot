/****************************************************
 * AS5600 Angle of Attack Class for Ardupilot platform
 * Author: Cole Mero
 * Date: 15 Feb 2021
 * File: RPM_AS5600.cpp
 * Based on: www.ams.com
 *
 * Description:  This class has been designed to
 * access the AS5600 magnetic encoder sensor to
 * read the angle of attack and record it for experimental purposes
 *
 * It was adapted from the Arduino library available for the AS5600 sensor.
 *
***************************************************/

#include <unistd.h>
#include "RPM_AS5600.h"
#include <AP_Common/AP_Common.h>
#include <GCS_MAVLink/GCS.h>


extern const AP_HAL::HAL &hal;

/**************************************************
 * Method: RPM_AS5600
 * In: none
 * Out: none
 * Description: constructor for RPM_AS5600
***************************************************/
 RPM_AS5600::RPM_AS5600()
{
    bus = 2; //Sets the bus number for the device, unclear what this number should be, trial and error to make it work
    address = 0x36; //This is the I2C address for the device, it is set by the manufacturer
}

bool RPM_AS5600::init()
{
    dev = hal.i2c_mgr->get_device(bus, address);

    if (!dev){
        return false;
    }

    WITH_SEMAPHORE(dev->get_semaphore());
    dev->set_speed(AP_HAL::Device::SPEED_LOW);
    dev->set_retries(2);

    dev->register_periodic_callback(
        50000,
        FUNCTOR_BIND_MEMBER(&RPM_AS5600::_timer, void));

    return true;
}

/* mode = 0, output PWM, mode = 1 output analog (full range from 0% to 100% between GND and VDD*/
//void RPM_AS5600::setOutPut(uint8_t mode){
//    uint8_t config_status;
//    if (!dev->read_registers(REG_CONF_LO, &config_status, 1)){
//        return;
//    }
//    if (mode == 1){
//        config_status = config_status & 0xcf;
//    }else{
//        config_status = config_status & 0xef;
//    }
//
//    if (!dev->write_register(REG_CONF_LO, config_status)){
//        return;
//    }
//}


/*******************************************************
 * Method: update
 * In: none
 * Out: value of raw angle register
 * Description: gets raw value of magnet position.
 * start, end, and max angle settings do not apply
*******************************************************/
void RPM_AS5600::update(void)
{
  WITH_SEMAPHORE(sem);

  //Gets the two relevant register values and multiplies by conversion factor to get degrees
  uint16_t angleRaw = ((highByte << 8) | lowByte)*0.087;

  AP::logger().Write("AOAR", "TimeUS, Angle", "QH", AP_HAL::micros64(), angleRaw);

  gcs().send_text(MAV_SEVERITY_INFO, "Angle: %d", angleRaw);
}


void RPM_AS5600::_timer(void)
{
    gcs().send_text(MAV_SEVERITY_INFO, "update called - regHiRead: %d regLoRead: %d", regHiRead, regLoRead);

    uint8_t low, high;

    {
        WITH_SEMAPHORE(dev->get_semaphore());

        if (!dev->read_registers(REG_RAW_ANG_HI, &high, 1)){
            return;
        }

        regHiRead = true;
        gcs().send_text(MAV_SEVERITY_INFO, "REG_RAW_ANG_HI: %d", high);

        if (!dev->read_registers(REG_RAW_ANG_LO, &low, 1)){
            return;
        }

        gcs().send_text(MAV_SEVERITY_INFO, "REG_RAW_ANG_LO: %d", low);
        regLoRead = true;
    }

    WITH_SEMAPHORE(sem);
    lowByte = low;
    highByte = high;
}


/*******************************************************
 * Method: setMaxAngle
 * In: new maximum angle to set OR none
 * Out: value of max angle register
 * Description: sets a value in maximum angle register.
 * If no value is provided, method will read position of
 * magnet. Setting this register zeros out max position
 * register.
*******************************************************/
/*uint16_t RPM_AS5600::setMaxAngle(uint16_t newMaxAngle)
{
   if(newMaxAngle == -1){
    maxAngle = getRawAngle();
  } else {
    maxAngle = newMaxAngle;
  }

  WITH_SEMAPHORE(dev->get_semaphore());

  if (!dev->write_register(REG_MANG_HI, HIGHBYTE(rawStartAngle))){
        return -1;
    }
    if (!dev->write_register(REG_MANG_LO, LOWBYTE(rawStartAngle))){
        return -1;
    }
    if (!dev->read_registers(REG_MANG_HI, &highByte, 1) || !dev->read_registers(REG_MANG_LO, &lowByte, 1)){
        return -1;
    }

    maxAngle = ((highByte << 8) | lowByte);
  return maxAngle;
}*/


/*******************************************************
 * Method: getMaxAngle
 * In: none
 * Out: value of max angle register
 * Description: gets value of maximum angle register.
*******************************************************/
uint16_t RPM_AS5600::getMaxAngle()
{
    WITH_SEMAPHORE(dev->get_semaphore());

    if (!dev->read_registers(REG_MANG_HI, &highByte, 1) || !dev->read_registers(REG_MANG_LO, &lowByte, 1)){
         return -1;
     }

     maxAngle = ((highByte << 8) | lowByte);

     return maxAngle;
}


/*******************************************************
 * Method: setStartPosition
 * In: new start angle position
 * Out: value of start position register
 * Description: sets a value in start position register.
 * If no value is provided, method will read position of
 * magnet.
*******************************************************/
/*uint16_t RPM_AS5600::setStartPosition(uint16_t startAngle = -1)
{
  if(startAngle == -1)
  {
    rawStartAngle = getRawAngle();
  } else {
    rawStartAngle = startAngle;
  }

  WITH_SEMAPHORE(dev->get_semaphore());

  if (!dev->write_register(REG_ZPOS_HI, HIGHBYTE(rawStartAngle))){
      return -1;
  }
  if (!dev->write_register(REG_ZPOS_LO, LOWBYTE(rawStartAngle))){
      return -1;
  }
  if (!dev->read_registers(REG_ZPOS_HI, &highByte, 1) || !dev->read_registers(REG_ZPOS_LO, &lowByte, 1)){
      return -1;
  }

  uint16_t zPosition = ((highByte << 8) | lowByte);

  return(zPosition);
}*