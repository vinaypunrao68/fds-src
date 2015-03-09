/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

##########
# DO NOT COMPILE THIS TRIFT FILE USING THE MAKE SYSTEM!
# This file is intended to aggregate our various thrift
# files into one large file to be used for generating
# API HTML documentation, not source code to be compiled.
##########

/*
 * XDI/connector facing APIs. These APIs are consumed by our connectors
 * and connector projecting our external XDI interface.
 */
include "xdi.thrift"
include "config_api.thrift"

/*
 * OM APIs. These are the APIs that OM exposes to other internal
 * services.
 */
include "om_api.thrift"

/*
 * AM APIs. These are the APIs that AM exposes to other internal
 * services.
 */
include "am_api.thrift"

/*
 * DM APIs. These are the APIs that DM exposes to other internal
 * services.
 */
include "dm_api.thrift"

/*
 * SM APIs. These are the APIs that SM exposes to other internal
 * services.
 */
include "sm_api.thrift"

/*
 * General service APIs. These are the APIs that each service exposes
 * to other internal services.
 */
include "svc_api.thrift"