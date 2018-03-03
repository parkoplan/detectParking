Automatic Parking Detection
===========================

This repository contains:

- C++ code
- Python prototype
- 1 video for testing

Running
-------

`detect-parking <Video_Filename or Camera_Number> <ParkingData_Filename>`

Where:
- <Camera_Number> can be 0, 1, ... for webcam or usb camera.
- <ParkingData_Filename> should be simple txt file with lines of: id x1 y1 x2 y2 x3 y3 x4 y4.
  The x,y are the quadrilateral 4 points which mark the parking spot.
  `See \datasets\parkinglot_1.txt` for example.

Required Dlls from OpenCV: opencv_core300.dll, opencv_ffmpeg300.dll, opencv_highgui300.dll, opencv_imgcodecs300.dll, opencv_imgproc300.dll, opencv_videoio300.dll.

Public IP cameras for testing
-----------------------------

 - http://31.16.111.66:8083/cgi-bin/faststream.jpg?stream=full,fps=30 (Parking)
 - http://80.14.234.191:81/mjpg/video.mjpg (Port)

Parameters
----------

You can play with the threshold in `config.cfg`

Building
--------

 - CMake file in project root
 - Meson build file in `cpp` folder

Screenshots
-----------

[Video 1](https://youtu.be/bPeGC8-PQJg)

![Parking Lot 1](/docs/parking_lot_img1.jpg)

![Parking Lot 2](/docs/parking_lot_img2.jpg)
