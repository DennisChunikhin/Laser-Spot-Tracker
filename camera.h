#ifndef CAMERA_H
#define CAMERA_H


#define NUM_BUFFERS 5 // Decrease if having memory issues, increase if frames dropping
#define TIMEOUT 1000 // ms to wait for image before timing out
#define BYTES_PER_R8G8B8 3


struct TaskData {
	PYLON_DEVICE_HANDLE hDev;
	GtkPicture *image_window;
};


FILE *setupDataFile();
void writeData(FILE *fp, const struct beamProperties *p);
GBytes *mono8_to_rgb_bytes(unsigned char *data, int width, int height);

void camera(GTask *task, gpointer source_obj, gpointer task_data, GCancellable *cancellable);

#endif
