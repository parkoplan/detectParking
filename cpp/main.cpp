#include <stdio.h>
#include <string>
#include <sstream>
#include <fstream>

#include <opencv2/opencv.hpp>

#include "Parking.h"
#include "utils.h"
#include "ConfigLoad.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;

#ifdef _WIN32
cv::Mat hwnd2mat(HWND hwnd) 
{
  HDC hwindowDC, hwindowCompatibleDC;

  int height, width, srcheight, srcwidth;
  HBITMAP hbwindow;
  cv::Mat src;
  BITMAPINFOHEADER  bi;

  hwindowDC = GetDC(hwnd);
  hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
  SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

  RECT windowsize;    // get the height and width of the screen
  GetClientRect(hwnd, &windowsize);

  int srcTop = 100;
  int srcLeft = 0;
  srcheight = windowsize.bottom * 0.75;
  srcwidth = windowsize.right / 2;
  height = windowsize.bottom * 0.75;
  width = windowsize.right / 2;

  src.create(height, width, CV_8UC4);

  // create a bitmap
  hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
  bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
  bi.biWidth = width;
  bi.biHeight = -height;  //this is the line that makes it draw upside down or not
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;

  // use the previously created device context with the bitmap
  SelectObject(hwindowCompatibleDC, hbwindow);
  // copy from the window device context to the bitmap device context
  StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, srcLeft, srcTop, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
  GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

                                                                                                     // avoid memory leak
  DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hwnd, hwindowDC);

  return src;
}
#endif

void fillParkingWithCars(std::vector<cv::Rect>& cars)
{
  ofstream outputFile("parkinglot_new.txt", ofstream::out | ofstream::trunc);
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

  vector<cv::Rect> cars;
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

#ifdef _WIN32
  bool screenCapture = (videoFilename == "screen");
#else
  bool screenCapture = false;
#endif

  // Open Camera or Video	File
  cv::VideoCapture cap;

  if ( videoFilename == "0" || videoFilename == "1" || videoFilename == "2")
  {
    cout << "Loading Connected Camera..." << endl;
    if (!cap.open(stoi(videoFilename))) {
      cout << "Error opening camera" << endl;
      return -1;
    }
    cv::waitKey(500);
  } else {
    cap.open(videoFilename);
  }
  if (!cap.isOpened() && !screenCapture)
  {
    cout << "Could not open http or file stream: " << videoFilename << endl;
    return -1;
  }

  // Initiliaze variables
  cv::Mat frame, frame_blur, frame_gray, frame_out, roi, laplacian;
  cv::Scalar delta, color;
  char c;  // Char from keyboard
  ostringstream oss;
  cv::Size blur_kernel = cv::Size(5, 5);
  cv::namedWindow("Video", cv::WINDOW_AUTOSIZE);
  double lastUpdateTime = 0;

#ifdef _WIN32
  HWND hwndDesktop = GetDesktopWindow();
#endif

  // Loop through Video
  while (true) {
    if (screenCapture) {
#ifdef _WIN32
      frame = hwnd2mat(hwndDesktop);
#else
      cout << "screenCapture is unsupported on linux" << endl;
      return 0;
#endif
    } else {
      cap.read(frame);
    }

    if (frame.empty())
    {
      cout << "Error reading frame" << endl;
      // okay, use previous frame if present
      if (!frame_out.empty()) {
        frame = frame_out;
      } else {
        return -1;
      }
    } else {
      frame_out = frame.clone();
    }

    if (findParkingPlaces) {
      detectAndDisplay(frame_out);
    } else {
      cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
      cv::GaussianBlur(frame_gray, frame_blur, blur_kernel, 3, 3);

      cout << ConfigLoad::options["DETECT_PARKING"] << endl;

      if (ConfigLoad::options["DETECT_PARKING"] == "true" && !findParkingPlaces)
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
        color = (park.getStatus())
            ? cv::Scalar(0, 255, 0)
            : cv::Scalar(0, 0, 255);
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
      if (lastUpdateTime == 0 || time - lastUpdateTime > 100) {
        lastUpdateTime = time;
        // deploy parking status
        ofstream outputFile("parking.txt", ofstream::out | ofstream::trunc);
        for (Parking park : parking_data) {
          outputFile << park.getId() << " " << park.getStatus() << endl;
        }
        outputFile.close();
      }

      cv::imshow("Video", frame_out);
    }

    // Show Video
    c = (char)cv::waitKey(delay);

    // escape actually
    if (c == 27) break;
  }

  return 0;
}
