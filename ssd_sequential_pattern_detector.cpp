/*
 * ssd_sequential_writes_detector.cpp
 *
 *  Created on: Jul 23, 2012
 *      Author: niv
 */

#include "ssd.h"

using namespace ssd;
using namespace std;

#define LIFE_TIME 400 // the number of sequential writes before we recognize the pattern as sequential

Sequential_Pattern_Detector::Sequential_Pattern_Detector()
: registration_counter(0), listener(NULL) {}

Sequential_Pattern_Detector::~Sequential_Pattern_Detector() {
	delete listener;
}

void Sequential_Pattern_Detector::register_event(logical_address lb, double time) {

	// the next LBA is registered
	if (sequential_writes_key_lookup.count(lb) == 1) {
		logical_address key = sequential_writes_key_lookup[lb];
		sequential_writes_tracking& swm = *sequential_writes_identification_and_data[key];
		swm.counter++;
		swm.last_LBA_timestamp = time;
		sequential_writes_key_lookup.erase(lb - 1);
		sequential_writes_key_lookup[lb + 1] = key;
	}
	// the sequential pattern has restarted
	else if (sequential_writes_identification_and_data.count(lb) == 1 && sequential_writes_key_lookup.count(lb) == 0) {
		sequential_writes_tracking& swm = *sequential_writes_identification_and_data[lb];
		swm.num_times_pattern_has_repeated++;
		sequential_writes_key_lookup.erase(lb + swm.counter);
		sequential_writes_key_lookup.erase(lb + swm.counter + 1);
		sequential_writes_key_lookup[lb] = lb;
		sequential_writes_key_lookup[lb + 1] = lb;
		swm.counter = 0;
		printf("SEQUENTIAL PATTERN RESTARTED!\n");
	}
	// the LBA is seen here for the first time
	else if (sequential_writes_key_lookup.count(lb) == 0) {
		sequential_writes_key_lookup[lb + 1] = lb;
		sequential_writes_identification_and_data[lb] = new sequential_writes_tracking(time);
	}


	if (++registration_counter % 50 == 0) {
		remove_old_sequential_writes_metadata(time);
	}
}

int Sequential_Pattern_Detector::get_current_offset(logical_address lb) {
	if (sequential_writes_key_lookup.count(lb) == 0) {
		return -1;
	} else {
		logical_address key = sequential_writes_key_lookup[lb];
		sequential_writes_tracking& swm = *sequential_writes_identification_and_data[key];
		return swm.counter;
	}
}

int Sequential_Pattern_Detector::get_num_times_pattern_has_repeated(logical_address lb) {
	if (sequential_writes_key_lookup.count(lb) == 0) {
		return -1;
	} else {
		logical_address key = sequential_writes_key_lookup[lb];
		sequential_writes_tracking& swm = *sequential_writes_identification_and_data[key];
		return swm.num_times_pattern_has_repeated;
	}
}

double Sequential_Pattern_Detector::get_arrival_time_of_last_io_in_pattern(logical_address lb) {
	if (sequential_writes_key_lookup.count(lb) == 0) {
		return -1;
	} else {
		logical_address key = sequential_writes_key_lookup[lb];
		sequential_writes_tracking& swm = *sequential_writes_identification_and_data[key];
		return swm.last_LBA_timestamp;
	}
}

int Sequential_Pattern_Detector::get_sequential_write_id(logical_address lb) {
	if (sequential_writes_key_lookup.count(lb) == 0) {
		return -1;
	} else {
		logical_address key = sequential_writes_key_lookup[lb];
		return key;
	}
}

void Sequential_Pattern_Detector::set_listener(Sequential_Pattern_Detector_Listener * new_listener) {
	listener = new_listener;
}

/*void Sequential_Pattern_Detector::remove(logical_address lb) {
	if (sequential_writes_key_lookup.count(lb) == 1) {
		logical_address key = sequential_writes_key_lookup[lb];
		delete sequential_writes_identification_and_data[key];
		sequential_writes_identification_and_data.erase(key);
		sequential_writes_key_lookup.erase(lb);
		sequential_writes_key_lookup.erase(lb + 1);
	}
}*/

// TODO: invoke this method from somewhere
void Sequential_Pattern_Detector::remove_old_sequential_writes_metadata(double time) {
	map<logical_address, sequential_writes_tracking*>::iterator iter = sequential_writes_identification_and_data.begin();
	while(iter != sequential_writes_identification_and_data.end())
	{
		logical_address key = (*iter).first;
		if ((*iter).second->last_LBA_timestamp + 400 < time) {
			printf("deleting seq write with key %d:\n", key);
			uint next_expected_lba = (*iter).second->counter + key;
			sequential_writes_key_lookup.erase(next_expected_lba);
			delete (*iter).second;;
			sequential_writes_identification_and_data.erase(iter++);
			if (listener != NULL) {
				listener->sequential_event_metadata_removed(key);
			}
		} else {
			++iter;
		}

	}
}

Sequential_Pattern_Detector::sequential_writes_tracking::sequential_writes_tracking(double time)
	: counter(1),
	  last_LBA_timestamp(time),
	  num_times_pattern_has_repeated(0)
{}

Sequential_Pattern_Detector::sequential_writes_tracking::~sequential_writes_tracking() {
}
