#include "Grabber.h"

Grabber::Grabber(int tasks):
  runningTasks(0),
  grabExposure(1300),
  grabHeight(32),
  imageHeight(600),
  grabBinning(1),
  grabGain(800)
{
  maxTasks = tasks;
}

Grabber::~Grabber() {

}

bool Grabber::canStartTask(){
  return runningTasks<maxTasks;
}

bool Grabber::streamer(){
  return runningTasks<maxTasks;
}



void Grabber::run_capture(){
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
      std::cout << "Using device " << camera.GetDeviceInfo().GetModelName() << std::endl;

      INodeMap& nodemap = camera.GetNodeMap();

      // Open the camera for accessing the parameters.
      camera.Open();

      // Get camera device information.
      std::cout << "Camera Device Information" << std::endl
           << "=========================" << std::endl;
      std::cout << "Vendor           : "
           << CStringPtr( nodemap.GetNode( "DeviceVendorName") )->GetValue() << std::endl;
      std::cout << "Model            : "
           << CStringPtr( nodemap.GetNode( "DeviceModelName") )->GetValue() << std::endl;
      std::cout << "Firmware version : "
           << CStringPtr( nodemap.GetNode( "DeviceFirmwareVersion") )->GetValue() << std::endl;
      std::cout << "ExposureTimeRaw Min : "
           << CIntegerPtr( nodemap.GetNode( "ExposureTimeRaw") )->GetMin() << std::endl;
      std::cout << "ExposureTimeRaw Max : "
           << CIntegerPtr( nodemap.GetNode( "ExposureTimeRaw") )->GetMax() << std::endl;
      std::cout << "ExposureTimeBaseAbs Value : "
           <<  nodemap.GetNode( "ExposureTimeBaseAbs")  << std::endl;

    CIntegerPtr gainRaw(nodemap.GetNode("GainRaw"));

    if(gainRaw.IsValid()) {
     gainRaw->SetValue(grabGain);
     std::cout << "GainRaw Min : "
          <<  gainRaw->GetMin()  << std::endl;
     std::cout << "GainRaw Max : "
          <<  gainRaw->GetMax()  << std::endl;
     std::cout << "GainRaw Value : "
          <<  gainRaw->GetValue()  << std::endl;

    }

            // Camera settings.
      std::cout << "Camera Device Settings" << std::endl
           << "======================" << std::endl;



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
      std::cout << "Old PixelFormat  : " << oldPixelFormat << std::endl;

      // Set the pixel format to Mono8 if available.
      if ( IsAvailable( pixelFormat->GetEntryByName( "Mono8")))
      {
          pixelFormat->FromString( "Mono8");
          std::cout << "New PixelFormat  : " << pixelFormat->ToString() << std::endl;
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
              /*
              if (count % 3==0){
                boost::thread mythread(barcode,img,count);
              }
              */
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
              std::cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << std::endl;
          }
      }
  }catch (GenICam::GenericException &e){
    // Error handling.
    std::cerr << "An exception occurred." << std::endl
    << e.GetDescription() << std::endl
    << e.what() << std::endl;
  }
}

void Grabber::run_streamer(){
    using namespace http::server;
    // Run server in background thread.
    std::size_t num_threads = 3;
    std::string doc_root = "./";
    int frame_count = 0;
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 1;
    int thickness = 1;
    int baseline=0;
    std::string imgtext="";
    boost::format frmt("No.: %1%");

    //this initializes the redirect behavor, and the /_all handlers
    server_ptr s = init_streaming_server("0.0.0.0", "8080", doc_root, num_threads);
    streamer_ptr stmr(new streamer);//a stream per image, you can register any number of these.
    register_streamer(s, stmr, "/stream_0");
    s->start();
    while (true){


      mutex.lock();
      cv::Mat image = currentImage.clone();
      mutex.unlock();

      //fill image somehow here. from camera or something.
      bool wait = false; //don't wait for there to be more than one webpage looking at us.
      int quality = 75; //quality of jpeg compression [0,100]
      int ms = 60;
      if( (currentImage.cols==0)|| (currentImage.rows==0)){
        image = cv::Mat(cv::Size(640, 480), CV_8UC3, cv::Scalar(std::rand() % 255, std::rand() % 255, std::rand() % 255));
        ms = 1000;
      }


      frame_count++;
      baseline=0;
      imgtext = (frmt % frame_count).str();
      cv::Size textSize = cv::getTextSize(imgtext, fontFace, fontScale, thickness, &baseline);
      baseline += thickness;
      // center the text
      cv::Point textOrg((image.cols - textSize.width)/2, (image.rows + textSize.height)/2);
      cv::putText(image, imgtext, textOrg, fontFace, fontScale,   cv::Scalar::all(255), thickness, 8);



      int n_viewers = stmr->post_image(image,quality, wait);
      //use boost sleep so that our loop doesn't go out of control.
      boost::this_thread::sleep(boost::posix_time::milliseconds(ms)); //30 FPS
    }
}
