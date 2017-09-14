
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

#include "args.hxx"
#include "Grabber.h"
//#include "../mjpeg/mjpeg_server.hpp"


// #### Ocrs


std::string std_str_machine="0";
std::string std_str_storepath="/tmp/";
std::string std_str_resultpath="/tmp/";

const char* str_db_host = "localhost";
const char* str_db_user = "root";
const char* str_db_name = "sorter";
const char* str_db_password = "";
const char* str_db_encoding = "utf8";

int int_pixel_cm_x = 73;
int int_pixel_cm_y = 73;
int int_blockSize=55;
int int_subtractMean=20;


// --- Ocrs




double debug_start_time = (double)cv::getTickCount();
double debug_last_time = (double)cv::getTickCount();
double debug_window_offset = 0;

bool bDebugTime=true;
void debugTime(std::string str){
  if (bDebugTime){
    double time_since_start = ((double)cv::getTickCount() - debug_start_time)/cv::getTickFrequency();
    double time_since_last = ((double)cv::getTickCount() - debug_last_time)/cv::getTickFrequency();
    std::cout << str << ": " << time_since_last << "s " << "(total: " << time_since_start  << "s)" << std::endl;
  }
  debug_last_time = (double)cv::getTickCount();
}

bool is_digits(const std::string &str){
    return std::all_of(str.begin(), str.end(), ::isdigit); // C++11
}


int main(int argc, char* argv[])
{


    int exitCode = 0;
    std::cout << "hardware_concurrency:  " << boost::thread::hardware_concurrency() << std::endl;
    args::ArgumentParser parser("Ocrs reconize barcodes live from camera.", "");
    args::HelpFlag help(parser, "help", "Display this help menu", { "help"});
    args::Flag debug(parser, "debug", "Show debug messages", {'d', "debug"});
    //args::Flag stresstest(parser, "stresstest", "donig stresstest", {'s', "stresstest"});

    //args::Flag debugwindow(parser, "debugwindow", "Show debug window", {'w', "debugwindow"});
    args::Flag debugtime(parser, "debugtime", "Show times", {'t', "debugtime"});

    args::ValueFlag<int> exposure(parser, "exposure", "exposure (1300)", {"exposure"});
    args::ValueFlag<int> lineheight(parser, "lineheight", "height of one captured image (32)", { "lineheight"});
    args::ValueFlag<int> maxheight(parser, "maxheight", "max image height (4000)", { "maxheight"});

    args::ValueFlag<int> binning(parser, "binning", "binning (default 1)", {"binning"});
    args::ValueFlag<int> gain(parser, "gain", "gain (default 800)", { "gain"});


    args::ValueFlag<int> bc_thres_start(parser, "bc_thres_start", "bc_thres_start (default 60)", { "bc_thres_start"});
    args::ValueFlag<int> bc_thres_stop(parser, "bc_thres_stop", "bc_thres_stop (default 160)", { "bc_thres_stop"});
    args::ValueFlag<int> bc_thres_step(parser, "bc_thres_step", "bc_thres_step (default 20)", { "bc_thres_step"});
    args::Flag bc_clahe(parser, "bc_clahe", "bc_clahe (default off)", { "bc_clahe"});
    //args::ValueFlag<std::string> filename(parser, "filename", "The filename", {'f',"file"});





    // args::ValueFlag<std::string> stresstest_images(parser, "path", "Path for image stress test", {"stresstest_images"});

    args::ValueFlag<std::string> db_host(parser, "host", "The database server host", {'h',"host"});
    args::ValueFlag<std::string> db_name(parser, "name", "The database name", {'n',"name"});
    /*
    currently not supported
    args::ValueFlag<int> db_port(parser, "port", "The database server port", {'p',"port"});
    */
    args::ValueFlag<std::string> db_user(parser, "user", "The database server username", {'u',"user"});
    args::ValueFlag<std::string> db_pass(parser, "password", "The database server password", {'x',"password"});
    args::ValueFlag<std::string> db_encoding(parser, "encoding", "The database server encoding", {'e',"encoding"});


    args::ValueFlag<int> pixel_cm_x(parser, "cm_x", "Pixel per CM X (default 73)", {"cmx"});
    args::ValueFlag<int> pixel_cm_y(parser, "cm_y", "Pixel per CM Y (default 73)", {"cmy"});
    args::ValueFlag<int> blocksize(parser, "blocksize", "adaptiveThreshold Blocksize (default 55)", {"blocksize"});
    args::ValueFlag<int> substractmean(parser, "substractmean", "adaptiveThreshold subtractMean (default 20)", {"substractmean"});
    args::Flag calculatemean(parser, "calculatemean", "calculatemean (default off)", {"calculatemean"});
    args::ValueFlag<std::string> machine(parser, "machine", "The machine ID", {"machine"});


    args::ValueFlag<std::string> arg_resultpath(parser, "resultpath", "The resultpath (default /tmp/)", {"resultpath"});
    args::ValueFlag<std::string> arg_storepath(parser, "storepath", "The storepath (default /tmp/)", {"storepath"});


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



    int int_exposure = 1300;
    if (exposure) { int_exposure = args::get(exposure); }
    int int_maxheight = 4000;
    if (maxheight) { int_maxheight = args::get(maxheight); }

    int int_gain = 800;
    if (gain) { int_gain = args::get(gain); }
    int int_binning = 1;
    if (binning) { int_binning = args::get(binning); }
    int int_lineheight = 32;
    if (lineheight) { int_lineheight = args::get(lineheight); }



    int int_bc_thres_start = 60;
    if (bc_thres_start) { int_bc_thres_start = args::get(bc_thres_start); }
    int int_bc_thres_stop = 160;
    if (int_bc_thres_stop) { int_bc_thres_stop = args::get(bc_thres_stop); }
    int int_bc_thres_step = 20;
    if (bc_thres_step) { int_bc_thres_step = args::get(bc_thres_step); }



    // ### Ocrs

    if (blocksize) { int_blockSize = args::get(blocksize); }
    if (substractmean) { int_subtractMean = args::get(substractmean); }
    if (pixel_cm_x) { int_pixel_cm_x = args::get(pixel_cm_x); }
    if (pixel_cm_y) { int_pixel_cm_y = args::get(pixel_cm_y); }

    if (db_encoding){ str_db_encoding= (args::get(db_encoding)).c_str(); }
    if (db_host){ str_db_host= (args::get(db_host)).c_str(); }
    if (db_user){ str_db_user= (args::get(db_user)).c_str(); }
    if (db_name){ str_db_name= (args::get(db_name)).c_str(); }
    if (db_pass){ str_db_password= (args::get(db_pass)).c_str(); }
    if (machine){ std_str_machine=args::get(machine); }

    if (arg_resultpath){ std_str_resultpath=args::get(arg_resultpath); }
    if (arg_storepath){ std_str_storepath=args::get(arg_storepath); }

    // --- Ocrs
    Grabber *grabber=new Grabber();
    grabber->setResultImagePath(std_str_resultpath);
    grabber->setStoreImagePath(std_str_storepath);
    grabber->configOCR(
      debug==1,
      debugtime==1,
      int_blockSize,
      int_subtractMean,
      int_pixel_cm_x,
      int_pixel_cm_y,
      calculatemean==1,
      bc_clahe==1,
      int_bc_thres_start,
      int_bc_thres_stop,
      int_bc_thres_step
    );
    grabber->configCamera(
      int_exposure,
      int_lineheight,
      int_maxheight,
      int_binning,
      int_gain
    );

    grabber->configConnection(
      std_str_machine,
      str_db_host,
      str_db_user,
      str_db_name,
      str_db_password,
      str_db_encoding
    );

    boost::thread* thr = new boost::thread(&Grabber::run, grabber );
    thr->join();

    return exitCode;
}
