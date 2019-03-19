

#include <opencv2/objdetect.hpp>
#include <opencv2/imgcodecs.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <zbar.h>
#include "../args.hxx"
#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <iostream>


int main(int argc, char* argv[])
{


    int exitCode = 0;
    args::ArgumentParser parser("Ocrs reconize barcodes live from camera.", "");
    args::HelpFlag help(parser, "help", "Display this help menu", { "help"});
    args::ValueFlag<std::string> filename_param(parser, "filename", "The machine ID", {'f',"filename"});

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

    const char* fname = "";
    fname= (args::get(filename_param)).c_str();
    cv::Mat img = imread(fname, cv::IMREAD_COLOR );
    
    cv::Mat gray;
    cv::Mat useimage;
cv::cvtColor(img, useimage, CV_BGR2GRAY);
/*
cv::QRCodeDetector qrDecoder = cv::QRCodeDetector::QRCodeDetector();
 
  cv::Mat bbox, rectifiedImage;
 
  std::string data = qrDecoder.detectAndDecode(grayo, bbox, rectifiedImage);
  if(data.length()>0)
  {
    std::cout << "Decoded Data : " << data << std::endl;
 
    cv::display(grayo, bbox);
    rectifiedImage.convertTo(rectifiedImage, CV_8UC3);
    cv::imshow("Rectified QRCode", rectifiedImage);
 
    cv::waitKey(0);
  }
  else
    std::cout << "QR Code not detected" << std::endl;
}
*/


    int i_bc_thres_stop=25;
    int i_bc_thres_start=25;
    int i_bc_thres_step = 10;

    int subtractMean = 5;
    int blockSize = 55;
    for (int thres=i_bc_thres_stop;((thres>=i_bc_thres_start));thres-=i_bc_thres_step){

        //std::cout << " loop " << thres << std::endl;
        
        cv::adaptiveThreshold(
            useimage,
            gray,
            255,
            CV_ADAPTIVE_THRESH_GAUSSIAN_C,
            CV_THRESH_BINARY,//blockSize,calcmeanValue(src));/*,
            thres,//blockSize,
            subtractMean
        );



        //cv::threshold(useimage,gray,thres,255, CV_THRESH_BINARY );
        zbar::Image* _image;
        zbar::ImageScanner* _imageScanner;

        _image = new zbar::Image(gray.cols, gray.rows, "Y800", nullptr, 0);
        _imageScanner = new zbar::ImageScanner();

        _imageScanner->set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
    /*
        _imageScanner->set_config(zbar::ZBAR_QRCODE, zbar::ZBAR_CFG_ASCII, 1);

    _imageScanner->set_config(zbar::ZBAR_CODE128, zbar::ZBAR_CFG_ADD_CHECK, 1);
    _imageScanner->set_config(zbar::ZBAR_CODE128, zbar::ZBAR_CFG_EMIT_CHECK, 0);

    _imageScanner->set_config(zbar::ZBAR_CODE128, zbar::ZBAR_CFG_ENABLE, 1);
    _imageScanner->set_config(zbar::ZBAR_CODE128, zbar::ZBAR_CFG_ADD_CHECK, 1);
    _imageScanner->set_config(zbar::ZBAR_CODE128, zbar::ZBAR_CFG_EMIT_CHECK, 0);

    _imageScanner->set_config(zbar::ZBAR_CODE39, zbar::ZBAR_CFG_ENABLE, 1);
    _imageScanner->set_config(zbar::ZBAR_CODE39, zbar::ZBAR_CFG_ADD_CHECK, 1);
    _imageScanner->set_config(zbar::ZBAR_CODE39, zbar::ZBAR_CFG_EMIT_CHECK, 0);

    _imageScanner->set_config(zbar::ZBAR_I25, zbar::ZBAR_CFG_ENABLE, 1);
    _imageScanner->set_config(zbar::ZBAR_I25, zbar::ZBAR_CFG_ADD_CHECK, 1);
    _imageScanner->set_config(zbar::ZBAR_I25, zbar::ZBAR_CFG_EMIT_CHECK, 0);
    */
    _image->set_data((uchar *)gray.data, gray.cols * gray.rows);

    int n = _imageScanner->scan(*_image);

        for(zbar::Image::SymbolIterator symbol = _image->symbol_begin(); symbol != _image->symbol_end(); ++symbol) {
            std::cout << thres << " " << "without thres Code " << symbol->get_data().c_str() << " Type " << symbol->get_type_name().c_str() << std::endl;
        }
        _image->set_data(NULL, 0);

    }
}