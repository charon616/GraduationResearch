//  視線情報のCSVファイルを読み込んでfixationを検出する
//

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/features2d.hpp>

using namespace std;
using namespace cv;

float hThres = 0.3;
int width = 960;
int height = 540;
int cutSize = 60;     // 画像サイズ960*540
int sacThres = 3;     // サッカードの閾値
int normThres = 120;  // アイコン注視の距離閾値
int fixThres = 4;     // 何フレーム見たら注視とするか
int r, g, b;

int mode = 1;         // 単体分析(0), 比較分析(1)

int main(int argc, char** argv) {
    if (mode == 0) {
        // csv読み込み
        string csvName;
        csvName = ("sample.csv");
        ifstream csv(csvName);
        if (!csv) {
            cout << "import failed";
            return -1;
        }
        int type;
        cout << "0: raw  1: fix  2: sal  ->";
        cin >> type;
        
        // 被験者名
        string name;
        cout << "input participant name: ";
        cin >> name;
        
        // csvをmatに格納
        vector<float> temp;
        string str;
        while (getline(csv, str)) {
            string token;
            istringstream stream(str);
            while (getline(stream, token, ',')) {
                temp.push_back(stof(token));
            }
        }
        int row = temp.size() / 3;
        Mat_<float> data = Mat::zeros(row, 3, CV_32F);
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < 3; j++) {
                data.at<float>(i, j) = temp[i * 3 + j];
            }
        }
        
        // 座標直す
        for (int i = 0; i < row; i++) {
            if (data.at<float>(i, 2) > (1 - hThres) && data.at<float>(i, 2) < (1 + hThres)) {
                data.at<float>(i, 0) = data.at<float>(i, 0) / data.at<float>(i, 2);
                data.at<float>(i, 1) = data.at<float>(i, 1) / data.at<float>(i, 2);
            } else {
                data.at<float>(i, 0) = data.at<float>(i, 0) / data.at<float>(i, 2);
                data.at<float>(i, 1) = data.at<float>(i, 1) / data.at<float>(i, 2);
                data.at<float>(i, 2) = 0;  
            }
        }
        
        // 視線移動をセグメンテーション
        vector<int> seq1, seq2, seq3, seq4, seq5; // 視線座標を領域分割した結果を格納
        vector<int> t1, t2, t3, t4, t5;           // その領域にとどまった時間を格納
        vector<int> fix1, fix2, fix3, fix4, fix5; // seqからサッカードを除去したもの
        vector<int> tf1, tf2, tf3, tf4, tf5;      // その領域にとどまった時間を格納
        vector<int> ts1, ts2, ts3, ts4, ts5;      // fixの元動画でのフレーム番号
        int segCol = width / cutSize;
        int lastNum = 0;
        Point lastPoint;
        int frameCount = 0;                       // 重畳動画(frameNum = 1025)でのフレーム数
        Mat fixDraw1(Size(width, height), CV_8UC3, Scalar(0, 0, 0));
        Mat fixDraw2(Size(width, height), CV_8UC3, Scalar(0, 0, 0));
        Mat fixDraw3(Size(width, height), CV_8UC3, Scalar(0, 0, 0));
        Mat fixDraw4(Size(width, height), CV_8UC3, Scalar(0, 0, 0));
        Mat fixDraw5(Size(width, height), CV_8UC3, Scalar(0, 0, 0));
        
        for (int i = 1; i < 16; i++) {
            line(fixDraw1, Point(i * 60, 0), Point(i * 60, height - 1), Scalar(100, 100, 100));
            line(fixDraw2, Point(i * 60, 0), Point(i * 60, height - 1), Scalar(100, 100, 100));
            line(fixDraw3, Point(i * 60, 0), Point(i * 60, height - 1), Scalar(100, 100, 100));
            line(fixDraw4, Point(i * 60, 0), Point(i * 60, height - 1), Scalar(100, 100, 100));
            line(fixDraw5, Point(i * 60, 0), Point(i * 60, height - 1), Scalar(100, 100, 100));
        }
        for (int i = 1; i < 9; i++) {
            line(fixDraw1, Point(0, i * 60), Point(width - 1, i * 60), Scalar(100, 100, 100));
            line(fixDraw2, Point(0, i * 60), Point(width - 1, i * 60), Scalar(100, 100, 100));
            line(fixDraw3, Point(0, i * 60), Point(width - 1, i * 60), Scalar(100, 100, 100));
            line(fixDraw4, Point(0, i * 60), Point(width - 1, i * 60), Scalar(100, 100, 100));
            line(fixDraw5, Point(0, i * 60), Point(width - 1, i * 60), Scalar(100, 100, 100));
        }
        
        // アイコン見てる時のfixationを描画
        // アイコン提示１秒前から描画開始
        
        // アイコン1のfixationを描画
        for (int i = 0; i < 150; i++) {
            if (data.at<float>(i, 2) != 0) {
                int tmp = int(data.at<float>(i, 0) / cutSize + 1) + int(segCol * int(data.at<float>(i, 1) / cutSize));
                if (seq1.empty()) {
                    seq1.push_back(tmp);
                    t1.push_back(1);
                } else if (seq1.back() != tmp) {
                    seq1.push_back(tmp);
                    t1.push_back(1);
                } else {
                    t1.back()++;
                }
            }
        }
        for (int i = 0; i < seq1.size(); i++) {
            if (t1[i] >= sacThres) {
                Point currPoint(data.at<float>(frameCount, 0), data.at<float>(frameCount, 1));
                Scalar color = elementColor(255 * i / seq1.size());
                fix1.push_back(seq1[i]);
                tf1.push_back(t1[i]);
                ts1.push_back(frameCount);
                if (lastNum == 0) {
                    circle(fixDraw1, currPoint, 2 * t1[i], color, 2, 4);
                } else {
                    line(fixDraw1, lastPoint, currPoint, Scalar(0, 0, 255));
                    circle(fixDraw1, currPoint, 2 * t1[i], color, 2, 4);
                }
                lastNum = frameCount;
                lastPoint = currPoint;
            }
            frameCount += t1[i];
        }
        frameCount = 275;
        lastNum = 0;  // 初期化
        
        // アイコン2のfixationを描画
        for (int i = 275; i < 375; i++) {
            if (data.at<float>(i, 2) != 0) {
                int tmp = int(data.at<float>(i, 0) / cutSize + 1) + int(segCol * int(data.at<float>(i, 1) / cutSize));
                if (seq2.empty()) {
                    seq2.push_back(tmp);
                    t2.push_back(1);
                } else if (seq2.back() != tmp) {
                    seq2.push_back(tmp);
                    t2.push_back(1);
                } else {
                    t2.back()++;
                }
            }
        }
        for (int i = 0; i < seq2.size(); i++) {
            if (t2[i] >= sacThres) {
                Point currPoint(data.at<float>(frameCount, 0), data.at<float>(frameCount, 1));
                Scalar color = elementColor(255 * i / seq2.size());
                fix2.push_back(seq2[i]);
                tf2.push_back(t2[i]);
                ts2.push_back(frameCount);
                if (lastNum == 0) {
                    circle(fixDraw2, currPoint, 2 * t2[i], color, 2, 4);
                } else {
                    line(fixDraw2, lastPoint, currPoint, Scalar(0, 0, 255));
                    circle(fixDraw2, currPoint, 2 * t2[i], color, 2, 4);
                }
                lastNum = frameCount;
                lastPoint = currPoint;
            }
            frameCount += t2[i];
        }
        frameCount = 425;
        lastNum = 0;  // 初期化
        
        // アイコン3のfixationを描画
        for (int i = 425; i < 700; i++) {
            if (data.at<float>(i, 2) != 0) {
                int tmp = int(data.at<float>(i, 0) / cutSize + 1) + int(segCol * int(data.at<float>(i, 1) / cutSize));
                if (seq3.empty()) {
                    seq3.push_back(tmp);
                    t3.push_back(1);
                } else if (seq3.back() != tmp) {
                    seq3.push_back(tmp);
                    t3.push_back(1);
                } else {
                    t3.back()++;
                }
            }
        }
        for (int i = 0; i < seq3.size(); i++) {
            if (t3[i] >= sacThres) {
                Point currPoint(data.at<float>(frameCount, 0), data.at<float>(frameCount, 1));
                Scalar color = elementColor(255 * i / seq3.size());
                fix3.push_back(seq3[i]);
                tf3.push_back(t3[i]);
                ts3.push_back(frameCount);
                if (lastNum == 0) {
                    circle(fixDraw3, currPoint, 2 * t3[i], color, 2, 4);
                } else {
                    line(fixDraw3, lastPoint, currPoint, Scalar(0, 0, 255));
                    circle(fixDraw3, currPoint, 2 * t3[i], color, 2, 4);
                }
                lastNum = frameCount;
                lastPoint = currPoint;
            }
            frameCount += t3[i];
        }
        frameCount = 800;
        lastNum = 0;  // 初期化
        
        // アイコン4のfixationを描画
        for (int i = 800; i < 875; i++) {
            if (data.at<float>(i, 2) != 0) {
                int tmp = int(data.at<float>(i, 0) / cutSize + 1) + int(segCol * int(data.at<float>(i, 1) / cutSize));
                if (seq4.empty()) {
                    seq4.push_back(tmp);
                    t4.push_back(1);
                } else if (seq4.back() != tmp) {
                    seq4.push_back(tmp);
                    t4.push_back(1);
                } else {
                    t4.back()++;
                }
            }
        }
        for (int i = 0; i < seq4.size(); i++) {
            if (t4[i] >= sacThres) {
                Point currPoint(data.at<float>(frameCount, 0), data.at<float>(frameCount, 1));
                Scalar color = elementColor(255 * i / seq4.size());
                fix4.push_back(seq4[i]);
                tf4.push_back(t4[i]);
                ts4.push_back(frameCount);
                if (lastNum == 0) {
                    circle(fixDraw4, currPoint, 2 * t4[i], color, 2, 4);
                } else {
                    line(fixDraw4, lastPoint, currPoint, Scalar(0, 0, 255));
                    circle(fixDraw4, currPoint, 2 * t4[i], color, 2, 4);
                }
                lastNum = frameCount;
                lastPoint = currPoint;
            }
            frameCount += t4[i];
        }
        frameCount = 900;
        lastNum = 0;  // 初期化
        
        // アイコン4のfixationを描画
        for (int i = 900; i < 1000; i++) {
            if (data.at<float>(i, 2) != 0) {
                int tmp = int(data.at<float>(i, 0) / cutSize + 1) + int(segCol * int(data.at<float>(i, 1) / cutSize));
                if (seq5.empty()) {
                    seq5.push_back(tmp);
                    t5.push_back(1);
                } else if (seq5.back() != tmp) {
                    seq5.push_back(tmp);
                    t5.push_back(1);
                } else {
                    t5.back()++;
                }
            }
        }
        for (int i = 0; i < seq5.size(); i++) {
            if (t5[i] >= sacThres) {
                Point currPoint(data.at<float>(frameCount, 0), data.at<float>(frameCount, 1));
                Scalar color = elementColor(255 * i / seq5.size());
                fix5.push_back(seq5[i]);
                tf5.push_back(t5[i]);
                ts5.push_back(frameCount);
                if (lastNum == 0) {
                    circle(fixDraw5, currPoint, 2 * t5[i], color, 2, 4);
                } else {
                    line(fixDraw5, lastPoint, currPoint, Scalar(0, 0, 255));
                    circle(fixDraw5, currPoint, 2 * t5[i], color, 2, 4);
                }
                lastNum = frameCount;
                lastPoint = currPoint;
            }
            frameCount += t5[i];
        }
        
        string typeName;
        switch (type) {
            case 0:
                typeName = "raw";
                break;
            case 1:
                typeName = "fix";
                break;
            case 2:
                typeName = "sal";
                break;
        }
        
        string fixName = "fix" + name + typeName + ".csv";
        ofstream fix(fixName);
        fix << "icon1" << endl;
        fix << format(fix1, 2) << endl;
        fix << format(ts1, 2) << endl;
        fix << format(tf1, 2) << endl;
        fix << "" << endl;
        fix << "icon2" << endl;
        fix << format(fix2, 2) << endl;
        fix << format(ts2, 2) << endl;
        fix << format(tf2, 2) << endl;
        fix << "" << endl;
        fix << "icon3" << endl;
        fix << format(fix3, 2) << endl;
        fix << format(ts3, 2) << endl;
        fix << format(tf3, 2) << endl;
        fix << "" << endl;
        fix << "icon4" << endl;
        fix << format(fix4, 2) << endl;
        fix << format(ts4, 2) << endl;
        fix << format(tf4, 2) << endl;
        fix << "" << endl;
        fix << "icon5" << endl;
        fix << format(fix5, 2) << endl;
        fix << format(ts5, 2) << endl;
        fix << format(tf5, 2) << endl;
        
        string drawName1 = "draw" + name + typeName + "1.bmp";
        string drawName2 = "draw" + name + typeName + "2.bmp";
        string drawName3 = "draw" + name + typeName + "3.bmp";
        string drawName4 = "draw" + name + typeName + "4.bmp";
        string drawName5 = "draw" + name + typeName + "5.bmp";
        
        imwrite(drawName1, fixDraw1);
        imwrite(drawName2, fixDraw2);
        imwrite(drawName3, fixDraw3);
        imwrite(drawName4, fixDraw4);
        imwrite(drawName5, fixDraw5);
        
        // アイコンの注視時間を計算
        if (type != 0) {
            vector<int> d1, d2, d3, d4, d5;
            vector<int> start(5, 0);
            vector<int> end(5, 0);
            Point icon1, icon2, icon3, icon4, icon5;
            switch (type) {
                case 1:
                    icon1 = Point(762, 387);
                    icon2 = Point(737, 402);
                    icon3 = Point(737, 327);
                    icon4 = Point(662, 337);
                    icon5 = Point(737, 362);
                    break;
                case 2:
                    icon1 = Point(737, 387);
                    icon2 = Point(362, 362);
                    icon3 = Point(162, 312);
                    icon4 = Point(512, 362);
                    icon5 = Point(712, 362);
                    break;
            }
            for (int i = 25; i < 150; i++) {
                Point gaze(data.at<float>(i, 0), data.at<float>(i, 1));
                d1.push_back(norm(gaze - icon1));
                if (start[0] = 0) {
                    if (norm(gaze - icon1) <= normThres) {
                        start[0] = i;
                    }
                } else if (end[0] = 0) {
                    if (norm(gaze - icon1) > normThres) {
                        end[0] = i;
                        if (end[0] - start[0] < fixThres) {
                            start[0] = 0;
                            end[0] = 0;
                        }
                    }
                }
            }
            for (int i = 300; i < 375; i++) {
                Point gaze(data.at<float>(i, 0), data.at<float>(i, 1));
                d2.push_back(norm(gaze - icon2));
                if (start[1] = 0) {
                    if (norm(gaze - icon2) <= normThres) {
                        start[1] = i;
                    }
                } else if (end[1] = 0) {
                    if (norm(gaze - icon2) > normThres) {
                        end[1] = i;
                        if (end[1] - start[1] < fixThres) {
                            start[1] = 0;
                            end[1] = 0;
                        }
                    }
                }
            }
            for (int i = 450; i < 700; i++) {
                Point gaze(data.at<float>(i, 0), data.at<float>(i, 1));
                d3.push_back(norm(gaze - icon3));
                if (start[2] = 0) {
                    if (norm(gaze - icon3) <= normThres) {
                        start[2] = i;
                    }
                } else if (end[2] = 0) {
                    if (norm(gaze - icon3) > normThres) {
                        end[2] = i;
                        if (end[2] - start[2] < fixThres) {
                            start[2] = 0;
                            end[2] = 0;
                        }
                    }
                }
            }
            for (int i = 825; i < 875; i++) {
                Point gaze(data.at<float>(i, 0), data.at<float>(i, 1));
                d4.push_back(norm(gaze - icon4));
                if (start[3] = 0) {
                    if (norm(gaze - icon4) <= normThres) {
                        start[3] = i;
                    }
                } else if (end[3] = 0) {
                    if (norm(gaze - icon4) > normThres) {
                        end[3] = i;
                        if (end[3] - start[3] < fixThres) {
                            start[3] = 0;
                            end[3] = 0;
                        }
                    }
                }
            }
            for (int i = 925; i < 1000; i++) {
                Point gaze(data.at<float>(i, 0), data.at<float>(i, 1));
                d5.push_back(norm(gaze - icon5));
                if (start[4] = 0) {
                    if (norm(gaze - icon5) <= normThres) {
                        start[4] = i;
                    }
                } else if (end[4] = 0) {
                    if (norm(gaze - icon5) > normThres) {
                        end[4] = i;
                        if (end[4] - start[4] < fixThres) {
                            start[4] = 0;
                            end[4] = 0;
                        }
                    }
                }
            }
            
            ofstream dis("iconDis.csv");
            dis << format(d1, 2) << endl;
            dis << "" << endl;
            dis << format(d2, 2) << endl;
            dis << "" << endl;
            dis << format(d3, 2) << endl;
            dis << "" << endl;
            dis << format(d4, 2) << endl;
            dis << "" << endl;
            dis << format(d5, 2) << endl;
            dis << "icon1, icon2, icon3, icon4, icon5" << endl;
            dis << format(start, 2) << endl;
            dis << format(end, 2) << endl;
        }
        
    } else { // mode == 1
        
        string csvName1;
        cout << "input csv name 1: ";
        cin >> csvName1;
        ifstream csv1(csvName1);
        if (!csv1) {
            cout << "import failed";
            return -1;
        }
        string csvName2;
        cout << "input csv name 2: ";
        cin >> csvName2;
        ifstream csv2(csvName2);
        if (!csv2) {
            cout << "import failed";
            return -1;
        }
        
        // csvをmatに格納
        vector<float> temp1;
        string str1;
        while (getline(csv1, str1)) {
            string token;
            istringstream stream(str1);
            while (getline(stream, token, ',')) {
                temp1.push_back(stof(token));
            }
        }
        int row = temp1.size() / 3;
        Mat_<float> data1 = Mat::zeros(row, 3, CV_32F);
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < 3; j++) {
                data1.at<float>(i, j) = temp1[i * 3 + j];
            }
        }
        
        // 座標直す
        for (int i = 0; i < row; i++) {
            if (data1.at<float>(i, 2) >(1 - hThres) && data1.at<float>(i, 2) < (1 + hThres)) {
                data1.at<float>(i, 0) = data1.at<float>(i, 0) / data1.at<float>(i, 2);
                data1.at<float>(i, 1) = data1.at<float>(i, 1) / data1.at<float>(i, 2);
            } else {
                data1.at<float>(i, 2) = 0;   // 変換以上フレームは0
            }
        }
        
        // csvをMatに格納
        vector<float> temp;
        string str2;
        while (getline(csv2, str2)) {
            string token;
            istringstream stream(str2);
            while (getline(stream, token, ',')) {
                temp.push_back(stof(token));
            }
        }
        Mat_<float> data2 = Mat::zeros(row, 3, CV_32F);
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < 3; j++) {
                data2.at<float>(i, j) = temp[i * 3 + j];
            }
        }
        
        // 座標直す
        for (int i = 0; i < row; i++) {
            if (data2.at<float>(i, 2) >(1 - hThres) && data2.at<float>(i, 2) < (1 + hThres)) {
                data2.at<float>(i, 0) = data2.at<float>(i, 0) / data2.at<float>(i, 2);
                data2.at<float>(i, 1) = data2.at<float>(i, 1) / data2.at<float>(i, 2);
            } else {
                data2.at<float>(i, 0) = data2.at<float>(i, 0) / data2.at<float>(i, 2);
                data2.at<float>(i, 1) = data2.at<float>(i, 1) / data2.at<float>(i, 2);
                data2.at<float>(i, 2) = 0;   // •ÏŠ·ˆÙíƒtƒŒ[ƒ€‚Í0
            }
        }
        
        int frameCount = 75;
        vector<float> d1, d2, d3, d4, d5;
        for (int i = 0; i < 175; i++) {
            Point p1(data1.at<float>(i, 0), data1.at<float>(i, 1));
            Point p2(data2.at<float>(i, 0), data2.at<float>(i, 1));
            d1.push_back(norm(p1 - p2));
        }
        for (int i = 300; i < 375; i++) {
            Point p1(data1.at<float>(i, 0), data1.at<float>(i, 1));
            Point p2(data2.at<float>(i, 0), data2.at<float>(i, 1));
            d2.push_back(norm(p1 - p2));
        }
        for (int i = 450; i < 700; i++) {
            Point p1(data1.at<float>(i, 0), data1.at<float>(i, 1));
            Point p2(data2.at<float>(i, 0), data2.at<float>(i, 1));
            d3.push_back(norm(p1 - p2));
        }
        for (int i = 825; i < 875; i++) {
            Point p1(data1.at<float>(i, 0), data1.at<float>(i, 1));
            Point p2(data2.at<float>(i, 0), data2.at<float>(i, 1));
            d4.push_back(norm(p1 - p2));
        }
        for (int i = 925; i < 1000; i++) {
            Point p1(data1.at<float>(i, 0), data1.at<float>(i, 1));
            Point p2(data2.at<float>(i, 0), data2.at<float>(i, 1));
            d5.push_back(norm(p1 - p2));
        }
        
        ofstream distance("distance.csv");
        distance << format(d1, 2) << endl;
        distance << "" << endl;
        distance << format(d2, 2) << endl;
        distance << "" << endl;
        distance << format(d3, 2) << endl;
        distance << "" << endl;
        distance << format(d4, 2) << endl;
        distance << "" << endl;
        distance << format(d5, 2) << endl;
        
    }
}


/**********************************************
 Buscherのアルゴリズムによるfixationの検出
 *******************************************/


float D_s = 20; //単位はpixel
float D_l = 50;
float sacThres = 0.1;
int noise_num = 0;
bool fixation = false;


//距離D_s以内に0.1秒以上視点が連続して存在していればfixationを始めたと判断
if( > 0.1){
    fixation = true;
    
    //その次の視点とfixationを始めてからの視点との距離が全てD_l以内であればその間もfixationを続けていると判定
    if(distsance < D_l){
        
        noise_num = ;
    }else{
        fixation = false;
        noise_num ++;
    }
    
    //連続で3点以上の視点がノイズと判定されればfixationを終えたと判断
    if(noise_num >= 3;) {
        fixation = false;
        //最初の視点から最後の視点までの重心を計算
        //重心を中心としてfixationを検出
        //最初の視点のタイムスタンプがfixationのタイムスタンプ
        //最初の視点から最後の視点までの時間をfixationの継続時間とする
    }
}

//実際は別ファイルにする
/********************************************
 複数人のfixationからヒートマップを作成
*******************************************/
//それぞれのfixation を中心とした正規分布を作成する
//正規分布の平均μ はfixation の座標であり，N[pixel] を標準偏差σ としている．
//それぞれの正規分布にそれぞれのfixation の時間を掛けることで，分布に重み付けをしている．
//ユーザが多くの時間注目していない部分でも，重要な箇所であることがあるため，提案手法ではこの重み付けは行わない
//この正規分布を足し合わせ，被験者ごとに分布を作成
//分布の値の差による影響を軽減するために，被験者ごとに分布の値をその分布の最大値で割り，分布を正規化する．
ページごとに全ての被験者の正規化された分布を足し合
わせ，一つのヒートマップを作成する．図5 に，ヒートマップ
の例を示す．図5 では，色が赤いほど値が大きいことを示し，
ユーザがその部分を注目していることを示している．また，青
いほど値が小さくユーザがその部分を注目していないことを示
している


#include <random>

int main()
{
    random_device seed_gen;
    default_random_engine engine(seed_gen());
    
    // 平均0.0、標準偏差1.0で分布させる
    normal_distribution<> dist(0.0, 1.0);
    
    std::ofstream file("normal_distribution.tsv");
    for (size_t n = 0; n < 1000 * 1000; ++n) {
        // 正規分布で乱数を生成する
        double result = dist(engine);
        file << result << "\t\n";
    }
}

void FixationToHeatMap(){
    
}












































