#pragma once
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

/**
 * A struct containing a pointer to a mmapped buffer (mapped during map_buffers)
 */
struct buffer {
    void *start;
    size_t length;
};

/**
 * Uses mmap to map the requested buffers retrieved from a call to request_mmap_buffers
 * @param fd - The file descriptor of the open device
 * @param request_buffers - The result from a call to request_mmap_buffers
 * @return - A pointer to an array of mmapped buffer structs
 */
struct buffer* map_buffers(int fd, struct v4l2_requestbuffers* request_buffers);

/**
 * Unmaps and frees an array of buffer structs
 * @param buffers - a pointer to an array of buffer structs
 * @param buffers_length - Number of buffer structs in the array
 */
void cleanup_buffers(struct buffer* buffers, size_t buffers_length);

/**
 * Queue buffers 0 through num_buffers
 * @param fd - Open file descriptor to the device
 * @param num_buffers - The total number of buffers to queue
 */
void queue_buffers(int fd, size_t num_buffers);

/**
 * Tries to dequeue a buffer
 * @param fd - File descriptor to open device
 * @return The index of the dequeued buffer or < 0 for an error
 */
int dequeue_buffer(int fd);

/**
 * Re-queues a buffer (identified by its index) so that it can be used again
 * @param fd - File descriptor to open device
 * @param buffer_index - Index of the buffer to re-queue
 * @return
 */
int requeue_buffer(int fd, int buffer_index);

/**
 * Starts the camera stream
 * @param fd - The file descriptor for the open video capture device
 * @return - 0 for success, -1 for failure (passes return value from ioctl VIDIOC_STREAMON call )
 */
int start_stream(int fd);

/**
 * Stops the camera stream
 * @param fd - The file descriptor for the open video capture device
 * @return - 0 for success, -1 for failure (passes return value from ioctl VIDIOC_STREAMOFF call )
 */
int stop_stream(int fd);