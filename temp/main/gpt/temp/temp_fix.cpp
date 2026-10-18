'''18일 이후 task의 각 result별 break,continue여부 설정필요. substract참고'''
void pi_comm::rx_task(void *arg){
    pi_comm *self = static_cast<pi_comm *>(arg);
    while(true){
        BaseType_t notified = xTaskNotifyWait(0,0xFFFFFFFF,&notify_value,portMAX_DELAY);
        '''
        아래 rx_packet함수가 진행중일때 callback에서 notify가 오고, 
        해당 바이트가 다 처리되어 cdc의 rx_transfer버퍼가 비어있을수 있다.
        그러나 notify_value는 task에서 처음 받아올때만 초기화 시켜서 나중에 다시 돌아왔을때, 
        cdc의 rx_transfer버퍼가 비어있음에도 flag가 true일수 있다.
        따라서 cdc_read할때마다 buffer가 full이 아니라면 notify_value를 초기화하고,
        full이라면 notify_value를 초기화 하면 안된다.-->value를 직접 컨트롤은 위험함으로 다른방법알아보기.
        '''
        if (notified == pdFALSE){
            '''
            FreeRTOSConfig.h 에서 INCLUDE_vTaskSuspend는 무한대기 api를 지원하는 여부
            만약 false이면 최대 틱만큼 wait하고 pdFALSE반납하게 됨. 그럴경우 다시 task loop돌게 해야함.
            '''
            continue;
        }
        while (true){
            pi_protocol::rx_result result = self->rx_packet(rxpacket;);

            // connect byte debug stream.
            if (self->buffer_full || result == pi_protocol::rx_result::SUCCESS){
                '''
                지금 처리
                success: 성공패킷을 보내고 버퍼를 당기면 더 이상 바이트 stream이 연결안됨. 성공패킷 이전(0~idx)보내고 당기자.
                corrupt: 버퍼에 더 이상 공간 부족하니 확인했던 손실바이트들(0~idx) 보내자.
                나중에 처리하게 될 것
                failure: 버퍼에 아직 더 확인할게 있어서 끝까지 확인하고 문제되면 그때 처리.
                timeout: 지금은 끊겼지만 나중에 들어오는 스트림과 연결될수 있으니 지금은 처리 x.
                처리 필요 x
                cdc_err: 바이트스트림에 관한 문제가 아니니 따로 debug 보내지 않고 경고만 보낸다.
                '''
                size_t sent_bytes = 0;
                while (sent_bytes < self->idx)
                {
                    pi_protocol::txpacket_t txpacket{};
                    txpacket.data_len = std::min(self->idx - sent_bytes,sizeof(txpacket.data));
                    txpacket.id = self->debug_packet_id++;
                    txpacket.inst = ??; // DEBUG_BYTES
                    txpacket.err = ??;

                    memcpy(txpacket.data,self->rx_buffer + sent_bytes,txpacket.data_len);
                    if (xQueueSend(self->tx_queue, &txpacket, pdMS_TO_TICKS(10)) != pdTRUE) {
                        // Do not reset the queue, Preserve the unsent debug bytes and retry later.
                    }'''지금은 tx 큐 가득차는 경우 없어서 send 실패없다고 가정.'''
                    sent_bytes += txpacket.data_len;
                }
                
                if(result == pi_protocol::rx_result::SUCCESS) // 다음부터는 byte stream이 의미적으로 연결안됨.
                    self->debug_packet_id = 0;
                
                if (sent_bytes > 0){ '''tx-debug 실패 없어다고 가정해서 memmove'''
                    memmove(self->rx_buffer,self->rx_buffer + sent_bytes,self->read_available - sent_bytes);
                    self->idx = 0;
                    self->read_available -= sent_bytes;
                }
            }
            
            if(result == pi_protocol::rx_result::SUCCESS){
                pi_protocol::rxpacket_t rxpacket;
                uint8_t packet_len = rx_buffer[idx + pi_protocol::RX_LENGTH_IDX];
                rxpacket.data_len = packet_len - 9; // rxpacket_len = DATA(N) + 9 
                rxpacket.id = rx_buffer[idx + pi_protocol::RX_ID_IDX];
                rxpacket.inst = rx_buffer[idx + pi_protocol::RX_INSTRUCTION_IDX];
                memcpy(rxpacket.data,&rx_buffer[idx+pi_protocol::RX_DATA_IDX],rxpacket.data_len);
                unstuffing(rxpacket.data, &rxpacket.data_len);

                memmove(&rx_buffer[idx],rx_buffer + idx + packet_len,read_available - (idx + packet_len));
                self->read_available -= packet_len;

                if (xQueueSend(self->rx_queue, &rxpacket, 0) != pdTRUE){
                    '''지금은 rx 큐 가득차는 경우 없어서 send 실패없다고 가정.'''
                }
                '''
                1) 정확하게 정상패킷만 온경우:
                    만약 break하고 task wait확인한다면?
                    if 바로 task가 wait 반환 (이전flag때문일수도)
                        if rx_size =0 & !ready
                            if wait 하다가 timeout
                                if read_av =0
                                    result = wait후 task while break
                        if rx_size !=0 
                            정상작동
                    if task가 wait반환 안하면 그냥 그대로 무한대기시키기
                    만약 break없이 함수 바로가면?
                    if rx_size =0 & !ready
                        if 함수가 wait 바로 반환(이전flag때문 일수도...? 직접 읽어봐야함)
                            함수의 continue로 read다시 진행
                        if wait 하다가 timeout
                            if read_av =0
                                result = wait후 task while break
                    if rx_size !=0
                        정상작동
                2) 정상패킷뒤에 미확인 바이트가 있는 경우: 꼭 task의 wait확인 없이 바로 rx_packet실행
                    만약 break하고 task wait확인한다면?
                    if task가 wait바로 반환한다면?
                        if rx_size =0 & !ready
                            if wait 하다가 timeout
                                if read_av =0
                                    result = wait후 task while break
                        if rx_size !=0 
                            정상작동
                    if task가 wait반환 안하면 미확인 바이트 남게되어서 안됨.
                    만약 break없이 함수바로가면?
                    if rx_size =0 & !ready
                        if 함수가 wait 바로 반환(이전flag때문 일수도...? 직접 읽어봐야함)
                            함수의 continue로 read다시 진행
                        if wait 하다가 timeout
                            if read_av !=0
                                result = timeout
                            if read_av =0
                                result = wait후 task while break
                    if rx_size !=0
                        정상작동


                '''
                if (self->read_available == idx + self->packet_len){
                    
                    break;
                }
                else if(self->read_available > idx + self->packet_len){
                    continue;
                }
            }


            // send warning about strange case
            if(result == pi_protocol::rx_result::FAILURE){ // CRC_ERR, just check next
                self->idx += pi_protocol::HEADER_LEN; // header가 될수 없는 바이트 건너뛰기 
                
                txpacket_t txpacket;
                txpacket.data_len = ??; // n-bytes
                txpacket.id = self->warning_packet_id++;
                txpacket.inst = ??; // CRC_ERR
                txpacket.err = ??;
                memcpy(txpacket.data, "abcd...(n)", txpacket.data_len);
                if (xQueueSend(self->tx_queue, &txpacket, pdMS_TO_TICKS(10)) != pdTRUE) {
                    // Do not reset the queue, Preserve the unsent debug bytes and retry later.
                }'''지금은 tx 큐 가득차는 경우 없어서 send 실패없다고 가정.'''
                '''
                idx옮겼으니 wait확인없이 바로 rx_packet진행
                1) 만약 break하고 task의 wait확인한다면?
                    if wait가 바로 반환(이전flag때문일수도?)
                        if read=0 & !ready
                            if 함수가 wait 바로 반환(위에서 이전flag는 읽었으니, 바로 반환될리는 없는데?)
                                함수의 continue로 read다시 진행
                            if wait 하다가 timeout(위에서 이전flag는 읽었으니, 바이트 없는데 반환할 일 없음.)
                        if read !=0 
                            정상작동
                    if wait반환안하면? task의 wait에서 무한대기하며 기존 남은 바이트 확인 못함.
                2) 만약 break없이 바로 함수간다면?
                    if rx_size =0 & !ready
                        if 함수가 wait 바로 반환(이전flag때문 일수도...? 직접 읽어봐야함)
                            함수의 continue로 read다시 진행
                        if wait 하다가 timeout
                            if read_av !=0
                                result = timeout
                    if rx_size !=0
                        정상작동
                '''
            }
            if(result == pi_protocol::rx_result::TIMEOUT){
                txpacket_t txpacket;
                txpacket.data_len = ??; // n-bytes
                txpacket.id = self->warning_packet_id++;
                txpacket.inst = ??; // TIMEOUT_ERR
                txpacket.err = ??;
                memcpy(txpacket.data, "abcd...(n)", txpacket.data_len);
                if (xQueueSend(self->tx_queue, &txpacket, pdMS_TO_TICKS(10)) != pdTRUE) {
                    // Do not reset the queue, Preserve the unsent debug bytes and retry later.
                }'''지금은 tx 큐 가득차는 경우 없어서 send 실패없다고 가정.'''
                '''
                ================
                1) 만약 break하고 task의 wait확인한다면?
                    if wait가 바로 반환(이전flag때문일수도?)
                        if read=0 & !ready
                            if 함수가 wait 바로 반환(위에서 이전flag는 읽었으니, 바로 반환될리는 없는데?)
                                함수의 continue로 read다시 진행
                            if wait 하다가 timeout
                                if 이전 timeout이 발생했고, 바이트가 변화?
                                    result = timeout
                                if 이전이 timeout이 아니면
                                    result = timeout
                                if 이전이 timeout 발생했지만, 바이트가 변화안했다면?
                                    result = waiting, task break하고 task wait무한대기
                        if read !=0 
                            정상작동
                    if wait반환안하면? task의 wait에서 무한대기
                2) 만약 break없이 바로 함수간다면?
                    if rx_size =0 & !ready
                        if 함수가 wait 바로 반환(이전flag때문 일수도...? 직접 읽어봐야함)
                            함수의 continue로 read다시 진행
                        if wait 하다가 timeout
                            if 이전 timeout이 발생했고, 바이트가 변화?
                                result = timeout
                            if 이전이 timeout이 아니면
                                result = timeout
                            if 이전이 timeout 발생했지만, 바이트가 변화안했다면?
                                result = waiting, task break하고 task wait무한대기
                    if rx_size !=0
                        정상작동
                '''
                break;
            }
            if(result & pi_protocol::rx_error::CDC_ERR){
                '''ESP-IDF에 여러 에러값이 정의되어 있지만, 공식적 반환값은 ESP_OK, ESP_ERR_INVALID_STATE뿐
                게다가 esp의 모든 uart를 다 사용하고 있고 라즈베리파이와 esp가 유일하게 cdc로 연결된 상황에서 
                cdc_err이 발생한다면 err정보를 받을 수 없음. 단순히 재부팅 밖에.. 
                그래서 err발생해도 조치할 방법이 없으니, cdc통신부분만 코드로 검증하고, cdc_err는 발생하지 않는다고 가정한다.
                **ESP_ERR_INVALID_STATE의 원인**
                1) tinyusb_cdcacm_init()이 성공적으로 실행되지 않음
                2) 전달한 itf에 해당하는 CDC-ACM 인터페이스가 초기화되지 않음
                3) 해당 인터페이스가 해제되었거나 초기화 상태가 잘못됨'''
                txpacket_t txpacket;
                const char *err_name = esp_err_to_name(self->cdc_err);
                txpacket.data_len = std::min(strlen(err_name), sizeof(txpacket.data) - 1);
                txpacket.id = self->warning_packet_id++;
                txpacket.inst = ??; // CDC_ERR
                txpacket.err = ??; 
                memcpy(txpacket.data, err_name, txpacket.data_len);
                if (xQueueSend(self->tx_queue, &txpacket, pdMS_TO_TICKS(10)) != pdTRUE) {
                    // Do not reset the queue, Preserve the unsent debug bytes and retry later.
                }'''지금은 tx 큐 가득차는 경우 없어서 send 실패없다고 가정.'''
                '''
                ====
                1) 만약 break하고 task의 wait확인한다면?
                    if wait가 바로 반환(이전flag때문일수도?)
                        if read=0 & !ready
                            if 함수가 wait 바로 반환(위에서 이전flag는 읽었으니, 바로 반환될리는 없는데?)
                                함수의 continue로 read다시 진행
                            if wait 하다가 timeout
                                result = timeout
                        if read !=0 
                            정상작동
                        if cdc_err
                            연속cdc_err count 일정이상 넘으면 result = wait,break하고 task에서 무한대기
                    if wait반환안하면? task의 wait에서 무한대기

                2) 만약 break없이 바로 함수간다면?
                    if rx_size =0 & !ready
                        if 함수가 wait 바로 반환(이전flag때문 일수도...? 직접 읽어봐야함)
                            함수의 continue로 read다시 진행
                        if wait 하다가 timeout
                            result = timeout
                    if rx_size !=0
                        정상작동
                    if cdc_err
                            연속cdc_err count 일정이상 넘으면 result = wait,break하고 task에서 무한대기
                '''


            }

            // break처리
            '''
            success: read_avail보고 확인할게 남아있으면 packet함수 한번더 해야함.
            failure: crc_err로 idx를 조금 뒤로 넘겼으니, 추가 확인 필요해서 packet 함수 한번더 해야함.
            cdc_err: 발생안한다고 가정. or 재시도시 고쳐진다고 가정.  packet함수 한번더 해야함.
            timeout: 바이트가 더 필요한데 애초에 읽을게 없으니, break하고 notify기다려야함.
            '''
        }
    }
}

pi_protocol::rx_result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    pi_protocol::rx_result result = pi_protocol::rx_result::NOTHING;
    while(true){
        std::size_t rx_size = 0;
        cdc_err = tinyusb_cdcacm_read(itf, &rx_buffer[read_available], rx_buffer_size - read_available, &rx_size);
        '''for safe extra iteration, dont clear notify-value.'''
        if (cdc_err != ESP_OK){
            '''ESP-IDF에 여러 에러값이 정의되어 있지만, 공식적 반환값은 ESP_OK, ESP_ERR_INVALID_STATE뿐
            **ESP_ERR_INVALID_STATE의 원인**
            1) tinyusb_cdcacm_init()이 성공적으로 실행되지 않음
            2) 전달한 itf에 해당하는 CDC-ACM 인터페이스가 초기화되지 않음
            3) 해당 인터페이스가 해제되었거나 초기화 상태가 잘못됨'''
            result = pi_protocol::rx_result::CDC_ERR;
            break;
        }
        if(rx_size != 0){
            read_available += rx_size;
        }
        '''cdc_read를 while안에서 rx_size가 0이 될때까지 읽게 할까?'''

        
        bool ready = (!found &&            read_available >= idx + pi_protocol::HEADER_LEN) || // !found일때 header 읽기 가능?
                     (found && !confirm && read_available >= idx + pi_protocol::HEADER_LEN + 3) || // found일때 len,id,inst 필드 읽기 가능?
                     (confirm &&           read_available >= idx + rx_buffer[idx + pi_protocol::RX_LENGTH_IDX]); // confirm일때 packet 전체 읽기 가능?
        
        if (rx_size == 0 && !ready){
            BaseType_t notified = xTaskNotifyWait(0,0xFFFFFFFF,&notify_value,pdMS_TO_TICKS(static_cast<uint32_t>(pi_protocol::RX_TIMEOUT_MS)));
            if (notified == pdFALSE)
                result = pi_protocol::rx_result::RX_TIMEOUT;
                '''나중에 들어오는 바이트들과 연결될 수 있으니, idx,found,confirm등은 유지'''
            else if(notified == pdTRUE)
                continue;
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
                    if (rx_buffer[idx + pi_protocol::RX_LENGTH_IDX] <= pi_protocol::RXPACKET_MAX_LEN  &&
                        rx_buffer[idx + pi_protocol::RX_LENGTH_IDX] >= pi_protocol::RXPACKET_MIN_LEN &&
                        rx_buffer[idx + pi_protocol::RX_ID_IDX] > prev_id && '''wrap-around주의'''
                        rx_buffer[idx + pi_protocol::RX_INSTRUCTION_IDX] != ??){// len,id,inst 필드 정상
                        confirm = true;
                        uint8_t packet_len = rx_buffer[idx + pi_protocol::RX_LENGTH_IDX];
                        if(read_available >= idx + packet_len){ // data 읽기 가능
                            // CRC(little endian L,H)
                            uint16_t calculated_crc = updateCRC(0, &rx_buffer[idx], packet_len-2); //without crc 2 byte
                            uint16_t crc = static_cast<uint16_t>(rx_buffer[idx + packet_len-2]) | // L byte
                                          (static_cast<uint16_t>(rx_buffer[idx + packet_len-1]) << 8); // H byte
                            
                            if(crc == calculated_crc){
                                result = pi_protocol::rx_result::SUCCESS;
                            }
                            else{
                                result = pi_protocol::rx_result::FAILURE;
                            }
                            found = false;
                            confirm = false;
                        }
                    }
                    else{
                        found = false;
                        confirm = false;
                        idx += pi_protocol::HEADER_LEN; // header가 될수 없는 바이트 건너뛰기 
                    }
                }
            }
        }
            
        if(read_available == sizeof(rx_buffer)){
            buffer_full = true;
            '''나중에 들어오는 바이트들과 연결될 수 있으니, idx,found,confirm등은 유지'''
        }
        if(result != pi_protocol::rx_result::NOTHING)
            break;
    }
    return result;
}



