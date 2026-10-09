void pi_comm::rx_task(void *arg){
    pi_comm *self = static_cast<pi_comm *>(arg);
    pi_protocol::rxpacket_t rxpacket;
    while(true){
        TickType_t wait_ticks;
        if(self->rx_start_time_us != 0){
            const int64_t elapsed_us = esp_timer_get_time() - self->rx_start_time_us;
            if (elapsed_us > pi_protocol::RX_TIMEOUT_US)
                status.err |= pi_protocol::Comm_err::RX_TIMEOUT;
            else
                wait_ticks = pdMS_TO_TICKS(static_cast<uint32_t>((pi_protocol::RX_TIMEOUT_US - elapsed_us + 999) / 1000));
                if (wait_ticks == 0) // Avoid immediate return when tick resolution is coarse.
                    wait_ticks = 1;
        }
        else
            wait_ticks = portMAX_DELAY;

        if(!(status.err & pi_protocol::Comm_err::RX_TIMEOUT)){
            BaseType_t notified = xTaskNotifyWait(0,0xFFFFFFFF,&notify_value,wait_ticks);
            if (notified == pdFALSE && self->rx_start_time_us != 0)
                // status.err |= timeout;
        }
        while (true){
            if(!(status.err & pi_protocol::Comm_err::RX_TIMEOUT))
                Comm_Result result = self->rx_packet(rxpacket;);

            if(result = success)
                // 완료된 패킷저장 -> process에서 해도될듯?
                if (xQueueSend(self->rx_queue, &self->rxpacket, 0) != pdTRUE){
                    xQueueReset(self->rx_queue);
                    xQueueSend(self->rx_queue, &self->rxpacket, 0);
                }
                // idx, read_av 완료된 패킷만 비우고 당기기 -> process에서 해도될듯?

                size_t sent_bytes = 0;
                while (sent_bytes < idx) // 0~idx전까지 debug용 바이트스트림 연결시키기.
                {
                    txpacket_t txpacket{};
                    txpacket.data_len = std::min(debug_len - sent_bytes,sizeof(txpacket.data));
                    txpacket.id = debug_packet_id;
                    txpacket.inst = /* debug */;
                    txpacket.err = self->status.errors;

                    memcpy(txpacket.data,rx_buffer + sent_bytes,txpacket.data_len);
                    if (xQueueSend(self->tx_queue, &txpacket, 0) != pdTRUE){
                        // break; TX_QUEUE_FULL: preserve unsent bytes
                    }
                    sent_bytes += txpacket.data_len;
                    debug_packet_id++;
                }
                if (sent_bytes > 0){
                    memmove(rx_buffer,rx_buffer + sent_bytes,read_available - sent_bytes);
                    idx -= sent_bytes;
                    read_available -= sent_bytes;
                }

            if(result = failure) // crc_err
                txpacket_t txpacket;
                txpacket.data_len = packet_len;
                txpacket.id = debug_packet_id++;
                txpacket.inst = ??; //debug
                txpacket.err = status.errors;
                memcpy(txpacket.data,&rx_buffer[idx],txpacket.data_len);
                if(xQueueSend(self->tx_queue,&self->txpacket,0) != pdTRUE){
                    // break; TX_QUEUE_FULL: preserve unsent bytes
                }

                idx += header_len;
                status.err &= ~pi_protocol::Comm_Error::CRC_ERR;

            if(status.err & full)
                txpacket_t txpacket;
                txpacket.data_len = std::min(idx,sizeof(txpacket.data));
                txpacket.id = debug_packet_id++;
                txpacket.inst = ??; //debug
                txpacket.err = status.errors;
                memcpy(txpacket.data,rx_buffer,txpacket.data_len);
                if(xQueueSend(self->tx_queue,&self->txpacket,0) != pdTRUE){
                    // break; TX_QUEUE_FULL: preserve unsent bytes
                }
                status.err &= ~pi_protocol::Comm_Error::BUFFER_FULL;
                
            if(status.err != 0)
                if(status.err & timeout)
                    
                    txpacket_t txpacket;
                    txpacket.data_len = 2;
                    txpacket.id = warning_packet_id++;
                    txpacket.inst = ??; // timeout warning
                    txpacket.err = status.errors;
                    txpacket.data = [??,??];
                    if(xQueueSend(self->tx_queue,&self->txpacket,0) != pdTRUE){
                        // break; TX_QUEUE_FULL: preserve unsent bytes
                    }
                    // idx,found,confirm,start,read_av초기화
                    idx = 0;
                    found = false;
                    confirm = false;
                    rx_start_time_us = 0;
                    read_available = 0;
                    status.err &= ~pi_protocol::Comm_Error::RX_TIMEOUT;

                if(status.err & cdc_err)
                    txpacket_t txpacket;
                    txpacket.data_len = 2;
                    txpacket.id = warning_packet_id++;
                    txpacket.inst = ??; // USB connect warning
                    txpacket.err = status.errors;
                    txpacket.data = [??,??];
                    if(xQueueSend(self->tx_queue,&self->txpacket,0) != pdTRUE){
                        // break; TX_QUEUE_FULL: preserve unsent bytes
                    }
                    status.err &= ~pi_protocol::Comm_Error::CDC_ERR;
            
            if(result = pending ||'''에러일때 break해야하는데''')
                break; // cdc callback의 notify받도록 break
        }
    }
}


//26-10-08
task(){
    while(true){
        if(rx_start_time!=0)
            if (elapsed_time >= timeout)
                status.err |= timeout;
            else
                wait_time = timeout - elapsed_time;
        else
            wait_time = max;

        if(!(status.err & timeout))
            notify = wait(wait_time)
            if (!notify)
                // status.err |= timeout;

        while (true){
            if(!(status.err & timeout))
                result = process();

            if(result = success)
                // 완료된 패킷저장
                if(rxpacket_queue send != True)
                    // 이전 rxpacket 큐 reset
                    // 완성패킷 큐send 
                // idx, read_av 완료된 패킷만 비우고 당기기
                if(read_av > idx)
                    // one_more_check = true;버퍼의 미확인데이터 한번더 보게끔
                if(idx!=0)
                    // 0~idx전까지 debug용 바이트스트림 연결시키기.
                // found, start 초기화;

            if(result = pending)
                // cdc callback의 notify받도록 break
                break;

            if(result = failure) // crc_err
                // 패킷이라 판단했던 바이트 보내기
                // idx += header_len;
                // one_more_check = true;

            if(status.err & full)
                // 0 ~ idx까지 debug 바이트 최대한 보내기.
                // status.err &= ~full;

            if(status.err != 0)
                if(status.err & timeout)
                    // timeout정보 보내기
                    // idx,found,start,read_av초기화
                    // status.err &= ~timeout;
                if(status.err & cdc_err)
                    // usb연결상태 경고보내기.
                    // status.err &= ~cdc_err;
        }
    }
}