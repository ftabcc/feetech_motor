//26-10-06
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
            if(status.err & full)
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
            if (rx_start_time == 0)
                rx_start_time = time; 

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
        if(elapsed_time > timeout)
            // status.err |= timeout
            break;
    }
    if (rx_start_time != 0)       
        elapsed_time = time - rx_start_time;
        if(elapsed_time > timeout)
            // status.err |= timeout
    return status;
}

'''save process'''
while true
    if(쓰기가능?)
        //읽기
        if (read!=0)
            //버퍼쓰기
        else
            if(read_available < rx_buffer_size)
                // 부족, 추가로 읽어오기
            else:
                // 가득참, idx전까지 최대로 보내고,당기기
    if(buff full)
        break;

    if(!found)
        if(read_avail >= idx + header_len)
            whlie(idx <= read_avail - header_len)
                //검색
                if(header확인)
                    break;
                if(nothing search)
                    idx = read_avail - header_len;
                    if(read_available < rx_buffer_size)
                        // 부족, 추가로 읽어오기
                    else:
                        // 가득참, idx전까지 최대로 보내고,당기기
                    break;
                
                idx+=1;
        else: // 부족
            if(read_available < rx_buffer_size)
                // 부족, 추가로 읽어오기 
            else:
                // 가득참, idx전까지 최대로 보내고,당기고,읽기
            if(buf full)
                보내고 당기고
            읽기


    if(found)
        if (len,id,inst 필드 읽기 가능?)
            if(len,id,inst 가능)
                if (packet 필드 읽기 가능?)
                    if(crc=calculated_crc)
                        // found = false
                        // idx, raed_av = 0
                        break;
                    else:
                        //(필수)패킷이라 판단했던 바이트 + (가능한) 이전바이트 보내기
                        if(read_available < rx_buffer_size)
                            // 부족, 추가로 읽어오기
                        else:
                            // 가득참, idx전까지 최대로 보내고,당기기
                else 
                    if(read_available < rx_buffer_size)
                        // 부족, 추가로 읽어오기
                    else:
                        // 가득참, idx전까지 최대로 보내고,당기기
            else
                // found = false
                // idx += header_len
                if(read_available < rx_buffer_size)
                        // 부족, 추가로 읽어오기
                    else:
                        // 가득참, idx전까지 최대로 보내고,당기기

        else 
            if(read_available < rx_buffer_size)
                // 부족, 추가로 읽어오기
            else:
                // 가득참, idx전까지 최대로 보내고,당기기
