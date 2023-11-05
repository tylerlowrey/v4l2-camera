#include <linux/videodev2.h>

/**
 * Setup the camera in YUYV format
 * @param camera_file_path - File path to the camera (ex: /dev/video0)
 * @param width - Desired width of image from camera
 * @param height - Desired height of image from camera
 * @return fd - The file descriptor to the camera in use or -1 if an error occurred
 */
int setup_camera(char* camera_file_path, int width, int height);

/**
 * Get mmap buffer information
 * @param camera_fd - An open file descriptor to be used
 * @param buffer_count - The requested number of buffers
 * @return request_buffers - a pointer to a v4l2_requestbuffers struct or null if an error occurred
 */
struct v4l2_requestbuffers* request_mmap_buffers(int camera_file_descriptor, int buffer_count);

struct buffer {
    void *start;
    size_t length;
};

/**
 * Uses mmap to map the requested buffers retrieved from a call to request_mmap_buffers
 * @param request_buffers
 * @return - An array of mmapped buffer structs
 */
struct buffer* map_buffers(struct v4l2_requestbuffers* request_buffers);
