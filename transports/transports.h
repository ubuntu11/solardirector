
#ifndef __SDTRANSPORTS_H
#define __SDTRANSPORTS_H

#include "common.h"
#include "driver.h"

extern solard_driver_t null_transport;
extern solard_driver_t bt_driver;
extern solard_driver_t can_driver;
extern solard_driver_t ip_driver;
extern solard_driver_t rdev_driver;
extern solard_driver_t serial_driver;
extern solard_driver_t http_driver;

#endif
