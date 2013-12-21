/*
 *	wmediumd, wireless medium simulator for mac80211_hwsim kernel module
 *	Copyright (c) 2011 cozybit Inc.
 *
 *	Author: Javier Lopez    <jlopex@cozybit.com>
 *		Javier Cardona  <javier@cozybit.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *	02110-1301, USA.
 */

#include <libconfig.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "probability.h"
#include "wmediumd.h"
#include "globals_mobility_medium.h"

extern struct jammer_cfg jam_cfg;
extern double *prob_matrix;
extern int size;

/*
 *	Funtion to replace all ocurrences of a "old" string for a "new" string
 *	inside a "str" string
 */

char *str_replace(const char *str, const char *old, const char *new)
{
	char *ret, *r;
	const char *p, *q;
	size_t len_str = strlen(str);
	size_t len_old = strlen(old);
	size_t len_new = strlen(new);
	size_t count;

	for(count = 0, p = str; (p = strstr(p, old)); p += len_old)
		count++;

	ret = malloc(count * (len_new - len_old) + len_str + 1);
	if(!ret)
		return NULL;

	for(r = ret, p = str; (q = strstr(p, old)); p = q + len_old) {
		count = q - p;
		memcpy(r, p, count);
		r += count;
		strcpy(r, new);
		r += len_new;
	}
	strcpy(r, p);
	return ret;
}

/*
 *	Writes a char* buffer to a destination file
 */

int write_buffer_to_file(char *file, char *buffer)
{
	FILE *p = NULL;

	p = fopen(file, "w");
	if (p== NULL) {
		return 1;
	}

	fwrite(buffer, strlen(buffer), 1, p);
	fclose(p);

	return 0;
}

/*
 *	Writes a sample configuration with matrix filled with a value to a file
 */

int write_config(char *file, int ifaces, float value)
{
	FILE *out;
	char *ptr, *ptr2;
	size_t size;
	config_t cfg;
	config_setting_t *root, *setting, *group, *array, *list;
	int i, j, rates = 12;

	/*Init tmp file stream*/
	out = open_memstream(&ptr, &size);
	if (out == NULL) {
		printf("Error generating stream\n");
		exit(EXIT_FAILURE);
	}
	/*Init config*/
	config_init(&cfg);

	/*Create a sample config schema*/
	root = config_root_setting(&cfg);
	/* Add some settings to the ifaces group. */
	group = config_setting_add(root, "ifaces", CONFIG_TYPE_GROUP);
	setting = config_setting_add(group, "count", CONFIG_TYPE_INT);
	config_setting_set_int(setting, ifaces);
	array = config_setting_add(group, "ids", CONFIG_TYPE_ARRAY);

	for(i = 0; i < ifaces; ++i) {
		setting = config_setting_add(array, NULL, CONFIG_TYPE_STRING);
		char buffer[25];
		sprintf (buffer, "42:00:00:00:%02d:00", i);
		config_setting_set_string(setting, buffer);
	}

	/* Add some settings to the prob group. */
	group = config_setting_add(root, "prob", CONFIG_TYPE_GROUP);
	setting = config_setting_add(group, "rates", CONFIG_TYPE_INT);
	config_setting_set_int(setting, rates);
	list = config_setting_add(group, "matrix_list", CONFIG_TYPE_LIST);
	for (j = 0; j < rates ; j++) {
		array = config_setting_add(list, NULL, CONFIG_TYPE_ARRAY);
		int diag_count = 0;
		for(i = 0; i < ifaces*ifaces; ++i) {
			setting = config_setting_add(array, NULL,
						     CONFIG_TYPE_FLOAT);
			if (diag_count == 0)
				config_setting_set_float(setting, -1.0);
			else
				config_setting_set_float(setting, value);
			diag_count++;
			if (diag_count > ifaces)
				diag_count = 0;
		}
	}
	/* Write in memory out file */
	config_write(&cfg, out);
	config_destroy(&cfg);
	fclose(out);

	/* Let's do some post processing */
	ptr2 = str_replace(ptr, "], ", "],\n\t");
	free(ptr);
	ptr = str_replace(ptr2, "( ", "(\n\t");
	free(ptr2);
	/* Let's add comments to the config file */
	ptr2 = str_replace(ptr, "ifaces :", "#\n# wmediumd sample config file\n#\n\nifaces :");
	free(ptr);
	ptr = str_replace(ptr2, "prob :", "\n#\n# probability matrices are defined in a rowcentric way \n# probability matrices are ordered from slower to fastest, check wmediumd documentation for more info\n#\n\nprob :");
	printf("%s",ptr);

	/*write the string to a file*/
	if(write_buffer_to_file(file, ptr)) {
		printf("Error while writing file.\n");
		free(ptr);
		exit(EXIT_FAILURE);
	}
	printf("New configuration successfully written to: %s\n", file);

	/*free ptr*/
	free(ptr);
	exit(EXIT_SUCCESS);
}

/*
 *	Loads a config file into memory
 */

int load_config(const char *file)
{

	config_t cfg, *cf;
	const config_setting_t *ids, *prob_list, *mat_array, *jammer_s;
	int count_ids, rates_prob, i, j;
	int count_value, rates_value;

	/*initialize the config file*/
	cf = &cfg;
	config_init(cf);

	/*read the file*/
	if (!config_read_file(cf, file)) {
		printf("Error loading file %s at line:%d, reason: %s\n",
		file,
		config_error_line(cf),
		config_error_text(cf));
		config_destroy(cf);
		exit(EXIT_FAILURE);
    	}

	/* get jammer settings */
	if ((jammer_s = config_lookup(cf, "jam"))) {
		switch (config_setting_type(jammer_s)) {
		case CONFIG_TYPE_STRING:
			if (!strcmp(config_setting_get_string(jammer_s), "all")) {
				jam_cfg.jam_all = 1;
			}
			break;
		case CONFIG_TYPE_ARRAY:
			jam_cfg.nmacs = config_setting_length(jammer_s);
			jam_cfg.macs = malloc(sizeof(struct mac_address) * jam_cfg.nmacs);
			if (!jam_cfg.macs) {
				printf("couldn't allocate jamming mac table!\n");
				exit(EXIT_FAILURE);
			}
			for (i = 0; i < jam_cfg.nmacs; i++) {
				jam_cfg.macs[i] = string_to_mac_address(
						      config_setting_get_string_elem(jammer_s, i));
			}
			break;
		}
	}

	/*let's parse the values*/
	config_lookup_int(cf, "ifaces.count", &count_value);
	ids = config_lookup(cf, "ifaces.ids");
	count_ids = config_setting_length(ids);

	/*cross check*/
	if (count_value != count_ids) {
		printf("Error on ifaces.count");
		exit(EXIT_FAILURE);
	}

	size = count_ids;
	printf("#_if = %d\n",count_ids);
	/*Initialize the probability*/
	prob_matrix = init_probability(count_ids);

	/*Fill the mac_addr*/
	for (i = 0; i < count_ids; i++) {
    		const char *str =  config_setting_get_string_elem(ids, i);
    		put_mac_address(string_to_mac_address(str),i);
    	}
	/*Print the mac_addr array*/
	print_mac_address_array();

	config_lookup_int(cf, "prob.rates", &rates_value);
	prob_list = config_lookup(cf,"prob.matrix_list");

	/*Get rates*/
	rates_prob = config_setting_length(prob_list);

	/*Some checks*/
	if(!config_setting_is_list(prob_list)
	   && rates_prob != rates_value) {
		printf("Error on prob_list");
		exit(EXIT_FAILURE);
	}

	/*Iterate all matrix arrays*/
	for (i=0; i < rates_prob ; i++) {
		int x = 0, y = 0;
		mat_array = config_setting_get_elem(prob_list,i);
		/*If any error break execution*/
		if (config_setting_length(mat_array) != count_ids*count_ids) {
    			exit(EXIT_FAILURE);
		}
		/*Iterate all values on matrix array*/
		for (j=0; j < config_setting_length(mat_array); ) {
			MATRIX_PROB(prob_matrix,count_ids,x,y,i) =
			config_setting_get_float_elem(mat_array,j);
			//printf("%f, ", config_setting_get_float_elem(mat_array,j));
			x++;
			j++;
			/* if we finalized this row */
			if (j%count_ids==0) {
				y++;
				x=0;
				//printf("*******j:%d,count_ids:%d \n",j,count_ids);
			}
		}
	}

	config_destroy(cf);
	return (EXIT_SUCCESS);
}

/*
 * Theoretical maximum distance in meters calculated with a link budged including free space equation
 */
void calcule_max_distance(double carr_freq, double trans_pow, double trans_gain,
		double rec_gain, double rec_min_pow, double *dmax) {

	double lambda, pi = 3.14159265358979323846, c = 299792458.0, first, second,
			third;

	lambda = c / carr_freq;

	first = (trans_pow * trans_gain * rec_gain) / rec_min_pow;
	second = abs(pow(first,1.0/2.0));
	third = (4.0 * pi) / lambda;

	*dmax = second / third;
}

/*
 * This function prints mobility and medium configurations
 */
void print_mobility_medium_configuration() {
	int i, j;
	printf("===============================================\n");
	printf("      PRINT MOBILITI MEDIUM CONFIGURATION      \n");
	printf("===============================================\n\n");
	printf("dmax               =  %f [meters](theoretical max distance)\n",
			mob_med_cfg.dmax);
	printf(
			"interference_tunner=  %d (modeled with additional loss probability of %d %)\n",
			mob_med_cfg.interference_tunner,
			(mob_med_cfg.interference_tunner * 100 / 1000));
	printf("fading_probability =  %d (probability that fading appears %d %)\n",
			mob_med_cfg.fading_probability,
			(mob_med_cfg.fading_probability * 100 / 1000));
	printf(
			"fading_intensity   =  %d (fading intensity modeled with additional loss probability %d %)\n",
			mob_med_cfg.fading_intensity,
			(mob_med_cfg.fading_intensity * 100 / 1000));
	printf("count_ids          =  %d\n\n", mob_med_cfg.count_ids);
	printf("MAC ADRESSES");
	for (i = 0; i < mob_med_cfg.count_ids; i++) {
		printf("\n\nA[%d]:%02X:%02X:%02X:%02X:%02X:%02X\n", i,
				mob_med_cfg.radios[i].mac.addr[0],
				mob_med_cfg.radios[i].mac.addr[1],
				mob_med_cfg.radios[i].mac.addr[2],
				mob_med_cfg.radios[i].mac.addr[3],
				mob_med_cfg.radios[i].mac.addr[4],
				mob_med_cfg.radios[i].mac.addr[5]);
		printf("Radio positions (time|x|y) [seconds|meters|meters] :\n");
		for (j = 0; j < mob_med_cfg.radios[i].count_positions; j++) {
			printf("%.1f|%d|%d", mob_med_cfg.radios[i].positions[j].time,
					mob_med_cfg.radios[i].positions[j].x,
					mob_med_cfg.radios[i].positions[j].y);
			if (j < mob_med_cfg.radios[i].count_positions - 1)
				printf(" -> ");

		}

	}
	printf("\n===============================================\n");
	printf("                   END                         \n");
	printf("===============================================\n");
}

/*
 * Load required mobility and medium params from config file
 */
int load_mobility_medium_config(const char *file) {

	config_t cfg, *cf;
	const config_setting_t *retries, *pos_time_list, *mat_array;
	const char *base = NULL;
	int count, n;
	double carrier_frequency, transmit_power, transmit_gain, receiver_gain,
			receiver_min_power;

	cf = &cfg;
	config_init(cf);

	//Check if config file is readable
	if (!config_read_file(cf, file)) {
		printf("Error loading file %s at line:%d, reason: %s\n", file,
				config_error_line(cf), config_error_text(cf));
		config_destroy(cf);
		exit(EXIT_FAILURE);
	}

	//Variable reading and writting to settings struct mob_med_cfg
	//Read of permanent data
	if (config_lookup_int(cf, "debug", (int *) &mob_med_cfg.debug) != CONFIG_TRUE) {
		printf("\n\nError reading config file parameter: debug.\n\n");
		exit(EXIT_FAILURE);
	}
	if (config_lookup_int(cf, "interference_tunner",
			(int *) &mob_med_cfg.interference_tunner) != CONFIG_TRUE) {
		printf(
				"\n\nError reading config file parameter: interference_tunner.\n\n");
		exit(EXIT_FAILURE);
	}
	if (config_lookup_int(cf, "fading_probability",
			(int *) &mob_med_cfg.fading_probability) != CONFIG_TRUE) {
		printf(
				"\n\nError reading config file parameter: fading_probability.\n\n");
		exit(EXIT_FAILURE);
	}
	if (config_lookup_int(cf, "fading_intensity",
			(int *) &mob_med_cfg.fading_intensity) != CONFIG_TRUE) {
		printf(
				"\n\nError reading config file parameter: fading_intensity.\n\n");
		exit(EXIT_FAILURE);
	}

	//Read of temporal data to calculate dmax
	if (config_lookup_float(cf, "carrier_frequency",
			(double *) &carrier_frequency) != CONFIG_TRUE) {
		printf(
				"\n\nError reading config file parameter: carrier_frequency.\n\n");
		exit(EXIT_FAILURE);
	}
	if (config_lookup_float(cf, "transmit_power",
			(double *) &transmit_power)!= CONFIG_TRUE) {
		printf("\n\nError reading config file parameter: transmit_power.\n\n");
		exit(EXIT_FAILURE);
	}
	if (config_lookup_float(cf, "transmit_gain",
			(double *) &transmit_gain) != CONFIG_TRUE) {
		printf("\n\nError reading config file parameter: transmit_gain.\n\n");
		exit(EXIT_FAILURE);
	}
	if (config_lookup_float(cf, "receiver_gain",
			(double *) &receiver_gain) != CONFIG_TRUE) {
		printf("\n\nError reading config file parameter: receiver_gain.\n\n");
		exit(EXIT_FAILURE);
	}
	if (config_lookup_float(cf, "receiver_min_power",
			(double *) &receiver_min_power) != CONFIG_TRUE) {
		printf(
				"\n\nError reading config file parameter: receiver_min_power.\n\n");
		exit(EXIT_FAILURE);
	}

	if (mob_med_cfg.debug == 1) {
		printf("=================DEBUG INFO====================\n");
		printf("carrier_frequency: %f [Hz]\n", carrier_frequency);
		printf("transmit_power: %f [Watts]\n", transmit_power);
		printf("transmit_gain: %f [Linear]\n", transmit_gain);
		printf("receiver_gain: %f [Linear] \n", receiver_gain);
		printf("receiver_min_power: %.15f [Watts]\n", receiver_min_power);
	}

	//Calculate dmax
	calcule_max_distance(carrier_frequency, transmit_power, transmit_gain,
				receiver_gain, receiver_min_power, &mob_med_cfg.dmax);

	int i, j, count_radios, count_ids, pos_time_list_lenght;
	config_setting_t *ids;
	config_lookup_int(cf, "ifaces_with_mobility.count_ids",
			(int *) &count_radios);

	mob_med_cfg.radios= malloc(count_radios * sizeof(struct radio_mobility));

	ids = config_lookup(cf, "ifaces_with_mobility.ids");
	mob_med_cfg.count_ids = config_setting_length(ids);

	//Cross check
	if (count_radios != mob_med_cfg.count_ids) {
		printf("\n\nError reading config file parameter: count_ids.\n\n");
		exit(EXIT_FAILURE);
	}

	//Fill the mac_addr
	for (i = 0; i < mob_med_cfg.count_ids; i++) {
		config_lookup_int(cf, "ifaces_with_mobility.count_positions_time",
				(int *) &mob_med_cfg.radios[i].count_positions);
		const char *str = config_setting_get_string_elem(ids, i);
		mob_med_cfg.radios[i].positions = malloc(mob_med_cfg.radios[i].count_positions * sizeof(struct position_time));
		mob_med_cfg.radios[i].mac = string_to_mac_address(str);
	}

	pos_time_list = config_lookup(cf,
			"ifaces_with_mobility.positions_time_container");

	//Get lenght
	pos_time_list_lenght = config_setting_length(pos_time_list);

	//Some checks
	if (count_radios != pos_time_list_lenght) {
		printf(
				"\n\nError reading config file parameter: ifaces_with_mobility.positions_time_container.\n\n");
		exit(EXIT_FAILURE);
	}

	const char *temp;
	//Iterate all matrix arrays
	for (i = 0; i < pos_time_list_lenght; i++) {
		mat_array = config_setting_get_elem(pos_time_list, i);
		//Iterate all values on matrix array
		for (j = 0; j < config_setting_length(mat_array);) {
			temp = config_setting_get_string_elem(mat_array, j);
			//printf("%s \n ", temp);
			sscanf(temp, "%f|%d|%d", &mob_med_cfg.radios[i].positions[j].time,
					&mob_med_cfg.radios[i].positions[j].x,
					&mob_med_cfg.radios[i].positions[j].y);
			//printf("on ram memory: %f|%d|%d| \n ",
			//		mob_med_cfg.radios[i].positions[j].time,
			//		mob_med_cfg.radios[i].positions[j].x,
			//		mob_med_cfg.radios[i].positions[j].y);
			j++;
		}
	}

	//Print the mobility settings
	print_mobility_medium_configuration();

	config_destroy(cf);
	return 0;

}


/*
 * Alternative function to load_config() to load app required data without probability matrix.
 */
int load_basic_config(const char *file) {

	config_t cfg, *cf;
	const config_setting_t *ids, *prob_list, *mat_array, *jammer_s;
	int count_ids, rates_prob, i, j;
	int count_value, rates_value;

	/*initialize the config file*/
	cf = &cfg;
	config_init(cf);

	/*read the file*/
	if (!config_read_file(cf, file)) {
		printf("Error loading file %s at line:%d, reason: %s\n", file,
				config_error_line(cf), config_error_text(cf));
		config_destroy(cf);
		exit(EXIT_FAILURE);
	}

	/* get jammer settings */
	if ((jammer_s = config_lookup(cf, "jam"))) {
		switch (config_setting_type(jammer_s)) {
		case CONFIG_TYPE_STRING:
			if (!strcmp(config_setting_get_string(jammer_s), "all")) {
				jam_cfg.jam_all = 1;
			}
			break;
		case CONFIG_TYPE_ARRAY:
			jam_cfg.nmacs = config_setting_length(jammer_s);
			jam_cfg.macs = malloc(sizeof(struct mac_address) * jam_cfg.nmacs);
			if (!jam_cfg.macs) {
				printf("couldn't allocate jamming mac table!\n");
				exit(EXIT_FAILURE);
			}
			for (i = 0; i < jam_cfg.nmacs; i++) {
				jam_cfg.macs[i] = string_to_mac_address(
						config_setting_get_string_elem(jammer_s, i));
			}
			break;
		}
	}

	/*let's parse the values*/
	config_lookup_int(cf, "ifaces_with_mobility.count_ids",
			(int *) &count_value);
	ids = config_lookup(cf, "ifaces_with_mobility.ids");
	count_ids = config_setting_length(ids);
	size = count_ids;
	/*cross check*/
	if (count_value != count_ids) {
		printf("Error on ifaces_with_mobility.count_ids");
		exit(EXIT_FAILURE);
	}

	printf("#_if = %d\n", count_ids);
	/*Initialize the probability*/
	prob_matrix = init_probability(count_ids);
	/*Fill the mac_addr*/
	for (i = 0; i < count_ids; i++) {
		const char *str = config_setting_get_string_elem(ids, i);
		put_mac_address(string_to_mac_address(str), i);
	}

	/*Print the mac_addr array*/
	print_mac_address_array();

	config_destroy(cf);
	return (EXIT_SUCCESS);
}



