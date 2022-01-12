/**
 *
 *  Filename:      das_atteste.c
 *
 *  Description:   Attestation engine
 *
 *  Created by strawmanbobi 2022-01-10
 *
 *  Copyright (c) 2022 Aliyun-IoT
 *
 **/

#include "das_service.h"
#include "das_hardware.h"

#include "das.h"

int das_attest(const char* tag, ...)
{
	int ret = -1;
	va_list args;
	int i = 0;
	das_attest_handler_t* attest_hdlr = NULL;

#if (DAS_SERVICE_STACK_PROTECTION_ENABLED)
	stack_protection(tag);
#endif

	va_start(args, tag);

	for (i = 0; das_attest_handler_table[i]; i++) {
		attest_hdlr = das_attest_handler_table[i];
        
		if (!attest_hdlr->tag || !attest_hdlr->handler) {
			DAS_LOG("invalid tag handler, index: %d, tag: %s.\n", i, attest_hdlr->tag);
			break;
		}

		if (!strcmp(attest_hdlr->tag, tag)) {
			ret = attest_hdlr->handler(args);
			break;
		}
	}

	va_end(args);

	return ret;
}
