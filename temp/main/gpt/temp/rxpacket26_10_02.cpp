//substract



void pi_comm::rx_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    pi_protocol::rxpacket_t rxpacket;
    constexpr int64_t RX_TIMEOUT_US = static_cast<int64_t>(RX_TIMEOUT_MS) * 1000;
    
    while (true)
    {
        TickType_t wait_ticks;

        if (self->rx_parse_start_time_us != 0){
            const int64_t elapsed_us = esp_timer_get_time() - self->rx_parse_start_time_us;
            if (elapsed_us >= RX_TIMEOUT_US)
            {
                self->rx_parse_start_time_us = 0;
                read_available = 0;
                idx = 0;
                found = false;
                // RX timeout 처리
                self->tx_packet();
                continue;
            }
            const int64_t remain_us = RX_TIMEOUT_US - elapsed_us;
            wait_ticks = pdMS_TO_TICKS(static_cast<uint32_t>((remain_us + 999) / 1000));
            if (wait_ticks == 0) // Avoid immediate return when tick resolution is coarse.
                wait_ticks = 1;
        else
            wait_ticks = portMAX_DELAY;
        }
        BaseType_t notified = xTaskNotifyWait(0,0xFFFFFFFF,&notify_value,wait_ticks);
        if (notified == pdFALSE && self->rx_parse_start_time_us != 0) // RX_timeout
        {
            self->rx_parse_start_time_us = 0;
            self->tx_packet();
            continue;
        }

        while (true)
        {
            Comm_Result result = self->rx_packet(rxpacket;);
            switch (result)
            {
                case Comm_Result::SUCCESS:
                    if (xQueueSend(self->rx_queue,&self->rxpacket,0) != pdTRUE)
                        // BUF_NUM_OVER
                    continue;
                case Comm_Result::NO_DATA:
                case Comm_Result::NEED_MORE_DATA:
                    break;
                case Comm_Result::BUFFER_FULL:

                    tx_err(); //
                    idx = 0;
                    found = false;
                    break;
            }
            break;
        }
    }
}


// 26-10-03.buff full시 tx_debug보내고 보낸만큼 memmove하기
//rx_task랑 rx_packet set로 만드는중.
'''26-10-03이후 여기서 부터 수정필요.'''
void pi_comm::rx_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    pi_protocol::rxpacket_t rxpacket;
    
    while (true)
    {
        TickType_t wait_ticks;

        if (self->rx_parse_start_time_us != 0){
            const int64_t elapsed_us = esp_timer_get_time() - self->rx_parse_start_time_us;
            if (elapsed_us >= pi_protocol::RX_TIMEOUT_US)
            {
                ''' 한번더 확인할 기회주기.'''
                self->rx_parse_start_time_us = 0;
                send_len = min(read_available,pi_protocol::RXPACKET_MAX_LEN);

                txpacket_t txpacket;
                txpacket.data_len = send_len;
                txpacket.id = ;
                txpacket.inst = debug;
                txpacket.err = timeout;
                memcpy(txpacket.data,&rx_buffer[read_available - send_len],send_len);

                idx = 0;
                found = false;
                read_available = 0;
                if(xQueueSend(self->tx_queue,&self->txpacket,0) != pdTRUE){
                    // TXQUEUE FULL;
                }
                continue;
            }
            wait_ticks = pdMS_TO_TICKS(static_cast<uint32_t>((pi_protocol::RX_TIMEOUT_US - elapsed_us; + 999) / 1000));
            if (wait_ticks == 0) // Avoid immediate return when tick resolution is coarse.
                wait_ticks = 1;
        else:
            wait_ticks = portMAX_DELAY;
        }
        BaseType_t notified = xTaskNotifyWait(0,0xFFFFFFFF,&notify_value,wait_ticks);
        if (notified == pdFALSE && self->rx_parse_start_time_us != 0)
            continue; // 위에서 timeout check

        while (true)
        {
            Comm_Result result = self->rx_packet(rxpacket;);
            switch (result)
            {
                case Comm_Result::SUCCESS:
                    if (xQueueSend(self->rx_queue,&self->rxpacket,0) != pdTRUE)
                        // RX_QUEUE FULLl, 가장 최근 하나만 남기기.
                    continue;
                case Comm_Result::NO_DATA:
                case Comm_Result::NEED_MORE_DATA:
                    break;
                case Comm_Result::CDC_ERR:
                    txpacket_t txpacket;
                    txpacket.data_len = send_len;
                    txpacket.id = ;
                    txpacket.inst = debug;
                    txpacket.err = CDC_ERR;
                    read_available -= debug_len;
                    idx -= debug_len;
                    found = false;

                    if(xQueueSend(self->rx_queue,&self->rxpacket,0) != pdTRUE){
                        // TX_QUEUE FULL;
                    }
                    break;
                case Comm_Result::BUFFER_FULL:
                    txpacket_t txpacket;
                    txpacket.data_len = send_len;
                    txpacket.id = ;
                    txpacket.inst = debug;
                    txpacket.err = BUFFER_FULL;
                    memcpy(txpacket.data,rx_buffer,send_len);
                    memmove(rx_buffer,rx_buffer + send_len,read_available - send_len);
                    read_available -= send_len;
                    idx -=send_len;
                    found = false;

                    if(xQueueSend(self->rx_queue,&self->rxpacket,0) != pdTRUE){
                        // TX_QUEUE FULL;
                    }
            }
            break;
        }
    }
}
pi_protocol::Comm_Result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    if (rx_parse_start_time_us == 0)
        rx_parse_start_time_us = esp_timer_get_time();

    while(true)
    {   
        if(rx_buffer_size > read_available){
            std::size_t rx_size = 0;
            const esp_err_t ret = tinyusb_cdcacm_read(itf, &rx_buffer[read_available], rx_buffer_size - read_available, &rx_size);
            if (ret != ESP_OK){
                result = Comm_Result::CDC_ERR;
                break;
            }
            if (rx_size == 0){
                result = Comm_Result::NEED_MORE_DATA;
                break;
            }
            read_available += rx_size;
        }
        else:{
            break;
        }

        if(!found){
            if (read_available < header_len){
                limit_idx = read_available - header_len;
                while (idx <= limit_idx) // limit까지만 헤더 확인 가능
                {
                    uint8_t *p = (uint8_t *)memchr(&rx_buffer[idx], 0xFF, (size_t)(limit_idx - idx + 1)); // memchr(시작주소, 찾을값, 검색할바이트수);
                    if (p == nullptr){
                        idx = limit_idx;   // 남은 구간에 0xFF 없으므로 더 볼 필요 없음
                        if(read_available < rx_buffer_size)
                            result = Comm_Result::NEED_MORE_DATA;
                        else:
                            result = Comm_Result::BUFFER_FULL;
                            send_len = pi_protocol::RXPACKET_MAX_LEN;
                        break;
                    }
                    idx = (uint16_t)(p - rx_buffer);
                    if ((rx_buffer[idx + 1] == 0xFF) &&(rx_buffer[idx + 2] == 0xFD) &&(rx_buffer[idx + 3] == 0)){
                        found = true;
                        break;
                    }
                    idx += 1;
                }
            }
        }
        if(found)
        {
            if(read_available > idx + header_len + 3){ // len,id,inst 필드 읽기 가능
                if(rx_buffer[idx + pi_protocol::PKT_LENGTH] > pi_protocol::RXPACKET_MAX_LEN  || // 잘못된 LEN 
                    rx_buffer[idx + pi_protocol::PKT_LENGTH] < pi_protocol::RXPACKET_MIN_LEN || // 잘못된 LEN
                    rx_buffer[idx + pi_protocol::PKT_ID] <= prev_id || // 잘못된 id
                    rx_buffer[idx + pi_protocol::PKT_INSTRUCTION] = ?? // 잘못된 inst
                ){
                    idx += HEADER_LEN; // 헤더일 수 없는 바이트 건너뛰기
                    found = false;
                    continue;
                }
            
                const uint16_t packet_len = rx_buffer[idx + pi_protocol::PKT_LENGTH];
                if(idx + packet_len <= read_available){ // packet_len만큼 읽기 가능여부
                    // CRC(little endian L,H)
                    uint16_t crc = static_cast<uint16_t>(rx_buffer[idx + packet_len-2]) | // L byte
                                    (static_cast<uint16_t>(rx_buffer[idx + packet_len-1]) << 8); // H byte
                    uint16_t calculated_crc = updateCRC(0, &rx_buffer[idx], packet_len-2); //without crc 2 byte
                    if(crc == calculated_crc){
                        // (FF FF FD 00 LEN ID INST DATA CRC_L CRC_H)
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
                        break;
                    }
                    else:{
                        debug_len = min(idx+packet_len,pi_protocol::RXPACKET_MAX_LEN);
                        memcpy(debug_data,&rx_buffer[idx+packet_len-debug_len],debug_len); 
                        '''그냥 바로 debug_data말고 txpacket.data에 복사?'''
                        // 손실패킷내 추가적 패킷 점검을 위해 memmove는 하지 않는다.
                        result = pi_protocol::Comm_Result::CRC_ERR;
                        break;
                        // idx += HEADER_LEN; // 헤더일 수 없는 바이트 건너뛰기
                        // found = false;
                        // continue;
                    }
                }
            }
            // 정상적이라고 가정되는 packet_len이 덜받아짐.
            if(read_available == rx_buffer_size){
                send_len = min(idx,pi_protocol::RXPACKET_MAX_LEN);
                result = pi_protocol::Comm_Result::BUFFER_FULL;
                break;
            }
            continue;
        }
    }
    return result;
}




// 26-10-03.buff full시 packet_max_len만큼 남기고 memmove하기
pi_protocol::Comm_Result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    while(true)
    {   
        if(rx_buffer_size > read_available){
            std::size_t rx_size = 0;
            const esp_err_t ret = tinyusb_cdcacm_read(itf, &rx_buffer[read_available], rx_buffer_size - read_available, &rx_size);
            if (ret != ESP_OK)
                result = Comm_Result::CDC_ERR;
                break;
            if (rx_size == 0)
                result = Comm_Result::NEED_MORE_DATA;
                break;
            read_available += rx_size;
        }
        else:{
            break;
        }

        if(!found)
        {
            limit_idx = read_available - header_len;
            while (idx <= limit_idx) // limit이전까지만 헤더 확인 가능'
            {
                uint8_t *p = (uint8_t *)memchr(&rx_buffer[idx], 0xFF, (size_t)(limit_idx - idx + 1)); // memchr(시작주소, 찾을값, 검색할바이트수);
                if (p == nullptr)
                {
                    idx = limit_idx;   // 남은 구간에 0xFF 없으므로 더 볼 필요 없음
                    if(idx = rx_buffer_size - header_len)
                        //디버그용으로 프로토콜상 보낼수있는 최대만큼 남겨두고 memmove로 버퍼 비워주기.
                    result = Comm_Result::NEED_MORE_DATA;
                    else:
                        // result = Comm_Result::BUFFER_FULL;
                    break;
                }
                idx = (uint16_t)(p - rx_buffer);
                if ((rx_buffer[idx + 1] == 0xFF) &&(rx_buffer[idx + 2] == 0xFD) &&(rx_buffer[idx + 3] == 0))
                {
                    found = true;
                    break;
                }
                idx += 1;
            }
        }
        if(found)
        {
            if(idx + header_len + 3 <= read_available) // len,id,inst 필드 읽기 가능
                if(rx_buffer[idx + pi_protocol::PKT_LENGTH] > pi_protocol::RXPACKET_MAX_LEN  || // 잘못된 LEN 
                    rx_buffer[idx + pi_protocol::PKT_LENGTH] < pi_protocol::RXPACKET_MIN_LEN || // 잘못된 LEN
                    rx_buffer[idx + pi_protocol::PKT_ID] <= prev_id || // 잘못된 id
                    rx_buffer[idx + pi_protocol::PKT_INSTRUCTION] = ?? // 잘못된 inst
                ){
                    idx += HEADER_LEN; // 헤더일 수 없는 바이트 건너뛰기
                    found = false;
                    continue;
                }
            else:
                continue;
            //정상적이라고 가정되는 header,len,id,inst 필드에 대해 data읽고 crc확인
            const uint16_t packet_len = rx_buffer[idx + pi_protocol::PKT_LENGTH];
            if(idx + packet_len <= read_available) // packet_len만큼 읽기 가능여부
            {
                // CRC(little endian L,H)
                uint16_t crc = static_cast<uint16_t>(rx_buffer[packet_len-2]) | // L byte
                                (static_cast<uint16_t>(rx_buffer[packet_len-1]) << 8); // H byte
                uint16_t calculated_crc = updateCRC(0, &rx_buffer[idx], packet_len-2); //without crc 2 byte

                if(crc == calculated_crc){
                    // (FF FF FD 00 LEN ID INST DATA CRC_L CRC_H)
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
                    break;
                }
                else:{
                    idx += HEADER_LEN; // 헤더일 수 없는 바이트 건너뛰기
                    found = false;
                    continue;
                }
            }
            else:{ // 정상적이라고 가정되는 packet_len이 덜받아짐.
                if(read_available == rx_buffer_size)
                    //디버그용으로 프로토콜상 보낼수있는 최대만큼 남겨두고 memmove로 버퍼 비워주기.
                continue;
            }
        }
    }
    return result;
}

// 26-10-03임시저장.
pi_protocol::Comm_Result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    while(true)
    {   
        if(rx_buffer_size > read_available){
            std::size_t rx_size = 0;
            const esp_err_t ret = tinyusb_cdcacm_read(itf, &rx_buffer, rx_buffer_size - read_available, &rx_size);
            if (ret != ESP_OK)
                result = Comm_Result::CDC_ERR;
                break;
            if (rx_size == 0)
                result = Comm_Result::NEED_MORE_DATA;
                break;
            read_available += rx_size;
        }
        else:{
            break;
        }

        if(!found)
        {
            limit_idx = read_available - header_len;
            while (idx <= limit_idx) // limit이전까지만 헤더 확인 가능'
            {
                uint8_t *p = (uint8_t *)memchr(&rx_buffer[idx], 0xFF, (size_t)(limit_idx - idx + 1)); // memchr(시작주소, 찾을값, 검색할바이트수);
                if (p == nullptr)
                {
                    idx = limit_idx;   // 남은 구간에 0xFF 없으므로 더 볼 필요 없음
                    if(idx = rx_buffer_size - header_len)
                        //디버그용으로 프로토콜상 보낼수있는 최대만큼 남겨두고 memmove로 버퍼 비워주기.
                    result = Comm_Result::NEED_MORE_DATA;
                    else:
                        // result = Comm_Result::BUFFER_FULL;
                    break;
                }
                idx = (uint16_t)(p - rx_buffer);
                if ((rx_buffer[idx + 1] == 0xFF) &&(rx_buffer[idx + 2] == 0xFD) &&(rx_buffer[idx + 3] == 0))
                {
                    found = true;
                    break;
                }
                idx += 1;
            }
        }
        if(found)
        {
            if(idx + header_len + 3 <= read_available) // len,id,inst 필드 읽기 가능
                if(rx_buffer[idx + pi_protocol::PKT_LENGTH] > pi_protocol::RXPACKET_MAX_LEN  || // 잘못된 LEN 
                    rx_buffer[idx + pi_protocol::PKT_LENGTH] < pi_protocol::RXPACKET_MIN_LEN || // 잘못된 LEN
                    rx_buffer[idx + pi_protocol::PKT_ID] <= prev_id || // 잘못된 id
                    rx_buffer[idx + pi_protocol::PKT_INSTRUCTION] = ?? // 잘못된 inst
                ){
                    idx += HEADER_LEN; // 헤더일 수 없는 바이트 건너뛰기
                    found = false;
                    continue;
                }
            else:
                continue;
            //정상적이라고 가정되는 header,len,id,inst 필드에 대해 data읽고 crc확인
            const uint16_t packet_len = rx_buffer[idx + pi_protocol::PKT_LENGTH];
            if(idx + packet_len <= read_available) // packet_len만큼 읽기 가능여부
            {
                // CRC(little endian L,H)
                uint16_t crc = static_cast<uint16_t>(rx_buffer[packet_len-2]) | // L byte
                                (static_cast<uint16_t>(rx_buffer[packet_len-1]) << 8); // H byte
                uint16_t calculated_crc = updateCRC(0, &rx_buffer[idx], packet_len-2); //without crc 2 byte

                if(crc == calculated_crc){
                    // (FF FF FD 00 LEN ID INST DATA CRC_L CRC_H)
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
                    break;
                }
                else:{
                    idx += HEADER_LEN; // 헤더일 수 없는 바이트 건너뛰기
                    found = false;
                    continue;
                }
            }
            else:{ // 정상적이라고 가정되는 packet_len이 덜받아짐.
                if(read_available == rx_buffer_size)
                    //디버그용으로 프로토콜상 보낼수있는 최대만큼 남겨두고 memmove로 버퍼 비워주기.
                continue;
            }
        }
    }
    return result;
}


//26-10-02 링버퍼기반작동 rx_packet. task가 아닌 함수내에서 링버퍼 등록 및 수정함. but 링버퍼대문에 너무복잡해짐.
pi_protocol::Comm_Result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    while(true)
    {
        while (true) // rx_buffer에 저장가능한 만큼 cdc rx transfer읽어서 저장.
        {
            uint32_t cdc_available = tud_cdc_n_available(itf);
            if (cdc_available == 0)
            {
                if(rx_buffer.available()) //링버퍼는 읽을거 있는데, write할거 없다.
                    break; 
                else:
                    return Comm_Result::NO_DATA; //가능한 경우인가?
            }

            uint8_t* write_ptr = nullptr;
            std::size_t write_len = 0;
            if (rx_buffer.get_write_ptr(write_ptr, cdc_available, write_len))
            {
                std::size_t rx_size = 0;
                const esp_err_t ret = tinyusb_cdcacm_read(itf, write_ptr, write_len, &rx_size);
                if (ret != ESP_OK)
                    return Comm_Result::CDC_ERR;
                if (rx_size == 0)
                    return Comm_Result::NO_DATA;
                rx_buffer.commit_write(rx_size);
            }
            else:
                break; // 링버퍼에 쓸 수 있는건 다 썼으니 아래에서 소비해줘야함.->사실 완성전까지 소비안함.
            //     return Comm_Result::BUF_LEN_OVER;
        }

        while(rx_buffer.available() > 0)
        {   
            std::size_t header_idx;
            if (rx_buffer.find(header, header_len, 0, header_idx))
            {
                //header_idx부터 min length만큼 버퍼가 
                if(rx_buffer.peek(header_idx + header_len,packet_len)) //len field 읽기가능?
                    if(packet_len <= pi_protocol::RXPACKET_MAX_LEN && packet_len >= pi_protocol::RXPACKET_MIN_LEN)
                        if(rx_buffer.available() >= header_idx + packet_len) // data field 읽기 가능?
                            rx_buffer.peek(header_idx,packet_len,)
                        else:
                    else:
                        // len 오류
                else:
                    if (rx_buffer.full())
                    {
                        min_length만큼 read해서 저장공간 늘려줌
                        result = Comm_Result::NEED_MORE_DATA;
                    }
                    else:   
            }
            else: // NOT FOUND
            {   
                if (rx_buffer.full())
                    //header-1만큼 바이트 남겨두고 다 읽어서 저장공간늘려줌.
                    //읽은 바이트는 debug용으로 보냄.
                break;
            }
            else:

            rx_buffer.read(packet, packet_length);// 패킷완성후 읽기
        }
        return result;
    }
}

// esp-pi
pi_protocol::Comm_Result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    (void)event;
    const uint16_t min_length       = 11;   // temp버퍼의 프로토콜 구조상 될 수 있는 최소 길이
    const uint16_t max_length = 255;    // temp버퍼의 최대 길이
    uint16_t real_len = 0;              // packet의 실제 길이
    uint8_t temp[max_length]  = {};  // rx패킷을 찾기전에 잠시 저장하는 공간.

    size_t rx_size            = 0;     // cdc로 이번에 실제로 읽은 바이트 수
    uint16_t rx_length        = 0;    // 현재 temp버퍼 바이트 수
    uint16_t wait_length      = min_length;   // 지금 기다리는 전체 패킷 길이 (최소 상태패킷 길이로 시작)

    uint16_t idx              = 0;    // 이번에 확인하기 시작하는 바이트 idx. idx이전은 헤더시작불가능 영역. 
    bool     found            = false;// 헤더패턴 찾음 여부
    bool     header_confirmed = false;// 헤더패턴 + 내용검증(Reserved+Length+Instruction) 검증 여부
    const uint16_t HEADER_LEN = 3;   //  FF FF FD + byte stuffing 여부 바이트(FD면 byte-stuffing)
    int      result           = COMM_FAIL; // 종류: COMM_SUCCESS, COMM_FAIL(default), [COMM_RX_CORRUPT, COMM_BUF_OVER, COMM_RX_TIMEOUT, COMM_CDC_ERR]

    if (wait_length > max_length)
    {
        result = pi_protocol::Comm_Result::BUF_LEN_OVER;
        break;
    }
    esp_err_t ret = tinyusb_cdcacm_read(itf,&temp[rx_length],wait_length - rx_length,&rx_size); // 어느CDC,어디저장,최대저장바이트수,실제읽은 바이트 어디저장
    if (ret != ESP_OK)
    {
        result = pi_protocol::Comm_Result::CDC_ERR;
        break;
    }

    rx_length += rx_size;
    if (rx_length >= wait_length)
    {
        if (!header_confirmed)
        {
            uint16_t limit = rx_length - HEADER_LEN;
            if (!found)
            {
            while (idx < limit) // limit이전까지만 헤더 확인 가능
            {
                uint8_t *p = (uint8_t *)memchr(&temp[idx], 0xFF, (size_t)(limit - idx)); // memchr(시작주소, 찾을값, 검색할바이트수);
                if (p == nullptr)
                {
                    idx = limit;   // 남은 구간에 0xFF 없으므로 더 볼 필요 없음
                    break;
                }
                idx = (uint16_t)(p - temp);
                if ((temp[idx + 1] == 0xFF) &&(temp[idx + 2] == 0xFD) &&(temp[idx + 3] != 0xFD))
                {
                    found = true;
                    break;
                }
                idx += 1;
            }
            if (!found)
            {
                wait_length = idx + min_length;
                continue;
            }
            }
            
            if (found) // 헤더패턴 확인
            {
            if (temp[idx + PKT_RESERVED] != 0x00 || temp[idx + PKT_LENGTH] > RXPACKET_MAX_LEN || temp[idx + PKT_INSTRUCTION] != 0x55) // 내용 검증
            {
                wait_length += HEADER_LEN;
                idx += HEADER_LEN;
                found = false;
                continue;
            }
            // 헤더패턴 + 내용 검증 후 = 진짜 헤더
            uint16_t real_len = temp[idx + PKT_LENGTH] + PKT_LENGTH + 1;
            if (idx + real_len > rx_length)// 실제 패킷 남은거 더 받아오게 
            {
                wait_length = idx + real_len;
                header_confirmed = true;
                continue;
            }
            header_confirmed = true;
            }
        }

        if (header_confirmed)
        {
            uint16_t crc = temp[rx_length-1];
            result = (updateCRC(&temp[idx], rx_length - idx) == crc) ? COMM_SUCCESS : COMM_RX_CORRUPT; // updateCRC(시작주소,검증길이)
            memmove(&rxpacket[0], &temp[idx], real_len); // memmove(목적지 시작주소, 원본시작주소, 이동할바이트수);
            break;
        }
    }
    else    // rx_length < wait_length: 아직 필요한 만큼 못 받음.timeout 확인.
    {
        const int64_t elapsed_us = esp_timer_get_time() - self->rx_parse_start_time_us;
        if (elapsed_us >= RX_TIMEOUT_US)
        {
            if (rx_length == 0)
                result = pi_protocol::Comm_Result::RX_TIMEOUT;
            else
                result = pi_protocol::Comm_Result::RX_CORRUPT;
            break;
        }
        else:
            result = pi_protocol::Comm_Result::NEED_MORE_DATA;
    }

    return result;
}

pi_protocol::Comm_Result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    constexpr uint16_t HEADER_LEN = 4;
    constexpr uint16_t MIN_PACKET_LEN = 11; // rxpacket_len = DATA(N) + 9 (FF FF FD 00 LEN ID INST DATA CRC_L CRC_H)
    uint8_t byte = 0;


    Comm_Result result = Comm_Result::NEED_MORE_DATA;
    while (rx_buffer.read(byte)) // read one byte
    {
        rx_debug_buffer.write(&byte, 1);
        if (rx_parse_len < HEADER_LEN)
        {
            switch (rx_parse_len)
            {
                case 0:
                {
                    if (byte == 0xFF)
                    {
                        rx_parse_start_time_us = esp_timer_get_time();
                        rx_parse_buffer[0] = byte;
                        rx_parse_len = 1;
                    }
                    break;
                }
                case 1:
                {
                    if (byte == 0xFF)
                    {
                        rx_parse_buffer[1] = byte;
                        rx_parse_len = 2;
                    }
                    else
                    {
                        rx_parse_len = 0;
                    }
                    break;
                }
                case 2:
                {
                    if (byte == 0xFD)
                    {
                        rx_parse_buffer[2] = byte;
                        rx_parse_len = 3;
                    }
                    else if (byte == 0xFF) // can be 2nd of a new header
                    {
                        break;
                    }
                    else
                    {
                        rx_parse_len = 0;
                    }
                    break;
                }
                case 3:
                {
                    if (byte == 0x00)
                    {
                        rx_parse_buffer[3] = byte;
                        rx_parse_len = 4;
                    }
                    else if (byte == 0xFF) // can be 1st of a new header
                    {
                        rx_parse_buffer[0] = 0xFF; 
                        rx_parse_len = 1;
                    }
                    else
                    {
                        rx_parse_len = 0;
                    }
                    break;
                }
            }
            continue;
        }

        // Read LEN
        if (rx_parse_len == 4)
        {
            rx_packet_len = byte + 9; // rxpacket_len = DATA(N) + 9 (FF FF FD 00 LEN ID INST DATA CRC_L CRC_H)
            if (rx_packet_len < MIN_PACKET_LEN || rx_packet_len > pi_protocol::RXPACKET_MAX_LEN) // invalid packet length
            {
                rx_parse_len = 0;
                rx_packet_len = 0;
                continue;
            }
            rx_parse_buffer[rx_parse_len++] = byte;
            if (!rx_buffer.read(byte))
            {
                result = 
            }
            rx_debug_buffer.write(&byte, 1);
        }

        // Read ID
        if (rx_parse_len == 5)
        {
            if (byte != 0x...) // invalid id
            {
                rx_parse_len = 0;
                rx_packet_len = 0;
                continue;
            }
            rx_parse_buffer[rx_parse_len++] = byte;
            rx_buffer.read(byte);
            rx_debug_buffer.write(&byte, 1);
        }

        // Read INSTRUCTION
        if (rx_parse_len == 6)
        {
            if (byte != 0x01 && byte != 0x55... ) // invalid inst
            {
                rx_parse_len = 0;
                rx_packet_len = 0;
                continue;
            }
            rx_parse_buffer[rx_parse_len++] = byte;
            rx_buffer.read(byte);
            rx_debug_buffer.write(&byte, 1);
        }


        // Read DATA + CRC
        while (rx_parse_len < rx_packet_len && rx_buffer.read(byte))
        {
            rx_debug_buffer.write(&byte, 1);
            rx_parse_buffer[rx_parse_len++] = byte; 
        }
        if (rx_parse_len < rx_packet_len)// Packet not complete yet
        {break;}
        
        // CRC check
        uint16_t crc = static_cast<uint16_t>(rx_parse_buffer[rx_packet_len - 2]) | (static_cast<uint16_t>(rx_parse_buffer[rx_packet_len - 1]) << 8);
        uint16_t calculated_crc = updateCRC(0, rx_parse_buffer, rx_packet_len - 2);

        if (calculated_crc != crc)
        {
            result = Comm_Result::RX_CORRUPT;
            break;
        }

        // if (rx_packet_len - 8 > sizeof(rxpacket.data)) // data so long
        // {
        //     rx_parse_len = 0;
        //     rx_packet_len = 0;
        //     return Comm_Result::BUF_LEN_OVER;
        // }

        rxpacket.data_len = rx_packet_len - 8;
        rxpacket.inst = rx_parse_buffer[PKT_INSTRUCTION];
        memcpy(rxpacket.data,&rx_parse_buffer[6],rxpacket.data_len);
        result = unstuffing(rxpacket.data, &rxpacket.data_len);
        break;
    }

    rx_parse_len = 0;
    rx_packet_len = 0;
    return result;
}

// esp-pi
static int protocol::rxPacket(int itf)
{
    (void)event;
    const uint16_t min_length       = 11;   // temp버퍼의 프로토콜 구조상 될 수 있는 최소 길이
    const uint16_t max_length = 255;    // temp버퍼의 최대 길이
    uint16_t real_len = 0;              // packet의 실제 길이
    uint8_t temp[max_length]  = {};  // rx패킷을 찾기전에 잠시 저장하는 공간.

    size_t rx_size            = 0;     // cdc로 이번에 실제로 읽은 바이트 수
    uint16_t rx_length        = 0;    // 현재 temp버퍼 바이트 수
    uint16_t wait_length      = min_length;   // 지금 기다리는 전체 패킷 길이 (최소 상태패킷 길이로 시작)

    uint16_t idx              = 0;    // 이번에 확인하기 시작하는 바이트 idx. idx이전은 헤더시작불가능 영역. 
    bool     found            = false;// 헤더패턴 찾음 여부
    bool     header_confirmed = false;// 헤더패턴 + 내용검증(Reserved+Length+Instruction) 검증 여부
    const uint16_t HEADER_LEN = 3;   //  FF FF FD + byte stuffing 여부 바이트(FD면 byte-stuffing)
    int      result           = COMM_FAIL; // 종류: COMM_SUCCESS, COMM_FAIL(default), [COMM_RX_CORRUPT, COMM_BUF_OVER, COMM_RX_TIMEOUT, COMM_CDC_ERR]

    while (true)
    {
        if (wait_length > max_length)
        {
            result = COMM_BUF_OVER;
            break;
        }
        esp_err_t ret = tinyusb_cdcacm_read(itf,&temp[rx_length],wait_length - rx_length,&rx_size); // 어느CDC,어디저장,최대저장바이트수,실제읽은 바이트 어디저장
        if (ret != ESP_OK)
        {
            result = COMM_CDC_ERR;
            break;
        }

        rx_length += rx_size;
        if (rx_length >= wait_length)
        {
            if (!header_confirmed)
            {
                uint16_t limit = rx_length - HEADER_LEN;
                if (!found)
                {
                while (idx < limit) // limit이전까지만 헤더 확인 가능
                {
                    uint8_t *p = (uint8_t *)memchr(&temp[idx], 0xFF, (size_t)(limit - idx)); // memchr(시작주소, 찾을값, 검색할바이트수);
                    if (p == nullptr)
                    {
                        idx = limit;   // 남은 구간에 0xFF 없으므로 더 볼 필요 없음
                        break;
                    }
                    idx = (uint16_t)(p - temp);
                    if ((temp[idx + 1] == 0xFF) &&(temp[idx + 2] == 0xFD) &&(temp[idx + 3] != 0xFD))
                    {
                        found = true;
                        break;
                    }
                    idx += 1;
                }
                if (!found)
                {
                    wait_length = idx + min_length;
                    continue;
                }
                }
                
                if (found) // 헤더패턴 확인
                {
                if (temp[idx + PKT_RESERVED] != 0x00 || temp[idx + PKT_LENGTH] > RXPACKET_MAX_LEN || temp[idx + PKT_INSTRUCTION] != 0x55) // 내용 검증
                {
                    wait_length += HEADER_LEN;
                    idx += HEADER_LEN;
                    found = false;
                    continue;
                }
                // 헤더패턴 + 내용 검증 후 = 진짜 헤더
                uint16_t real_len = temp[idx + PKT_LENGTH] + PKT_LENGTH + 1;
                if (idx + real_len > rx_length)// 실제 패킷 남은거 더 받아오게 
                {
                    wait_length = idx + real_len;
                    header_confirmed = true;
                    continue;
                }
                header_confirmed = true;
                }
            }

            if (header_confirmed)
            {
                uint16_t crc = temp[rx_length-1];
                result = (updateCRC(&temp[idx], rx_length - idx) == crc) ? COMM_SUCCESS : COMM_RX_CORRUPT; // updateCRC(시작주소,검증길이)
                memmove(&rxpacket[0], &temp[idx], real_len); // memmove(목적지 시작주소, 원본시작주소, 이동할바이트수);
                break;
            }
        }
        else    // rx_length < wait_length: 아직 필요한 만큼 못 받음.timeout 확인.
        {
            if (port->isPacketTimeout() == true)
            {
                if (rx_length == 0)
                {result = COMM_RX_TIMEOUT;}
                else
                {result = COMM_RX_CORRUPT;}
                break;
            }
        }
        // #if defined(ESP_PLATFORM)
        // taskYIELD()는 "같은 우선순위의 다른 태스크"에게만 양보한다. 이 태스크가 idle보다 높은 우선순위에서 계속 ready 상태라면 idle 태스크(및 watchdog feed)가 전혀 실행되지
        // 못해 Task Watchdog reset을 유발할 수 있다. 확실히 안전하려면 최소 1 tick의 실제 delay가 필요하다. 다만 이 delay는 (특히 half-duplex 응답을 기다리는 구간에서) 왕복
        // 타이밍에 영향을 줄 수 있으므로, tick rate를 충분히 높이거나(예: 1kHz), 응답을 이미 기다리는 도중이 아니라 유휴 구간에서만 이 폴링이 자주 발생하도록 상위 타임아웃 설계와 맞춰 사용하는 것을 권장한다.
        vTaskDelay(1);
    }

    port->is_using_ = false;
    return result;
}