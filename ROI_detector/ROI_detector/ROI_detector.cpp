// ROI_detector.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <Windows.h>
#include <tobii_research.h>
#include <tobii_research_eyetracker.h>
#include <tobii_research_streams.h>
#include "tobii_research_calibration.h"

//アイトラッカーの取得のサンプル
TobiiResearchStatus find_all_eyetrackers_example() {
	TobiiResearchEyeTrackers* eyetrackers = NULL;
	TobiiResearchStatus result;
	size_t i = 0;
	result = tobii_research_find_all_eyetrackers(&eyetrackers);
	if (result != TOBII_RESEARCH_STATUS_OK) {
		printf("Finding trackers failed. Error: %d\n", result);
		return result;
	}
	for (i = 0; i < eyetrackers->count; i++) {
		TobiiResearchEyeTracker* eyetracker = eyetrackers->eyetrackers[i];
		char* address;
		char* serial_number;
		char* device_name;
		tobii_research_get_address(eyetracker, &address);
		tobii_research_get_serial_number(eyetracker, &serial_number);
		tobii_research_get_device_name(eyetracker, &device_name);
		printf("%s\t%s\t%s\n", address, serial_number, device_name);
		tobii_research_free_string(address);
		tobii_research_free_string(serial_number);
		tobii_research_free_string(device_name);
	}
	printf("Found %d Eye Trackers \n\n", (int)eyetrackers->count);
	tobii_research_free_eyetrackers(eyetrackers);
	return result;
}

static void sleep_ms(int time) {
	Sleep(time);
}

void gaze_data_callback(TobiiResearchGazeData* gaze_data, void* user_data) {
	memcpy(user_data, gaze_data, sizeof(*gaze_data));
}

//見ている位置の取得のサンプル
void gaze_data_example(TobiiResearchEyeTracker* eyetracker) {
	TobiiResearchGazeData gaze_data;
	char* serial_number;
	tobii_research_get_serial_number(eyetracker, &serial_number);
	printf("Subscribing to gaze data for eye tracker with serial number %s.\n", serial_number);
	tobii_research_free_string(serial_number);
	TobiiResearchStatus status = tobii_research_subscribe_to_gaze_data(eyetracker, gaze_data_callback, &gaze_data);
	if (status != TOBII_RESEARCH_STATUS_OK)
		return;
	/* Wait while some gaze data is collected. */
	sleep_ms(2000);
	status = tobii_research_unsubscribe_from_gaze_data(eyetracker, gaze_data_callback);
	printf("Unsubscribed from gaze data with status %i.\n", status);
	printf("Last received gaze package:\n");
	printf("System time stamp: %"  PRId64 "\n", gaze_data.system_time_stamp);
	printf("Device time stamp: %"  PRId64 "\n", gaze_data.device_time_stamp);
	printf("Left eye 2D gaze point on display area: (%f, %f)\n",
		gaze_data.left_eye.gaze_point.position_on_display_area.x,
		gaze_data.left_eye.gaze_point.position_on_display_area.y);
	printf("Right eye 3d gaze origin in user coordinates (%f, %f, %f)\n",
		gaze_data.right_eye.gaze_origin.position_in_user_coordinates.x,
		gaze_data.right_eye.gaze_origin.position_in_user_coordinates.y,
		gaze_data.right_eye.gaze_origin.position_in_user_coordinates.z);
	/* Wait while some gaze data is collected. */
	sleep_ms(2000);
}


//キャリブレーションのサンプル
void calibration_example(TobiiResearchEyeTracker* eyetracker) {
	/* Enter calibration mode. */
	TobiiResearchStatus status = tobii_research_screen_based_calibration_enter_calibration_mode(eyetracker);
	char* serial_number;
	tobii_research_get_serial_number(eyetracker, &serial_number);
	printf("Entered calibration mode for eye tracker with serial number %s \n.", serial_number);
	tobii_research_free_string(serial_number);
	/* Define the points on screen we should calibrate at. */
	/* The coordinates are normalized, i.e. (0.0, 0.0) is the upper left corner and (1.0, 1.0) is the lower right corner. */
	{
#define NUM_OF_POINTS  5U
		TobiiResearchNormalizedPoint2D points_to_calibrate[NUM_OF_POINTS] = \
		{ {0.5f, 0.5f}, { 0.1f, 0.1f }, { 0.1f, 0.9f }, { 0.9f, 0.1f }, { 0.9f, 0.9f }};
		size_t i = 0;
		for (; i < NUM_OF_POINTS; i++) {
			TobiiResearchNormalizedPoint2D* point = &points_to_calibrate[i];
			printf("Show a point on screen at (%f,%f).\n", point->x, point->y);
			//  Wait a little for user to focus.
			sleep_ms(700);
			printf("Collecting data at (%f,%f).\n", point->x, point->y);
			if (tobii_research_screen_based_calibration_collect_data(eyetracker, point->x, point->y) != TOBII_RESEARCH_STATUS_OK) {
				/* Try again if it didn't go well the first time. */
				/* Not all eye tracker models will fail at this point, but instead fail on ComputeAndApply. */
				tobii_research_screen_based_calibration_collect_data(eyetracker, point->x, point->y);
			}
		}
		printf("Computing and applying calibration.\n");
		TobiiResearchCalibrationResult* calibration_result = NULL;
		status = tobii_research_screen_based_calibration_compute_and_apply(eyetracker, &calibration_result);
		if (status == TOBII_RESEARCH_STATUS_OK && calibration_result->status == TOBII_RESEARCH_CALIBRATION_SUCCESS) {
			printf("Compute and apply returned %i and collected at %zu points.\n", status, calibration_result->calibration_point_count);
		}
		else {
			printf("Calibration failed!\n");
		}
		/* Free calibration result when done using it */
		tobii_research_free_screen_based_calibration_result(calibration_result);
		/* Analyze the data and maybe remove points that weren't good. */
		TobiiResearchNormalizedPoint2D* recalibrate_point = &points_to_calibrate[1];
		printf("Removing calibration point at (%f,%f).\n", recalibrate_point->x, recalibrate_point->y);
		status = tobii_research_screen_based_calibration_discard_data(eyetracker, recalibrate_point->x, recalibrate_point->y);
		/* Redo collection at the discarded point */
		printf("Show a point on screen at (%f,%f).\n", recalibrate_point->x, recalibrate_point->y);
		tobii_research_screen_based_calibration_collect_data(eyetracker, recalibrate_point->x, recalibrate_point->y);
		/* Compute and apply again. */
		printf("Computing and applying calibration.\n");
		status = tobii_research_screen_based_calibration_compute_and_apply(eyetracker, &calibration_result);
		if (status == TOBII_RESEARCH_STATUS_OK && calibration_result->status == TOBII_RESEARCH_CALIBRATION_SUCCESS) {
			printf("Compute and apply returned %i and collected at %zu points.\n", status, calibration_result->calibration_point_count);
		}
		else {
			printf("Calibration failed!\n");
		}
		/* Free calibration result when done using it */
		tobii_research_free_screen_based_calibration_result(calibration_result);
		/* See that you're happy with the result. */
	}
	/* The calibration is done. Leave calibration mode. */
	status = tobii_research_screen_based_calibration_leave_calibration_mode(eyetracker);
	printf("Left calibration mode.\n");
}


int main() {
	//find_all_eyetrackers_example();

	TobiiResearchEyeTrackers* eyetrackers = NULL;
	size_t i = 0;
	TobiiResearchStatus result;
	result = tobii_research_find_all_eyetrackers(&eyetrackers);
	TobiiResearchEyeTracker* eyetracker = eyetrackers->eyetrackers[i];
	
	//calibration_example(eyetracker);
	//gaze_data_example(eyetracker);
	
	return 0;
}
