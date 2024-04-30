#include "Crossing_3_Way_T.h"


State& Crossing_3_Way_T::get_instance(){
    static Crossing_3_Way_T singleton;
    return singleton;
}



void Crossing_3_Way_T::on_entry(Statemachine* statemachine){
    /*
    Get IPC-hub messages regarding traffic signs;
    Create array of traffic-signs (types and distances);
    Array an statemachine->run übergeben;

    //Bekommen Array vom struct übergeben
    struct TrafficSign{
        Type trafficSign
        int distance
    }


    ocMember member = ocMember(ocMemberId::, "Traffic Sign");
    member.attach();
    ocIpcSocket *socket = member.get_socket();
    ocLogger *logger = member.get_logger();
    ocPacket sup;
    sup.set_message_id(ocMessageId::Subscribe_To_Messages);
    sup.clear_and_edit().write(ocMessageId::);
    socket->send_packet(sup);
    ocPacket recv_packet;
    int counter = 0;

    while (true) {
       
        int result = socket->read_packet(recv_packet);

        if (result < 0) {
            logger->error("Error reading the IPC socket: (%i) %s", errno, strerror(errno));
            break;
        }

        switch (recv_packet.get_message_id())
        {
        case ocMemberId:: :
            auto reader = recv_packet.read_from_start();
            break;
        
        default:
            ocMessageId msg_id = recv_packet.get_message_id();
            ocMemberId mbr_id = recv_packet.get_sender();
            logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
            break;
        }
        
    }

    statemachine->run(Crossing_3_Way_T::get_instance, trafficSigns[]);    
    */
    
}


void Crossing_3_Way_T::run(Statemachine* statemachine, void* data){
    /*

    bool drive_left = false;
    bool drive_right = false;

    for sign in array:
        if (distance < 50){ //50cm == width of crossing; If distance larger, than sign is irrelevant for crossing
            switch(sign_type){
                case Stop:
                    drive.stop(2000); //stop for 2s
                    break;
                case RightOfWay:
                    drive_right = true;
                    break;
                case Left:
                    drive_left = true;
                    break;
                case Right:
                    drive_right = true;
                    break;
            }
        }

    if(drive_left && drive_right){
        drive_left = false;
    }

    //if obstacle, stop

    if(drive_left){
        drive.turn_left();
    } else if (drive_right){
        drive.turn_right();
    } else{
        drive.turn_right();
    }

    statemachine->change_state(Normal_Drive::getInstance());
    */
}



void Crossing_3_Way_T::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
