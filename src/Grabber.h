// Grabber.h
#ifndef GRABBER_H
#define GRABBER_H


#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <zbar.h>

#include <pylon/PylonIncludes.h>
#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <pylon/gige/BaslerGigEInstantCamera.h>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>

#include <boost/thread/scoped_thread.hpp>
#include <iostream>


#include "../mjpeg/mjpeg_server.hpp"


#include "../../ocrs/new/ocr_ext.h"
#include "../../ocrs/new/ImageRecognizeEx.h"
#include "../../ocrs/new/glob_posix.h"



using namespace Pylon;
using namespace GenApi;
typedef Pylon::CBaslerGigEInstantCamera Camera_t;
using namespace Basler_GigECameraParams;

class Grabber{
public:
  Grabber();
  ~Grabber();

  void run();
  void beep();
  void setStoreImagePath(std::string value);
  std::string getStoreImagePath();
  void setResultImagePath(std::string value);
  std::string getResultImagePath();
  std::string getFileName();
  std::string getResultState();

  void dbConnect();


void configCameraAVG(int start,int stop);

  void configCamera(
    int grabExposure,
    int grabHeight,
    int maxImageHeight,
    int grabBinning,
    int grabGain
  );

  void configConnection(
    std::string machine,
    std::string db_host,
    std::string db_user,
    std::string db_name,
    std::string db_password,
    std::string db_encoding
  );
  void configOCR(
    bool debug,
    bool debugtime,
    int blocksize,
    int substractmean,
    int pixel_cm_x,
    int pixel_cm_y,
    bool calculatemean,
    bool bc_clahe,
    int bc_thres_start,
    int bc_thres_stop,
    int bc_thres_step,
    float float_meanfaktor,
    bool noocr,
    bool rls
  );
private:
  int runningTasks;




  std::string storePath;
  std::string resultPath;

  int maxTasks;
  int avgStepSize;

  bool canStartTask();

  void run_streamer();
  void run_capture();
  std::string getFileName(std::string customer);
  void ocrthread(cv::Mat img);
  void startocr(cv::Mat img);



  std::string str_machine;
  std::string str_db_host;
  std::string str_db_user;
  std::string str_db_name;
  std::string str_db_password;
  std::string str_db_encoding;


  int i_blockSize;
  int i_substractMean;
  bool b_debug;
  bool b_debugtime;

  int i_pixel_cm_x;
  int i_pixel_cm_y;
  bool b_calculatemean;


  bool b_forceFPCode;
  bool b_bc_clahe;
  int i_bc_thres_start;
  int i_bc_thres_stop;
  int i_bc_thres_step;

  int glb_grabExposure;
  int glb_grabHeight;
  int glb_maxImageHeight;
  int glb_grabBinning;
  int glb_grabGain;


  int avg_start;
  int avg_stop;

  float f_meanfactor;

  cv::Mat currentImage;
  cv::Mat streamImage;

  boost::mutex mutex;

  bool b_noocr;
  bool b_rls;

  MYSQL *con;

};
#endif
