#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "camera.h"

int setup_camera(char* camera_file_path, int width, int height) {
    struct v4l2_capability device_capabilities;
    int video_device_fd = open(camera_file_path, O_RDWR | O_NONBLOCK);

    if (video_device_fd == -1) {
        perror("Unable to open the requested video device");
        return -1;
    }

    if (ioctl(video_device_fd, VIDIOC_QUERYCAP, &device_capabilities) == -1) {
        perror("VIDIOC_QUERYCAP");
        return -1;
    }

    printf("Video card: %s\n", device_capabilities.card);
    printf("Device Capabilities Flags: %x\n", device_capabilities.capabilities);
    printf("Supports single-planar API: %s\n", device_capabilities.capabilities & 0x1 ? "true" : "false");
    printf("Supports multi-planar API: %s\n", device_capabilities.capabilities & 0x1000 ? "true" : "false");

    bool isMMappedStreamingSupported = device_capabilities.capabilities & 0x04000000;
    printf("Supports memory mapped streaming: %s\n", isMMappedStreamingSupported ? "true" : "false");

    if (!isMMappedStreamingSupported) {
        printf("This video device does not support memory mapped streaming. exiting...\n");
        return -1;
    }

    struct v4l2_format set_format_command;
    memset(&set_format_command, 0, sizeof(set_format_command));
    set_format_command.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    set_format_command.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    set_format_command.fmt.pix.width = width;
    set_format_command.fmt.pix.height = height;
    set_format_command.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(video_device_fd, VIDIOC_S_FMT, &set_format_command) == -1) {
        perror("VIDIOC_S_FMT");
        return -1;
    }

    struct v4l2_format get_format_command;
    memset(&get_format_command, 0, sizeof(get_format_command));
    get_format_command.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(video_device_fd, VIDIOC_G_FMT, &get_format_command) == -1) {
        perror("VIDIOC_G_FMT");
        return -1;
    }

    // Convert fourCC to readable string
    char fourCC[5];
    strncpy(fourCC, (char*) &get_format_command.fmt.pix.pixelformat, 4);

    printf("Video capture format info [width: %d, height: %d, pixelformat: %s]\n",
           get_format_command.fmt.pix.width,
           get_format_command.fmt.pix.height,
           fourCC);

    return video_device_fd;
}

struct v4l2_requestbuffers* request_mmap_buffers(int camera_fd, int buffer_count){
    struct v4l2_requestbuffers* request_buffers = (struct v4l2_requestbuffers*) malloc(sizeof(struct v4l2_requestbuffers));
    memset(&request_buffers, 0, sizeof(request_buffers));
    request_buffers->count = buffer_count;
    request_buffers->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request_buffers->memory = V4L2_MEMORY_MMAP;

    if (ioctl(camera_fd, VIDIOC_REQBUFS, &request_buffers) == -1) {
        if (errno == EINVAL) {
            printf("Video capturing or mmap-streaming is not supported\n");
        } else {
            perror("VIDIOC_REQBUFS");
        }
        return NULL;
    }

    if (request_buffers->count != buffer_count) {
        printf("Requested %d buffers but received %d\n", buffer_count, request_buffers->count);
        return NULL;
    }

    return request_buffers;
}

struct buffer* map_buffers(struct v4l2_requestbuffers* request_buffers) {
    return NULL;
}