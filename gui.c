#include <stdio.h>
#include <stdlib.h>
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
	GFile *save_path;

	GtkWindow *window;

	GtkWidget *start_acquisition;
	GtkWidget *stop_acquisition;
	GtkWidget *toggle_save;
	GtkWidget *select_save_path;

	GtkLabel *x_centroid;
	GtkLabel *y_centroid;
	GtkLabel *x_std;
	GtkLabel *y_std;

	GtkPicture *img_window;

	char **deviceNames;
	GtkWidget *deviceDropdown;

	GtkWidget *exp_time;
	GtkWidget *gain;

	int64_t exp_time_min;
	int64_t exp_time_max;

	int64_t gain_min;
	int64_t gain_max;
};

static struct AppData appData = {.opened=false, .exp_time_min=0, .exp_time_max=0, .gain_min=0, .gain_max=0};


/* Thread Control */

// Execute at end of async camera thread
static void camera_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
	TaskData *task_data = & appData.workerThread->task_data;

	// Clean up
	g_mutex_clear(task_data->lock);
	g_object_unref(thread->task);

	// Enable start/device selection buttons
	gtk_widget_set_sensitive(appData.start_acquisition, TRUE);
	gtk_widget_set_sensitive(appData.deviceDropdown, TRUE);

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

		// Initialize thread data
		thread->task_data = (struct TaskData){.hDev=appData.hDev, .image_window=appData.img_window, .save=false, .save_path=appData.save_path, .x_centroid=appData.x_centroid, .y_centroid=appData.y_centroid, .x_std=appData.x_std, .y_std=appData.y_std};

		// Initialize lock
		thread->task_data.lock = (GMutex *)malloc(sizeof(GMutex));
		g_mutex_init(thread->task_data.lock);
		g_task_set_task_data(thread->task, &thread->task_data, NULL);

		g_task_run_in_thread(thread->task, camera);

		// Disable button
		gtk_widget_set_sensitive(widget, FALSE);
		gtk_widget_set_sensitive(appData.stop_acquisition, TRUE);
		gtk_widget_set_sensitive(appData.deviceDropdown, FALSE);
		gtk_widget_set_sensitive(appData.toggle_save, TRUE);
	}
}

// Stop acquisition button
static void stop_acquisition(GtkWidget *widget, gpointer data_) {
	struct WorkerThread *thread = appData.workerThread;

	if (gtk_widget_is_sensitive(widget)) {
		// Disable button
		gtk_widget_set_sensitive(widget, FALSE);
		gtk_widget_set_sensitive(appData.toggle_save, FALSE);
		gtk_button_set_label(appData.toggle_save, "Start Save");

		// Stop camera thread
		g_cancellable_cancel(thread->cancellable);
	}
}

// Stop/start save
static void toggle_save(GtkWidget *widget, gpointer data_) {
	if (gtk_widget_is_sensitive(widget)) {
		struct TaskData *task_data = & appData.workerThread->task_data;

		g_mutex_lock(task_data->lock);
		if (task_data->save) {
			// Stop save
			task_data->save = false;
			gtk_button_set_label(widget, "Start Save");

			gtk_widget_set_sensitive(appData.select_save_path, TRUE);
		}
		else {
			// Start save
			gtk_widget_set_sensitive(appData.select_save_path, FALSE);

			task_data->save = true;
			gtk_button_set_label(widget, "Stop Save");
		}
		g_mutex_unlock(task_data->lock);
	}
}

// Camera session select dropdown
static void select_camera(GtkWidget *widget, gpointer data_) {
	if (gtk_widget_is_sensitive(widget)) {
		GENAPIC_RESULT res;
		int index = gtk_drop_down_get_selected(widget) - 1;

		// Close device if already open
		if (appData.opened) {
			res = PylonDeviceClose(appData.hDev);
			CHECK(res);

			appData.opened = false;

			gtk_widget_set_sensitive(appData.start_acquisition, FALSE);
			gtk_widget_set_sensitive(appData.exp_time, FALSE);
			gtk_widget_set_sensitive(appData.gain, FALSE);
		}
		
		// Open selected device
		if (index >= 0) {
			_Bool isAccessible;
			res = PylonIsDeviceAccessible(index, PYLONC_ACCESS_MODE_CONTROL | PYLONC_ACCESS_MODE_STREAM, &isAccessible);
			CHECK(res);

			if (isAccessible) {
				printf("Open camera %d\n", index);
				// Open Camera
				res = PylonCreateDeviceByIndex( index, &appData.hDev );
				CHECK(res);

				res = PylonDeviceOpen( appData.hDev, PYLONC_ACCESS_MODE_CONTROL | PYLONC_ACCESS_MODE_STREAM );
				CHECK(res);

				appData.opened = true;

				gtk_widget_set_sensitive(appData.start_acquisition, TRUE);
				gtk_widget_set_sensitive(appData.exp_time, TRUE);
				gtk_widget_set_sensitive(appData.gain, TRUE);
			} else {
				gtk_message_dialog_new( GTK_WINDOW(appData.window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, "Could not open camera: likely in use by another session.\n");
			}
		}
	}
}

// Set label to given integer camera setting value
static void set_feature_label(GtkEntryBuffer *entry_buffer, char *feature_name) {
	GENAPIC_RESULT res;

	// Get feature value
	int64_t feature_val = 0;
	if (PylonDeviceFeatureIsReadable(appData.hDev, feature_name)) {
		res = PylonDeviceGetIntegerFeature(appData.hDev, feature_name, &feature_val);
		CHECK(res);
	}

	char feature_str[6];
	sprintf(feature_str, "%5d", feature_val);

	gtk_entry_buffer_set_text(entry_buffer, feature_str, 6);

}

// Set exposure time
static void set_exp_time(GtkWidget *widget, gpointer data) {
	GENAPIC_RESULT res;

	GtkEntryBuffer *buf = gtk_entry_get_buffer(GTK_ENTRY(widget));
	char *entry_text = gtk_entry_buffer_get_text(buf);

	int64_t entry_val = atoll(entry_text);
	if (entry_val >= appData.exp_time_min && entry_val <= appData.exp_time_max) {
		// Set exposure time
		if (PylonDeviceFeatureIsWritable(appData.hDev, "ExposureTimeRaw")) {
			res = PylonDeviceSetIntegerFeature(appData.hDev, "ExposureTimeRaw", entry_val);
			CHECK(res);
		}
	}

	set_feature_label(buf, "ExposureTimeRaw");
}

// Set gain
static void set_gain(GtkWidget *widget, gpointer data) {
	GENAPIC_RESULT res;

	GtkEntryBuffer *buf = gtk_entry_get_buffer(GTK_ENTRY(widget));
	char *entry_text = gtk_entry_buffer_get_text(buf);

	int64_t entry_val = atoll(entry_text);
	if (entry_val >= appData.gain_min && entry_val <= appData.gain_max) {
		// Set gain
		if (PylonDeviceFeatureIsWritable(appData.hDev, "GainRaw")) {
			res = PylonDeviceSetIntegerFeature(appData.hDev, "GainRaw", entry_val);
			CHECK(res);
		}
	}

	set_feature_label(buf, "GainRaw");
}

// Save file path dialog callbacks
static void save_path_dialog_response(GtkDialog *dialog, int response) {
	if (response=GTK_RESPONSE_ACCEPT) {
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		
		g_object_unref(appData.save_path);
		appData.save_path = gtk_file_chooser_get_file(chooser);
	}

	gtk_window_destroy(GTK_WINDOW(dialog));
}

static void save_path_dialog_callback(GtkWidget *widget, gpointer data) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Folder", appData.window, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
	gtk_window_present(GTK_WINDOW(dialog));
	g_signal_connect(dialog, "response", G_CALLBACK( save_path_dialog_response ), NULL);
}



// Initialize GUI Window
static void activate(GtkApplication *app, gpointer data_) {
	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "builder.ui", NULL);

	GObject *window = gtk_builder_get_object(builder, "window");
	gtk_window_set_application(GTK_WINDOW(window), app);
	appData.window = window;

	// Set menu bar
	/*GtkBuilder *menu_builder = gtk_builder_new();
	gtk_builder_add_from_file(menu_builder, "menubar.ui", NULL);
	GObject *titlebar = gtk_builder_get_object(builder, "menu");
	gtk_window_set_titlebar(GTK_WINDOW(window), titlebar);*/

	// Button Signal Handlers
	GObject *widget = gtk_builder_get_object(builder, "start_acquisition");
	g_signal_connect(widget, "clicked", G_CALLBACK(start_acquisition), NULL);
	gtk_widget_set_sensitive(widget, FALSE);
	appData.start_acquisition = widget;

	widget = gtk_builder_get_object(builder, "stop_acquisition");
	g_signal_connect(widget, "clicked", G_CALLBACK(stop_acquisition), NULL);
	gtk_widget_set_sensitive(widget, FALSE);
	appData.stop_acquisition = widget;

	widget = gtk_builder_get_object(builder, "save_btn");
	g_signal_connect(widget, "clicked", G_CALLBACK(toggle_save), NULL);
	appData.toggle_save = widget;

	widget = gtk_builder_get_object(builder, "select_save_path");
	g_signal_connect(widget, "clicked", G_CALLBACK(save_path_dialog_callback), NULL);
	appData.select_save_path = widget;

	// Data Output
	widget = gtk_builder_get_object(builder, "x_centroid");
	appData.x_centroid = GTK_LABEL(widget);

	widget = gtk_builder_get_object(builder, "y_centroid");
	appData.y_centroid = GTK_LABEL(widget);

	widget = gtk_builder_get_object(builder, "x_std");
	appData.x_std = GTK_LABEL(widget);

	widget = gtk_builder_get_object(builder, "y_std");
	appData.y_std = GTK_LABEL(widget);

	// Camera Settings Inputs
	widget = gtk_builder_get_object(builder, "exp_time");
	GtkEntryBuffer *buf = gtk_entry_get_buffer(GTK_ENTRY(widget));
	set_feature_label(buf, "ExposureTimeRaw");
	gtk_widget_set_sensitive(widget, FALSE);
	g_signal_connect(widget, "activate", G_CALLBACK(set_exp_time), NULL);
	appData.exp_time = widget;

	widget = gtk_builder_get_object(builder, "gain");
	GtkEntryBuffer *buf = gtk_entry_get_buffer(GTK_ENTRY(widget));
	set_feature_label(buf, "GainRaw");
	gtk_widget_set_sensitive(widget, FALSE);
	g_signal_connect(widget, "activate", G_CALLBACK(set_gain), NULL);
	appData.gain = widget;

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

	// Default save filepath (current directory)
	appData.save_path = g_file_new_for_path(".");

	// Min/Max setting values
	if (PylonDeviceFeatureIsReadable(hDev, "ExposureTimeRaw")) {
		res = PylonDeviceGetIntegerFeatureMin(hDev, "ExposureTimeRaw", &appData.exp_time_min);
		CHECK(res);
		res = PylonDeviceGetIntegerFeatureMax(hDev, "ExposureTimeRaw", &appData.exp_time_max);
		CHECK(res);
	}
	if (PylonDeviceFeatureIsReadable(hDev, "GainRaw")) {
		res = PylonDeviceGetIntegerFeatureMin(hDev, "GainRaw", &appData.gain_min);
		CHECK(res);
		res = PylonDeviceGetIntegerFeatureMax(hDev, "GainRaw", &appData.gain_max);
		CHECK(res);
	}

	// Build application
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
