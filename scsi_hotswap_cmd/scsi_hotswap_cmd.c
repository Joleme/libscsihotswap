/*
 * scsi_hotswap_cmd.c
 *
 * SCSI/FibreChannel Disk Hotswap userland application using libhotswap
 *
 * Author: MontaVista Software, Inc.
 *         Steven Dake (sdake@mvista.com)
 *         Dave Jiang (djiang@mvista.com)
 *         source@mvista.com
 *
 * Copyright (C) 2002,2005 MontaVista Software Inc.
 *
 * This software is distributed under the new BSD license
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its 
 *   contributors may be used to endorse or promote products derived from this 
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "../libscsihotswap/libscsihotswap.h"

void usage (void) {
	printf ("usage:\n");
	printf ("\tscsi_hotinsert_by_id [host] [channel] [lun] [id]\n");
	printf ("\tscsi_hotremove_by_id [host] [channel] [lun] [id]\n");
	printf ("\tscsi_blkdev_remove_by_id [host] [channel] [lun] [id]\n");
	printf ("\tscsi_hotinsert_by_wwn_wildcard [fc wwn]\n");
	printf ("\tscsi_hotremove_by_wwn_wildcard [fc wwn]\n\n");
	printf ("\tscsi_blkdev_remove_by_wwn_wildcard [fc wwn]\n\n");
	printf ("\t(prepending an 0x indicates a hex input.)\n");
}

int main (int argc, char *argv[]) 
{
	int result = 0;
	int host, channel, lun, id = -1;
	unsigned long long ieee_no = -1;
	char *self;

	/*
	 * Decode command line arguments
	 */
	if (argc == 2) {
		ieee_no = strtoull (argv[1], &argv[1], 0);
	} else
	if (argc == 5) {
		host = strtoul (argv[1], &argv[1], 0);
		channel = strtoul (argv[2], &argv[2], 0);
		lun = strtoul (argv[3], &argv[3], 0);
		id = strtoul (argv[4], &argv[4], 0);
	} else {
		usage ();
		return 0;
	}

	self = strrchr(argv[0], '/');
	if (self)
		self++;
	else
		self = argv[0];

	/*
	 * scsi_hotswap_insert_by_scsi_id
	 */
	if (strcmp (self, "scsi_hotinsert_by_id") == 0) {
		result = scsi_hotswap_by_id(SCSI_HOTSWAP_INSERT,
				host, channel, lun, id);
	} else

	if (strcmp (self, "scsi_hotremove_by_id") == 0) {
		result = scsi_hotswap_by_id(SCSI_HOTSWAP_REMOVE, 
				host, channel, lun, id);
	} else

	if (strcmp (self, "scsi_blkdev_remove_by_id") == 0) {
		result = scsi_hotswap_by_id(SCSI_HOTSWAP_BLKDEV_REMOVE, 
				host, channel, lun, id);
	} else

	if (strcmp (self, "scsi_hotinsert_by_wwn_wildcard") == 0) {
		result = scsi_hotswap_by_ieee_wildcard(SCSI_HOTSWAP_INSERT, ieee_no);
	} else

	if (strcmp (self, "scsi_blkdev_remove_by_wwn_wildcard") == 0) {
		result = scsi_hotswap_by_ieee_wildcard(SCSI_HOTSWAP_BLKDEV_REMOVE, 
				ieee_no);
	} else

	if (strcmp (self, "scsi_hotremove_by_wwn_wildcard") == 0) {
		result = scsi_hotswap_by_ieee_wildcard(SCSI_HOTSWAP_REMOVE, ieee_no);
	} else {
		fprintf (stderr, "%s: Unknown command\n", argv[0]);
		result = 0;
		return (-ENXIO);
	}

	if (argc == 2) {
		printf ("%s: wwn_no=0x%16llx\n", argv[0], ieee_no);
	} else
	if (strstr (argv[0], "ieee_no")==0) {
		printf ("%s: host=%d channel=%d lun=%d id=%d\n", argv[0], host, channel, lun, id);
	} 

	if (result) {
		printf ("%s: Couldn't complete operation because of error code %d\n", argv[0], result);
	} else {
		printf ("Operation succeeded completed\n");
	}
	return 0;
}
