#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <pylonc/PylonC.h>

#include "camera.h"
#include "initialize.h"


void getDeviceNames(size_t numDevices, char *names[]);


/* Data */

// Holds information about the camera acquisition thread
struct WorkerThread {
	GCancellable *cancellable;
	GTask *task;

	struct TaskData task_data;
};

// Used to pass around application data
struct AppData {
	struct WorkerThread *workerThread;
	PYLON_DEVICE_HANDLE hDev;
	_Bool opened;

	GtkWindow *window;
	GtkWidget *start_acquisition;
	GtkWidget *stop_acquisition;
	GtkPicture *img_window;

	char **deviceNames;
	GtkWidget *deviceDropdown;
};

static struct AppData appData = {.opened=false};


/* Thread Control */

// Execute at end of async camera thread
static void camera_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
	puts("Done");
}


/* GUI callback functions */

// Start acquisition button
static void start_acquisition(GtkWidget *widget, gpointer data_) {
	struct WorkerThread *thread = appData.workerThread;

	if (gtk_widget_is_sensitive(widget)) {
		// Start camera thread
		g_cancellable_reset(thread->cancellable);
		thread->task = g_task_new(NULL, thread->cancellable, camera_done, NULL);

		thread->task_data = (struct TaskData){appData.hDev, appData.img_window};
		g_task_set_task_data(thread->task, &thread->task_data, NULL);

		g_task_run_in_thread(thread->task, camera);

		// Disable button
		gtk_widget_set_sensitive(widget, FALSE);
		gtk_widget_set_sensitive(appData.stop_acquisition, TRUE);
		gtk_widget_set_sensitive(appData.deviceDropdown, FALSE);
	}
}

// Stop acquisition button
static void stop_acquisition(GtkWidget *widget, gpointer data_) {
	struct WorkerThread *thread = appData.workerThread;

	if (gtk_widget_is_sensitive(widget)) {
		// Stop camera thread
		g_cancellable_cancel(thread->cancellable);
		g_object_unref(thread->task);

		// Disable button
		gtk_widget_set_sensitive(widget, FALSE);
		gtk_widget_set_sensitive(appData.start_acquisition, TRUE);
		gtk_widget_set_sensitive(appData.deviceDropdown, TRUE);
	}
}

// Camera session select dropdown
static void select_camera(GtkWidget *widget, gpointer data_) {
	if (gtk_widget_is_sensitive(widget)) {
		GENAPIC_RESULT res;
		int index = gtk_drop_down_get_selected(widget) - 1;

		// Close device if already open
		printf("Opened: %d\n", appData.opened);
		if (appData.opened) {
			printf("Close device\n");
			res = PylonDeviceClose(appData.hDev);
			CHECK(res);

			appData.opened = false;

			gtk_widget_set_sensitive(appData.start_acquisition, FALSE);
		}
		
		// Open selected device
		printf("%d\n", index);
		if (index >= 0) {
			_Bool isAccessible;
			res = PylonIsDeviceAccessible(index, PYLONC_ACCESS_MODE_CONTROL | PYLONC_ACCESS_MODE_STREAM, &isAccessible);
			CHECK(res);

			printf("Is accessible: %d\n", isAccessible);


			if (isAccessible) {
				printf("Open camera %d\n", index);
				// Open Camera
				res = PylonCreateDeviceByIndex( index, &appData.hDev );
				CHECK(res);

				res = PylonDeviceOpen( appData.hDev, PYLONC_ACCESS_MODE_CONTROL | PYLONC_ACCESS_MODE_STREAM );
				CHECK(res);

				appData.opened = true;

				gtk_widget_set_sensitive(appData.start_acquisition, TRUE);
			} else {
				gtk_message_dialog_new( GTK_WINDOW(appData.window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, "Could not open camera: likely in use by another session.\n");
			}
		}
	}
}





// Initialize GUI Window
static void activate(GtkApplication *app, gpointer data_) {
	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "builder.ui", NULL);

	GObject *window = gtk_builder_get_object(builder, "window");
	gtk_window_set_application(GTK_WINDOW(window), app);
	appData.window = window;

	// Set menu bar
	GtkBuilder *menu_builder = gtk_builder_new();
	gtk_builder_add_from_file(menu_builder, "menubar.ui", NULL);
	GObject *titlebar = gtk_builder_get_object(builder, "menu");
	gtk_window_set_titlebar(GTK_WINDOW(window), titlebar);

	// Signal handlers
	GObject *widget = gtk_builder_get_object(builder, "start_acquisition");
	g_signal_connect(widget, "clicked", G_CALLBACK(start_acquisition), NULL);
	gtk_widget_set_sensitive(widget, FALSE);
	appData.start_acquisition = widget;

	widget = gtk_builder_get_object(builder, "stop_acquisition");
	g_signal_connect(widget, "clicked", G_CALLBACK(stop_acquisition), NULL);
	gtk_widget_set_sensitive(widget, FALSE);
	appData.stop_acquisition = widget;

	//window = gtk_application_window_new(app);
	//gtk_window_set_title(GTK_WINDOW(window), "Laser Tracker");
	//gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
	//data->window = window;
	
	//box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	//gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	//gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	//gtk_window_set_child(GTK_WINDOW(window), box);
	
	// Camera select dropdown
	widget = gtk_builder_get_object(builder, "cam_session_box");

	appData.deviceDropdown = gtk_drop_down_new_from_strings(appData.deviceNames);
	free(appData.deviceNames);

	g_signal_connect(appData.deviceDropdown, "notify::selected", G_CALLBACK(select_camera), NULL);
	gtk_box_append(GTK_BOX(widget), appData.deviceDropdown);

	//gtk_window_set_titlebar(GTK_WINDOW(window), appData.deviceDropdown);
	
	// Image Display
	widget = gtk_builder_get_object(builder, "img_window");
	appData.img_window = GTK_PICTURE(widget);

	// Show window
	gtk_widget_set_visible(GTK_WIDGET(window), TRUE);

	g_object_unref(builder);
}


/* Main Loop */

int main(int argc, char *argv[]) {

	/* Initialize Pylon */

	GENAPIC_RESULT res; // Return value of pylon methods

	// Pylon handles
	//PYLON_DEVICE_HANDLE hDev; // Handle for a pylon device
	//PYLON_STREAMGRABBER_HANDLE hGrabber;
	//PYLON_WAITOBJECT_HANDLE hWait;

	// Image buffers
	unsigned char *buffers[NUM_BUFFERS];
	PYLON_STREAMBUFFER_HANDLE bufHandles[NUM_BUFFERS];

	size_t nStreams, payloadSize; // Number of streams, size of an image frame
	int64_t sizeX, sizeY; // Image AOI
	//struct beamProperties *beamProps = (struct beamProperties*)malloc(sizeof(struct beamProperties)); // Image properties
	
	size_t numDevices;

	/* Setup  Camera */

	PylonInitialize();

	// Get all connected devices
	res = PylonEnumerateDevices(&numDevices);
	CHECK(res);

	appData.deviceNames = (char **)malloc(sizeof(char *)*(numDevices+2));
	*appData.deviceNames = "CAMERA SELECT";
	getDeviceNames(numDevices, appData.deviceNames);
	*(appData.deviceNames+numDevices+1) = NULL;

	//printf("%d\n", numDevices);
	//for (char *p = *names; p; p++)
	//	printf("%s\n", p);


	/* Setup GUI */

	//struct AppData *data = (struct AppData*)malloc(sizeof(struct AppData));

	//GtkStringList *deviceList = gtk_string_list_new(names);
	//gtk_drop_down_new_from_strings( (char **){"CAMERA SELECT", NULL} );
	//appData.deviceDropdown = gtk_drop_down_new_from_strings(names);
	//appData.deviceDropdown = gtk_drop_down_new((GListModel *)deviceList, NULL);
	//free(names);
	//g_object_unref(deviceList);

	appData.workerThread = (struct WorkerThread *)malloc(sizeof(struct WorkerThread));
	appData.workerThread->cancellable = g_cancellable_new();

	GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);


	// Start application loop (in main thread)
	int status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	return status;
}


// Populates names string array with device names starting at index 1
void getDeviceNames(size_t numDevices, char **names) {
	GENAPIC_RESULT res;
	PylonDeviceInfo_t *pDi = (PylonDeviceInfo_t *)malloc(sizeof(PylonDeviceInfo_t));

	for (size_t i = 0; i < numDevices; i++) {
		res = PylonGetDeviceInfo(i, pDi);
		CHECK(res);

		*(++names) = (char *)malloc(strlen(pDi->FriendlyName)*sizeof(char));
		strcpy(*names, pDi->FriendlyName);
	}

	free(pDi);
}
