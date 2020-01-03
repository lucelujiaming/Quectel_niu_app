#include <stdio.h>
#include <stdlib.h>

#include <ql_oe.h>
#include "ql_common.h"
#include "niu_device.h"

NUINT32 niu_device_get_timestamp()
{
    return (NUINT32)time(NULL);
}