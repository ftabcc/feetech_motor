void pi_comm::rx_task(void *arg){
    pi_comm *self = static_cast<pi_comm *>(arg);
    wait = true;
    while(true){
        TickType_t wait_ticks;
        if(self->rx_wait_start_time_us != 0){
            const int64_t elapsed_us = esp_timer_get_time() - self->rx_wait_start_time_us;
            if (elapsed_us > pi_protocol::RX_TIMEOUT_US && self->read_available != 0)
                self->rx_errors |= pi_protocol::rx_error::RX_TIMEOUT;
            else{
                wait_ticks = pdMS_TO_TICKS(static_cast<uint32_t>((pi_protocol::RX_TIMEOUT_US - elapsed_us + 999) / 1000));
                if (wait_ticks == 0) // Avoid immediate return when tick resolution is coarse.
                    wait_ticks = 1;
                }
        }
        else
            wait_ticks = portMAX_DELAY;

        if(!(self->rx_errors & pi_protocol::rx_error::RX_TIMEOUT)){
            BaseType_t notified = xTaskNotifyWait(0,0xFFFFFFFF,&notify_value,wait_ticks);
            if (notified == pdFALSE && self->read_available != 0) // 읽던 바이트가 있을때
                self->rx_errors |= pi_protocol::rx_error::RX_TIMEOUT;
                '''tick이 정확하지 않아서 위에서 시간단위로 확인 필요.'''
        }
        while (true){
            if(!(self->rx_errors & pi_protocol::rx_error::RX_TIMEOUT))
                pi_protocol::rx_result result = self->rx_packet(rxpacket;);

            if(result == pi_protocol::rx_result::SUCCESS){
                // 보낼 rxpacket 완성하기
                pi_protocol::rxpacket_t rxpacket;
                uint8_t packet_len = rx_buffer[idx + pi_protocol::RX_LENGTH_IDX];
                rxpacket.data_len = packet_len - 9; // rxpacket_len = DATA(N) + 9 
                rxpacket.id = rx_buffer[idx + pi_protocol::RX_ID_IDX];
                rxpacket.inst = rx_buffer[idx + pi_protocol::RX_INSTRUCTION_IDX];
                memcpy(rxpacket.data,&rx_buffer[idx+pi_protocol::RX_DATA_IDX],rxpacket.data_len);
                unstuffing(rxpacket.data, &rxpacket.data_len);
                // 완성된 packet빼고 buffer당기기
                memmove(&rx_buffer[idx],rx_buffer + idx + packet_len,read_available - (idx + packet_len));
                self->read_available -= packet_len;
                // 패킷 보내기
                if (xQueueSend(self->rx_queue, &rxpacket, 0) != pdTRUE){
                    xQueueReset(self->rx_queue);
                    xQueueSend(self->rx_queue, &rxpacket, 0);
                }
            }
            if(result == pi_protocol::rx_result::FAILURE){ // CRC_ERR, just check next
                self->idx += pi_protocol::HEADER_LEN;
            }
            
            // connect byte debug stream. timeout: [0,all], full & success: [0,idx)
            if(self->rx_errors != 0 || result == pi_protocol::rx_result::SUCCESS ||){
                size_t sent_bytes = 0;
                while (sent_bytes < idx)
                {
                    pi_protocol::txpacket_t txpacket{};
                    txpacket.data_len = std::min(untill - sent_bytes,sizeof(txpacket.data));
                    txpacket.id = debug_packet_id;
                    txpacket.inst = /* debug */;
                    txpacket.err = self->rx_errors;

                    memcpy(txpacket.data,rx_buffer + sent_bytes,txpacket.data_len);
                    if (xQueueSend(self->tx_queue, &txpacket, pdMS_TO_TICKS(10)) != pdTRUE) {
                        // Do not reset the queue, Preserve the unsent debug bytes and retry later.
                    }'''지금은 tx큐 가득차는 경우없다고 가정.'''
                    sent_bytes += txpacket.data_len;
                    debug_packet_id++;
                }
                
                if(result == success) // 다음부터는 byte stream이 의미적으로 연결안됨.
                    debug_packet_id = 0;
                
                if (sent_bytes > 0){ '''tx-debug 실패가능.성공한 바이트들만 당기기 진행필요'''
                    memmove(rx_buffer,rx_buffer + sent_bytes,read_available - sent_bytes);
                    self->idx = 0;
                    self->read_available -= sent_bytes;
                }
                self->rx_errors &= ~pi_protocol::rx_error::BUFFER_FULL;
                if(self->rx_errors & pi_protocol::rx_error::RX_TIMEOUT){
                    self->rx_errors &= ~pi_protocol::rx_error::RX_TIMEOUT;
                    self->rx_wait_start_time_us = 0;
                    self->idx = 0;
                    self->read_available = 0;
                    self->found = false;
                    self->confirm = false;
                    break;
                }
                '''cdc 가 full밑에있으면 처리 못할수도'''
                if(self->rx_errors & pi_protocol::rx_error::CDC_ERR){
                    txpacket_t txpacket;
                    const char *err_name = esp_err_to_name(ret);
                    txpacket.data_len = std::min(strlen(err_name), sizeof(txpacket.data) - 1);
                    txpacket.id = warning_packet_id++;
                    txpacket.inst = ??; // CDC_ERR
                    txpacket.err = self->rx_errors;
                    memcpy(txpacket.data, err_name, len);
                    if (xQueueSend(self->tx_queue, &txpacket, pdMS_TO_TICKS(10)) != pdTRUE) {
                        // Do not reset the queue, Preserve the unsent debug bytes and retry later.
                    }'''지금은 tx큐 가득차는 경우없다고 가정.'''
                    self->rx_errors &= ~pi_protocol::rx_error::CDC_ERR;
                    break;
                }
            }
                
            if(result == pi_protocol::rx_result::WAITING){
                break; // cdc callback의 notify받도록 break
            }
        }
    }
}

'''15일 이후. rx_packet은 타임아웃 처리를 내부에 넣고, 함수 호출시 하나의 패킷을 파싱하고 result도출하게 변경하기. 즉 tiemout전까지 함수 내부에서 sleep_loop돌기.'''
pi_protocol::rx_result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    pi_protocol::rx_result result = pi_protocol::rx_result::NOTHING;
    while(true){
        std::size_t rx_size = 0;
        const esp_err_t ret = tinyusb_cdcacm_read(itf, &rx_buffer[read_available], rx_buffer_size - read_available, &rx_size);
        '''for safe extra iteration, dont clear notify-value.'''
        if (ret != ESP_OK){
            result = pi_protocol::rx_result::CDC_ERR;
        }
        if(rx_size != 0){
            read_available += rx_size;
        }

        bool ready = (!found &&            read_av >= idx + pi_protocol::HEADER_LEN) || // !found일때 header 읽기 가능?
                     (found && !confirm && read_av >= idx + pi_protocol::HEADER_LEN + 3) || // found일때 len,id,inst 필드 읽기 가능?
                     (confirm &&           read_av >= idx + rx_buffer[idx + pi_protocol::RX_LENGTH_IDX]); // confirm일때 packet 전체 읽기 가능?
        
        if (rx_size == 0 && !ready){
            BaseType_t notified = xTaskNotifyWait(0,0xFFFFFFFF,&notify_value,pdMS_TO_TICKS(static_cast<uint32_t>(pi_protocol::RX_TIMEOUT_MS)));
            if (notified == pdFALSE)
                result = pi_protocol::rx_result::RX_TIMEOUT;
        }

        }

        if(rx_size == 0 && !ready){
            result = pi_protocol::rx_result::WAITING;
            if(read_available != 0 && rx_wait_start_time_us == 0)
                rx_wait_start_time_us = esp_timer_get_time();
        }

        if (ready){
            if(!found){
                if (read_available >= idx + pi_protocol::HEADER_LEN){ // header 필드 읽기 가능
                    while (idx <= read_available - pi_protocol::HEADER_LEN){// limit까지만 헤더 확인 가능
                        uint8_t *p = (uint8_t *)memchr(&rx_buffer[idx], 0xFF, (size_t)(read_available - header_len - idx + 1));
                        if (p != nullptr){
                            idx = (uint16_t)(p - rx_buffer);
                            if ((rx_buffer[idx + 1] == 0xFF) &&(rx_buffer[idx + 2] == 0xFD) &&(rx_buffer[idx + 3] == 0)){
                                found = true;
                                break;
                            }
                        }
                        else{// 남은 구간에 0xFF 없으므로 더 볼 필요 없음
                            idx = read_available - pi_protocol::HEADER_LEN + 1;
                            break;
                        }
                        idx += 1;
                    }
                }
            }
            
            if(found){
                if (read_available >= idx + pi_protocol::HEADER_LEN + 3){ //len,id,inst 필드 읽기 가능?
                    if(rx_buffer[idx + pi_protocol::RX_LENGTH_IDX] <= pi_protocol::RXPACKET_MAX_LEN  &&
                    rx_buffer[idx + pi_protocol::RX_LENGTH_IDX] >= pi_protocol::RXPACKET_MIN_LEN &&
                    rx_buffer[idx + pi_protocol::RX_ID_IDX] > prev_id && '''wrap-around주의'''
                    rx_buffer[idx + pi_protocol::RX_INSTRUCTION_IDX] != ??){// len,id,inst 필드 정상
                        confirm = true;
                        uint8_t packet_len = rx_buffer[idx + pi_protocol::RX_LENGTH_IDX];
                        if(read_available >= idx + packet_len){ // data 읽기 가능
                            // CRC(little endian L,H)
                            uint16_t crc = static_cast<uint16_t>(rx_buffer[idx + packet_len-2]) | // L byte
                                            (static_cast<uint16_t>(rx_buffer[idx + packet_len-1]) << 8); // H byte
                            uint16_t calculated_crc = updateCRC(0, &rx_buffer[idx], packet_len-2); //without crc 2 byte
                            if(crc == calculated_crc){// (FF FF FD 00 LEN ID INST DATA CRC_L CRC_H)
                                result = pi_protocol::rx_result::SUCCESS; 
                            }
                            else:{
                                result = pi_protocol::rx_result::FAILURE;
                            }
                            found = false;
                            confirm = false;
                            rx_wait_start_time_us = 0;
                        }
                    }
                    else{
                        found = false;
                        confirm = false;
                        idx += pi_protocol::HEADER_LEN;
                    }
                }
            }
        }
            
        if(read_available == sizeof(pi_protocol::rx_buffer)){
            rx_errors |= pi_protocol::rx_error::BUFFER_FULL;
        }
        if(result != pi_protocol::rx_result::NOTHING || rx_errors != 0)
            break;
    }
    return result;
}



