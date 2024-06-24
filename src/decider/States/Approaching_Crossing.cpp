#include "Approaching_Crossing.h"
#include "Is_At_Crossing.h"
#include "../Driver.h"
#include <math.h>


State& Approaching_Crossing::get_instance(){
    static Approaching_Crossing singleton;
    return singleton;
}

void Approaching_Crossing::initialize(){
    if(!is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.clear_and_edit()
            .write(ocMessageId::Intersection_Detected)
            .write(ocMessageId::Lane_Detection_Values);
        socket->send_packet(sup);

        is_initialized = true;
    }
}



void Approaching_Crossing::on_entry(Statemachine* statemachine){
    initialize();

    statemachine->run(nullptr);
}


void Approaching_Crossing::run(Statemachine* statemachine, void* data){
    bool is_at_crossing = false;
    ocPacket recv_packet;

    uint32_t min_distance = 10;
    uint32_t max_distance = 25;
    uint32_t distance = 0xFFFF;

    int16_t speed = 15;
    int16_t min_speed = 15;
    int8_t steering_front = 0;


    while (!is_at_crossing) {
       
        int result = socket->read_packet(recv_packet);
        ocTime now = ocTime::now();

        if (result < 0) {
            logger->error("Error reading the IPC socket: (%i) %s", errno, strerror(errno));
        } else {

            switch (recv_packet.get_message_id()){
                case ocMessageId::Intersection_Detected:{
                    auto reader = recv_packet.read_from_start();
                    distance = reader.read<uint32_t>();
                    uint8_t crossing_type = reader.read<uint8_t>();
                    logger->log("Distance: %d", distance);
                }break;

                case ocMessageId::Lane_Detection_Values:{
                    auto reader = recv_packet.read_from_start();
                    speed = reader.read<int16_t>();
                    steering_front = reader.read<int8_t>();
                    
                }break;
                
                default:{
                    ocMessageId msg_id = recv_packet.get_message_id();
                    ocMemberId mbr_id = recv_packet.get_sender();
                    logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                }break;
            }
        }

        if(distance <= min_distance) {
            is_at_crossing = true;
        } else if (distance > max_distance){
            Driver::drive(speed / 4, steering_front);
            Driver::wait(0.1);
        } else {
            Driver::drive(min_speed, steering_front);
            Driver::wait(0.1);
        }

        
        /*
        //algorithm for slowing down towards 2cm/s
        double* arr = smooth_speed(speed);
        for(int i = 0; i < 100; i++) {
            int result = socket->read_packet(recv_packet);

            if (result >= 0) {
                switch (recv_packet.get_message_id()){
                    case ocMessageId::Intersection_Detected:{
                        auto reader = recv_packet.read_from_start();
                        distance = reader.read<uint32_t>();
                        uint8_t crossing_type = reader.read<uint8_t>();
                    }break;

                    case ocMessageId::Lane_Detection_Values:{
                        auto reader = recv_packet.read_from_start();
                        speed = reader.read<int16_t>();
                        steering_front = reader.read<int8_t>();
                    }break;

                    default:
                        break;
                }

                int16_t next_speed = arr[i] > min_speed ? arr[i] : min_speed;
                Driver::drive(next_speed, steering_front);    //drive_forward(arr[i]);
                Driver::wait(0.1);
            }

            if(distance <= threshold) {
                is_at_crossing = true;
                break;
            }
            
        }

        free(arr);
        */
    }

    
    logger->log("Changing state from Approaching_Crossing to Is_At_Crossing");
    statemachine->change_state(Is_At_Crossing::get_instance());
    
}



void Approaching_Crossing::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}

double* Approaching_Crossing::smooth_speed(int16_t current_speed) {
    int range = 100;
    int nzero = current_speed;
    float a = 0.98; //0<a<1 current: 2%
    int y = 0;

    double* results = (double*)malloc(sizeof(double) * 100);

    for(int i = 0; i < range; i++) {
        y = nzero * pow(a, i);
        results[i] = y;
    }

    return results;

}