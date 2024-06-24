#include "../common/ocTypes.h"
#include "../common/ocMember.h"
#include <signal.h>
#include <csignal>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>

// uncomment to use on systems that have no CUDA
// #define FORBID_CUDA

// if this is set the BEV is redrawn only with the
// detected lines for easier debugging/visuliazation. This
// is normally disabled since it requires an entire Matrix copy
// and adds other graphical overhead by the drawing itself

// 0 for lane BEV and 2 for intersection BEV
#define VIDEO_OUTPUT 0


// use cuda acceleration by default
#ifndef FORBID_CUDA
using namespace cv::cuda;
#else
using namespace cv;
#endif

using cv::Point2f;
using cv::Mat;
using cv::Point;

using cv::Size_;

static bool running = true;
ocLogger *logger;

Point2f src_vertices_lane_detection[4];
Point2f src_vertices_intersection_detection[4];
Point2f dst_vertices[4];
Mat M_lane_detection;
Mat M_intersection_detection;

static constexpr auto BLUR_SIZE = 7;
static constexpr auto POST_CANNY_BLUE_SIZE = 9;

static void signal_handler(int)
{
    running = false;
}

void initializeTransformParams() {
    src_vertices_lane_detection[0] = Point2f(70,210);
    src_vertices_lane_detection[1] = Point2f(330,210);
    src_vertices_lane_detection[2] = Point2f(780, 310);
    src_vertices_lane_detection[3] = Point2f(-380, 310);

    src_vertices_intersection_detection[0] = Point2f(130,190);
    src_vertices_intersection_detection[1] = Point2f(270,190);
    src_vertices_intersection_detection[2] = Point2f(1200, 400);
    src_vertices_intersection_detection[3] = Point2f(-800, 400);


    dst_vertices[0] = Point2f(0, 0);
    dst_vertices[1] = Point2f(400, 0);
    dst_vertices[2] = Point2f(400, 400);
    dst_vertices[3] = Point2f(0, 400);

    M_lane_detection = getPerspectiveTransform(src_vertices_lane_detection, dst_vertices);
    M_intersection_detection = getPerspectiveTransform(src_vertices_intersection_detection, dst_vertices);
}

void toBirdsEyeView(Mat &src, Mat &dst, Mat &transofmation) {
    warpPerspective(src, dst, transofmation, dst.size());
}

int main() {
    // Catch some signals to allow us to gracefully shut down the process
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    

    ocMember member(ocMemberId::Image_Processing, "Image_Processing");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    logger = member.get_logger();
    ocSharedMemory *shared_memory = member.get_shared_memory();

    ocPacket ipc_packet;
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Camera_Image_Available);
    socket->send_packet(ipc_packet);

    initializeTransformParams();

    // Listen for Camera Image Available Message on IPC

    while(running) {
        int32_t socket_status;
        while (0 < (socket_status = socket->read_packet(ipc_packet, false)))
        {
            switch(ipc_packet.get_message_id())
                {
                    case ocMessageId::Camera_Image_Available:
                    {

                        // Move to shared memory

                        ocTime frameTime;
                        uint32_t frameNumber;
                        ptrdiff_t memoryAdressOffset;
                        size_t dataSize;

                        ipc_packet.read_from_start()
                            .read<ocTime>(&frameTime)
                            .read<uint32_t>(&frameNumber)
                            .read<ptrdiff_t>(&memoryAdressOffset)
                            .read<size_t>(&dataSize);

                        ocCamData *tempCamData = &shared_memory->cam_data[shared_memory->last_written_cam_data_index];

                        shared_memory->bev_data[0] = (ocBevData) {
                            tempCamData->frame_time,
                            tempCamData->frame_number,
                            0,(int32_t) 400,
                            0,(int32_t) 400,
                            {}
                        };

                        shared_memory->bev_data[2] = (ocBevData) {
                            tempCamData->frame_time,
                            tempCamData->frame_number,
                            0,(int32_t) 400,
                            0,(int32_t) 400,
                            {}
                        };

                        // TODO: Consider changing the internal implementation to use
                        // OpenCV. Currently it's a single threaded loop! Convert img
                        // Convert img from color to bw
                        convert_to_gray_u8(tempCamData->pixel_format, tempCamData->img_buffer, tempCamData->width, tempCamData->height, shared_memory->bev_data[0].img_buffer, 400, 400);

                        shared_memory->last_written_bev_data_index = VIDEO_OUTPUT;

                        // Apply birds eye view

                        Mat src(400, 400, CV_8UC1, shared_memory->bev_data[0].img_buffer);

                        Mat dst_lane(400, 400, CV_8UC1, shared_memory->bev_data[0].img_buffer);
                        Mat dst_intersection(400, 400, CV_8UC1, shared_memory->bev_data[2].img_buffer);

                        // intersection needs to be calculated first since lane writes to itself!
                        toBirdsEyeView(src, dst_intersection, M_intersection_detection);
                        toBirdsEyeView(src, dst_lane, M_lane_detection);

                        GaussianBlur(dst_lane, dst_lane, Size_(BLUR_SIZE, BLUR_SIZE), 0);
                        Canny(dst_lane, dst_lane, 10, 50, 3, true);
                        GaussianBlur(dst_lane, dst_lane, Size_(POST_CANNY_BLUE_SIZE, POST_CANNY_BLUE_SIZE), 0);

                        GaussianBlur(dst_intersection, dst_intersection, Size_(BLUR_SIZE, BLUR_SIZE), 0);
                        Canny(dst_intersection, dst_intersection, 50, 200, 3, true);
                        GaussianBlur(dst_intersection, dst_intersection, Size_(POST_CANNY_BLUE_SIZE, POST_CANNY_BLUE_SIZE), 0);

                        // notify others about available picture
                        ipc_packet.set_sender(ocMemberId::Image_Processing);
                        ipc_packet.set_message_id(ocMessageId::Birdseye_Image_Available);
                        socket->send_packet(ipc_packet);
                        
#ifndef hideContours

                        ipc_packet.set_sender(ocMemberId::Image_Processing);
                        ipc_packet.set_message_id(ocMessageId::Lines_Available);

                        // notify other about found lines in BEV
                        socket->send_packet(ipc_packet);


#endif
                    } break;
                    default:
                    {
                        ocMessageId msg_id = ipc_packet.get_message_id();
                        ocMemberId  mbr_id = ipc_packet.get_sender();
                        logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                    } break;
                }
        }
    }

    return 0;
}
