CC=gcc
TARGET=coin-counter
CFLAGS=-Isource -std=c++11
LIBS=-L/usr/lib -lstdc++ -lopencv_core -lopencv_highgui -lopencv_videoio -lopencv_imgproc -lopencv_imgcodecs -ltinkerforge -pthread


coin-counter: clean
	$(CC) $(CFLAGS) main.cpp -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
