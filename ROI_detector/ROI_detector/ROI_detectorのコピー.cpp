// 視線情報を受け取ってCSVに書き込む
//

#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <fstream>
#include <Windows.h>

#include <tobii_research.h>
#include <tobii_research_eyetracker.h>
#include <tobii_research_streams.h>
#include "tobii_research_calibration.h"

int count = 200;

static void sleep_ms(int time) {
    Sleep(time);
}

void gaze_data_callback(TobiiResearchGazeData* gaze_data, void* user_data) {
    memcpy(user_data, gaze_data, sizeof(*gaze_data));
}

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
    
    int timestamp = gaze_data.system_time_stamp;
    float[count] left_x, left_y, right_x, right_y;
    //******************************
    //CSVへ書き出し
    fstream fs;
    //ファイルを開く
    fs.open("/Users/karinkiho/Desktop/output.csv", ios::out);
    if(! fs.is_open()) {
        return EXIT_FAILURE;
    }
    // ヘッダー行を書き出す
    fs << "TimeStamp, left_x, left_y, right_x, right_y" << endl;

    for( i = 0; i < count; i++ ) {
        
        left_x[i] = gaze_data.left_eye.gaze_point.position_on_display_area.x;
        left_y[i] = gaze_data.left_eye.gaze_point.position_on_display_area.y;
        right_x[i] = gaze_data.right_eye.gaze_point.position_on_display_area.x;
        left_y[i] = gaze_data.right_eye.gaze_point.position_on_display_area.y;
        
        fprintf( fp, "%,%f,%f,%f,%f\n"
                timestamp, left_x[i], left_y[i], right_x[i], right_y[i] );
    }
    
    
    fs.close();
    cout << "Export to csv has been completed!" << endl;
    
    //******************************

    /* Wait while some gaze data is collected. */
    sleep_ms(2000);
}

int main() {
    //アイトラッカーの取得
    TobiiResearchEyeTrackers* eyetrackers = NULL;
    size_t i = 0;
    TobiiResearchStatus result;
    result = tobii_research_find_all_eyetrackers(&eyetrackers);
    TobiiResearchEyeTracker* eyetracker = eyetrackers->eyetrackers[i];
    
    gaze_data_example(eyetracker);

	return 0;
}
