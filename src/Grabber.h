// Grabber.h
#ifndef GRABBER_H
#define GRABBER_H
class Grabber{
public:
  Grabber();
  ~Grabber();


private:
  int runningTasks;
  int grabExposure;
  int grabHeight;
  int imageHeight;
  int grabBinning;
  int grabGain;

  int maxTasks;
  bool canStartTask();

  void run_streamer();
  void run_capture();

};
#endif
