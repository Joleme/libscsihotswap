# Makefile
#
# SCSI/FibreChannel Hotswap Userland Interface
#
# Author: MontaVista Software, Inc.
#        Steven Dake (sdake@mvista.com)
#        source@mvista.com
#
# Copyright 2002,2005 MontaVista Software Inc.
#
# This software is distributed under the new BSD license
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# - Redistributions of source code must retain the above copyright notice, 
#   this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright notice, 
#   this list of conditions and the following disclaimer in the documentation 
#   and/or other materials provided with the distribution.
# - Neither the name of the MontaVista Software, Inc. nor the names of its 
#   contributors may be used to endorse or promote products derived from this 
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.

CFLAGS = -Wall
LDFLAGS = -L../libscsihotswap -lscsihotswap

OBJS = scsi_hotswap_cmd.o

all: scsi_hotswap_cmd scsi_hotswap_insert_by_scsi_id
scsi_hotswap_cmd: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

scsi_hotswap_insert_by_scsi_id:
	ln -sf scsi_hotswap_cmd scsi_hotinsert_by_id
	ln -sf scsi_hotswap_cmd scsi_hotremove_by_id
	ln -sf scsi_hotswap_cmd scsi_blkdev_remove_by_id
	ln -sf scsi_hotswap_cmd scsi_hotinsert_by_wwn_wildcard
	ln -sf scsi_hotswap_cmd scsi_hotremove_by_wwn_wildcard
	ln -sf scsi_hotswap_cmd scsi_blkdev_remove_by_wwn_wildcard

clean:
	rm -f scsi_hotswap_cmd.o scsi_hotswap_cmd 
	rm -f scsi_hotinsert_by_id
	rm -f scsi_hotremove_by_id
	rm -f scsi_blkdev_remove_by_id
	rm -f scsi_hotinsert_by_wwn_wildcard
	rm -f scsi_hotremove_by_wwn_wildcard
	rm -f scsi_blkdev_remove_by_wwn_wildcard
