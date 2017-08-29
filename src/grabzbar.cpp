
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
#include <boost/thread/scoped_thread.hpp>
#include <iostream>

#include "args.hxx"
#include "../streamer/mjpeg_server.hpp"


boost::mutex mutex;

using namespace Pylon;
using namespace GenApi;
using namespace std;
typedef Pylon::CBaslerGigEInstantCamera Camera_t;
using namespace Basler_GigECameraParams;

bool showImageWindow = false;
bool showDebug = false;
bool barcodeClahe = false;

int thres_start = 60;
int thres_stop = 160;
int thres_step = 20;

cv::Mat currentImage;



int glb_grabExposure=0;
int glb_grabHeight=0;
int glb_imageHeight=0;
int glb_grabBinning=0;
int glb_grabGain=0;


bool is_digits(const std::string &str){
    return std::all_of(str.begin(), str.end(), ::isdigit); // C++11
}

void showImage(cv::Mat& src){
  if (showImageWindow){
    cv::namedWindow("grabzbar", CV_WINDOW_AUTOSIZE );
    cv::imshow("grabzbar", src );
    cv::waitKey(1);
  }
}

void barcode(cv::Mat part,int count) {

  cv::Mat gray;
  cv::Mat norm;
  cv::Mat mask;
  int type = cv::NORM_MINMAX;
  int dtype = -1;
  int min=0;
  int max=255;
  cv::Point point;
  cv::Size ksize(5,5);

  int rel=0;
  int tmp=0;
  bool codeRetry=false;
  if (showDebug){
    //std::cout << "barcode_internal " << count << std::endl;
  }

  cv::Mat image_clahe;
  if (barcodeClahe==true){
    cv::Mat lab_image;
    cv::cvtColor(part, lab_image, CV_BGR2Lab);

    // Extract the L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);  // now we have the L image in lab_planes[0]

    // apply the CLAHE algorithm to the L channel
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(4);
    cv::Mat dst;
    clahe->apply(lab_planes[0], dst);

    // Merge the the color planes back into an Lab image
    dst.copyTo(lab_planes[0]);
    cv::merge(lab_planes, lab_image);

   // convert back to RGB
   cv::cvtColor(lab_image, image_clahe, CV_Lab2BGR);
  }else{
    image_clahe=part.clone();
  }



  // counting here down
  for (int thres=thres_start;((thres<thres_stop)&&( codeRetry==false ));thres+=thres_step){
    gray=image_clahe.clone();
    cv::threshold(gray,gray,thres,255, CV_THRESH_BINARY );
    cv::normalize(gray, norm, min, max, type, dtype, mask);
    cv::GaussianBlur(norm, norm, ksize, 0);

    zbar::ImageScanner scanner;
    scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);

    scanner.set_config(zbar::ZBAR_CODE128, zbar::ZBAR_CFG_ENABLE, 1);
    scanner.set_config(zbar::ZBAR_CODE128, zbar::ZBAR_CFG_ADD_CHECK, 1);
    scanner.set_config(zbar::ZBAR_CODE128, zbar::ZBAR_CFG_EMIT_CHECK, 0);

    scanner.set_config(zbar::ZBAR_CODE39, zbar::ZBAR_CFG_ENABLE, 1);
    scanner.set_config(zbar::ZBAR_CODE39, zbar::ZBAR_CFG_ADD_CHECK, 1);
    scanner.set_config(zbar::ZBAR_CODE39, zbar::ZBAR_CFG_EMIT_CHECK, 0);

    scanner.set_config(zbar::ZBAR_I25, zbar::ZBAR_CFG_ENABLE, 1);
    scanner.set_config(zbar::ZBAR_I25, zbar::ZBAR_CFG_ADD_CHECK, 1);
    scanner.set_config(zbar::ZBAR_I25, zbar::ZBAR_CFG_EMIT_CHECK, 0);


    zbar::Image image(norm.cols, norm.rows, "Y800", (uchar *)norm.data, norm.cols * norm.rows);
    scanner.scan(image);
    for(zbar::Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
      if (showDebug){
        std::cout << "thres " << thres << " Code " << symbol->get_data().c_str() << " Type " << symbol->get_type_name().c_str() << std::endl;
      }
      std::string code = std::string(symbol->get_data().c_str());
      std::string type = std::string(symbol->get_type_name().c_str());

      if (
        (
          ( (type=="I2/5") && (is_digits(code)) ) ||
          ( (type!="I2/5")  )
        ) && (
          code.substr(0,4) != "0000"
        )
      ){

        if (showDebug){
          std::cout << "Code Length: " << code.length()-1 << std::endl;
        }
        if (type=="I2/5"){
          std::cout << type << ": " << code.substr(0,code.length()-1) << std::endl;
        }else{
          std::cout << type << ": " << code << std::endl;
        }

      }



      }
      image.set_data(NULL, 0);
      scanner.recycle_image(image);
    }


  }




int capture(int grabExposure,int grabHeight, int imageHeight, int grabBinning, int grabGain){
  int exitCode = 0;
  int nbBuffers=10*2;
  int totalImageSize=0;
  int grabbedImageSize=0;
  int count=0;

  // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
  // is initialized during the lifetime of this object.
  Pylon::PylonAutoInitTerm autoInitTerm;
  try
  {
      // Create an instant camera object with the camera device found first.
      //CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());

      // Only look for cameras supported by Camera_t
      CDeviceInfo info;
      info.SetDeviceClass( Camera_t::DeviceClass());

      // Create an instant camera object with the first found camera device matching the specified device class.
      Camera_t camera( CTlFactory::GetInstance().CreateFirstDevice( info));


      // Print the model name of the camera.
      cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

      INodeMap& nodemap = camera.GetNodeMap();

      // Open the camera for accessing the parameters.
      camera.Open();

      // Get camera device information.
      cout << "Camera Device Information" << endl
           << "=========================" << endl;
      cout << "Vendor           : "
           << CStringPtr( nodemap.GetNode( "DeviceVendorName") )->GetValue() << endl;
      cout << "Model            : "
           << CStringPtr( nodemap.GetNode( "DeviceModelName") )->GetValue() << endl;
      cout << "Firmware version : "
           << CStringPtr( nodemap.GetNode( "DeviceFirmwareVersion") )->GetValue() << endl;
      cout << "ExposureTimeRaw Min : "
           << CIntegerPtr( nodemap.GetNode( "ExposureTimeRaw") )->GetMin() << endl;
      cout << "ExposureTimeRaw Max : "
           << CIntegerPtr( nodemap.GetNode( "ExposureTimeRaw") )->GetMax() << endl;
      cout << "ExposureTimeBaseAbs Value : "
           <<  nodemap.GetNode( "ExposureTimeBaseAbs")  << endl;

           CIntegerPtr gainRaw(nodemap.GetNode("GainRaw"));

    if(gainRaw.IsValid()) {

     cout << "GainRaw Min : "
          <<  gainRaw->GetMin()  << endl;
     cout << "GainRaw Max : "
          <<  gainRaw->GetMax()  << endl;
     cout << "GainRaw Value : "
          <<  gainRaw->GetValue()  << endl;

    }

            // Camera settings.
      cout << "Camera Device Settings" << endl
           << "======================" << endl;



      camera.BinningHorizontal.SetValue(grabBinning);
      //
      camera.ExposureAuto.SetValue(ExposureAuto_Off);
      camera.ExposureTimeRaw.SetValue(grabExposure);
      //camera.ExposureTime.GetMin()

      CIntegerPtr width( nodemap.GetNode( "Width"));
      CIntegerPtr height( nodemap.GetNode( "Height"));
      height->SetValue(grabHeight);


      if(camera.PixelFormat.GetValue() == PixelFormat_Mono8){
        totalImageSize = camera.Width.GetValue()*imageHeight;
        currentImage = cv::Mat(imageHeight, camera.Width.GetValue(), CV_8UC1, cv::Scalar(0));
      }else{

      }

      // Access the PixelFormat enumeration type node.
      CEnumerationPtr pixelFormat( nodemap.GetNode( "PixelFormat"));
      // Remember the current pixel format.
      String_t oldPixelFormat = pixelFormat->ToString();
      cout << "Old PixelFormat  : " << oldPixelFormat << endl;

      // Set the pixel format to Mono8 if available.
      if ( IsAvailable( pixelFormat->GetEntryByName( "Mono8")))
      {
          pixelFormat->FromString( "Mono8");
          cout << "New PixelFormat  : " << pixelFormat->ToString() << endl;
      }

      // Get the image buffer size
      const size_t imageSize = (size_t)(camera.PayloadSize.GetValue());


      // The parameter MaxNumBuffer can be used to control the count of buffers
      // allocated for grabbing. The default value of this parameter is 10.
      camera.MaxNumBuffer = nbBuffers;
      //camera.MaxBufferSize = imageSize;
      //camera.MaxBufferSize.SetValue(imageSize);

      // We won't queue more than nbBuffers image buffers at a time
      //camera.MaxNumBuffer.SetValue(nbBuffers);

      // Start the grabbing of c_countOfImagesToGrab images.
      // The camera device is parameterized with a default configuration which
      // sets up free-running continuous acquisition.
      camera.StartGrabbing();// c_countOfImagesToGrab);

      // This smart pointer will receive the grab result data.
      CGrabResultPtr ptrGrabResult;
      int cthread=0;
      // Camera.StopGrabbing() is called automatically by the RetrieveResult() method
      // when c_countOfImagesToGrab images have been retrieved.
      while ( camera.IsGrabbing()){
          // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
          camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException);

          // Image grabbed successfully?
          if (ptrGrabResult->GrabSucceeded()){
              // Access the image data.
              grabbedImageSize = ptrGrabResult->GetWidth()*ptrGrabResult->GetHeight();
              std::memcpy( currentImage.ptr(), currentImage.ptr()+grabbedImageSize, totalImageSize-grabbedImageSize);
              std::memcpy( currentImage.ptr()+totalImageSize-grabbedImageSize, ptrGrabResult->GetBuffer(), grabbedImageSize );
              cv::Mat img = currentImage.clone();
              //barcode(img);
              if (count % 3==0){
                boost::thread mythread(barcode,img,count);
              }


              showImage(currentImage);


              if (showDebug){
                if (count % 100==0){
                  std::cout << "count: " << count << " width: " << ptrGrabResult->GetWidth()  << std::endl;
                }

              }
              if (count>65500){
                count=0;
              }
              count++;

          }else{
              cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
          }
      }
  }catch (GenICam::GenericException &e){
    // Error handling.
    cerr << "An exception occurred." << endl
    << e.GetDescription() << endl
    << e.what() << endl;
    exitCode = 1;
  }
  return exitCode;
}

void run_capture(){
   capture(glb_grabExposure,glb_grabHeight, glb_imageHeight, glb_grabBinning, glb_grabGain);
}


void run_streamer()
{
    using namespace http::server;
    // Run server in background thread.
    std::size_t num_threads = 3;
    std::string doc_root = "./";
    //this initializes the redirect behavor, and the /_all handlers
    server_ptr s = init_streaming_server("0.0.0.0", "8080", doc_root, num_threads);
    streamer_ptr stmr(new streamer);//a stream per image, you can register any number of these.
    register_streamer(s, stmr, "/stream_0");
    s->start();
    while (true){


      mutex.lock();
      std::cout << "run streamer" << std::endl;

      cv::Mat image = currentImage.clone();
      std::cout << "run streamer -" << currentImage.cols << std::endl;
      mutex.unlock();

      //fill image somehow here. from camera or something.
      bool wait = false; //don't wait for there to be more than one webpage looking at us.
      int quality = 75; //quality of jpeg compression [0,100]
      int ms = 33;
      if( (currentImage.cols==0)|| (currentImage.rows>0)){
        image = cv::Mat(cv::Size(640, 480), CV_8UC3, cv::Scalar(std::rand() % 255, std::rand() % 255, std::rand() % 255));
        ms = 1000;
      }
      int n_viewers = stmr->post_image(image,quality, wait);

      //use boost sleep so that our loop doesn't go out of control.
      boost::this_thread::sleep(boost::posix_time::milliseconds(ms)); //30 FPS
    }
}


int main(int argc, char* argv[])
{


    int exitCode = 0;

    args::ArgumentParser parser("Ocrs reconize barcodes live from camera.", "");
    args::HelpFlag help(parser, "help", "Display this help menu", { "help"});
    args::Flag debug(parser, "debug", "Show debug messages", {'d', "debug"});
    args::Flag window(parser, "window", "Show image window", {'w', "window"});

    args::ValueFlag<int> exposure(parser, "exposure", "exposure (1300)", {"exposure"});
    args::ValueFlag<int> lineheight(parser, "lineheight", "height of one captured image (32)", { "lineheight"});
    args::ValueFlag<int> height(parser, "height", "max image height", { "height"});

    args::ValueFlag<int> binning(parser, "binning", "binning (default 1)", {"binning"});
    args::ValueFlag<int> gain(parser, "gain", "gain (default 800)", { "gain"});
    //args::ValueFlag<std::string> filename(parser, "filename", "The filename", {'f',"file"});


    try{
        parser.ParseCLI(argc, argv);
    }catch (args::Help){
        std::cout << parser;
        return 0;
    }catch (args::ParseError e){
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    if (window==1){
      showImageWindow=true;
    }
    if (debug==1){
      showDebug=true;
    }

    int int_exposure = 1300;
    if (exposure) { int_exposure = args::get(exposure); }
    int int_height = 400;
    if (height) { int_height = args::get(height); }

    int int_gain = 800;
    if (gain) { int_gain = args::get(gain); }
    int int_binning = 1;
    if (binning) { int_binning = args::get(binning); }
    int int_lineheight = 32;
    if (lineheight) { int_lineheight = args::get(lineheight); }


    glb_grabExposure=int_exposure;
    glb_grabHeight=int_lineheight;
    glb_imageHeight=int_height;
    glb_grabBinning=int_binning;
    glb_grabGain=int_gain;

    boost::thread t_streamer{run_streamer};
    boost::thread t_capture{run_capture};
    t_capture.join();
    t_streamer.join();
    //capture(int_exposure,int_lineheight,int_height,int_binning,int_gain);
    return exitCode;
}
