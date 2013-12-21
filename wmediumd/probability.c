/*
 *	wmediumd, wireless medium simulator for mac80211_hwsim kernel module
 *	Copyright (c) 2011 cozybit Inc.
 *
 *	Author:	Javier Lopez	<jlopex@cozybit.com>
 *		Javier Cardona	<javier@cozybit.com>
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals_mobility_medium.h"
#include <sys/time.h>
#include <math.h>
#include "wmediumd.h"

#include "probability.h"
#include "ieee80211.h"

static int array_size = 0;
static struct mac_address *indexer;


void put_mac_address(struct mac_address addr, int pos)
{
	int i;
	void *ptr = indexer;

 	for (i=0; i < pos ; i++) {
 		ptr = ptr + sizeof(struct mac_address);
 	}
 	memcpy(ptr, &addr, sizeof(struct mac_address));
}


/*
 * returns the a mac_address ptr for a given index
 */

struct mac_address * get_mac_address(int pos) {

	void * ptr = indexer;
	ptr = ptr + (sizeof(struct mac_address)*pos);

	return ((pos >= array_size) ?  NULL : (struct mac_address*)ptr);
}

/*
 * 	Returns the position of the address in the array.
 * 	If the mac_address is not found returns -1
 */

int find_pos_by_mac_address(struct mac_address *addr) {

	int i=0;

	void * ptr = indexer;
	while(memcmp(ptr,addr,sizeof(struct mac_address)) && i < array_size)
	{
		i++;
		ptr = ptr + sizeof(struct mac_address);
	}

	return ((i >= array_size) ?  -1 :  i);
}

/*
 * 	Prints the values of the Mac Adress Array
 */

void print_mac_address_array() {

	int i=0;
	void * ptr = indexer;

	while (i < array_size) {
		struct mac_address *a = malloc(sizeof(struct mac_address));
		memcpy(a,ptr,sizeof(struct mac_address));
		printf("A[%d]:%02X:%02X:%02X:%02X:%02X:%02X\n",
		       i,a->addr[0], a->addr[1], a->addr[2],
		       a->addr[3], a->addr[4], a->addr[5]);
		i++;
		ptr = ptr + sizeof(struct mac_address);
	}
}

/*
 * 	Fills the probability matrix with a given value
 */

void fill_prob_matrix(double *aMatrix,double aValue) {
	int i =0 , j=0 , k=0;


	for (k=0; k < IEEE80211_AVAILABLE_RATES; k++) {
		for (i = 0 ; i < array_size  ; i++ ) {
			for (j = 0; j < array_size; j++) {
				if (i==j)
					MATRIX_PROB(aMatrix,array_size,i,j,k)=-1;
				else {
					double prob_per_link = aValue + (aValue / 15) * k;
					prob_per_link = prob_per_link > 1.0 ? 1.0 : prob_per_link;
					MATRIX_PROB(aMatrix,array_size,i,j,k)=prob_per_link;
				}
			}
		}
	}
}

/*
 * 	Prints the values of the probability matrix
 */

void print_prob_matrix (double *aMatrix) {

	int i,j,k;

	for (k=0; k < IEEE80211_AVAILABLE_RATES; k++) {
		for (i=0; i < array_size ; i++) {
			for (j=0; j < array_size; j++) {
				printf("[%10f]",MATRIX_PROB(aMatrix,array_size,i,j,k));
			}
			printf("\n");
		}
		printf("Matrix rate = %d\n",k);
	}
}

/*
 *	Returns the loss probability for a given radio link, and rate
 * 	If an error occurs returns -1;
 */

double find_prob_by_addrs_and_rate (double *aMatrix, struct mac_address *src,
				    struct mac_address *dst, int rate_idx) {

	int x = find_pos_by_mac_address(src);
	int y = find_pos_by_mac_address(dst);

	//printf("[%d][%d] rate:%d ploss=%f ",x,y,rate_idx,MATRIX_PROB(aMatrix,array_size,x,y,rate_idx));

	if (x == -1 || y ==-1)
		return -1;
	return MATRIX_PROB(aMatrix,array_size,x,y,rate_idx);
}


/*
 *	Returns the loss probability for a given radio link, mobility and medium
 * 	If an error occurs returns -1;
 */

int find_prob_by_addrs_mobility_and_medium(struct mac_address *src,
		struct mac_address *dst) {

	//Probabilities are defined per 1000 units to perform them using integers
	int random_value = rand() % 1000 + 1, distance_prob = 0, loss_probability =
			0, fading = 0, src_radio_pos, dst_radio_pos, time_pos;
	unsigned long int execution_time;

	struct timeval current_time;
	gettimeofday(&current_time, NULL );

	execution_time = current_time.tv_sec - mob_med_cfg.start_execution_timestamp;

	src_radio_pos = find_radio_pos_by_mac_address(src);
	dst_radio_pos = find_radio_pos_by_mac_address(dst);
	//Both stations need to have same time line definition, for that we can use or src or dst to take the time.
	if (execution_time
			> (int) mob_med_cfg.radios[src_radio_pos].positions[(mob_med_cfg.last_def_position
					+ 1)].time
			&& mob_med_cfg.last_def_position
					< (mob_med_cfg.radios[src_radio_pos].count_positions - 1)) {
		mob_med_cfg.last_def_position++;
		mob_med_cfg.dcurrent =
				find_distance_two_points(
						mob_med_cfg.radios[src_radio_pos].positions[mob_med_cfg.last_def_position].x,
						mob_med_cfg.radios[src_radio_pos].positions[mob_med_cfg.last_def_position].y,
						mob_med_cfg.radios[dst_radio_pos].positions[mob_med_cfg.last_def_position].x,
						mob_med_cfg.radios[dst_radio_pos].positions[mob_med_cfg.last_def_position].y); /*meters*/
	}

	if (mob_med_cfg.dcurrent <= mob_med_cfg.dmax / 4) {
		distance_prob = 0;
	} else {
		distance_prob = (((float) mob_med_cfg.dcurrent - (float) mob_med_cfg.dmax / 4.0)
				/ ((float) mob_med_cfg.dmax - ((float) mob_med_cfg.dmax / 4.0)))
				* 1000; //1000 is a scale factor
	}

	if (random_value < mob_med_cfg.fading_probability) {
		fading = mob_med_cfg.fading_intensity;
	} else {
		fading = 0;
	}

	loss_probability = distance_prob + mob_med_cfg.interference_tunner + fading;

	if (mob_med_cfg.debug == 1) {
		printf(
				"DEBUG INFO. POSITIONS: p1=(%d,%d) p2=(%d,%d) EXEC_TIME: %dsec. LAST_ARR_INDEX: %d DISTANCE_BETWEEN_TX_RX: %d\n",
				mob_med_cfg.radios[src_radio_pos].positions[mob_med_cfg.last_def_position].x,
				mob_med_cfg.radios[src_radio_pos].positions[mob_med_cfg.last_def_position].y,
				mob_med_cfg.radios[dst_radio_pos].positions[mob_med_cfg.last_def_position].x,
				mob_med_cfg.radios[dst_radio_pos].positions[mob_med_cfg.last_def_position].y,
				(int)execution_time, mob_med_cfg.last_def_position, mob_med_cfg.dcurrent);

		printf(
				"PROBABILITIES. random_value %d | distance_prob %d (%d%) | loss_probability %d (%d%) | fading %d| dcurrent %d | dmax %d \n",
				random_value, distance_prob, (distance_prob / 10),
				loss_probability, (loss_probability / 10), fading, mob_med_cfg.dcurrent,
				(int) mob_med_cfg.dmax);
	}

	return loss_probability;
}

/*
 * Returns the position inside the radios array given a mac_address
 */
int find_radio_pos_by_mac_address(struct mac_address *addr) {

	int i = 0;

	while (memcmp(&mob_med_cfg.radios[i].mac, addr, sizeof(struct mac_address))
			!= 0 && i < mob_med_cfg.count_ids) {
		i++;
	}

	return i;
}

/*
 * Calculates the distance between two points on a 2D plane
 */
int find_distance_two_points(int x1, int y1, int x2, int y2) {

	return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));

}

/*
 *	Init all the probability data
 *	Returns a pointer to the probability matrix
 */

double * init_probability(int size) {

	array_size = size;
	indexer = malloc(sizeof(struct mac_address)*array_size);

	if (indexer==NULL) {
		printf("Problem allocating vector");
		exit(1);
	}

 	/*Let's create the matrix */
 	double * mat = malloc(sizeof(double)*(size*size)*IEEE80211_AVAILABLE_RATES);
 	/* Zero-it */
 	memset(mat,0,sizeof(double)*(size*size)*IEEE80211_AVAILABLE_RATES);
 	return mat;
}
