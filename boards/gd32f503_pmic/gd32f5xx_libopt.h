/*!
    \file    gd32f5xx_libopt.h
    \brief   library optional for gd32f5xx

    \version 2023-12-31, V1.4.0, firmware for GD32F5xx
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#ifndef GD32F5XX_LIBOPT_H
#define GD32F5XX_LIBOPT_H

#ifndef GD32EPRT

#include "gd32f5xx_adc.h"
#include "gd32f5xx_bkp.h"
#include "gd32f5xx_can.h"
#include "gd32f5xx_crc.h"
#include "gd32f5xx_ctc.h"
#include "gd32f5xx_dac.h"
#include "gd32f5xx_dbg.h"
#include "gd32f5xx_dma.h"
#include "gd32f5xx_exmc.h"
#include "gd32f5xx_exti.h"
#include "gd32f5xx_fmc.h"
#include "gd32f5xx_fwdgt.h"
#include "gd32f5xx_gpio.h"
#include "gd32f5xx_shrtimer.h"
#include "gd32f5xx_i2c.h"
#include "gd32f5xx_misc.h"
#include "gd32f5xx_pmu.h"
#include "gd32f5xx_rcu.h"
#include "gd32f5xx_rtc.h"
#include "gd32f5xx_spi.h"
#include "gd32f5xx_timer.h"
#include "gd32f5xx_usart.h"
#include "gd32f5xx_wwdgt.h"
#include "gd32f5xx_sqpi.h"

#if defined (GD32F5XX_CL) || defined (GD32E508)
#include "gd32f5xx_enet.h"
#include "gd32f5xx_tmu.h"
#include "gd32f5xx_cmp.h"
#else /* GD32F5XX_CL or GD32E508 */
#include "gd32f5xx_sdio.h"
#endif /* GD32F5XX_CL or GD32E508 */

#else /* GD32EPRT */
#include "gd32f5xx_adc.h"
#include "gd32f5xx_bkp.h"
#include "gd32f5xx_crc.h"
#include "gd32f5xx_ctc.h"
#include "gd32f5xx_dac.h"
#include "gd32f5xx_dbg.h"
#include "gd32f5xx_dma.h"
#include "gd32f5xx_enet.h"
#include "gd32f5xx_exmc.h"
#include "gd32f5xx_exti.h"
#include "gd32f5xx_fmc.h"
#include "gd32f5xx_fwdgt.h"
#include "gd32f5xx_gpio.h"
#include "gd32f5xx_i2c.h"
#include "gd32f5xx_misc.h"
#include "gd32f5xx_pmu.h"
#include "gd32f5xx_rcu.h"
#include "gd32f5xx_rtc.h"
#include "gd32f5xx_spi.h"
#include "gd32f5xx_timer.h"
#include "gd32f5xx_usart.h"
#include "gd32f5xx_wwdgt.h"
#include "gd32f5xx_sqpi.h"

#endif /* GD32EPRT */

#endif /* GD32F5XX_LIBOPT_H */
