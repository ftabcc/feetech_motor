process(){
    while(true){
        std::size_t rx_size = 0;
        const esp_err_t ret = tinyusb_cdcacm_read(itf, &rx_buffer[read_available], rx_buffer_size - read_available, &rx_size);
        if (ret != ESP_OK){
            statis.errors |= pi_protocol::Comm_Error::CDC_ERR;
            break;
        }
        bool ready = (!found &&  read_av >= idx + header_len) || // !found일때 header 읽기 가능?
                (found && !confirm && read_av >= idx + PKT_INST) || // found일때 len,id,inst 필드 읽기 가능?
                (confirm && read_av >= idx + packet_len); // confirm일때 packet 전체 읽기 가능?
        
        if(rx_size != 0)
            read_available += rx_size;
            if (rx_start_time ==0)
                rx_start_time = time;
        if(rx_size == 0 && !ready)
            status.result = pi_protocol::Comm_Result::PENDING;

        if (ready){
            if(!found){
                if (read_available >= idx + header_len){ // header 필드 읽기 가능
                    while (idx <= read_available - header_len){// limit까지만 헤더 확인 가능
                        uint8_t *p = (uint8_t *)memchr(&rx_buffer[idx], 0xFF, (size_t)(limit_idx - idx + 1));
                        if (p != nullptr){
                            idx = (uint16_t)(p - rx_buffer);
                            if ((rx_buffer[idx + 1] == 0xFF) &&(rx_buffer[idx + 2] == 0xFD) &&(rx_buffer[idx + 3] == 0)){
                                found = true;
                                break;
                            }
                        }
                        else{// 남은 구간에 0xFF 없으므로 더 볼 필요 없음
                            idx = read_available - header_len + 1;
                            break;
                        }
                        idx += 1;
                    }
                }
            }
            
            if(found){
                if (read_available >= idx + header_len + 3){ //len,id,inst 필드 읽기 가능?
                    if(rx_buffer[idx + pi_protocol::PKT_LENGTH] <= pi_protocol::RXPACKET_MAX_LEN  &&
                    rx_buffer[idx + pi_protocol::PKT_LENGTH] >= pi_protocol::RXPACKET_MIN_LEN &&
                    rx_buffer[idx + pi_protocol::PKT_ID] > prev_id && '''wrap-around주의'''
                    rx_buffer[idx + pi_protocol::PKT_INSTRUCTION] != ??){// len,id,inst 필드 정상
                        confirm = true;
                        const uint16_t packet_len = rx_buffer[idx + pi_protocol::PKT_LENGTH];
                        if(read_available >= idx + packet_len){ // data 읽기 가능
                            // CRC(little endian L,H)
                            uint16_t crc = static_cast<uint16_t>(rx_buffer[idx + packet_len-2]) | // L byte
                                            (static_cast<uint16_t>(rx_buffer[idx + packet_len-1]) << 8); // H byte
                            uint16_t calculated_crc = updateCRC(0, &rx_buffer[idx], packet_len-2); //without crc 2 byte
                            if(crc == calculated_crc){// (FF FF FD 00 LEN ID INST DATA CRC_L CRC_H)
                                rxpacket.data_len = packet_len - 9; // rxpacket_len = DATA(N) + 9 
                                rxpacket.id = rx_buffer[idx + pi_protocol::PKT_ID];
                                rxpacket.inst = rx_buffer[idx + pi_protocol::PKT_INSTRUCTION];
                                memcpy(rxpacket.data,&rx_buffer[idx+pi_protocol::PKT_DATA],rxpacket.data_len);
                                result = unstuffing(rxpacket.data, &rxpacket.data_len);

                                // prepare for next
                                memmove(rx_buffer,rx_buffer + idx + packet_len,read_available - (idx + packet_len));
                                read_available -= idx + packet_len;
                                idx = 0;
                                found = false;
                                rx_start_time_us = 0;
                                
                                status.result |= pi_protocol::Comm_Result::SUCCESS; 
                            }
                            else:{
                                status.result |= pi_protocol::Comm_Result::FAILURE;
                            }
                            found = false;
                            confirm = false;
                            rx_start_time_us = 0;
                        }
                    }
                    else{
                        found = false;
                        confirm = false;
                        idx += header_len;
                    }
                }
            }
        }
            
        if(read_available == pi_protocol::rx_buffer_size){
            status.errors |= pi_protocol::Comm_Error::BUFFER_FULL;
        }
        if(rx_start_time_us != 0 && (esp_timer_get_time() - rx_start_time_us) > pi_protocol::RX_TIMEOUT_US){
            status.erros |= pi_protocol::Comm_Error::RX_TIMEOUT;
        }
        if(stuts.result != Null || status.err != 0)
            break;
    }
    return status;
}

// 26-10-08
process(){
    while(true){
        '''읽기 단계.'''
        // 가능한 만큼 cdc_read;
        if(ret!=ok)
            // status.err |= cdc_err;
            // break;
        
        if (read==0 && !one_more_check)
            // status.result = pendig;
            // break;
        if (one_more_check)
            one_more_check = false;

        '''헤더 찾기 단계'''
        if(!found)
            if (rx_start_time ==0)
                rx_start_time = time;
            if(read_avail >= idx + header_len)
                whlie(idx <= read_avail - header_len)
                    search;
                    if(header check)
                        found = true;
                        break;
                    if(nothing search)
                        idx = read_avail - header_len + 1;
                        break;
                    idx+=1;

        '''패킷 확인 단계'''
        if(found)
            if (len,id,inst 필드 읽기 가능)
                if(len,id,inst 정상)
                    if (data 필드 읽기 가능)
                        if(crc=calculated_crc)
                            // status.result = success;
                        else:
                            // found = false;
                            // status.result = failure;
                            
                else
                    // found = false;
                    // idx += header_len;
        '''err추가 확인단계'''
        if(read_av == rx_buffer_size)
            // status.err |= full
            break;
        if(status.result != pending)
            break;
        if(time - rx_start_time > timeout && status.result != success)
            // status.err |= timeout
            break;
        if(stuts.result != pending)
            break;
    }
    if (rx_start_time != 0)       
        if(time - rx_start_time > timeout)
            // status.err |= timeout
    return status;
}
