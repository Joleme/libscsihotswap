/*
 * libscsihotswap.c
 * SCSI/FibreChannel Disk Hotswap userland implementation
 * Author: MontaVista Software, Inc.
 * 	   Dave Jiang (djiang@mvista.com)
 * 	   source@mvista.com
 *
 * Copyright (c) 2005-2010 (c) MontaVista Software, LLC.
 * All rights reserved.
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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <dirent.h>
#include <stdlib.h>
#include "libscsihotswap.h"

struct scsi_device_strings {
	char vendor[9];
	char model[17];
	char rev[5];
	char serial[9];
};

union ieee_id_map {
	unsigned long long ieee_id;
	unsigned char ieee_id_u8[8];
};

struct scsi_device {
	struct scsi_device_strings scsi_device_strings;
	unsigned long long ieee_id;
	unsigned char lun;
	unsigned char host;
	unsigned char channel;
	unsigned char id;
	unsigned int partmap;
};

struct scsi_get_idlun {
	unsigned char id;
	unsigned char lun;
	unsigned char channel;
	unsigned char host;
	unsigned int host_unique_id;
};

struct inquiry_command {
	unsigned int input_size;
	unsigned int output_size;
	char cmd[6];
};

struct inquiry_result {
	unsigned int input_size;
	unsigned int output_size;
	unsigned char device_type;
	unsigned char device_modifier;
	unsigned char version;
	unsigned char data_format;
	unsigned char length;
	unsigned char reserved1;
	unsigned char reserved2;
	unsigned char state;
	unsigned char vendor[8];
	unsigned char model[16];
	unsigned char rev[4];
	unsigned char serial[12];
	unsigned char reserved[39];
};

struct extended_inquiry_result {
	unsigned int input_size;
	unsigned int output_size;
	unsigned char junk[8];
	unsigned char ieee_id[8];
};


struct scsi_device scsi_device_array[256];
unsigned int max_scsi_devs = 0;

/*
 * hotinsert a scsi disk by host,channle,id,lun
 * local function called by hotswap_by_id
 */
int scsi_hotinsert_by_id(unsigned int host,
		unsigned int channel,
		unsigned int lun,
		unsigned int id)
{
	char path_to_disk[256];
	char path_to_scan[256];
	char buf[128];
	int fd, sysfd;

	memset(path_to_disk, 0, 256);
	sprintf(path_to_disk, "/sys/class/scsi_device/%u:%u:%u:%u/device/state",
			host, channel, id, lun);
	fd = open(path_to_disk, O_RDONLY);

	/* hey we already exist! */
	if(fd >= 0)
	{
		close(fd);
		return (0);
	}

	memset(path_to_scan, 0, 256);
	sprintf(path_to_scan, "/sys/class/scsi_host/host%u/scan", host);

	/* open sysfs entry for scan */
	sysfd = open(path_to_scan, O_WRONLY);
	/* host does not exist, can't insert */
	if(sysfd < 0) 
		return (-1);

	sprintf(buf, "%u %u %u", channel, id, lun);

	write(sysfd, buf, strlen(buf));
	close(sysfd);

	return 0;
}

static int blkdev_filter(const struct dirent *entry)
{
	return (strcmp(entry->d_name, ".") != 0
	        && strcmp(entry->d_name, "..") != 0);
}

/*
 * hotremove a scsi disk by host,channle,id,lun
 * local function called by hotswap_by_id
 * This function uses the blkdev remove that cleans up all upper
 * layer stuff (i.e. invokes fumount, cleans up open file handles)
 */
int scsi_blkdevremove_by_id(unsigned int host,
		unsigned int channel,
		unsigned int lun,
		unsigned int id)
{
	struct dirent **namelist;
	int n, sysfd, retval;
	char path_to_remove[256];

	path_to_remove[255] = '\0';
	sprintf(path_to_remove,
		"/sys/class/scsi_device/%u:%u:%u:%u/device/block/",
		host, channel, id, lun);
	n = scandir(path_to_remove, &namelist, blkdev_filter, alphasort);

	if (n <= 0)
	{
		if (n == 0)
			free(namelist);
		fprintf(stderr,
			"Disk (host:%u|bus:%u:lun:%u|id:%u) does not exist\n",
			host, channel, lun, id);
		return -1;
	}

	retval = -1;
	while (n--)
	{
		snprintf(path_to_remove, 256,
			"/sys/class/scsi_device/%u:%u:%u:%u/device/block/%s/remove",
			host, channel, id, lun, namelist[n]->d_name);
		free(namelist[n]);

		sysfd = open(path_to_remove, O_WRONLY);
		if(sysfd < 0)
		{
			fprintf(stderr, "Fail to open %s\n", path_to_remove);
			continue;
		}
		write(sysfd, "1", 1);
		close(sysfd);
		retval = 0;	/* success if at least one dev is removed ok */
	}

	return retval;
}

/*
 * hotremove a scsi disk by host,channle,id,lun
 * local function called by hotswap_by_id
 * This function just removes the scsi disk from system
 * Should only be invoked if the disk is not in use by system
 */
int scsi_hotremove_by_id(unsigned int host,
		unsigned int channel,
		unsigned int lun,
		unsigned int id)
{
	char path_to_del[256];
	int sysfd;

	memset(path_to_del, 0, 256);
	sprintf(path_to_del, "/sys/class/scsi_device/%u:%u:%u:%u/device/delete",
			host, channel, id, lun);
	sysfd = open(path_to_del, O_WRONLY);
	if(sysfd < 0)
	{
		fprintf(stderr, "Disk (host:%u|bus:%u:lun:%u|id:%u) does not exist\n",
				host, channel, lun, id);
		return (0);
	}
	write(sysfd, "1", 1);
	close(sysfd);
	return 0;
}

/*
 * This function provides the exported call to hotswap by host,
 * channle, id, and lun set. By providing the actino of insert,
 * remove, or blkdev_remove the appropriate helper functions are 
 * invoked.
 */
int scsi_hotswap_by_id(enum scsi_hotswap_action action,
		unsigned int host, 
		unsigned int channel,
		unsigned int lun,
		unsigned int id)
{
	int retval = 0;

	switch(action)
	{
		case SCSI_HOTSWAP_INSERT:
			retval = scsi_hotinsert_by_id(host, channel, lun, id);
			break;
		case SCSI_HOTSWAP_REMOVE:
			retval = scsi_hotremove_by_id(host, channel, lun, id);
			break;
		case SCSI_HOTSWAP_BLKDEV_REMOVE:
			retval = scsi_blkdevremove_by_id(host, channel, lun, id);
			break;
		default:
			retval = -EINVAL;
			break;
	}
	return retval;
}

/*
 * Helper function that builds the dynamic table of SCSI disks
 */
int scan_scsis(void)
{
	int result = 0;
	int fd;
	unsigned char ioctl_data[256];
	struct scsi_device *device;
	int minor, scsi_cnt = 0;
	int i, j;
	struct inquiry_command *inquiry_command;
	struct inquiry_result *inquiry_result;
	struct extended_inquiry_result *extended_inquiry_result;
	union ieee_id_map ieee_id_map;
	struct scsi_get_idlun scsi_get_idlun;

	memset(scsi_device_array, 0, (sizeof(struct scsi_device) * 256));
	for (minor = 0; minor < 256; minor++) 
	{
		result = mknod("this", 0600 | S_IFCHR, 
				makedev(SCSI_GENERIC_MAJOR, minor));
		if(result < 0)
			continue;
		    
		fd = open("this", O_RDWR);	
		if(fd < 0) 
		{
			unlink("this");
			continue;
		}
	
		device = &scsi_device_array[scsi_cnt];
		memset (ioctl_data, 0, sizeof (ioctl_data));

		/*
		* Execute inquiry command to get SCSI serial number
		*/
		inquiry_command = (struct inquiry_command *)ioctl_data;
		inquiry_command->input_size = 0;
		inquiry_command->output_size = sizeof (struct inquiry_result);
		inquiry_command->cmd[0] = 0x12;
		inquiry_command->cmd[1] = 0x00;
		inquiry_command->cmd[2] = 0x00;
		inquiry_command->cmd[3] = 0x00;
		inquiry_command->cmd[4] = 96;
		inquiry_command->cmd[5] = 0x00;
		result = ioctl (fd, 1, inquiry_command);
		if(result < 0)
			return -EIO;
	
		inquiry_result = (struct inquiry_result *)ioctl_data;
		strncpy(device->scsi_device_strings.vendor, inquiry_result->vendor, 8);
		strncpy(device->scsi_device_strings.model, inquiry_result->model, 16);
		strncpy(device->scsi_device_strings.rev, inquiry_result->rev, 4);
		strncpy(device->scsi_device_strings.serial, 
				inquiry_result->serial, 12);
	
		/*
		* Get IEEE unique ID (FibreChannel WWN) from EVPD page 0x83
		*/
		inquiry_command->input_size = 0;
		inquiry_command->output_size = sizeof(struct extended_inquiry_result);
		inquiry_command->cmd[0] = 0x12;
		inquiry_command->cmd[1] = 0x01;
		inquiry_command->cmd[2] = 0x83;
		inquiry_command->cmd[3] = 0x00;
		inquiry_command->cmd[4] = 96;
		inquiry_command->cmd[5] = 0x00;
		extended_inquiry_result = (struct extended_inquiry_result *)ioctl_data;
		result = ioctl (fd, 1, inquiry_command);
		if(result < 0)
			return -EIO;
    
		for(i = 0, j = 7; i < 8; i++, j--) 
		{
	        ieee_id_map.ieee_id_u8[j] = extended_inquiry_result->ieee_id[i];
		}

		device->ieee_id = ieee_id_map.ieee_id;
		
		/*
		 * Get path to device
		 */
		result = ioctl (fd, SCSI_IOCTL_GET_IDLUN, &scsi_get_idlun);
		if(result < 0)
			return -EIO;
		device->host = scsi_get_idlun.host;
		device->channel = scsi_get_idlun.channel;
		device->id = scsi_get_idlun.id;
		device->lun = scsi_get_idlun.lun;
		
	    close(fd);
	    unlink("this");
		scsi_cnt++;
	}

	max_scsi_devs = scsi_cnt;
	return 0;
}

/*
 * Helper funciton to hotswap_by_ieee to hotremove a disk
 * identified by the unique ieee no
 */
int scsi_hotremove_by_ieee(enum scsi_hotswap_action action, 
		unsigned long long ieee_no)
{
	int retval = 0;
	int i;
	struct scsi_device *device;

	retval = scan_scsis();

	for(i=0; i<max_scsi_devs; i++)
	{
		device = &scsi_device_array[i];
		if(device->ieee_id == ieee_no)
		{
			if(action == SCSI_HOTSWAP_REMOVE)
				retval = scsi_hotremove_by_id(device->host, device->channel, 
						device->lun, device->id);
			else 
				retval = scsi_blkdevremove_by_id(device->host, device->channel,
						device->lun, device->id);
			return retval;
		}
	}

	return -EINVAL;
}

/* 
 * Helper function to initiate a full rescan of the SCSI subsystem.
 * This should pick up any disks that were hot inserted by system.
 */
int probe_scsis(void)
{
	char path_to_probe[256];
	char cmd_buf[] = "- - -";
	int sysfd;
	int i;

	memset(path_to_probe, 0, 256);
	
	for(i=0; i<256; i++)
	{
		sprintf(path_to_probe, "/sys/class/scsi_host/host%u/scan", i);
		sysfd = open(path_to_probe, O_WRONLY);
		if(sysfd < 0)
			continue;
		/* scan host via wildcard */
		write(sysfd, cmd_buf, strlen(cmd_buf));
		close(sysfd);
	}
	return 0;
}

/* 
 * This is the helper function invoked by hotswap_by_ieee_wildcard
 * to hot insert a drive identified by the unique ieee no.
 * 1. we build a table via scanning all visible scsi devices
 * 2. we initiate a full rescan to pull in all scsi disks
 * 3. we probe each disk and compare to table we built previously
 *    and we remove any disks that are not in the table unless
 *    it matches the ieee_no. We do not want disks to be visible 
 *    if the hotinsert was not requested
 */ 
int scsi_hotinsert_by_ieee(unsigned long long ieee_no)
{
	int retval = 0;
	int fd;
	unsigned char ioctl_data[256];
	struct scsi_device *device;
	int minor, scsi_cnt = 0;
	int i, j;
	unsigned long long temp_ieee = 0;
	union ieee_id_map ieee_id_map;
	struct inquiry_command *inquiry_command;
	struct extended_inquiry_result *extended_inquiry_result;
	struct scsi_get_idlun scsi_get_idlun;
	
	retval = scan_scsis();

	retval = probe_scsis();

	for (minor = 0; minor < 256; minor++) 
	{
		retval = mknod("this", 0600 | S_IFCHR, 
				makedev(SCSI_GENERIC_MAJOR, minor));
		if(retval < 0)
			continue;
		    
		fd = open("this", O_RDWR);	
		if(fd < 0) 
		{
			unlink("this");
			continue;
		}
	
		device = &scsi_device_array[scsi_cnt];
		memset (ioctl_data, 0, sizeof (ioctl_data));

		/*
		* Get IEEE unique ID (FibreChannel WWN) from EVPD page 0x83
		*/
		inquiry_command = (struct inquiry_command *)ioctl_data;
		inquiry_command->input_size = 0;
		inquiry_command->output_size = sizeof(struct extended_inquiry_result);
		inquiry_command->cmd[0] = 0x12;
		inquiry_command->cmd[1] = 0x01;
		inquiry_command->cmd[2] = 0x83;
		inquiry_command->cmd[3] = 0x00;
		inquiry_command->cmd[4] = 96;
		inquiry_command->cmd[5] = 0x00;
		extended_inquiry_result = (struct extended_inquiry_result *)ioctl_data;
		retval = ioctl(fd, 1, inquiry_command);
		if(retval < 0)
			return -EIO;
    
		for(i = 0, j = 7; i < 8; i++, j--) 
		{
	        ieee_id_map.ieee_id_u8[j] = extended_inquiry_result->ieee_id[i];
		}

		temp_ieee = ieee_id_map.ieee_id;
		
		/* existing device, we keep scanning */
		if(temp_ieee == device->ieee_id)
		{
			scsi_cnt++;
		}
		/* found a device that shouldn't be inserted */
		else if(temp_ieee != ieee_no)
		{
			/*
			* Get path to device
			*/
			retval = ioctl(fd, SCSI_IOCTL_GET_IDLUN, &scsi_get_idlun);
			if(retval < 0)
				return -EIO;
			retval = scsi_hotremove_by_id(scsi_get_idlun.host, 
					scsi_get_idlun.channel, 
					scsi_get_idlun.lun, 
					scsi_get_idlun.id);
		}

		close(fd);
		unlink("this");
	}
	return retval;
}

/*
 * Function that performs hotswap action based on unique ieee no
 */
int scsi_hotswap_by_ieee_wildcard(enum scsi_hotswap_action action, 
		unsigned long long ieee_no)
{
	int retval = 0;

	switch(action)
	{
		case SCSI_HOTSWAP_INSERT:
			retval = scsi_hotinsert_by_ieee(ieee_no);
			break;
		case SCSI_HOTSWAP_REMOVE:
		case SCSI_HOTSWAP_BLKDEV_REMOVE:
			retval = scsi_hotremove_by_ieee(action, ieee_no);
			break;
		default:
			retval = -EINVAL;
			break;
	}
	return retval;
}

