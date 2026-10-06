'''나중에 cdc_read를 task안에서 해볼까?'''

'''TASK'''
while(true)
    if(rx_start_time!=0)
        elapsed_time = time - rx_start_time
        if(elapsed_time > timeout)
            '''//rx_err보내기
            // idx,found,start,read_av초기화
            // continue'''
            // err에 timeout추가
        else
            wait_time = time_out - elapsed_time;
    else
        wait_time = max;

    notify = wait(wait_time)
    if (!notify)
        // err에 timeout추가

    while (true)
        result = process;

        // com_err조치. 중복가능
        if(err=crc_err)
            //(필수)패킷이라 판단했던 바이트 + (가능한) 이전바이트 최대 보내기
        if(err=buffer_full)
            // 가득참, idx전까지 최대로 보내고,당기기
        if(err=timeout)
            // idx,found,start,read_av초기화
        if(err=cdc_err)
            // 조치미정

        // com_result조치. 중복불가
        if(result = succes)
            if(rxpacket_queue == full)
                // 다비우기? 다른조치취하기?
            // 완성패킷 큐send, 추가로 패킷있을 수도 있어서 break안함.
        if(result = need_more_data)
            // 위에서 cdc callback의 notify받도록 break
            break;


'''process'''
while true

    '''읽기 단계. full인경우는 모두 밑에서 잡아내서 break되어 버퍼가 관리되었음.'''
    // 가능한 만큼 cdc_read
    if (read!=0)
        // 버퍼 쓰기
    else
        // status.result = nmd
        break;

    if(!found)
        if(read_avail >= idx + header_len)
            whlie(idx <= read_avail - header_len)
                //검색
                if(header확인)
                    found = true;
                    break;
                if(nothing search)
                    idx = read_avail - header_len;
                    break;
                idx+=1;

    if(found)
        if (len,id,inst 필드 읽기 가능?)
            if(len,id,inst 정상)
                if (packet 필드 읽기 가능?)
                    if(crc=calculated_crc)
                        // rxpacket 큐 생성 아직 안보냄.
                        // found = false
                        // idx, raed_av = 0
                        break;
                    else:
                        // status.err에 crc_err추가
            else
                // found = false
                // idx += header_len

    if(full)
        // status.err에 full추가
        break;
    else
        // status.result = nmd
                


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
