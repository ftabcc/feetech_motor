process(){
    while(true){
        //cdc_read(itf,&rx_buffer[read_av],rx_buffer_size - read_av, &rx_size);
        if(ret!=ok)
            // status.err |= cdc_err;
        ready = (!found &&  read_av >= idx + header_len) || // !found일때 header 읽기 가능?
                (found && !confirm && read_av >= idx + PKT_INST) || // found일때 len,id,inst 필드 읽기 가능?
                (confirm && read_av >= idx + packet_len); // confirm일때 packet 전체 읽기 가능?
        if(read == 0 && !ready)
            // status.result = pendig;
        if (read != 0 && rx_start_time ==0)
            rx_start_time = time;

        if (ready){
            if(!found){
                if(read_avail >= idx + header_len){
                    whlie(idx <= read_avail - header_len){
                        // *p = memchr();
                        if(p ~ p+3 = header)
                            found = true;
                            break;
                        if(p == null)
                            idx = read_avail - header_len + 1;
                            break;
                        idx+=1;
                    }
                }
            }
            
            if(found){
                if (read_av >= idx + PKT_INST){ //len,id,inst 필드 읽기 가능?
                    if(len,id,inst 정상){
                        confirm = true;
                        if (read_av >= idx + packet_len) //packet 전체 읽기 가능
                            if(crc=calculated_crc)
                                // status.result = success;
                            else:
                                // status.result = failure;
                            // found = false;
                            // confirm = false;
                            // rx_start_time = 0;
                    }
                    else{
                        // found = false;
                        // confirm = false;
                        // idx += header_len;
                    }
                }
            }
        }
            
        if(read_av == rx_buffer_size)
            // status.err |= full
        if(rx_start_time != 0 && time - rx_start_time > timeout)
            // status.err |= timeout
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
