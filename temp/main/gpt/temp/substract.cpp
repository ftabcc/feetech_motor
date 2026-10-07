//26-10-08
task(){
    while(true){
        if(rx_start_time!=0)
            if (elapsed_time >= timeout)
                wait_time = 0;
            else
                wait_time = timeout - elapsed_time;
        else
            wait_time = max;

        notify = wait(wait_time)
        if (!notify)
            // status.err |= timeout;

        while (true){
            if(!(status.err & timeout))
                result = process();

            '''status.err 중복가능'''
            if(idx!=0 || full || success || failure) // debug용 바이트스트림 연결시키기.
                while(true)
                    // 시작부터 idx전까지 가능한 많이 보내고, 당기기
                    if (idx == 0)
                        break;
                // status.err &= ~rx_desync;

            '''status.result 중복불가'''
            if(result = success)
                // 완료된 패킷저장
                if(rxpacket_queue send != True)
                    // 이전 rxpacket 큐 reset
                    // 완성패킷 큐send 
                // idx, raed_av 완료된 패킷만 비우고 당기기

                if(idx!=0)
                    // 0~idx전까지 debug용 바이트스트림 연결시키기.

                // found, start 초기화;
                // one_more_buffer_check = true;
            if(result = failure)
                if(crc_err)
                    // 패킷이라 판단했던 바이트 보내기

                    // idx += header_len;
                    // status.err &= ~crc_err;
                    // one_more_buffer_check = true;
                if(full)
                    // 0~idx전까지 debug용 바이트스트림 연결시키기.
                if(status.err & timeout)
                    // timeout정보 보내기
                    // idx,found,start,read_av초기화
                    // status.err &= ~timeout;
                if(status.err & cdc_err)
                    // usb연결상태 경고보내기.
                    // status.err &= ~cdc_err;

            if(result = pending)
                // cdc callback의 notify받도록 break
                break;
        }
    }
}

process(){
    while(true){
        '''읽기 단계.'''
        // 가능한 만큼 cdc_read;
        if(ret!=ok)
            // status.err |= cdc_err;
            // break;
        
        if (read==0 && != one_more_check)
            // one_more_check = false;
            // status.result = pendig;
            // break;
        if (read!=0)
            // 버퍼 쓰기

        '''헤더 찾기 단계'''
        if(!found)
            if (rx_start_time!=0)
                rx_start_time = time;
            if(read_avail >= idx + header_len)
                whlie(idx <= read_avail - header_len)
                    // 검색
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
                            // status.err |= rx_desync;
                            // status.result = success;
                            break;
                        else:
                            // found = false;
                            // status.err |= rx_desync;
                            // status.err |= crc_err;
                            // status.result = failure;
                            
                else
                    // found = false;
                    // idx += header_len;
        '''err추가 확인단계'''
        if(full)
            // status.err |= full
            break;

        elapsed_time = time - rx_start_time;
        if(elapsed_time > timeout && status.result != success)
            // status.err |= timeout
            break;
    }
    if (rx_start_time != 0)       
        elapsed_time = time - rx_start_time;
        if(elapsed_time > timeout && status.result != success)
            // status.err |= timeout
    return status;
}

'''save process'''

task(){
    while(true){
        if(rx_start_time!=0)
            elapsed_time = time - rx_start_time
            wait_time = time_out - elapsed_time;
            '''wrap around주의'''
        else
            wait_time = max;

        notify = wait(wait_time)
        if (!notify)
            // status.err |= timeout;

        while (true){
            if(!(status.err & timeout))
                result = process();

            '''status.err 중복가능'''
            if(status.err & rx_desync)
                while(true)
                    // 시작부터 idx전까지 가능한 많이 보내고, 당기기
                    if (idx == 0)
                        break;
                // status.err &= ~rx_desync;
            if(status.err & crc_err)
                '''crc_err가 full보다 우선처리필요.
                crc_err&idx=0,full이라면 crc_err하고 idx증가후 full처리 필요 '''
                // 패킷이라 판단했던 바이트 보내기
                // idx += header_len;
                // status.err &= ~crc_err;
            if(status.err & full) '''rx_desync나full은 따로 버퍼관리상태를 만들어서 사용. event flag와는 다름'''
                // 시작부터 idx전까지 가능한 많이 보내고, 당기기
                // status.err &= ~full;
            
            if(status.err & timeout)
                // timeout정보 보내기
                // idx,found,start,read_av초기화
                // status.err &= ~timeout;
            if(status.err & cdc_err)
                // usb연결상태 경고보내기.
                // status.err &= ~cdc_err;

            '''status.result 중복불가'''
            if(result = success)
                // 완료된 패킷저장
                // found, start 초기화;
                // idx, raed_av 완료된 패킷까지 비우고 당기기
                if(rxpacket_queue send != True)
                    // 이전 rxpacket 큐 reset
                // 완성패킷 큐send 
                // one_more_buffer_check = true;
            if(result = failure)
                // one_more_buffer_check = true;
            if(result = pending)
                // cdc callback의 notify받도록 break
                break;
        }
    }
}

process(){
    while(true){
        '''읽기 단계.'''
        ''' full인경우는 모두 밑에서 잡아내서 break되어 버퍼가 관리되었음.'''

        // 가능한 만큼 cdc_read;
        if(ret!=ok)
            // status.err |= cdc_err;
            // break;
        
        if (read==0 && != one_more_check)
            // one_more_check = false;
            // status.result = pendig;
            // break;
        if (read!=0)
            // 버퍼 쓰기
            if (elapsed_time >= timeout)
                wait_time = 0;
            else
                wait_time = timeout - elapsed_time;

        '''헤더 찾기 단계'''
        if(!found)
            if(read_avail >= idx + header_len)
                whlie(idx <= read_avail - header_len)
                    // 검색
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
                            // status.err |= rx_desync;
                            // status.result = success;
                            break;
                        else:
                            // found = false;
                            // status.err |= rx_desync;
                            // status.err |= crc_err;
                            // status.result = failure;
                            
                else
                    // found = false;
                    // idx += header_len;
        '''err추가 확인단계'''
        if(full)
            // status.err |= full
            break;

        elapsed_time = time - rx_start_time;
        if(elapsed_time > timeout && status.result != success)
            // status.err |= timeout
            break;
    }
    if (rx_start_time != 0)       
        elapsed_time = time - rx_start_time;
        if(elapsed_time > timeout && status.result != success)
            // status.err |= timeout
    return status;
}