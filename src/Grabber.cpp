#include "Grabber.h"

Grabber::Grabber():
  runningTasks(0),
  glb_grabExposure(1300),
  glb_grabHeight(32),
  glb_maxImageHeight(600),
  glb_grabBinning(1),
  glb_grabGain(800),
  int_pixel_cm_x(73),
  int_pixel_cm_y(73)
{
  maxTasks = boost::thread::hardware_concurrency();
  avgStepSize = 10;
  f_meanfactor=1.0;
  b_bc_clahe=false;
  b_forceFPCode=false;
}

Grabber::~Grabber() {

}

bool Grabber::canStartTask(){
  return runningTasks<maxTasks;
}



void Grabber::ocrthread(cv::Mat img) {
  mutex.lock();
  runningTasks++;
  mutex.unlock();


  MYSQL *con = nullptr;
  ImageRecognizeEx* ir = ocr_ext(
    con,
    str_machine,
    str_db_host.c_str(),
    str_db_user.c_str(),
    str_db_name.c_str(),
    str_db_password.c_str(),
    str_db_encoding.c_str(),
    i_blockSize,
    i_substractMean,
    b_debugtime,
    b_debug,
    false,//debugwindow==1
    b_calculatemean,
    "",
    f_meanfactor,
    b_bc_clahe,
    i_bc_thres_start,
    i_bc_thres_stop,
    i_bc_thres_step,
    b_forceFPCode
  );

  std::string sql_user = "set @sessionuser='not set'";
  if (mysql_query(con, sql_user.c_str())){
  }

  std::string sql_modell = "set @svmodell='Maschine'";
  if (mysql_query(con, sql_modell.c_str())){
  }

  std::string sql_regiogruppe = "set @regiogruppe='Zustellung'";
  if (mysql_query(con, sql_regiogruppe.c_str())){
  }

  ir->setPixelPerCM(int_pixel_cm_x,int_pixel_cm_y);
  ir->setImage(img);
  ir->rescale();
  ir->barcode();
  ir->correctSize();
  ir->largestContour(false);
  ExtractAddress* ea = ir->texts();


  mutex.lock();
  if (ea->foundAddress()){
    std::cout << "ZipCode " << ea->getZipCode() << std::endl;
  }else{
    std::cout << "No ZipCode " << std::endl;
  }
  mutex.unlock();

  std::string sql = "call quicksv";//("+"'"+ir->getBarcode()+"',"+"'"+ea->getZipCode()+"',"+"'"+ea->getTown()+"',"+"'"+ea->getStreetName()+"',"+"'"+ea->getHouseNumber()+"',"+"'"+ea->getZipCode()+"',"+"'"+ea->getSortRow()+"',"+"'"+ea->getSortBox()+"',"+"'"+ea->getString()+"',"+"'kundennummmer!!!'"+")";



  mysql_close(con);
  mutex.lock();

  std::cout << "#########################################" << std::endl;
  std::cout << "code: " << ir->getBarcode() << std::endl;
  std::cout << "zipcode: " << ea->getZipCode() << std::endl;
  std::cout << "town: " << ea->getTown() << std::endl;
  std::cout << "street: " << ea->getStreetName() << std::endl;
  std::cout << "housenumber: " << ea->getHouseNumber() << std::endl;
  std::cout << "sortiergang: " << ea->getSortRow() << std::endl;
  std::cout << "sortierfach: " << ea->getSortBox() << std::endl;
  std::cout << "#########################################" << std::endl;


  runningTasks--;
  mutex.unlock();
}



void Grabber::setStoreImagePath(std::string value){
  storePath = value;
}
std::string Grabber::getStoreImagePath(){
  return storePath;
}

void Grabber::setResultImagePath(std::string value){
  resultPath= value;
}
std::string Grabber::getResultImagePath(){
  return storePath;
}

std::string Grabber::getFileName(){
  std::string customer="";
  std::string line;
  std::ifstream myfile ("/opt/grab/customer.txt",std::ifstream::in);
  if (myfile.is_open()){
    while ( getline (myfile,line) ){
      customer = line;
    }
    myfile.close();
  }
  struct timeval ts;
  gettimeofday(&ts,NULL);
  boost::format fmt = boost::format("%s%sN%012d.%06d.tiff") % getStoreImagePath() % customer % ts.tv_sec % ts.tv_usec;
  return fmt.str();
}

void Grabber::run(){
  boost::thread* t_streamer = new boost::thread(&Grabber::run_streamer, this);
  boost::thread* t_capture = new boost::thread(&Grabber::run_capture, this);
  t_capture->join();
  t_streamer->join();
}

void Grabber::startocr(cv::Mat img){
  if (canStartTask()){
    boost::thread* thr = new boost::thread(&Grabber::ocrthread, this , img);
  }else{
    cv::imwrite((getFileName()).c_str(),img);
  }
}

void Grabber::configCamera(
  int grabExposure,
  int grabHeight,
  int maxImageHeight,
  int grabBinning,
  int grabGain
){
  glb_grabExposure = grabExposure;
  glb_grabHeight = grabHeight;
  glb_maxImageHeight = maxImageHeight;
  glb_grabBinning = grabBinning;
  glb_grabGain = grabGain;

}

void Grabber::configConnection(
  std::string machine,
  std::string db_host,
  std::string db_user,
  std::string db_name,
  std::string db_password,
  std::string db_encoding
){
  str_machine = machine;
  str_db_host = db_host;
  str_db_user = db_user;
  str_db_name = db_name;
  str_db_password = db_password;
  str_db_encoding = db_encoding;
}

void Grabber::configOCR(
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
  int bc_thres_step
){
  b_debug=debug;
  b_debugtime=debugtime;
  i_blockSize=blocksize;
  i_substractMean=substractmean;
  i_pixel_cm_x=pixel_cm_x;
  i_pixel_cm_y=pixel_cm_y;
  b_calculatemean=calculatemean;
  b_bc_clahe=bc_clahe;
  i_bc_thres_start=bc_thres_start;
  i_bc_thres_stop=bc_thres_stop;
  i_bc_thres_step=bc_thres_step;
}

void Grabber::run_capture(){
  mutex.lock();

  int nbBuffers=10*2;
  int totalImageSize=0;
  int grabbedImageSize=0;
  int count=0;
  int currentHeight=0;


  int adjustAVG = 1;
  int mainAVG = 0;
  int mainAVGSum = 0;
  int currentAVG = 0;
  int startAVG = 0;
  int stopAVG = 0;
  int i=0;
  int m=0;
  bool inimage = false;


  int _grabExposure = glb_grabExposure;
  int _grabHeight = glb_grabHeight;
  int _maxImageHeight = glb_maxImageHeight;
  int _grabBinning = glb_grabBinning;
  int _grabGain = glb_grabGain;

  mutex.unlock();

  while(true){
    adjustAVG=10;
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


      INodeMap& nodemap = camera.GetNodeMap();

      // Open the camera for accessing the parameters.
      camera.Open();

      // Get camera device information.
      mutex.lock();
      std::cout << "Camera Device Information" << std::endl;
      std::cout << "=========================" << std::endl;
      std::cout << "Vendor           : " << CStringPtr( nodemap.GetNode( "DeviceVendorName") )->GetValue() << std::endl;
      std::cout << "Model            : " << CStringPtr( nodemap.GetNode( "DeviceModelName") )->GetValue() << std::endl;
      std::cout << "Firmware version : " << CStringPtr( nodemap.GetNode( "DeviceFirmwareVersion") )->GetValue() << std::endl;
      std::cout << "ExposureTimeRaw Min : "  << CIntegerPtr( nodemap.GetNode( "ExposureTimeRaw") )->GetMin() << std::endl;
      std::cout << "ExposureTimeRaw Max : " << CIntegerPtr( nodemap.GetNode( "ExposureTimeRaw") )->GetMax() << std::endl;
      //std::cout << "ExposureTimeBaseAbs Value : "  << camera.ExposureTimeBaseAbs.GetValue() << std::endl;
      mutex.unlock();

      CIntegerPtr gainRaw(nodemap.GetNode("GainRaw"));

      if(gainRaw.IsValid()) {
        mutex.lock();
        std::cout << "GainRaw Min : " <<  gainRaw->GetMin()  << std::endl;
        std::cout << "GainRaw Max : " <<  gainRaw->GetMax()  << std::endl;
        std::cout << "GainRaw Value : " <<  gainRaw->GetValue()  << std::endl;
        mutex.unlock();
      }

      // Camera settings.
      mutex.lock();
      std::cout << "Setup Device Settings" << std::endl;
      std::cout << "=====================" << std::endl;
      mutex.unlock();
      camera.BinningHorizontal.SetValue(_grabBinning);
      //
      camera.ExposureAuto.SetValue(ExposureAuto_Off);
      camera.ExposureTimeRaw.SetValue(_grabExposure);
      if(gainRaw.IsValid()) {
        gainRaw->SetValue(_grabGain);
      }
      //camera.ExposureTime.GetMin()
      //CIntegerPtr width( nodemap.GetNode( "Width"));
      CIntegerPtr height( nodemap.GetNode( "Height"));
      height->SetValue(_grabHeight);
      mutex.lock();
      std::cout << "Grab Height Value : " <<  _grabHeight  << std::endl;
      std::cout << "Grab Exposure Value : " <<  camera.ExposureTimeRaw.GetValue()  << std::endl;

      std::cout << "Grab Exposure Value (in ns) : " <<  100 * camera.ExposureTimeRaw.GetValue())  << std::endl;

      std::cout << "Grab Gain Value : " <<  _grabGain  << std::endl;
      std::cout << "Grab Binning Value : " <<  _grabBinning  << std::endl;
      std::cout << "=====================" << std::endl;
      mutex.unlock();


      if(camera.PixelFormat.GetValue() == PixelFormat_Mono8){
        totalImageSize = camera.Width.GetValue()*_maxImageHeight;
        currentImage = cv::Mat(_maxImageHeight, camera.Width.GetValue(), CV_8UC1, cv::Scalar(0));
      }else{

      }

      // Access the PixelFormat enumeration type node.
      CEnumerationPtr pixelFormat( nodemap.GetNode( "PixelFormat"));
      // Remember the current pixel format.
      //String_t oldPixelFormat = pixelFormat->ToString();
      //std::cout << "Old PixelFormat  : " << oldPixelFormat << std::endl;

      // Set the pixel format to Mono8 if available.
      //if ( IsAvailable( pixelFormat->GetEntryByName( "Mono8")))
      //{
      //    pixelFormat->FromString( "Mono8");
      //    //std::cout << "New PixelFormat  : " << pixelFormat->ToString() << std::endl;
      //}

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

            const uint8_t *pImageBuffer = (uint8_t *) ptrGrabResult->GetBuffer();
            //cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;
            mainAVGSum = 0;
            m = (int)ptrGrabResult->GetImageSize();
            for(i=0;i<m;i+=avgStepSize){
              mainAVGSum+= (uint32_t) pImageBuffer[i];
            }
            currentAVG = (mainAVGSum/(m/avgStepSize));
            if ( adjustAVG > 0){
              mainAVG = currentAVG;
              startAVG =  mainAVG + 10;
              stopAVG =  mainAVG;// - 10;
              mutex.lock();
              std::cout << "startAVG: " << startAVG << std::endl;
              std::cout << "stopAVG: " << stopAVG << std::endl;
              mutex.unlock();
              adjustAVG--;
            }else if (inimage){
              if (currentAVG<=stopAVG){
                // the letter has ended
                mutex.lock();
                cv::Rect myROI(0, 0, currentImage.cols, currentHeight);
                cv::Mat result = currentImage(myROI);
                mutex.unlock();

                //boost::thread mythread(barcode,result);
                startocr(result);
                inimage=false;
              }

            }else if (!inimage){
              if (currentAVG>=startAVG){
                // the letter starts
                inimage=true;
                currentHeight=0;
              }
            }

            if (inimage){
              grabbedImageSize = ptrGrabResult->GetWidth()*ptrGrabResult->GetHeight();
              mutex.lock();
              std::memcpy(
                currentImage.ptr()+(currentHeight*ptrGrabResult->GetWidth()),
                ptrGrabResult->GetBuffer(),
                grabbedImageSize
              );
              mutex.unlock();

              /*
              std::memcpy( currentImage.ptr(), currentImage.ptr()+grabbedImageSize, totalImageSize-grabbedImageSize);
              std::memcpy( currentImage.ptr()+totalImageSize-grabbedImageSize, ptrGrabResult->GetBuffer(), grabbedImageSize );
              */
              currentHeight += ptrGrabResult->GetHeight();
              if (currentHeight>_maxImageHeight){
                mutex.lock();
                std::cerr << "image larger than max size (" << _maxImageHeight << ")" << std::endl;
                std::cerr << "stopping bbs service" << std::endl;
                system( "service bbs stop" );
                sleep(10);
                system( "service bbs start" );
                mutex.unlock();
              }
            }

        }else{
          mutex.lock();
          std::cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << std::endl;
          mutex.unlock();
        }
      }
    }catch (GenICam::GenericException &e){
      // Error handling.
      if (std::string(e.GetDescription())=="No device is available or no device contains the provided device info properties"){
        mutex.lock();
        std::cerr << "There is no camera, retry in 3.5 seconds" << std::endl;
        mutex.unlock();
      }else{
        mutex.lock();
        std::cerr << "An exception occurred." << std::endl
        << e.GetDescription() << std::endl
        << e.what() << std::endl;
        mutex.unlock();
      }
    }

    // wait for at 3.5 secs, see basler specs
    boost::this_thread::sleep(boost::posix_time::milliseconds(3500));
  }// while true
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