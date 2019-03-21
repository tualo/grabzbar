#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
	//open arduino device file (linux)
    std::ofstream cmd;
	cmd.open( "/dev/tty.usbserial-14610");
	//write to it

    //cmd << "Hello from C++!";
    char closebuffer[3];// =  [0xA0,0x01,0xA2];
    closebuffer[0]=0xA0;
    closebuffer[1]=0x01;
    closebuffer[2]=0xA2;
    
    char openbuffer[3];
    openbuffer[0]=0xA0;
    openbuffer[1]=0x00;
    openbuffer[2]=0xA1;
    
    cmd << openbuffer;
    cmd << 0xA0 << 0x01 << 0xA2;

	cmd.close();

	return 0;
}