#include <opencv2/opencv.hpp>
#include <string>
#include "ip_connection.h"
#include "brick_stepper.h"

#define HOST "localhost"
#define PORT 4223
#define UID "6JMdEv"

IPConnection ipcon;
Stepper stepper;
bool grab = true;
bool preview = false;

using namespace cv;
using namespace std;

std::string get_tegra_pipeline(int width, int height, int fps) {
    return "nvcamerasrc ! video/x-raw(memory:NVMM), width=(int)" + std::to_string(width) + ", height=(int)" +
           std::to_string(height) + ", format=(string)I420, framerate=(fraction)" + std::to_string(fps) +
           "/1 ! nvvidconv flip-method=0 ! video/x-raw, format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
}

void stepper_position_reached(int32_t position, void *user_data) 
{
	grab = true;
}

bool stepper_init()
{
	ipcon_create(&ipcon);

	// Create device object
    stepper_create(&stepper, UID, &ipcon);

	// Connect to brickd
    if(ipcon_connect(&ipcon, HOST, PORT) < 0) {
    	fprintf(stderr, "Could not connect\n");
        return 1;
	}
	// Register position reached callback to function cb_position_reached
	stepper_register_callback(&stepper, STEPPER_CALLBACK_POSITION_REACHED, (void *)stepper_position_reached, &stepper);

    stepper_set_motor_current(&stepper, 800); // 800mA
	stepper_set_step_mode(&stepper, 8); // 1/8 step mode
	stepper_set_max_velocity(&stepper, 100); // Velocity steps/s

    // de/acceleration (steps/s^2)
    stepper_set_speed_ramping(&stepper, 500, 500);
    
    // Leistungsversorgung an!
    stepper_enable(&stepper); 
    
    return 0;
}

void stepper_move(const int &steps)
{
   	stepper_set_steps(&stepper, steps); 
}

void stepper_destroy()
{
	// Leistungsversorgung aus!
	stepper_disable(&stepper); 
	
	stepper_destroy(&stepper);
	ipcon_destroy(&ipcon);
}

string getFileName(const int &count, const string &name)
{
	string pad = "";
	if (count < 10) pad = "00";
	if ((count < 100) && (count > 9)) pad = "0";
	string fileName = "./img/" + name + "_" + pad + to_string(count) + ".png";
	
	return fileName;
}

int main(int argc, char **argv) {
	if (strcmp("preview", argv[1]) == 0) preview = true;
	
	if (!preview) stepper_init();

	
	// Options
	int WIDTH = 500;
	int HEIGHT = 500;
	int FPS = 30;
	    
	// Create OpenCV capture object, ensure it works.
	cv::VideoCapture cap(get_tegra_pipeline(WIDTH, HEIGHT, FPS), cv::CAP_GSTREAMER);
	if (!cap.isOpened()) {
       std::cout << "Connection failed";
       return -1;
    }

   	// View video
    cv::Mat frame;

    int sampleCounter = 1;
    int samplesPerSide = 20;
    int numberOfCoins = 2;
    int stepsToMove = 1600 / samplesPerSide; // Schrittmotor braucht 1600 Schritte für eine volle Umdrehung
    
	int max_samples = 2 * samplesPerSide * numberOfCoins;

	while (sampleCounter <= max_samples) {
		cap >> frame;  // Get a new frame from camera

		// Display frame
       	imshow("Vorschau", frame);

		if (grab && !preview){
			string save_location = getFileName(sampleCounter, argv[2]);
	        imwrite(save_location, frame );
	        cout << "Bild gespeichert: " << save_location << endl;
        
	   		if (sampleCounter % samplesPerSide == 0)
			{
				if (sampleCounter % (2 * samplesPerSide) == 0)
				{
					cout << "Next ...";
				}
				else
				{
					cout << "Sample drehen ...";
				}
				getchar();  // auf "WEITER" warten
			}

	        sampleCounter++;		
			grab = false;
		    stepper_move(stepsToMove);
		}
		
	   cv::waitKey(1);
	}
	
	if (!preview) stepper_destroy(); 
}
