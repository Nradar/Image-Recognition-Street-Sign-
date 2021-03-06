#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
using namespace cv;
using namespace std;
#define WARPED_XSIZE 200
#define WARPED_YSIZE 300



Mat src; Mat src_gray, warped_result;
Mat speed_80, speed_40;
int canny_thresh = 120;


#define VERY_LARGE_VALUE 100000

#define NO_MATCH    0
#define STOP_SIGN            1
#define SPEED_LIMIT_40_SIGN  2
#define SPEED_LIMIT_80_SIGN  3


int main(int argc, char** argv)
{
	int readResult = NO_MATCH;
	speed_40 = imread("speed_40.bmp", 1);
	speed_80 = imread("speed_80.bmp", 1);

	// you run your program on these three examples (uncomment the two lines below)
	//string sign_name = "stop4";
	//string sign_name = "speedsign12";
	//string sign_name = "speedsign13";
	//string sign_name = "speedsign14";
	//string sign_name = "speedsign3";
	string sign_name = "speedsign4";
	//string sign_name = "speedsign5"; //erase blackbar
	string final_sign_input_name = sign_name + ".jpg";
	string final_sign_output_name = sign_name + "_result" + ".jpg";

	/// Load source image and convert it to gray
	src = imread(final_sign_input_name, 1);

	/// Convert image to gray and blur it
	cvtColor(src, src_gray, COLOR_BGR2GRAY);
	blur(src_gray, src_gray, Size(2, 2));
	warped_result = Mat(Size(WARPED_XSIZE, WARPED_YSIZE), src_gray.type());

	// here you add the code to do the recognition, and set the variable 
	// readResult to one of STOP_SIGN, SPEED_LIMIT_40_SIGN, SPEED_LIMIT_80_SIGN, or NO_MATCH

	
	Mat afterCanny;
	Canny(src_gray, afterCanny, 120, 120 * 2, 3);

	
	vector<vector<Point>> afterContour;
	vector<Vec4i> hierarchy;
	vector<Point> contour_polygon;
	int largest_area = 1000;
	//find contours
	findContours(afterCanny, afterContour, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE, Point(0, 0));
	
	

	Mat dstImg(src.size(), CV_8UC3, Scalar::all(0));

	//used to store the polygon with largest area
	vector<Point> largest_polygon;

	for (int i = 0; i < afterContour.size(); i = hierarchy[i][0]) {
		approxPolyDP(Mat(afterContour[i]), contour_polygon, arcLength(Mat(afterContour[i]), true)*0.02, true);

		

		//use the contour with largest area only
		if (isContourConvex(contour_polygon), fabs(contourArea(Mat(contour_polygon))) > largest_area) {
			//find the area of that countour
			largest_area = fabs(contourArea(Mat(contour_polygon)));
			largest_polygon = contour_polygon;
		}
	}


	

	//if the countour is 8 side, it is a stop sign
	if (largest_polygon.size() == 8) {
		readResult = STOP_SIGN;
	}
	//if the contour is 4 side, it is a speed sign
	else if (largest_polygon.size() == 4) {
		Point2f inputCoor[4], outputCoor[4];
		Mat perspectiveTransformMat(2, 4, CV_32FC1);
		Mat output;
		perspectiveTransformMat = Mat::zeros(src.rows, src.cols, src.type());

		//4 different corner inedex of the speed sign
		int corner0 = largest_polygon[0].x + largest_polygon[0].y;
		int corner1 = largest_polygon[1].x + largest_polygon[1].y;
		int corner2 = largest_polygon[2].x + largest_polygon[2].y;
		int corner3 = largest_polygon[3].x + largest_polygon[3].y;

		//the top left corner should have the minimum index
		int topLeftCorner = min(min(min(corner0, corner1), corner2), corner3);
		int bottomRightCorner = max(max(max(corner0, corner1), corner2), corner3);
		//the bottom right corner should have the maximum index

		//find which corner is the top left
		if (corner0 == topLeftCorner) {
			inputCoor[0] = largest_polygon[0];
		}
		else if (corner1 == topLeftCorner) {
			inputCoor[0] = largest_polygon[1];
		}
		else if (corner2 == topLeftCorner) {
			inputCoor[0] = largest_polygon[2];
		}
		else if (corner3 == topLeftCorner) {
			inputCoor[0] = largest_polygon[3];
		}

		//find top right point and bottom left point
		for (int i = 0; i< largest_polygon.size(); i++) {
			for (int j = 0; j < largest_polygon.size(); j++) {
				int currentSum = largest_polygon[j].x + largest_polygon[j].y;
				int currentSum2 = largest_polygon[i].x + largest_polygon[i].y;
				if (currentSum2 != topLeftCorner && currentSum2 != bottomRightCorner) {
					if (currentSum != topLeftCorner && currentSum != bottomRightCorner && j != i) {
						if (largest_polygon[i].x > largest_polygon[j].x) {
							inputCoor[1] = largest_polygon[i];
							inputCoor[2] = largest_polygon[j];
							break;
						}
						else if (largest_polygon[i].x <= largest_polygon[j].x) {
							inputCoor[1] = largest_polygon[j];
							inputCoor[2] = largest_polygon[i];
							break;
						}
					}
				}
			}
		}

		//find bottom right point
		if (corner0 == bottomRightCorner) {
			inputCoor[3] = largest_polygon[0];
		}
		else if (corner1 == bottomRightCorner) {
			inputCoor[3] = largest_polygon[1];
		}
		else if (corner2 == bottomRightCorner) {
			inputCoor[3] = largest_polygon[2];
		}
		else if (corner3 == bottomRightCorner) {
			inputCoor[3] = largest_polygon[3];
		}


		//create output coordinates
		outputCoor[0] = Point2f(0, 0);
		outputCoor[1] = Point2f(speed_40.cols - 1, 0);
		outputCoor[2] = Point2f(0, speed_40.rows - 1);
		outputCoor[3] = Point2f(speed_40.cols - 1, speed_40.rows - 1);

		//perform perspective transforma and warp image
		perspectiveTransformMat = getPerspectiveTransform(inputCoor, outputCoor);
		warpPerspective(src, output, perspectiveTransformMat, speed_40.size());

		//match with templates
		Mat match_result_40, match_result_80;
		matchTemplate(output, speed_40, match_result_40, TM_CCOEFF_NORMED);
		matchTemplate(output, speed_80, match_result_80, TM_CCOEFF_NORMED);

		//localizing the best match with minMaxLoc
		double min40, max40;
		double min80, max80;
		Point minLoc, maxLoc;
		minMaxLoc(match_result_40, &min40, &max40, &minLoc, 0);
	
		minMaxLoc(match_result_80, &min80, &max80, &minLoc, 0);
		

		Mat displayImg = output.clone();
		rectangle(displayImg, minLoc, Point(minLoc.x + speed_40.cols, minLoc.y + speed_40.rows), Scalar::all(0), 3);

		//filter the borader
		double discardThreshold;

		
		if (min80 >= 0.1 && min40 >= 0.1) {
			discardThreshold = 0.4;
		}
		else {
			discardThreshold = 0.04;
		}

		//final determination
		if (min80 >= min40 && min80 >= discardThreshold) {
			readResult = SPEED_LIMIT_80_SIGN;
		}
		else if (min80 < min40 && min40 >= discardThreshold) {
			readResult = SPEED_LIMIT_40_SIGN;
		}
		else {
			readResult = NO_MATCH;
		}

		
		imshow("Input", src);
		imshow("Template", displayImg);
	}





	string text;
	if (readResult == SPEED_LIMIT_40_SIGN) text = "Speed 40";
	else if (readResult == SPEED_LIMIT_80_SIGN) text = "Speed 80";
	else if (readResult == STOP_SIGN) text = "Stop";
	else if (readResult == NO_MATCH) text = "Fail";

	int fontFace = FONT_HERSHEY_SCRIPT_SIMPLEX;
	double fontScale = 2;
	int thickness = 3;
	cv::Point textOrg(10, 130);
	cv::putText(src, text, textOrg, fontFace, fontScale, Scalar::all(255), thickness, 8);

	/// Create Window
	String source_window = "Result";
	namedWindow(source_window, WINDOW_AUTOSIZE);
	imshow(source_window, src);
	imwrite(final_sign_output_name, src);

	waitKey(0);

	return(0);
}