while true
    if(쓰기가능?)
        //버퍼쓰기
    if(buff full)
        break;



    if(!found)
        if(read_avail >= idx + header_len)
            whlie(idx <= read_avail - header_len)
                //검색
                if(nothing search)
                    idx = read_avail - header_len;
                    if(read_available < rx_buffer_size)
                        // 부족, 추가로 읽어오기
                    else:
                        // 가득참, idx전까지 최대로 보내고,당기기
                    break;
        else:
            if(read_available < rx_buffer_size)
                // 부족, 추가로 읽어오기 
            '''더 받아올때, 버퍼가 가득찼는지 확인해야함. 아래 전체 수정필요.'''
            else:
                // 가득참, idx전까지 최대로 보내고,당기기



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