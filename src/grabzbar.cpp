
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


void showImage(cv::Mat& src){

  cv::namedWindow("grabzbar", CV_WINDOW_AUTOSIZE );
  cv::imshow("grabzbar", src );
  cv::waitKey(1);

}

int camera(int grabExposure,int grabHeight, int imageHeight){
    int nbBuffers=10;
    int totalImageSize=0;
    cv::Mat currentImage;

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
             << CStringPtr( nodemap.GetNode( "DeviceFirmwareVersion") )->GetValue() << endl << endl;

        // Camera settings.
        cout << "Camera Device Settings" << endl
             << "======================" << endl;


        camera.ExposureAuto.SetValue(ExposureAuto_Off);
        camera.ExposureTimeRaw.SetValue(grabExposure);
        //camera.ExposureTime.GetMin()

        CIntegerPtr width( nodemap.GetNode( "Width"));
        CIntegerPtr height( nodemap.GetNode( "Height"));
        height->SetValue(grabHeight);


        if(camera->PixelFormat.GetValue() == PixelFormat_Mono8){
          totalImageSize = pCamera->Width.GetValue()*imageHeight;
          currentImage = cv::Mat(imageHeight, camera->Width.GetValue(), CV_8UC1, Scalar(0));
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
        //camera.MaxNumBuffer = 10;
        camera.MaxBufferSize.SetValue(imageSize);

        // We won't queue more than nbBuffers image buffers at a time
        camera.MaxNumBuffer.SetValue(nbBuffers);

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
}

int main(int argc, char* argv[])
{



    args::ArgumentParser parser("Ocrs reconize barcodes live from camera.", "");
    args::HelpFlag help(parser, "help", "Display this help menu", { "help"});
    args::Flag debug(parser, "debug", "Show debug messages", {'d', "debug"});

    args::ValueFlag<int> height(parser, "height", "max image height", { "h"});
    //args::ValueFlag<std::string> filename(parser, "filename", "The filename", {'f',"file"});



    try
    {
        parser.ParseCLI(argc, argv);
    }catch (args::Help){
        std::cout << parser;
        return 0;
    }catch (args::ParseError e){
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }


    // The exit code of the sample application.
    int exitCode = 0;
    char filename[128];
    char result_filename[128];


    unsigned int offset,nx,ny;
    int adjustAVG = 1;

    int mainAVG = 0;
    int mainAVGSum = 0;
    int currentAVG = 0;

    int startAVG = 0;
    int stopAVG = 0;

    int i=0;
    int m=0;
    bool inimage = false;
    bool usetiff = true;
    //ofstream pFile;
    ofstream* pFile;// = new fstream();

    int imageCounter = 0;

    int grabExposure = 800;
    int grabHeight = 32;
    int currentHeight = 0;


    if(const char* env_start = std::getenv("GRAB_START_AVG")){
      startAVG = atoi(env_start);
      adjustAVG=0;
    }
    if(const char* env_stop = std::getenv("GRAB_STOP_AVG")){
      stopAVG = atoi(env_stop);
    }

    if(const char* env_path = std::getenv("GRAB_PATH")){
      prefix = env_path;
    }
    if(const char* env_exp = std::getenv("GRAB_EXPOSURE")){
      grabExposure = atoi(env_exp);
      cout << "setting exposure to " << grabExposure;
    }
    if(const char* env_hgt = std::getenv("GRAB_HEIGHT")){
      grabHeight = atoi(env_hgt);
    }


    // Comment the following two lines to disable waiting on exit.
    //cerr << endl << "Press Enter to exit." << endl;
    //while( cin.get() != '\n');

    if (inimage){
      pFile->close();
    }
    return exitCode;
}
