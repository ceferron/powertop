/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
#include "runtime_pm.h"

#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "../parameters/parameters.h"

#include <iostream>
#include <fstream>

runtime_pmdevice::runtime_pmdevice(const char *_name, const char *path)
{
	strcpy(sysfs_path, path);
	strcpy(name, _name);
	sprintf(humanname, "runtime-%s", _name);

	index = get_param_index(humanname);
	r_index = get_result_index(humanname);

	before_suspended_time = 0;
	before_active_time = 0;
        after_suspended_time = 0;
	after_active_time = 0;
}



void runtime_pmdevice::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	before_suspended_time = 0;
	before_active_time = 0;
        after_suspended_time = 0;
	after_active_time = 0;

	sprintf(filename, "%s/power/runtime_suspended_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return;
	file >> before_suspended_time;
	file.close();

	sprintf(filename, "%s/power/runtime_active_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return;
	file >> before_active_time;
	file.close();	
}

void runtime_pmdevice::end_measurement(void)
{
	char filename[4096];
	ifstream file;

	before_suspended_time = 0;
	before_active_time = 0;
        after_suspended_time = 0;
	after_active_time = 0;

	sprintf(filename, "%s/power/runtime_suspended_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return;
	file >> after_suspended_time;
	file.close();

	sprintf(filename, "%s/power/runtime_active_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return;
	file >> after_active_time;
	file.close();	
}

double runtime_pmdevice::utilization(void) /* percentage */
{
	return (after_active_time - before_active_time) / (0.0001 + after_active_time - before_active_time + after_suspended_time - before_suspended_time);
}

const char * runtime_pmdevice::device_name(void)
{
	return name;
}

const char * runtime_pmdevice::human_name(void)
{
	return humanname;
}


double runtime_pmdevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double utilization;

	power = 0;

	factor = get_parameter_value(index, bundle);
	utilization = get_result_value(r_index, result);
        power += utilization * factor / 100.0;

	return power;
}


static int device_has_runtime_pm(const char *sysfs_path)
{
	char filename[4096];
	ifstream file;
	unsigned long value;
	
	sprintf(filename, "%s/power/runtime_suspended_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return 0;
	file >> value;
	file.close();
	if (value)
		return 1;

	sprintf(filename, "%s/power/runtime_active_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return 0;
	file >> value;
	file.close();
	if (value)
		return 1;
	
	return 0;
}

void create_all_runtime_pm_devices(void)
{
	/* /sys/bus/pci/devices/0000\:00\:1f.0/power/runtime_suspended_time */

	struct dirent *entry;
	DIR *dir;
	char filename[4096];
	
	dir = opendir("/sys/bus/pci/devices/");
	if (!dir)
		return;
	while (1) {
		ifstream file;
		class runtime_pmdevice *dev;
		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		sprintf(filename, "/sys/bus/pci/devices/%s", entry->d_name);

		if (!device_has_runtime_pm(filename))
			continue;

		dev = new class runtime_pmdevice(entry->d_name, filename);
		all_devices.push_back(dev);
	}
	closedir(dir);
}
