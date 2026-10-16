// 26-10-17일 이후 temp_fix와 같이 수정
task(){
    wait = true;
    while(true){
        '''
        wait에 대한 상반된 고민
        1) notify_value에 대해 추가적으로 안올수도있고, 확인이 필요한 바이트가 rx_buffer에 있어서 notify_wait 안해야하나?
        2) 굳이 wait trig를 사용해야하나? 성공 또는 실패 후 패킷 처리를 했는데 바이트가 더있다는 것은 
        해당 패킷외의 다른 패킷 데이터가 들어오고 있었다는 뜻이고,
        애초에 task가 callback의 notify를 처음 받아오고 notify valure초기화하고 packet함수 처리할때
        추가적으로 다른패킷의 rx_transfer를 받았다는건데, 그렇다면 continue되어도 
        이미 notify value가 true일텐데?
        '''
        if(wait)
            notify_wait(max) // from rx_callback
        wait = true;

        first_loop = true;
        result = packet() // success,failure,timeout,rx_corrupt
        
        if(result = succes){
            if(read_avail > idx + packet_len){
                wait = false;
            }
            send_queue(packet);
            memmove(idx,idx+packet_len,read_avail-(idx+packet_len));
            read_avail -= packet_len;
            send_debug(0~idx);
            memmove(0,idx,read_avail-idx);
            read_avail -= idx;
            idx = 0
            // 그냥 send_queue,debug다 한뒤에 memmove한번에 할까?
        }
        if(result = failure){
            wait = false;
            idx += header_len; // 헤더가 될 수 없는 바이트는 건너뛰기
            send_err(crc_err);
        }
        if(result = rx_corrput){
            send_debug(0~idx);
            memmove(0,idx,read_avail-idx);
            read_avail -= idx;
            idx = 0
            wait = false; // rx_transfer가 남아있는데 rx_buffer가 가득 차서 못읽어왔을 수 있으니 기다리지말고 바로 함수 호출.
        }
        if(result = timeout){
            send_debug(0~idx);
            memmove(0,idx,read_avail-idx);
            read_avail -= idx;
            idx = 0
        }
    }
}

packet(){
    result = nothing;
    while(true){
        read();

        //ready확인해야함.
        // 첫 packet함수 호출이고, rx_buffer에 읽을게 있으면 read_size가 0 이어도 callback의 wait없이 확인process거쳐가자.
        if(read_size=0 && !(first_loop && read_avail!=0)){ 
            notify_wait(timeout) // from rx_callback.
            '''
            함수가 처음 호출되거나, 위로 다시 돌아왔을때 
            read_size를 읽는 다는것은 바이트가 더 필요하다는 뜻이다.
            일정시간동안 바이트가 안들어오는게 유지가 된다면,
            result = timeout으로 내보내고, 버퍼의 0 ~ idx까지 debug바이트 보내고 당긴다.
            '''
            if(!notified){
                result = timeout;
                break;
            }
            else{
                continue;
            }
        }
        else if(read_size != 0){
            read_avail+=read_size;
        }
        else if(first_loop && read_avail!=0){
            first_loop = false;
        }

        ready = (!found &&  read_av >= idx + header_len) || // !found일때 header 읽기 가능?
                (found && !confirm && read_av >= idx + PKT_INST) || // found일때 len,id,inst 필드 읽기 가능?
                (confirm && read_av >= idx + packet_len); // confirm일때 packet 전체 읽기 가능?
        if (ready){
            if(!found){
                if(read_avail >= idx + header_len){
                    whlie(idx <= read_avail - header_len){
                        *p = memchr();
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
                                '''
                                crc통과시 result = success
                                task에서 성공패킷을 버퍼에서 queue로 보내고, 뒤의 바이트가 있다면 당겨붙인다.
                                0~idx의 debug바이트 보내고, 성공패킷뒤로는 debug바이트의 stream이 연결안되어서
                                debug_packet_id를 0으로 초기화 한다.
                                '''
                                result = success;
                            else:
                                result = failure;
                            found = false;
                            confirm = false;
                    }
                    else{
                        found = false;
                        confirm = false;
                        idx += header_len;
                    }
                }
            }
        }
        
        '''
        버퍼 크기는 패킷최대크기의 2배로 잡는다.
        만약 버퍼가 가득 찼다는 것은 최소한 하나의 패킷 이상이 붕괴되어 들어왔다고 생각할수있다.
        result = rx_corrput로 break한뒤, task에서 idx까지 debug바이트를 보내고 버퍼 당긴다.
        '''
        if(result == nothing && read_avail == rx_buffer_size)
            result = rx_corrput
        if(result != nothing)
            break;
    }
    return result;
}


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
            '''debug스트림보낼때랑 성공,실패용으로 패킷보낼때 차이두고 보내야함.'''
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

            if(result = pending)
                // cdc callback의 notify받도록 break
                break;

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
        
        if (read==0 && !one_more_check)
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