#include "pi_comm.h"

#include <assert.h>
#include "esp_log.h"
#include "esp_err.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static void pi_comm::init(void *arg)
{
    // ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = &rx_callback,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tinyusb_cdcacm_init(&acm_cfg));

    rx_queue = xQueueCreate(pi_protocol::RXPACKET_MAX_NUM, sizeof(pi2esp_packet_t));
    tx_queue = xQueueCreate(pi_protocol::TXPACKET_MAX_NUM, sizeof(esp2pi_packet_t));

    xTaskCreate(rx_task, "rx_task", 4096, this, 10, &rx_task_handle);
    xTaskCreate(packet_process_task, "packet_process", 4096, this, 10, nullptr);
    xTaskCreate(tx_task, "tx_task", 4096, this, 10, nullptr);

}

void pi_comm::rx_callback(int itf, cdcacm_event_t *event)
{
    (void)event;
    constexpr size_t RX_TEMP_SIZE = 64;
    uint8_t temp[RX_TEMP_SIZE];
    size_t rx_size = 0;
    esp_err_t ret = tinyusb_cdcacm_read(itf, temp, sizeof(temp), &rx_size);
    if (ret != ESP_OK)
    {
        return; // CDC err
    } 
    if (rx_size == 0)
    {
        return; // nothing read
    }
    
    if (rx_buffer.write(temp, rx_size) != rx_size)
    {
        // rx buffer overflow
        return;
    }
    if (rx_debug_buffer.write(temp, rx_size) != rx_size)
    {
        // timeout 처리?...??
    }
    xTaskNotifyGive(rx_task_handle);
}

void pi_comm::rx_task(void *arg){
    pi_comm *self = static_cast<pi_comm *>(arg);
    pi_protocol::rxpacket_t rxpacket;
    while(true){
        TickType_t wait_ticks;
        if(self->rx_start_time_us != 0){
            const int64_t elapsed_us = esp_timer_get_time() - self->rx_start_time_us;
            if (elapsed_us > pi_protocol::RX_TIMEOUT_US)
                status.err |= pi_protocol::Comm_err::RX_TIMEOUT;
            else{
                wait_ticks = pdMS_TO_TICKS(static_cast<uint32_t>((pi_protocol::RX_TIMEOUT_US - elapsed_us + 999) / 1000));
                if (wait_ticks == 0) // Avoid immediate return when tick resolution is coarse.
                    wait_ticks = 1;
                }
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
                pi_protocol::Comm_Result result = self->rx_packet(rxpacket;);'''result,err,status 나눠서 의미부여.'''
            else if(status.err & pi_protocol::Comm_err::RX_TIMEOUT){
                if(self->idx != 0)
                    txpacket_t txpacket;
                    txpacket.data_len = 2;
                    txpacket.id = warning_packet_id++;
                    txpacket.inst = ??; // timeout warning
                    txpacket.err = status.errors;
                    txpacket.data = [??,??];
                    if(xQueueSend(self->tx_queue,&self->txpacket,0) != pdTRUE){
                        // break; TX_QUEUE_FULL: preserve unsent bytes
                    }
                    // reset: idx,found,confirm,start,read_av
                    status.err &= ~pi_protocol::Comm_Error::RX_TIMEOUT;
                }

            if(status.err & pi_protocol::Comm_Error::CDC_ERR){
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
            }
            if(result == pi_protocol::Comm_Result::SUCCESS){
                if (xQueueSend(self->rx_queue, &self->rxpacket, 0) != pdTRUE){
                    xQueueReset(self->rx_queue);
                    xQueueSend(self->rx_queue, &self->rxpacket, 0);
                }
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
                        '''실패시 구체화 필요. '''
                    }
                    sent_bytes += txpacket.data_len;
                    debug_packet_id++;
                }
                if (sent_bytes > 0){
                    memmove(rx_buffer,rx_buffer + sent_bytes,read_available - sent_bytes);
                    idx -= sent_bytes;
                    read_available -= sent_bytes;
                }
            }
            if(result == pi_protocol::Comm_Result::FAILURE){ // crc_err
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
            }
            
            if(read_available == pi_protocol::rx_buffer_size)
                txpacket_t txpacket;
                txpacket.data_len = std::min(idx,sizeof(txpacket.data));
                txpacket.id = debug_packet_id++;
                txpacket.inst = ??; //debug
                txpacket.err = status.errors;
                memcpy(txpacket.data,rx_buffer,txpacket.data_len);
                if(xQueueSend(self->tx_queue,&self->txpacket,0) != pdTRUE){
                    // break; TX_QUEUE_FULL: preserve unsent bytes
                }

                idx -= txpacket.data_len;
                read_available -= txpacket.data_len;
                status.err &= ~pi_protocol::Comm_Error::BUFFER_FULL;
                
            
            if(result == pi_protocol::Comm_Result::PENDING ||'''에러일때 break해야하는데''')
                break; // cdc callback의 notify받도록 break
        }
    }
}

pi_protocol::Comm_Result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    while(true){
        std::size_t rx_size = 0;
        const esp_err_t ret = tinyusb_cdcacm_read(itf, &rx_buffer[read_available], rx_buffer_size - read_available, &rx_size);
        '''for safe extra iteration, dont clear notify-value.'''
        if (ret != ESP_OK){
            statis.errors |= pi_protocol::Comm_Error::CDC_ERR;
            break;
        }
        if(rx_size != 0){
            read_available += rx_size;
            rx_start_time_us = 0;
        }

        bool ready = (!found &&  read_av >= idx + header_len) || // !found일때 header 읽기 가능?
                (found && !confirm && read_av >= idx + header_len + 3) || // found일때 len,id,inst 필드 읽기 가능?
                (confirm && read_av >= idx + rx_buffer[idx + pi_protocol::PKT_LENGTH]); // confirm일때 packet 전체 읽기 가능?
        
        if(rx_size == 0 && !ready && rx_start_time_us == 0){
            rx_start_time_us = esp_timer_get_time();
            status.result = pi_protocol::Comm_Result::PENDING;
        }

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
                        packet_len = rx_buffer[idx + pi_protocol::PKT_LENGTH];
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

                                memmove(&rx_buffer[idx],rx_buffer + idx + packet_len,read_available - (idx + packet_len));
                                read_available -= packet_len;
                                found = false;
                                
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
        if(stuts.result != Null || status.err != 0)
            break;
    }
    return status;
}

void pi_comm::packet_process_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    pi2esp_packet_t packet;

    while (true)
    {
        if (xQueueReceive(self->rx_queue, &packet, portMAX_DELAY) != pdTRUE)
        {continue;}

        switch (packet.inst)
        {
            case INST_REGISTER_TRAJECTORY:
            {
                trajectory_err_t err = self->trajectory.register_trajectory(packet);
                switch (err)
                {
                    case trajectory_err_t::SUCCESS:
                        break;

                    case trajectory_err_t::INVALID_LENGTH:
                        break;

                    case trajectory_err_t::INVALID_DURATION:
                        break;

                    case trajectory_err_t::BUFFER_FULL:
                        break;
                }

                break;
            }

            case INST_COMMAND:
                self->write_packet(packet);
                break;

            case INST_STOP:
                break;
            
            case INST_SLEEP: // stop sending packets, don't send rxtimeout err
                break;

            default:
                // Invalid instruction
                break;
        }
    }
}

void pi_comm::tx_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    pi_protocol::txpacket_t txpacket;
    while (true)
    {
        if (xQueueReceive(self->tx_queue, &txpacket, portMAX_DELAY) == pdTRUE)
        {self->tx_packet(&txpacket);}
    }
}

pi_protocol::Comm_Result pi_comm::tx_packet(pi_protocol::txpacket_t &txpacket)
{
    uint8_t tx_buffer[pi_protocol::TXPACKET_MAX_LEN];
    uint16_t tx_len = 0;

    Comm_Result result = stuffing(txpacket.data, &txpacket.data_len);
    if (result != Comm_Result::SUCCESS)
    {return result;}
        
    // 유의미한 조건문인가??...
    if (txpacket.data_len > sizeof(txpacket.data))
    {
        return Comm_Result::BUF_LEN_OVER;
    }

    tx_buffer[0] = 0xFF;
    tx_buffer[1] = 0xFF;
    tx_buffer[2] = 0xFD;
    tx_buffer[3] = 0x00;
    tx_buffer[4] = static_cast<uint8_t>(txpacket.data_len);
    tx_buffer[5] = txpacket.inst;
    tx_buffer[6] = txpacket.err;
    memcpy(&tx_buffer[7],txpacket.data,txpacket.data_len);

    // CRC
    uint16_t crc = updateCRC(0,tx_buffer,txpacket.data_len+7); // txpacket_len = data(n) + 9
    tx_buffer[txpacket.data_len+7] = static_cast<uint8_t>(crc & 0xFF);
    tx_buffer[txpacket.data_len+8] = static_cast<uint8_t>((crc >> 8) & 0xFF);

    if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, tx_buffer, txpacket.data_len+9) != ESP_OK)
    {return Comm_Result::CDC_ERR;}
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    return Comm_Result::SUCCESS;
}

// CRC16bit(0x8005)
unsigned short pi_comm::updateCRC(uint16_t start, uint8_t *addr, uint16_t size)
{
  uint16_t i;
  static const uint16_t crc_table[256] = {0x0000,
  0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011, 0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027,
  0x0022, 0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072, 0x0050, 0x8055, 0x805F, 0x005A, 0x804B,
  0x004E, 0x0044, 0x8041, 0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2, 0x00F0, 0x80F5, 0x80FF,
  0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1, 0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093,
  0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082, 0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197,
  0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1, 0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB,
  0x01FE, 0x01F4, 0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2, 0x0140, 0x8145, 0x814F,
  0x014A, 0x815B, 0x015E, 0x0154, 0x8151, 0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162, 0x8123,
  0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132, 0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104,
  0x8101, 0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A, 0x832B,
  0x032E, 0x0324, 0x8321, 0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371, 0x8353, 0x0356, 0x035C,
  0x8359, 0x0348, 0x834D, 0x8347, 0x0342, 0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3,
  0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2, 0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7,
  0x03B2, 0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381, 0x0280, 0x8285, 0x828F, 0x028A, 0x829B,
  0x029E, 0x0294, 0x8291, 0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2, 0x82E3, 0x02E6, 0x02EC,
  0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1, 0x8243,
  0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252, 0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264,
  0x8261, 0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231, 0x8213, 0x0216, 0x021C, 0x8219, 0x0208,
  0x820D, 0x8207, 0x0202 };

  for (uint16_t j = 0; j < data_blk_size; j++)
  {
    i = ((uint16_t)(crc_accum >> 8) ^ *data_blk_ptr++) & 0xFF;
    crc_accum = (crc_accum << 8) ^ crc_table[i];
  }

  return crc_accum;
}

int pi_comm::stuffing(uint8_t *data, int *len)
{   // 1차 탐색 2차 뒤부터 채우기(다이나믹셀 방식) vs 탐색하며 별도메모리에 추가하며 채우기
    if (!skip_stuffing)
    {
        
        if (data == nullptr || len == nullptr)
            return Comm_Result::FAIL;

        // Too short to contain FF FF FD
        if (*len < 3)
            return Comm_Result::SUCCESS;

        // 1st pass: Count required stuffing bytes
        int stuffing_count = 0;
        for (int i = 0; i + 2 < *len; i++)
        {
            if (data[i] == 0xFF && data[i + 1] == 0xFF && data[i + 2] == 0xFD)
            {stuffing_count++;}
        }

        // No stuffing required
        if (stuffing_count == 0)
            return Comm_Result::SUCCESS;

        // Check output buffer capacity
        if (*len + stuffing_count > pi_protocol::RXPACKET_MAX_LEN - 8)
            return Comm_Result::BUF_LEN_OVER;

        int read_idx = *len - 1;
        int write_idx = *len + stuffing_count - 1;

        // 2nd pass: Move data backward and insert FD
        while (read_idx >= 0)
        {
            // FF FF FD
            if (read_idx >= 2 && data[read_idx - 2] == 0xFF && data[read_idx - 1] == 0xFF && data[read_idx] == 0xFD)
            {
                // Add stuffing byte
                data[write_idx--] = 0xFD;

                // Move original FF FF FD
                data[write_idx--] = data[read_idx--]; // FD
                data[write_idx--] = data[read_idx--]; // FF
                data[write_idx--] = data[read_idx--]; // FF
            }
            else
            {data[write_idx--] = data[read_idx--];}// Move normal byte
        }

        // Update length
        *len += stuffing_count;
    }

    return Comm_Result::SUCCESS;
}

int pi_comm::unstuffing(uint8_t *data, int *len)
{
    if (!skip_stuffing)
    {
        if (data == nullptr || len == nullptr || *len < 0)
            return Comm_Result::FAIL;

        // Too short to contain FF FF FD FD
        if (*len < 4)
            return Comm_Result::SUCCESS;

        int read_idx = 0;
        int write_idx = 0;

        // Read from front and compact in-place
        while (read_idx < *len)
        {
            // FF FF FD FD
            if (read_idx + 3 < *len && data[read_idx] == 0xFF && data[read_idx + 1] == 0xFF && data[read_idx + 2] == 0xFD && data[read_idx + 3] == 0xFD)
            {
                // Copy original FF FF FD
                data[write_idx++] = data[read_idx++];
                data[write_idx++] = data[read_idx++];
                data[write_idx++] = data[read_idx++];
                read_idx++; // Skip stuffed FD
            }
            else
            {data[write_idx++] = data[read_idx++];} // Move normal byte
        }

        // Update length
        *len = write_idx;
    }

    return Comm_Result::SUCCESS;
}