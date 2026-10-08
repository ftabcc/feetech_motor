
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
            if (rx_start_time!=0)
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
    }
    if (rx_start_time != 0)       
        if(time - rx_start_time > timeout)
            // status.err |= timeout
            break;
    return status;
}
