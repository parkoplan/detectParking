#include <stdio.h>
#include <string>
#include <sstream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include "Parking.h"
#include "utils.h"
#include "ConfigLoad.h"

using namespace std;

void fillParkingWithCars(std::vector<cv::Rect>& cars)
{
  ofstream outputFile("parkinglot_new.txt", std::ofstream::out | std::ofstream::trunc);
  int i = 0;
  for (auto car : cars) {
    outputFile << i++ << " " 
      << car.x << " " << car.y << " "
      << car.x + car.width << " " << car.y << " "
      << car.x + car.width << " " << car.y + car.height << " "
      << car.x << " " << car.y + car.height << " "
      << car.x + car.width / 2 << " "
      << car.y + car.height / 2 << " "
      << "0"
      << endl;
  }
  outputFile.close();

}

void detectAndDisplay(cv::Mat frame)
{
  cv::CascadeClassifier cars_cascade; 
  cars_cascade.load("datasets/haarcascades/cars.xml");

  std::vector<cv::Rect> cars;
  cv::Mat frame_gray;
  cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
  equalizeHist(frame_gray, frame_gray);
  //-- Detect cars
  cars_cascade.detectMultiScale(frame_gray, cars, 1.1, 2);
  fillParkingWithCars(cars);
  for (size_t i = 0; i < cars.size(); i++) {
    cv::Point center(cars[i].x + cars[i].width / 2, cars[i].y + cars[i].height / 2);
    cv::ellipse(frame, center, cv::Size(cars[i].width / 2, cars[i].height / 2), 0, 0, 360, cv::Scalar(255, 0, 255), 4, 8, 0);
  }
  //-- Show what you got
  cv::imshow("Video", frame);
}

int main(int argc, char** argv)
{
  if (argc < 3)
  {
    printf("usage: DetectParking.exe <Video_Filename or Camera_Number> <ParkingData_Filename> <-f>\n\n");
    printf("<Camera_Number> can be 0, 1, ... for attached cameras\n");
    printf("<ParkingData_Filename> should be simple txt file with lines of: id x1 y1 x2 y2 x3 y3 x4 y4\n");
    printf("<-f> says that program will generate ParkingData file\n");
    return -1;
  }

  //Load configs
  ConfigLoad::parse();

  const string videoFilename = argv[1];
  vector<Parking> parking_data = parse_parking_file(argv[2]);
  const int delay = 1;

  string argv3 = (argc > 3) ? argv[3] : "";
  bool findParkingPlaces = (argv3 == "-f");

  // Open Camera or Video	File
  cv::VideoCapture cap;

  bool http = videoFilename.find("http") != string::npos;

  if ( videoFilename == "0" || videoFilename == "1" || videoFilename == "2" || http)
  {
    if (http) {
      cout << "Loading IP/htttp camera" << endl;
      if (!cap.open(videoFilename)) {
        cout << "Error opening http caamera" << endl;
        return -1;
      }
    } else {
      cout << "Loading Connected Camera..." << endl;
      if (!cap.open(stoi(videoFilename))) {
        cout << "Error opening caamera" << endl;
        return -1;
      }
    }
    cv::waitKey(500);
  }
  else
  {
    cap.open(videoFilename);
    //cap.set(cv::CAP_PROP_POS_FRAMES, 60000); // jump to frame
  }
  if (!cap.isOpened())
  {
    cout << "Could not open: " << videoFilename << endl;
    return -1;
  }
  const unsigned long int total_frames = cap.get(cv::CAP_PROP_FRAME_COUNT);
  cv::Size videoSize = cv::Size((int)cap.get(cv::CAP_PROP_FRAME_WIDTH),    // Acquire input size
    (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT));

  cv::VideoWriter outputVideo;
  if (ConfigLoad::options["SAVE_VIDEO"] == "true")
  {
    string::size_type pAt = videoFilename.find_last_of('.');                  // Find extension point
    const string videoOutFilename = videoFilename.substr(0, pAt) + "_out.avi";   // Form the new name with container

    //unused?
    //int ex = static_cast<int>(cap.get(cv::CAP_PROP_FOURCC));     // Get Codec Type- Int form

    //cv::VideoWriter::CV_FOURCC('C', 'R', 'A', 'M');
    outputVideo.open(videoOutFilename, -1, cap.get(cv::CAP_PROP_FPS), videoSize, true);
  }

  // Initiliaze variables
  cv::Mat frame, frame_blur, frame_gray, frame_out, roi, laplacian;
  cv::Scalar delta, color;
  char c;  // Char from keyboard
  ostringstream oss;
  cv::Size blur_kernel = cv::Size(5, 5);
  cv::namedWindow("Video", cv::WINDOW_AUTOSIZE);
  double lastUpdateTime = 0;

  // Loop through Video
  while (cap.isOpened())
  {
    cap.read(frame);
    if (frame.empty())
    {
      printf("Error reading frame\n");
      return -1;
    }

    double video_pos_frame = cap.get(cv::CAP_PROP_POS_FRAMES);

    frame_out = frame.clone();

    if (findParkingPlaces) {
      detectAndDisplay(frame_out);
    } else {
      cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
      cv::GaussianBlur(frame_gray, frame_blur, blur_kernel, 3, 3);

      cout << ConfigLoad::options["DETECT_PARKING"];
      printf("\n");

      if (ConfigLoad::options["DETECT_PARKING"] == "true")
      {
        for (Parking& park : parking_data)
        {
          // Check if parking is occupied
          roi = frame_blur(park.getBoundingRect());
          cv::Laplacian(roi, laplacian, CV_64F);
          delta = cv::mean(cv::abs(laplacian), park.getMask());
          park.setStatus(delta[0] < atof(ConfigLoad::options["PARK_LAPLACIAN_TH"].c_str()));
          printf("| %d: d=%-5.1f", park.getId(), delta[0]);
        }
        printf("\n");
      }

      // Parking Overlay
      for (Parking park : parking_data)
      {
        if (park.getStatus())  color = cv::Scalar(0, 255, 0);
        else color = cv::Scalar(0, 0, 255);
        cv::drawContours(frame_out, park.getContourPoints(), -1, color, 2, cv::LINE_AA);

        // Parking ID overlay
        cv::Point p = park.getCenterPoint();
        cv::putText(frame_out, to_string(park.getId()), cv::Point(p.x + 1, p.y + 1), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        cv::putText(frame_out, to_string(park.getId()), cv::Point(p.x - 1, p.y - 1), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        cv::putText(frame_out, to_string(park.getId()), cv::Point(p.x - 1, p.y + 1), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        cv::putText(frame_out, to_string(park.getId()), cv::Point(p.x + 1, p.y - 1), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        cv::putText(frame_out, to_string(park.getId()), p, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
      }

      double time = clock();
      if (lastUpdateTime == 0 || time - lastUpdateTime > 1000) {
        lastUpdateTime = time;
        ofstream outputFile("parking.txt", std::ofstream::out | std::ofstream::trunc);
        for (Parking park : parking_data) {
          outputFile << park.getId() << " " << park.getStatus() << endl;
        }
        outputFile.close();
      }

      // Text Overlay
      oss.str("");
      oss << (unsigned long int)video_pos_frame << "/" << total_frames;
      cv::putText(frame_out, oss.str(), cv::Point(5, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);

      // Save Video
      if (ConfigLoad::options["SAVE_VIDEO"] == "true")
      {
        outputVideo.write(frame_out);
      }
      cv::imshow("Video", frame_out);
    }

    // Show Video
    c = (char)cv::waitKey(delay);
    if (c == 27) break;
  }

  return 0;
}
